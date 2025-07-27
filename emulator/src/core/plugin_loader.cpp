#include "plugin_loader.h"
#include <dlfcn.h>
#include <iostream>
#include <sys/stat.h>

typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);

PluginLoader::PluginLoader() {
}

PluginLoader::~PluginLoader() {
    unloadPlugin();
}

bool PluginLoader::loadPlugin(const std::string& path) {
    unloadPlugin();
    
    // Load the dynamic library
    plugin_.handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!plugin_.handle) {
        std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
        return false;
    }
    
    if (!validatePlugin(plugin_.handle)) {
        cleanup();
        return false;
    }
    
    // Get the pluginEntry function
    PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(plugin_.handle, "pluginEntry");
    if (!pluginEntry) {
        std::cerr << "Plugin missing pluginEntry symbol: " << dlerror() << std::endl;
        cleanup();
        return false;
    }
    
    // Check API version
    uintptr_t version = pluginEntry(kNT_selector_version, 0);
    if (version != kNT_apiVersionCurrent) {
        std::cerr << "API version mismatch: " << version << " vs " << kNT_apiVersionCurrent << std::endl;
        cleanup();
        return false;
    }
    
    // Get number of factories (should be 1)
    uintptr_t numFactories = pluginEntry(kNT_selector_numFactories, 0);
    if (numFactories < 1) {
        std::cerr << "No factories in plugin" << std::endl;
        cleanup();
        return false;
    }
    
    // Get factory pointer for index 0
    plugin_.factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
    if (!plugin_.factory) {
        std::cerr << "Failed to get factory" << std::endl;
        cleanup();
        return false;
    }
    
    // API version already checked via selector
    
    // Get static requirements and allocate shared memory
    if (!plugin_.factory->calculateStaticRequirements) {
        std::cerr << "Plugin factory missing calculateStaticRequirements function" << std::endl;
        cleanup();
        return false;
    }
    
    _NT_staticRequirements staticReqs = {};
    plugin_.factory->calculateStaticRequirements(staticReqs);
    if (staticReqs.dram > 0) {
        // Use posix_memalign for better compatibility
        if (posix_memalign(&plugin_.shared_memory, 16, staticReqs.dram) != 0) {
            std::cerr << "Failed to allocate shared memory" << std::endl;
            cleanup();
            return false;
        }
        
        // Initialize the factory
        if (!plugin_.factory->initialise) {
            std::cerr << "Plugin factory missing initialise function" << std::endl;
            cleanup();
            return false;
        }
        
        _NT_staticMemoryPtrs staticPtrs = {};
        staticPtrs.dram = (uint8_t*)plugin_.shared_memory;
        plugin_.factory->initialise(staticPtrs, staticReqs);
    }
    
    // Get algorithm requirements and allocate instance memory
    if (!plugin_.factory->calculateRequirements) {
        std::cerr << "Plugin factory missing calculateRequirements function" << std::endl;
        cleanup();
        return false;
    }
    
    _NT_algorithmRequirements reqs = {};
    plugin_.factory->calculateRequirements(reqs, nullptr);
    if (reqs.dram > 0) {
        // Use posix_memalign for better compatibility
        if (posix_memalign(&plugin_.instance_memory, 16, reqs.dram) != 0) {
            std::cerr << "Failed to allocate instance memory" << std::endl;
            cleanup();
            return false;
        }
        
        // Construct the algorithm instance
        if (!plugin_.factory->construct) {
            std::cerr << "Plugin factory missing construct function" << std::endl;
            cleanup();
            return false;
        }
        
        _NT_algorithmMemoryPtrs algPtrs = {};
        algPtrs.dram = (uint8_t*)plugin_.instance_memory;
        plugin_.algorithm = plugin_.factory->construct(algPtrs, reqs, nullptr);
        if (!plugin_.algorithm) {
            std::cerr << "Algorithm construction failed" << std::endl;
            cleanup();
            return false;
        }
    }
    
    plugin_.path = path;
    plugin_.last_modified = getFileModTime(path);
    plugin_.is_loaded = true;
    
    std::cout << "Plugin loaded successfully: " << path << std::endl;
    return true;
}

void PluginLoader::unloadPlugin() {
    if (plugin_.is_loaded && plugin_.algorithm) {
        // No explicit destruct function in new API - algorithm cleanup is automatic
        plugin_.algorithm = nullptr;
    }
    
    // No terminate function in new API
    plugin_.factory = nullptr;
    
    cleanup();
    plugin_.is_loaded = false;
}

bool PluginLoader::validatePlugin(void* handle) {
    // Check for required symbols
    if (!dlsym(handle, "pluginEntry")) {
        std::cerr << "Plugin missing required pluginEntry symbol" << std::endl;
        return false;
    }
    
    return true;
}

void PluginLoader::cleanup() {
    if (plugin_.instance_memory) {
        free(plugin_.instance_memory);
        plugin_.instance_memory = nullptr;
    }
    
    if (plugin_.shared_memory) {
        free(plugin_.shared_memory);
        plugin_.shared_memory = nullptr;
    }
    
    if (plugin_.handle) {
        dlclose(plugin_.handle);
        plugin_.handle = nullptr;
    }
}

time_t PluginLoader::getFileModTime(const std::string& path) const {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

bool PluginLoader::needsReload() const {
    if (!plugin_.is_loaded) return false;
    
    time_t current_mod_time = getFileModTime(plugin_.path);
    return current_mod_time > plugin_.last_modified;
}

bool PluginLoader::reload() {
    if (!plugin_.is_loaded) return false;
    
    std::string path = plugin_.path;
    return loadPlugin(path);
}