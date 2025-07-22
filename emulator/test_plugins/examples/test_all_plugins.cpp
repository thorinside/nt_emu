#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <distingnt/api.h>

class PluginTester {
private:
    struct PluginInfo {
        std::string name;
        void* handle;
        _NT_factory* factory;
        _NT_algorithm* algorithm;
        void* memory;
        
        PluginInfo() : handle(nullptr), factory(nullptr), algorithm(nullptr), memory(nullptr) {}
        ~PluginInfo() {
            if (memory) free(memory);
            if (handle) dlclose(handle);
        }
    };
    
    std::vector<PluginInfo> plugins;
    
public:
    ~PluginTester() {
        cleanup();
    }
    
    void cleanup() {
        plugins.clear();
    }
    
    bool loadPlugin(const std::string& name) {
        std::string pluginPath = name + ".dylib";
        
        std::cout << "Loading " << name << "..." << std::endl;
        
        PluginInfo info;
        info.name = name;
        
        // Load the plugin
        info.handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!info.handle) {
            std::cout << "✗ Failed to load " << name << ": " << dlerror() << std::endl;
            return false;
        }
        
        // Get the plugin entry function
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(info.handle, "pluginEntry");
        
        if (!pluginEntry) {
            std::cout << "✗ No pluginEntry function in " << name << std::endl;
            return false;
        }
        
        // Test API version
        uintptr_t version = pluginEntry(kNT_selector_version, 0);
        if (version != kNT_apiVersionCurrent) {
            std::cout << "⚠ " << name << " uses API version " << version 
                     << " (current: " << kNT_apiVersionCurrent << ")" << std::endl;
        }
        
        // Get number of factories
        uintptr_t numFactories = pluginEntry(kNT_selector_numFactories, 0);
        if (numFactories == 0) {
            std::cout << "✗ " << name << " reports 0 factories" << std::endl;
            return false;
        }
        
        // Get first factory
        info.factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
        if (!info.factory) {
            std::cout << "✗ " << name << " returned null factory" << std::endl;
            return false;
        }
        
        std::cout << "✓ " << name << " loaded successfully" << std::endl;
        
        // Display GUID as four character ASCII
        char guidStr[5] = {0};
        guidStr[0] = (info.factory->guid >> 0) & 0xFF;
        guidStr[1] = (info.factory->guid >> 8) & 0xFF;
        guidStr[2] = (info.factory->guid >> 16) & 0xFF;
        guidStr[3] = (info.factory->guid >> 24) & 0xFF;
        
        std::cout << "  GUID: " << guidStr << std::endl;
        std::cout << "  Name: " << (info.factory->name ? info.factory->name : "NULL") << std::endl;
        std::cout << "  Description: " << (info.factory->description ? info.factory->description : "NULL") << std::endl;
        
        plugins.push_back(std::move(info));
        return true;
    }
    
    bool testPluginConstruction(size_t pluginIndex) {
        std::cout << "Entering testPluginConstruction(" << pluginIndex << ")" << std::flush;
        if (pluginIndex >= plugins.size()) {
            std::cout << " - invalid index" << std::endl;
            return false;
        }
        
        PluginInfo& info = plugins[pluginIndex];
        std::cout << "\nTesting " << info.name << " construction..." << std::flush;
        
        if (!info.factory->calculateRequirements) {
            std::cout << "✗ No calculateRequirements function" << std::endl;
            return false;
        }
        
        if (!info.factory->construct) {
            std::cout << "✗ No construct function" << std::endl;
            return false;
        }
        
        // Test static requirements if available
        if (info.factory->calculateStaticRequirements) {
            std::cout << "    Testing calculateStaticRequirements..." << std::endl;
            _NT_staticRequirements staticReqs = {0};
            info.factory->calculateStaticRequirements(staticReqs);
            std::cout << "    Static DRAM: " << staticReqs.dram << " bytes" << std::endl;
            
            // Initialize if needed
            if (staticReqs.dram > 0 && info.factory->initialise) {
                std::cout << "    Calling initialise..." << std::endl;
                void* staticMemory = malloc(staticReqs.dram);
                if (staticMemory) {
                    memset(staticMemory, 0, staticReqs.dram);
                    _NT_staticMemoryPtrs staticPtrs = { .dram = (uint8_t*)staticMemory };
                    info.factory->initialise(staticPtrs, staticReqs);
                    free(staticMemory);  // Clean up immediately for testing
                    std::cout << "    ✓ Initialise completed" << std::endl;
                }
            }
        } else {
            std::cout << "    No calculateStaticRequirements" << std::endl;
        }
        
        // Calculate requirements
        std::cout << "    Calling calculateRequirements..." << std::flush;
        _NT_algorithmRequirements reqs;
        memset(&reqs, 0, sizeof(reqs));
        info.factory->calculateRequirements(reqs, nullptr);
        std::cout << " ✓ Done" << std::endl;
        
        std::cout << "  Requirements:" << std::endl;
        std::cout << "    Parameters: " << reqs.numParameters << std::endl;
        std::cout << "    SRAM: " << reqs.sram << " bytes" << std::endl;
        std::cout << "    DRAM: " << reqs.dram << " bytes" << std::endl;
        
        // Allocate memory
        size_t totalMem = std::max((size_t)(reqs.sram + reqs.dram + reqs.dtc + reqs.itc), (size_t)1024);
        info.memory = malloc(totalMem);
        if (!info.memory) {
            std::cout << "✗ Failed to allocate memory" << std::endl;
            return false;
        }
        
        memset(info.memory, 0, totalMem);
        
        // Set up memory pointers
        _NT_algorithmMemoryPtrs memPtrs{};
        memPtrs.sram = (uint8_t*)info.memory;
        memPtrs.dram = memPtrs.sram + reqs.sram;
        memPtrs.dtc = memPtrs.dram + reqs.dram;
        memPtrs.itc = memPtrs.dtc + reqs.dtc;
        
        // Construct algorithm
        std::cout << "    Calling construct..." << std::flush;
        info.algorithm = info.factory->construct(memPtrs, reqs, nullptr);
        std::cout << " ✓ Done" << std::endl;
        
        if (!info.algorithm) {
            std::cout << "✗ Algorithm construction failed" << std::endl;
            return false;
        }
        
        std::cout << "✓ Algorithm constructed at: " << std::hex << info.algorithm << std::dec << std::endl;
        
        return testParameterAccess(pluginIndex);
    }
    
    bool testParameterAccess(size_t pluginIndex) {
        if (pluginIndex >= plugins.size()) return false;
        
        PluginInfo& info = plugins[pluginIndex];
        std::cout << "  Testing parameter access..." << std::endl;
        
        // Test parameter structure access (this was the source of our crashes)
        if (!info.algorithm) {
            std::cout << "    ✗ No algorithm available" << std::endl;
            return false;
        }
        
        try {
            // Test reading the parameters field
            const _NT_parameter* parametersPtr = info.algorithm->parameters;
            std::cout << "    ✓ Parameters field accessible: " << std::hex << parametersPtr << std::dec << std::endl;
            
            // Check for corruption (the issue we fixed)
            uintptr_t paramAddr = (uintptr_t)parametersPtr;
            if (paramAddr == 0x3f800000 || 
                (paramAddr >= 0x3f000000 && paramAddr <= 0x40000000)) {
                std::cout << "    ✗ Found corrupted parameters address!" << std::endl;
                return false;
            }
            
            // Test other fields
            std::cout << "    ✓ Parameter pages: " << std::hex << info.algorithm->parameterPages << std::dec << std::endl;
            std::cout << "    ✓ v array: " << std::hex << info.algorithm->v << std::dec << std::endl;
            
            return true;
            
        } catch (...) {
            std::cout << "    ✗ Exception accessing algorithm structure" << std::endl;
            return false;
        }
    }
    
    void runAllTests() {
        std::cout << "=== Example Plugin Compatibility Test Suite ===" << std::endl;
        
        std::vector<std::string> pluginNames = {
            "gain"  // Start with just one plugin to debug
        };
        
        // Load all plugins
        std::cout << "\n--- Loading Plugins ---" << std::endl;
        size_t loadedCount = 0;
        for (const auto& name : pluginNames) {
            if (loadPlugin(name)) {
                loadedCount++;
            }
        }
        
        std::cout << "\nLoaded " << loadedCount << "/" << pluginNames.size() << " plugins successfully" << std::endl;
        
        // Test construction and parameter access
        std::cout << "\n--- Testing Plugin Construction ---" << std::endl;
        size_t passedCount = 0;
        for (size_t i = 0; i < plugins.size(); i++) {
            if (testPluginConstruction(i)) {
                passedCount++;
            }
        }
        
        std::cout << "\n=== Results ===" << std::endl;
        std::cout << "Loaded: " << loadedCount << "/" << pluginNames.size() << " plugins" << std::endl;
        std::cout << "Passed: " << passedCount << "/" << loadedCount << " construction tests" << std::endl;
        
        if (passedCount == loadedCount && loadedCount == pluginNames.size()) {
            std::cout << "\n🎉 ALL TESTS PASSED! No API drift detected." << std::endl;
        } else {
            std::cout << "\n⚠ Some tests failed - further investigation needed." << std::endl;
        }
    }
};

int main() {
    PluginTester tester;
    tester.runAllTests();
    return 0;
}