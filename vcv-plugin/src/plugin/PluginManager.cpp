#define _DISTINGNT_SERIALISATION_INTERNAL
#include "PluginManager.hpp"
#include "../parameter/ParameterSystem.hpp"
#include "../json_bridge.h"
#include <rack.hpp>
#include <thread>
#include <chrono>

using namespace rack;

// External NT_screen buffer for symbol resolution
extern uint8_t NT_screen[128 * 64];

// Use proper interface to access thread-local storage (defined in json_bridge.h)
#include "../json_bridge.h"

PluginManager::PluginManager() {
    INFO("PluginManager initialized");
}

PluginManager::~PluginManager() {
    unloadPlugin();
}

bool PluginManager::loadPlugin(const std::string& path) {
    INFO("PluginManager::loadPlugin called with path: %s", path.c_str());
    try {
        unloadPlugin();
        
        // Platform-specific loading
        #ifdef ARCH_WIN
            pluginHandle = LoadLibraryA(path.c_str());
        #else
            // First, ensure our own symbols are globally available
            // Get path to our own plugin
            Dl_info info;
            if (dladdr((void*)&NT_screen, &info) && info.dli_fname) {
                INFO("Our plugin path: %s", info.dli_fname);
                // Reopen our own plugin with RTLD_GLOBAL to export symbols
                void* ourHandle = dlopen(info.dli_fname, RTLD_NOW | RTLD_GLOBAL);
                INFO("Reopened our plugin with RTLD_GLOBAL: %p", ourHandle);
            } else {
                INFO("Could not determine our plugin path");
            }
            
            // Check if we can find our own symbol now
            void* testSymbol = dlsym(RTLD_DEFAULT, "_NT_screen");
            INFO("Found _NT_screen symbol: %p", testSymbol);
            
            pluginHandle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        #endif
        
        if (!pluginHandle) {
            std::string error;
            #ifdef ARCH_WIN
                error = "Windows error"; // GetLastError() formatting
            #else
                const char* dlerr = dlerror();
                error = dlerr ? std::string(dlerr) : "Unknown error";
            #endif
            WARN("Failed to load plugin from path '%s': %s", path.c_str(), error.c_str());
            loadingMessage = "Error: Failed to load plugin - " + error;
            loadingMessageTimer = 4.0f;
            notifyError(error);
            return false;
        }
        
        // Try new API first (pluginEntry)
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector selector, uint32_t data);
        PluginEntryFunc pluginEntry;
        #ifdef ARCH_WIN
            pluginEntry = (PluginEntryFunc)GetProcAddress((HMODULE)pluginHandle, "pluginEntry");
        #else
            pluginEntry = (PluginEntryFunc)dlsym(pluginHandle, "pluginEntry");
        #endif
        
        if (!pluginEntry) {
            // Fall back to legacy API
            typedef _NT_factory* (*LegacyFactoryFunc)();
            LegacyFactoryFunc legacyFactory;
            #ifdef ARCH_WIN
                legacyFactory = (LegacyFactoryFunc)GetProcAddress((HMODULE)pluginHandle, "NT_factory");
            #else
                legacyFactory = (LegacyFactoryFunc)dlsym(pluginHandle, "NT_factory");
            #endif
            
            if (!legacyFactory) {
                WARN("Plugin '%s' does not export pluginEntry or NT_factory function", path.c_str());
                loadingMessage = "Error: Plugin missing required functions";
                loadingMessageTimer = 4.0f;
                unloadPlugin();
                notifyError("Plugin missing required functions");
                return false;
            }
            
            pluginFactory = legacyFactory();
        } else {
            // Use new API
            pluginFactory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
        }
        
        if (!validatePlugin()) {
            unloadPlugin();
            return false;
        }
        
        if (!initializePlugin()) {
            unloadPlugin();
            return false;
        }
        
        pluginPath = path;
        loadingMessage = "Plugin loaded successfully";
        loadingMessageTimer = 2.0f;
        
        // NOTE: setupUi should only be called when exiting parameter menu or on initial load
        // The module (NtEmu) will handle calling setupUi with actual pot values after load completes
        
        notifyLoaded();
        INFO("Successfully loaded plugin: %s", path.c_str());
        return true;
        
    } catch (const std::exception& e) {
        WARN("Exception loading plugin '%s': %s", path.c_str(), e.what());
        loadingMessage = "Error: Exception loading plugin - " + std::string(e.what());
        loadingMessageTimer = 4.0f;
        unloadPlugin();
        notifyError(e.what());
        return false;
    } catch (...) {
        WARN("Unknown exception loading plugin '%s'", path.c_str());
        loadingMessage = "Error: Unknown exception loading plugin";
        loadingMessageTimer = 4.0f;
        unloadPlugin();
        notifyError("Unknown exception loading plugin");
        return false;
    }
}

bool PluginManager::loadPlugin(const std::string& path, const std::vector<int32_t>& customSpecifications) {
    INFO("Loading plugin with %zu custom specifications", customSpecifications.size());
    for (size_t i = 0; i < customSpecifications.size(); i++) {
        INFO("  Custom spec[%zu] = %d", i, customSpecifications[i]);
    }
    
    // Store specifications in member variables for use during loading
    currentSpecifications = customSpecifications;
    useCustomSpecifications = true;
    
    // Call the main loading method
    bool result = loadPlugin(path);
    
    // Reset custom specification mode
    useCustomSpecifications = false;
    currentSpecifications.clear();
    
    return result;
}

void PluginManager::unloadPlugin() {
    INFO("Unloading plugin");
    
    cleanupPlugin();
    
    pluginAlgorithm = nullptr;
    pluginFactory = nullptr;
    
    if (pluginInstanceMemory) {
        std::free(pluginInstanceMemory);
        pluginInstanceMemory = nullptr;
    }
    
    if (pluginSharedMemory) {
        std::free(pluginSharedMemory);
        pluginSharedMemory = nullptr;
    }
    
    if (pluginHandle) {
        #ifdef ARCH_WIN
            FreeLibrary((HMODULE)pluginHandle);
        #else
            dlclose(pluginHandle);
        #endif
        pluginHandle = nullptr;
    }
    
    pluginPath.clear();
    pluginSpecifications.clear();
    
    notifyUnloaded();
}

void PluginManager::reloadPlugin() {
    if (pluginPath.empty()) {
        WARN("Cannot reload plugin: no plugin path stored");
        return;
    }
    
    std::string currentPath = pluginPath;
    std::vector<int32_t> currentSpecs = pluginSpecifications;
    
    unloadPlugin();
    
    // Small delay to allow system to fully unload the library
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Reload with same specifications if they exist
    if (!currentSpecs.empty()) {
        if (loadPlugin(currentPath, currentSpecs)) {
            INFO("Successfully reloaded plugin with specifications");
        } else {
            WARN("Failed to reload plugin with specifications");
        }
    } else {
        if (loadPlugin(currentPath)) {
            INFO("Successfully reloaded plugin");
        } else {
            WARN("Failed to reload plugin");
        }
    }
}

bool PluginManager::isLoaded() const {
    return pluginHandle && pluginFactory && pluginAlgorithm;
}

void PluginManager::addObserver(IPluginStateObserver* observer) {
    if (observer) {
        observers.push_back(observer);
    }
}

void PluginManager::removeObserver(IPluginStateObserver* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void PluginManager::updateLoadingTimer(float deltaTime) {
    if (loadingMessageTimer > 0.0f) {
        loadingMessageTimer -= deltaTime;
    }
}

bool PluginManager::validatePlugin() {
    if (!pluginFactory) {
        WARN("Plugin factory is null");
        loadingMessage = "Error: Invalid plugin factory";
        loadingMessageTimer = 4.0f;
        notifyError("Invalid plugin factory");
        return false;
    }
    
    if (!pluginFactory->construct) {
        WARN("Plugin factory missing construct function");
        loadingMessage = "Error: Plugin missing construct function";
        loadingMessageTimer = 4.0f;
        notifyError("Plugin missing construct function");
        return false;
    }
    
    return true;
}

bool PluginManager::initializePlugin() {
    try {
        // Calculate memory requirements
        _NT_staticRequirements staticReqs;
        _NT_algorithmRequirements algorithmReqs;
        memset(&staticReqs, 0, sizeof(staticReqs));
        memset(&algorithmReqs, 0, sizeof(algorithmReqs));
        
        // Get specifications for requirement calculation
        std::vector<int32_t> specValues;
        const int32_t* specifications = nullptr;
        
        if (pluginFactory->numSpecifications > 0 && pluginFactory->specifications) {
            if (useCustomSpecifications && !currentSpecifications.empty()) {
                specValues = currentSpecifications;
                pluginSpecifications = currentSpecifications; // Store for later use
            } else {
                // Use default specifications
                specValues.resize(pluginFactory->numSpecifications);
                for (uint32_t i = 0; i < pluginFactory->numSpecifications; i++) {
                    specValues[i] = pluginFactory->specifications[i].def;
                }
                pluginSpecifications = specValues; // Store for later use
            }
            specifications = specValues.data();
        }
        
        // Calculate requirements
        if (pluginFactory->calculateRequirements) {
            pluginFactory->calculateRequirements(algorithmReqs, specifications);
        }
        
        // For now, create a simple memory allocation strategy
        // In a full implementation, we'd allocate separate SRAM, DRAM, DTC, ITC regions
        uint32_t totalMemoryNeeded = algorithmReqs.sram + algorithmReqs.dram + algorithmReqs.dtc + algorithmReqs.itc;
        
        if (totalMemoryNeeded > 0) {
            pluginInstanceMemory = std::malloc(totalMemoryNeeded);
            if (!pluginInstanceMemory) {
                loadingMessage = "Error: Failed to allocate instance memory";
                loadingMessageTimer = 4.0f;
                notifyError("Failed to allocate instance memory");
                return false;
            }
            memset(pluginInstanceMemory, 0, totalMemoryNeeded);
            
            // TODO: Implement proper memory region allocation for SRAM, DRAM, DTC, ITC
        }
        
        // Construct algorithm with proper memory pointers structure
        _NT_algorithmMemoryPtrs memoryPtrs;
        memoryPtrs.sram = (uint8_t*)pluginInstanceMemory;
        memoryPtrs.dram = (uint8_t*)pluginInstanceMemory; // Simplified - same allocation
        memoryPtrs.dtc = (uint8_t*)pluginInstanceMemory;  // Simplified - same allocation
        memoryPtrs.itc = (uint8_t*)pluginInstanceMemory;  // Simplified - same allocation
        
        pluginAlgorithm = pluginFactory->construct(memoryPtrs, algorithmReqs, specifications);
        if (!pluginAlgorithm || !isValidPointer(pluginAlgorithm)) {
            loadingMessage = "Error: Failed to construct algorithm";
            loadingMessageTimer = 4.0f;
            notifyError("Failed to construct algorithm");
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        loadingMessage = "Error: Exception during plugin initialization - " + std::string(e.what());
        loadingMessageTimer = 4.0f;
        notifyError(e.what());
        return false;
    } catch (...) {
        loadingMessage = "Error: Unknown exception during plugin initialization";
        loadingMessageTimer = 4.0f;
        notifyError("Unknown exception during plugin initialization");
        return false;
    }
}

void PluginManager::cleanupPlugin() {
    // The NT API doesn't have an explicit destruct method
    // Algorithm cleanup is handled through memory deallocation
    if (pluginAlgorithm) {
        // Plugin algorithm will be cleaned up when memory is freed
        INFO("Cleaning up plugin algorithm");
    }
}

void PluginManager::notifyLoaded() {
    for (auto* observer : observers) {
        observer->onPluginLoaded(pluginPath);
    }
}

void PluginManager::notifyUnloaded() {
    for (auto* observer : observers) {
        observer->onPluginUnloaded();
    }
}

void PluginManager::restoreParameterValues(json_t* parameterValuesJ, ParameterSystem* parameterSystem) {
    if (!parameterValuesJ || !parameterSystem || !isLoaded()) {
        return;
    }
    
    INFO("PluginManager: Loading parameter values");
    // Note: extractParameterData() should already have been called during onPluginLoaded
    parameterSystem->loadParameterValues(parameterValuesJ);
}

void PluginManager::restorePluginState(const std::string& pluginStateJson) {
    if (pluginStateJson.empty() || !pluginAlgorithm || !pluginFactory || !pluginFactory->deserialise) {
        INFO("PluginManager: Skipping plugin state restoration - empty state or missing components");
        return;
    }
    
    try {
        INFO("PluginManager: Restoring plugin state: %s", pluginStateJson.c_str());
        
        // Parse JSON first to validate it
        nlohmann::json pluginJson;
        try {
            pluginJson = nlohmann::json::parse(pluginStateJson);
        } catch (const nlohmann::json::parse_error& e) {
            WARN("PluginManager: Invalid JSON in plugin state: %s", e.what());
            return;
        }
        
        // Validate pointers before use
        if (!pluginFactory || !pluginAlgorithm) {
            WARN("PluginManager: Plugin factory or algorithm is null");
            return;
        }
        
        INFO("PluginManager: Setting up JSON parse bridge");
        setCurrentJsonParse(std::unique_ptr<JsonParseBridge>(new JsonParseBridge(pluginJson)));
        
        // Verify the bridge was set correctly
        if (!getCurrentJsonParse()) {
            WARN("PluginManager: Failed to set JSON parse bridge");
            return;
        }
        
        _NT_jsonParse dummy_parse(nullptr, 0);
        
        INFO("PluginManager: Calling plugin deserialise method");
        INFO("PluginManager: pluginFactory=%p, deserialise=%p, pluginAlgorithm=%p", 
             (void*)pluginFactory, 
             (void*)(pluginFactory ? pluginFactory->deserialise : nullptr), 
             (void*)pluginAlgorithm);
        bool success = pluginFactory->deserialise(pluginAlgorithm, dummy_parse);
        INFO("PluginManager: Plugin state restored: %s", success ? "true" : "false");
        
        clearCurrentJsonParse();
    } catch (const std::exception& e) {
        WARN("PluginManager: Plugin state restoration failed: %s", e.what());
        if (getCurrentJsonParse()) {
            clearCurrentJsonParse();
        }
    }
}

void PluginManager::callSetupUi(float potValues[3]) {
    if (!pluginFactory || !pluginFactory->setupUi || !pluginAlgorithm) {
        INFO("PluginManager: callSetupUi skipped - missing components (factory=%p, setupUi=%p, algorithm=%p)", 
             (void*)pluginFactory, (void*)(pluginFactory ? pluginFactory->setupUi : nullptr), (void*)pluginAlgorithm);
        return;
    }
    
    // Critical safety check: ensure algorithm->v is initialized before calling setupUi
    if (!pluginAlgorithm->v) {
        WARN("PluginManager: callSetupUi skipped - algorithm->v is null (parameter system not initialized)");
        INFO("PluginManager: Algorithm address: %p, algorithm->v: %p", (void*)pluginAlgorithm, (void*)pluginAlgorithm->v);
        return;
    }
    
    try {
        INFO("PluginManager: Calling setupUi with pot values: %.3f %.3f %.3f", 
             potValues[0], potValues[1], potValues[2]);
        INFO("PluginManager: Algorithm address: %p, algorithm->v: %p", (void*)pluginAlgorithm, (void*)pluginAlgorithm->v);
        // Cast float array to _NT_float3 reference
        pluginFactory->setupUi(pluginAlgorithm, *reinterpret_cast<_NT_float3*>(potValues));
        INFO("PluginManager: setupUi returned pot values: %.3f %.3f %.3f", 
             potValues[0], potValues[1], potValues[2]);
    } catch (...) {
        WARN("PluginManager: Plugin crashed during setupUi");
    }
}

void PluginManager::initializeParameterSystem(ParameterSystem* parameterSystem) {
    if (!parameterSystem || !pluginAlgorithm || !pluginFactory) {
        INFO("PluginManager: initializeParameterSystem skipped - missing components");
        return;
    }
    
    INFO("PluginManager: Initializing parameter system");
    parameterSystem->extractParameterData();
}

void PluginManager::notifyError(const std::string& error) {
    for (auto* observer : observers) {
        observer->onPluginError(error);
    }
}

bool PluginManager::isValidPointer(void* ptr) const {
    if (!ptr) return false;
    
    // Basic pointer validation - check if it's in a reasonable address range
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    // Check for null and obviously invalid addresses
    if (addr < 0x1000) return false;  // Below 4KB is likely invalid
    
#ifdef ARCH_WIN
    // On Windows, user space is typically below 0x7FFFFFFFFFFF
    if (addr > 0x7FFFFFFFFFFF) return false;
#else
    // On Unix-like systems, check for reasonable user space addresses
    if (addr > 0x7FFFFFFFFFFF) return false;
#endif
    
    return true;
}