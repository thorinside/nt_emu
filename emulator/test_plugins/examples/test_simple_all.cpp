#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <distingnt/api.h>

int main() {
    std::cout << "=== Simple All Plugin Test ===" << std::endl;
    
    std::vector<std::string> pluginNames = {
        "gain", "multiple", "monosynth", "fourteen"
    };
    
    for (const auto& name : pluginNames) {
        std::cout << "\n--- Testing " << name << " ---" << std::endl;
        
        // Load plugin
        std::string pluginPath = name + ".dylib";
        void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cout << "✗ Failed to load " << name << ": " << dlerror() << std::endl;
            continue;
        }
        
        std::cout << "✓ Loaded " << name << std::endl;
        
        // Get plugin entry
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(handle, "pluginEntry");
        
        if (!pluginEntry) {
            std::cout << "✗ No pluginEntry function" << std::endl;
            dlclose(handle);
            continue;
        }
        
        // Get factory
        _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
        if (!factory) {
            std::cout << "✗ No factory" << std::endl;
            dlclose(handle);
            continue;
        }
        
        // Display GUID as four character ASCII
        char guidStr[5] = {0};
        guidStr[0] = (factory->guid >> 0) & 0xFF;
        guidStr[1] = (factory->guid >> 8) & 0xFF;
        guidStr[2] = (factory->guid >> 16) & 0xFF;
        guidStr[3] = (factory->guid >> 24) & 0xFF;
        
        std::cout << "✓ GUID: " << guidStr << " - " << factory->name << std::endl;
        
        // Test basic construction
        if (factory->calculateRequirements && factory->construct) {
            _NT_algorithmRequirements reqs = {0};
            factory->calculateRequirements(reqs, nullptr);
            
            if (reqs.sram > 0) {
                void* memory = malloc(reqs.sram + 1024);  // Extra safety buffer
                if (memory) {
                    memset(memory, 0, reqs.sram + 1024);
                    
                    _NT_algorithmMemoryPtrs memPtrs = {0};
                    memPtrs.sram = (uint8_t*)memory;
                    memPtrs.dram = memPtrs.sram + reqs.sram;
                    
                    _NT_algorithm* algorithm = factory->construct(memPtrs, reqs, nullptr);
                    if (algorithm) {
                        std::cout << "✓ Construction successful" << std::endl;
                        
                        // Test parameter access
                        try {
                            uintptr_t paramAddr = (uintptr_t)algorithm->parameters;
                            if (paramAddr == 0x3f800000 || 
                                (paramAddr >= 0x3f000000 && paramAddr <= 0x40000000)) {
                                std::cout << "✗ Found corrupted parameters address!" << std::endl;
                            } else {
                                std::cout << "✓ Parameters look valid" << std::endl;
                            }
                        } catch (...) {
                            std::cout << "✗ Exception accessing parameters" << std::endl;
                        }
                    } else {
                        std::cout << "✗ Construction failed" << std::endl;
                    }
                    
                    free(memory);
                }
            } else {
                std::cout << "⚠ Plugin requires 0 SRAM - skipping construction test" << std::endl;
            }
        } else {
            std::cout << "⚠ Missing required functions - skipping construction test" << std::endl;
        }
        
        dlclose(handle);
    }
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    return 0;
}