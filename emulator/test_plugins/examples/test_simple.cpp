#include <iostream>
#include <dlfcn.h>
#include <distingnt/api.h>

int main() {
    std::cout << "=== Simple Plugin Test ===" << std::endl;
    
    // Load gain plugin
    void* handle = dlopen("./gain.dylib", RTLD_LAZY);
    if (!handle) {
        std::cout << "✗ Failed to load gain: " << dlerror() << std::endl;
        return 1;
    }
    
    std::cout << "✓ Loaded gain plugin" << std::endl;
    
    // Get plugin entry
    typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
    PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(handle, "pluginEntry");
    
    if (!pluginEntry) {
        std::cout << "✗ No pluginEntry function" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    // Get factory
    _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
    if (!factory) {
        std::cout << "✗ No factory" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    std::cout << "✓ Got factory: " << factory->name << std::endl;
    
    // Display GUID as four character ASCII
    char guidStr[5] = {0};
    guidStr[0] = (factory->guid >> 0) & 0xFF;
    guidStr[1] = (factory->guid >> 8) & 0xFF;
    guidStr[2] = (factory->guid >> 16) & 0xFF;
    guidStr[3] = (factory->guid >> 24) & 0xFF;
    std::cout << "  GUID: " << guidStr << std::endl;
    
    // Test static requirements
    if (factory->calculateStaticRequirements) {
        std::cout << "  Has calculateStaticRequirements" << std::endl;
        _NT_staticRequirements staticReqs = {0};
        factory->calculateStaticRequirements(staticReqs);
        std::cout << "  Static DRAM: " << staticReqs.dram << " bytes" << std::endl;
    } else {
        std::cout << "  No calculateStaticRequirements" << std::endl;
    }
    
    // Test algorithm requirements
    if (factory->calculateRequirements) {
        std::cout << "  Testing calculateRequirements..." << std::endl;
        _NT_algorithmRequirements reqs = {0};
        factory->calculateRequirements(reqs, nullptr);
        std::cout << "  ✓ Calculated requirements:" << std::endl;
        std::cout << "    Parameters: " << reqs.numParameters << std::endl;
        std::cout << "    SRAM: " << reqs.sram << " bytes" << std::endl;
    } else {
        std::cout << "  ✗ No calculateRequirements" << std::endl;
    }
    
    dlclose(handle);
    std::cout << "\n✓ Test completed successfully" << std::endl;
    return 0;
}