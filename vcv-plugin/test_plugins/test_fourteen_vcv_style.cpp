#include <iostream>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <array>
#include "../emulator/include/distingnt/api.h"

int main() {
    std::cout << "=== Fourteen Plugin VCV-Style Test ===\n";
    
    // Load fourteen plugin
    void* handle = dlopen("./fourteen.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cout << "✗ Failed to load fourteen.dylib: " << dlerror() << std::endl;
        return 1;
    }
    
    std::cout << "✓ Loaded fourteen.dylib\n";
    
    typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
    PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(handle, "pluginEntry");
    if (!pluginEntry) {
        std::cout << "✗ No pluginEntry function\n";
        dlclose(handle);
        return 1;
    }
    
    _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
    if (!factory) {
        std::cout << "✗ No factory\n";
        dlclose(handle);
        return 1;
    }
    
    // Display GUID properly 
    char guidStr[5] = {0};
    guidStr[0] = (factory->guid >> 0) & 0xFF;
    guidStr[1] = (factory->guid >> 8) & 0xFF;
    guidStr[2] = (factory->guid >> 16) & 0xFF;
    guidStr[3] = (factory->guid >> 24) & 0xFF;
    std::cout << "✓ Plugin: " << guidStr << " - " << factory->name << "\n";
    
    // Get requirements
    _NT_algorithmRequirements reqs{};
    factory->calculateRequirements(reqs, nullptr);
    std::cout << "Memory Requirements:\n";
    std::cout << "  SRAM: " << reqs.sram << " bytes\n";
    std::cout << "  DRAM: " << reqs.dram << " bytes\n";
    std::cout << "  DTC:  " << reqs.dtc << " bytes\n";
    std::cout << "  ITC:  " << reqs.itc << " bytes\n";
    std::cout << "  Parameters: " << reqs.numParameters << "\n";
    
    // Allocate memory exactly like VCV plugin
    size_t totalMem = reqs.sram + reqs.dram + reqs.dtc + reqs.itc;
    void* memory = nullptr;
    if (posix_memalign(&memory, 16, totalMem) != 0) {
        std::cout << "✗ Memory allocation failed\n";
        dlclose(handle);
        return 1;
    }
    
    memset(memory, 0, totalMem);
    std::cout << "✓ Allocated " << totalMem << " bytes\n";
    
    // Set up memory pointers exactly like VCV plugin
    _NT_algorithmMemoryPtrs memPtrs{};
    uint8_t* ptr = (uint8_t*)memory;
    memPtrs.sram = ptr; ptr += reqs.sram;
    memPtrs.dram = ptr; ptr += reqs.dram;
    memPtrs.dtc = ptr;  ptr += reqs.dtc;
    memPtrs.itc = ptr;
    
    // Create routing matrix like VCV plugin (correct data type)
    std::array<int16_t, 256> routingMatrix;
    routingMatrix.fill(0);
    
    // Construct algorithm
    std::cout << "\n=== Testing Construction ===\n";
    _NT_algorithm* algorithm = factory->construct(memPtrs, reqs, nullptr);
    
    if (!algorithm) {
        std::cout << "✗ Algorithm construction failed\n";
        free(memory);
        dlclose(handle);
        return 1;
    }
    
    std::cout << "✓ Algorithm constructed at: " << std::hex << algorithm << std::dec << "\n";
    
    // *** KEY FIX: Connect parameter values array like VCV plugin ***
    algorithm->v = routingMatrix.data();
    std::cout << "✓ Connected parameter values array: " << std::hex << algorithm->v << std::dec << "\n";
    
    // Initialize routing matrix with parameter defaults
    if (algorithm->parameters) {
        for (uint32_t i = 0; i < reqs.numParameters && i < routingMatrix.size(); i++) {
            int16_t defaultValue = (int16_t)algorithm->parameters[i].def;
            routingMatrix[i] = defaultValue;
            std::cout << "  Parameter " << i << " (" << algorithm->parameters[i].name 
                      << ") default: " << defaultValue << "\n";
        }
    }
    
    // Test parameter access
    std::cout << "\n=== Testing Parameter Access ===\n";
    if (algorithm->v) {
        std::cout << "✓ Parameter values array connected successfully\n";
        for (uint32_t i = 0; i < reqs.numParameters && i < 6; i++) {
            std::cout << "  v[" << i << "] = " << algorithm->v[i] << "\n";
        }
    } else {
        std::cout << "✗ Parameter values array still null\n";
    }
    
    // Test the step function 
    std::cout << "\n=== Testing Step Function ===\n";
    
    const int numFrames = 4;
    const int numBuses = 28;  
    float* testBuses = (float*)calloc(numFrames * numBuses, sizeof(float));
    
    if (factory->step) {
        try {
            std::cout << "Calling step() with connected parameter values... ";
            factory->step(algorithm, testBuses, 1); // 1 * 4 = 4 frames
            std::cout << "✓ step() completed successfully!\n";
        } catch (...) {
            std::cout << "✗ step() threw exception\n";
        }
    } else {
        std::cout << "⚠ No step function available\n";
    }
    
    free(testBuses);
    free(memory);
    dlclose(handle);
    
    std::cout << "\n✓ VCV-style test completed successfully\n";
    return 0;
}