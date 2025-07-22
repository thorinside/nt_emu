#include <iostream>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include "../emulator/include/distingnt/api.h"

int main() {
    std::cout << "=== Fourteen Plugin DTC Debug Test ===" << std::endl;
    
    // Load fourteen plugin
    void* handle = dlopen("./fourteen.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cout << "✗ Failed to load fourteen.dylib: " << dlerror() << std::endl;
        return 1;
    }
    
    std::cout << "✓ Loaded fourteen.dylib" << std::endl;
    
    typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
    PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(handle, "pluginEntry");
    if (!pluginEntry) {
        std::cout << "✗ No pluginEntry function" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
    if (!factory) {
        std::cout << "✗ No factory" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    // Display GUID
    char guidStr[5] = {0};
    guidStr[0] = (factory->guid >> 0) & 0xFF;
    guidStr[1] = (factory->guid >> 8) & 0xFF;
    guidStr[2] = (factory->guid >> 16) & 0xFF;
    guidStr[3] = (factory->guid >> 24) & 0xFF;
    std::cout << "✓ Plugin: " << guidStr << " - " << factory->name << std::endl;
    
    // Test calculateRequirements
    _NT_algorithmRequirements reqs;
    memset(&reqs, 0, sizeof(reqs));
    
    if (!factory->calculateRequirements) {
        std::cout << "✗ No calculateRequirements function" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    factory->calculateRequirements(reqs, nullptr);
    std::cout << "Memory Requirements:" << std::endl;
    std::cout << "  SRAM: " << reqs.sram << " bytes" << std::endl;
    std::cout << "  DRAM: " << reqs.dram << " bytes" << std::endl;
    std::cout << "  DTC:  " << reqs.dtc << " bytes" << std::endl;
    std::cout << "  ITC:  " << reqs.itc << " bytes" << std::endl;
    std::cout << "  Parameters: " << reqs.numParameters << std::endl;
    
    // Allocate memory exactly like VCV plugin
    size_t totalMem = reqs.sram + reqs.dram + reqs.dtc + reqs.itc;
    if (totalMem == 0) {
        std::cout << "✗ No memory required - this seems wrong" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    void* memory = nullptr;
    if (posix_memalign(&memory, 16, totalMem) != 0) {
        std::cout << "✗ Memory allocation failed" << std::endl;
        dlclose(handle);
        return 1;
    }
    
    // Zero out all memory
    memset(memory, 0, totalMem);
    std::cout << "✓ Allocated " << totalMem << " bytes at " << std::hex << memory << std::dec << std::endl;
    
    // Set up memory pointers exactly like VCV plugin
    _NT_algorithmMemoryPtrs memPtrs;
    memset(&memPtrs, 0, sizeof(memPtrs));
    uint8_t* ptr = (uint8_t*)memory;
    memPtrs.sram = ptr; ptr += reqs.sram;
    memPtrs.dram = ptr; ptr += reqs.dram;
    memPtrs.dtc = ptr;  ptr += reqs.dtc;
    memPtrs.itc = ptr;
    
    std::cout << "Memory Layout:" << std::endl;
    std::cout << "  Base:  " << std::hex << memory << std::dec << std::endl;
    std::cout << "  SRAM:  " << std::hex << memPtrs.sram << std::dec << " (+" << ((uint8_t*)memPtrs.sram - (uint8_t*)memory) << ")" << std::endl;
    std::cout << "  DRAM:  " << std::hex << memPtrs.dram << std::dec << " (+" << ((uint8_t*)memPtrs.dram - (uint8_t*)memory) << ")" << std::endl;
    std::cout << "  DTC:   " << std::hex << memPtrs.dtc << std::dec << " (+" << ((uint8_t*)memPtrs.dtc - (uint8_t*)memory) << ")" << std::endl;
    std::cout << "  ITC:   " << std::hex << memPtrs.itc << std::dec << " (+" << ((uint8_t*)memPtrs.itc - (uint8_t*)memory) << ")" << std::endl;
    
    // Test construct
    if (!factory->construct) {
        std::cout << "✗ No construct function" << std::endl;
        free(memory);
        dlclose(handle);
        return 1;
    }
    
    std::cout << "\n=== Testing Construction ===" << std::endl;
    _NT_algorithm* algorithm = nullptr;
    
    try {
        algorithm = factory->construct(memPtrs, reqs, nullptr);
        std::cout << "✓ construct() returned: " << std::hex << algorithm << std::dec << std::endl;
        
        if (!algorithm) {
            std::cout << "✗ Algorithm is null" << std::endl;
        } else {
            std::cout << "✓ Algorithm constructed successfully" << std::endl;
            
            // Test parameter access
            std::cout << "\n=== Testing Parameter Access ===" << std::endl;
            if (algorithm->parameters) {
                std::cout << "✓ Parameters pointer: " << std::hex << algorithm->parameters << std::dec << std::endl;
                for (uint32_t i = 0; i < reqs.numParameters && i < 3; i++) {
                    std::cout << "  Parameter " << i << ": " << algorithm->parameters[i].name << std::endl;
                }
            } else {
                std::cout << "⚠ Parameters pointer is null" << std::endl;
            }
            
            // Test v array access
            if (algorithm->v) {
                std::cout << "✓ Parameter values array: " << std::hex << algorithm->v << std::dec << std::endl;
                for (uint32_t i = 0; i < reqs.numParameters && i < 6; i++) {
                    std::cout << "  v[" << i << "] = " << algorithm->v[i] << std::endl;
                }
            } else {
                std::cout << "⚠ Parameter values array is null" << std::endl;
            }
            
            // Test the step function with null buses to see if it crashes
            std::cout << "\n=== Testing Step Function (Crash Test) ===" << std::endl;
            
            // Create a fake bus array filled with zeros
            const int numFrames = 4; // 4 frames
            const int numBuses = 28;  // 28 buses
            float* testBuses = (float*)calloc(numFrames * numBuses, sizeof(float));
            
            if (factory->step) {
                try {
                    std::cout << "Calling step() with test buses..." << std::flush;
                    factory->step(algorithm, testBuses, 1); // 1 * 4 = 4 frames
                    std::cout << " ✓ step() completed successfully!" << std::endl;
                } catch (...) {
                    std::cout << " ✗ step() threw exception" << std::endl;
                }
            } else {
                std::cout << "⚠ No step function available" << std::endl;
            }
            
            free(testBuses);
        }
        
    } catch (...) {
        std::cout << "✗ construct() threw exception" << std::endl;
    }
    
    free(memory);
    dlclose(handle);
    
    std::cout << "\n✓ Test completed" << std::endl;
    return 0;
}