#include "plugin_loader.h"
#include <dlfcn.h>
#include <iostream>
#include <sys/stat.h>

typedef _NT_factory* (*FactoryFunc)();

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
    
    // Get the factory function
    FactoryFunc getFactory = (FactoryFunc)dlsym(plugin_.handle, "NT_getFactoryPtr");
    if (!getFactory) {
        std::cerr << "Plugin missing NT_getFactoryPtr symbol: " << dlerror() << std::endl;
        cleanup();
        return false;
    }
    
    plugin_.factory = getFactory();
    if (!plugin_.factory) {
        std::cerr << "Factory function returned null" << std::endl;
        cleanup();
        return false;
    }
    
    // Check API version compatibility
    if (!plugin_.factory->getAPIVersion) {
        std::cerr << "Plugin factory missing getAPIVersion function" << std::endl;
        cleanup();
        return false;
    }
    
    unsigned int apiVersion = plugin_.factory->getAPIVersion(plugin_.factory);
    if (apiVersion > kNT_apiVersion) {
        std::cerr << "Plugin API version " << apiVersion << " is newer than emulator version " << kNT_apiVersion << std::endl;
        cleanup();
        return false;
    }
    
    // Get static requirements and allocate shared memory
    if (!plugin_.factory->getStaticRequirements) {
        std::cerr << "Plugin factory missing getStaticRequirements function" << std::endl;
        cleanup();
        return false;
    }
    
    auto staticReqs = plugin_.factory->getStaticRequirements(plugin_.factory);
    if (staticReqs.memorySize > 0) {
        // Use posix_memalign for better compatibility
        if (posix_memalign(&plugin_.shared_memory, 16, staticReqs.memorySize) != 0) {
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
        
        if (plugin_.factory->initialise(plugin_.factory, plugin_.shared_memory) != 0) {
            std::cerr << "Factory initialization failed" << std::endl;
            cleanup();
            return false;
        }
    }
    
    // Get algorithm requirements and allocate instance memory
    if (!plugin_.factory->getRequirements) {
        std::cerr << "Plugin factory missing getRequirements function" << std::endl;
        cleanup();
        return false;
    }
    
    auto reqs = plugin_.factory->getRequirements(plugin_.factory);
    if (reqs.memorySize > 0) {
        // Use posix_memalign for better compatibility
        if (posix_memalign(&plugin_.instance_memory, 16, reqs.memorySize) != 0) {
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
        
        plugin_.algorithm = plugin_.factory->construct(plugin_.factory, plugin_.instance_memory);
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
    if (plugin_.is_loaded && plugin_.factory && plugin_.algorithm) {
        if (plugin_.factory->destruct) {
            plugin_.factory->destruct(plugin_.factory, plugin_.algorithm);
        }
        plugin_.algorithm = nullptr;
    }
    
    if (plugin_.factory) {
        if (plugin_.factory->terminate) {
            plugin_.factory->terminate(plugin_.factory);
        }
        plugin_.factory = nullptr;
    }
    
    cleanup();
    plugin_.is_loaded = false;
}

bool PluginLoader::validatePlugin(void* handle) {
    // Check for required symbols
    if (!dlsym(handle, "NT_getFactoryPtr")) {
        std::cerr << "Plugin missing required NT_getFactoryPtr symbol" << std::endl;
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