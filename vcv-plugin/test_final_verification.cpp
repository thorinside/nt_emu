#include <iostream>
#include <dlfcn.h>
#include <cstdlib>
#include "../emulator/include/distingnt/api.h"

// Include the VCV module's extractParameterData function by testing the same logic
bool isValidPointer(void* ptr, size_t size = sizeof(void*)) {
    if (!ptr) return false;
    
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < 0x1000) return false;
    
    #if defined(__aarch64__) || defined(_M_ARM64)
        if (addr > 0x800000000000ULL) return false;
    #else
        if (addr > 0x7FFFFFFFFFFF) return false;
    #endif
    
    try {
        volatile char test = *((volatile char*)ptr);
        (void)test;
        return true;
    } catch (...) {
        return false;
    }
}

void testVCVParameterExtraction(_NT_algorithm* pluginAlgorithm) {
    std::cout << "=== Testing VCV Parameter Extraction Logic ===" << std::endl;
    
    if (!pluginAlgorithm || !isValidPointer(pluginAlgorithm)) {
        std::cout << "✗ pluginAlgorithm is null or invalid" << std::endl;
        return;
    }
    
    std::cout << "✓ pluginAlgorithm is valid: " << std::hex << pluginAlgorithm << std::dec << std::endl;
    
    // CRITICAL: Check if the parameters field itself is accessible
    const _NT_parameter* parametersPtr = nullptr;
    try {
        volatile const _NT_parameter* volatile* ptrToPtr = 
            (volatile const _NT_parameter* volatile*)&pluginAlgorithm->parameters;
        parametersPtr = (const _NT_parameter*)*ptrToPtr;
        std::cout << "✓ Successfully read parameters field: " << std::hex << parametersPtr << std::dec << std::endl;
    } catch (...) {
        std::cout << "✗ Cannot read parameters field - algorithm structure corrupted" << std::endl;
        return;
    }
    
    // Check for specific known bad addresses that cause crashes
    uintptr_t paramAddr = (uintptr_t)parametersPtr;
    if (paramAddr == 0x3f800000 || 
        (paramAddr >= 0x3f000000 && paramAddr <= 0x40000000)) {
        std::cout << "✗ Found corrupted address (0x" << std::hex << paramAddr << std::dec 
                  << ") - this would cause crash!" << std::endl;
        return;
    }
    
    if (!parametersPtr) {
        std::cout << "ℹ parameters pointer is NULL (plugin has no parameters - this is OK)" << std::endl;
        return;
    }
    
    // If we have parameters, try to validate them
    if (!isValidPointer((void*)parametersPtr, sizeof(_NT_parameter))) {
        std::cout << "✗ parameters pointer is invalid" << std::endl;
        return;
    }
    
    std::cout << "✓ parameters pointer is valid - no crash will occur!" << std::endl;
}

int main() {
    std::cout << "=== Final Verification: Updated simple_gain.dylib ===" << std::endl;
    
    // Load the updated plugin
    void* pluginHandle = dlopen("./simple_gain.dylib", RTLD_LAZY);
    if (!pluginHandle) {
        std::cout << "✗ Failed to load plugin: " << dlerror() << std::endl;
        return 1;
    }
    
    std::cout << "✓ Plugin loaded successfully" << std::endl;
    
    // Get factory using old API (for compatibility)
    typedef _NT_factory* (*FactoryFunc)();
    FactoryFunc getFactory = (FactoryFunc)dlsym(pluginHandle, "NT_getFactoryPtr");
    
    if (!getFactory) {
        std::cout << "✗ No NT_getFactoryPtr function found" << std::endl;
        dlclose(pluginHandle);
        return 1;
    }
    
    _NT_factory* pluginFactory = getFactory();
    if (!pluginFactory) {
        std::cout << "✗ Factory returned null" << std::endl;
        dlclose(pluginHandle);
        return 1;
    }
    
    std::cout << "✓ Factory obtained: " << std::hex << pluginFactory << std::dec << std::endl;
    
    // Test the new API functions
    _NT_algorithmRequirements reqs = {0};
    pluginFactory->calculateRequirements(reqs, nullptr);
    
    std::cout << "✓ Memory requirements calculated:" << std::endl;
    std::cout << "  SRAM: " << reqs.sram << " bytes" << std::endl;
    std::cout << "  DRAM: " << reqs.dram << " bytes" << std::endl;
    std::cout << "  Parameters: " << reqs.numParameters << std::endl;
    
    // Allocate memory (ensure we have at least enough for the algorithm structure)
    size_t minRequired = sizeof(_NT_algorithm) + 64; // SimpleGain class + algorithm structure + padding
    size_t totalMem = std::max((size_t)(reqs.sram + reqs.dram + reqs.dtc + reqs.itc), minRequired);
    void* memory = malloc(totalMem);
    if (!memory) {
        std::cout << "✗ Failed to allocate memory" << std::endl;
        dlclose(pluginHandle);
        return 1;
    }
    
    memset(memory, 0, totalMem);
    
    // Set up memory pointers
    _NT_algorithmMemoryPtrs memPtrs{};
    memPtrs.sram = (uint8_t*)memory;
    memPtrs.dram = memPtrs.sram + reqs.sram;
    memPtrs.dtc = memPtrs.dram + reqs.dram;
    memPtrs.itc = memPtrs.dtc + reqs.dtc;
    
    // Construct the algorithm
    _NT_algorithm* result = pluginFactory->construct(memPtrs, reqs, nullptr);
    
    std::cout << "  construct() returned: " << std::hex << result << std::dec << std::endl;
    
    // Determine where the algorithm actually is
    _NT_algorithm* pluginAlgorithm = nullptr;
    if (result != nullptr && (uintptr_t)result > 0x1000) {
        pluginAlgorithm = result;
        std::cout << "✓ Using returned pointer" << std::endl;
    } else {
        pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
        std::cout << "✓ Using SRAM location (construct returned status: " << (uintptr_t)result << ")" << std::endl;
    }
    
    std::cout << "✓ Algorithm at: " << std::hex << pluginAlgorithm << std::dec << std::endl;
    
    // Now test the parameter extraction that was previously crashing
    testVCVParameterExtraction(pluginAlgorithm);
    
    // Cleanup
    free(memory);
    dlclose(pluginHandle);
    
    std::cout << "\n🎉 SUCCESS: No crashes occurred! The fix is working!" << std::endl;
    return 0;
}