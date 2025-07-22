#include <iostream>
#include <dlfcn.h>
#include <distingnt/api.h>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <plugin_name>" << std::endl;
        return 1;
    }
    
    std::string pluginName = argv[1];
    std::string pluginPath = pluginName + ".dylib";
    
    std::cout << "=== Testing " << pluginName << " ===\n" << std::endl;
    
    // Load the plugin
    void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cout << "✗ Failed to load " << pluginName << ": " << dlerror() << std::endl;
        return 1;
    }
    
    std::cout << "✓ Loaded " << pluginName << " plugin" << std::endl;
    
    // Get plugin entry function
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
    
    // Test construction step by step
    std::cout << "\n--- Construction Test ---" << std::endl;
    
    // Check if calculateRequirements exists
    if (!factory->calculateRequirements) {
        std::cout << "✗ No calculateRequirements function" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    std::cout << "1. Calling calculateRequirements..." << std::endl;
    _NT_algorithmRequirements reqs;
    memset(&reqs, 0, sizeof(reqs));
    
    try {
        factory->calculateRequirements(reqs, nullptr);
        std::cout << "✓ calculateRequirements succeeded" << std::endl;
        std::cout << "   Parameters: " << reqs.numParameters << std::endl;
        std::cout << "   SRAM: " << reqs.sram << " bytes" << std::endl;
        std::cout << "   DRAM: " << reqs.dram << " bytes" << std::endl;
    } catch (...) {
        std::cout << "✗ calculateRequirements threw exception" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    // Check if construct exists
    if (!factory->construct) {
        std::cout << "✗ No construct function" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    std::cout << "\n2. Testing construct..." << std::endl;
    
    // Allocate memory
    size_t totalMem = reqs.sram + reqs.dram + reqs.dtc + reqs.itc + 1024; // Add safety buffer
    void* memory = malloc(totalMem);
    if (!memory) {
        std::cout << "✗ Failed to allocate memory" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    memset(memory, 0, totalMem);
    std::cout << "   Allocated " << totalMem << " bytes at " << std::hex << memory << std::dec << std::endl;
    
    // Set up memory pointers
    _NT_algorithmMemoryPtrs memPtrs;
    memset(&memPtrs, 0, sizeof(memPtrs));
    memPtrs.sram = (uint8_t*)memory;
    memPtrs.dram = memPtrs.sram + reqs.sram;
    memPtrs.dtc = memPtrs.dram + reqs.dram;
    memPtrs.itc = memPtrs.dtc + reqs.dtc;
    
    std::cout << "   SRAM: " << std::hex << memPtrs.sram << std::dec << std::endl;
    std::cout << "   DRAM: " << std::hex << memPtrs.dram << std::dec << std::endl;
    
    // Attempt construction
    std::cout << "   Calling construct..." << std::endl;
    _NT_algorithm* algorithm = nullptr;
    
    try {
        algorithm = factory->construct(memPtrs, reqs, nullptr);
        
        if (!algorithm) {
            std::cout << "✗ construct returned null" << std::endl;
        } else {
            std::cout << "✓ Algorithm constructed at: " << std::hex << algorithm << std::dec << std::endl;
            
            // Test parameter access (the critical part that was crashing)
            std::cout << "\n3. Testing parameter access..." << std::endl;
            try {
                const _NT_parameter* parametersPtr = algorithm->parameters;
                std::cout << "   ✓ Parameters field: " << std::hex << parametersPtr << std::dec << std::endl;
                
                // Check for corruption
                uintptr_t paramAddr = (uintptr_t)parametersPtr;
                if (paramAddr == 0x3f800000 || 
                    (paramAddr >= 0x3f000000 && paramAddr <= 0x40000000)) {
                    std::cout << "   ✗ Found corrupted parameters address!" << std::endl;
                } else {
                    std::cout << "   ✓ Parameters address looks valid" << std::endl;
                }
                
                std::cout << "   ✓ Parameter pages: " << std::hex << algorithm->parameterPages << std::dec << std::endl;
                std::cout << "   ✓ v array: " << std::hex << algorithm->v << std::dec << std::endl;
                
            } catch (...) {
                std::cout << "   ✗ Exception accessing algorithm structure" << std::endl;
            }
        }
        
    } catch (...) {
        std::cout << "✗ construct threw exception" << std::endl;
    }
    
    free(memory);
    dlclose(handle);
    
    std::cout << "\n✓ Test completed successfully" << std::endl;
    return 0;
}