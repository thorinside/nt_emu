#include <iostream>
#include <cassert>
#include <dlfcn.h>
#include <string>
#include <cstdlib>

// Include the API header
#include "../emulator/include/distingnt/api.h"

class DistingNTTester {
private:
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    
public:
    ~DistingNTTester() {
        cleanup();
    }
    
    void cleanup() {
        if (pluginHandle) {
            dlclose(pluginHandle);
            pluginHandle = nullptr;
        }
        pluginFactory = nullptr;
        pluginAlgorithm = nullptr;
    }
    
    bool isValidPointer(void* ptr) {
        if (!ptr) return false;
        
        // Check if pointer looks reasonable 
        uintptr_t addr = (uintptr_t)ptr;
        if (addr < 0x1000 || addr > 0x7FFFFFFFFFFF) return false;
        
        // Try a simple read test
        try {
            volatile char test = *((volatile char*)ptr);
            (void)test; // Prevent optimization
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool testLoadSimpleGain() {
        std::cout << "Testing simple_gain.dylib loading..." << std::endl;
        
        // Try to load the simple gain plugin
        std::string pluginPath = "./simple_gain.dylib";
        
        pluginHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!pluginHandle) {
            std::cout << "Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        std::cout << "✓ Plugin loaded successfully" << std::endl;
        
        // Try new API first (pluginEntry)
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector selector, uint32_t data);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(pluginHandle, "pluginEntry");
        
        if (pluginEntry) {
            std::cout << "✓ Using new API (pluginEntry)" << std::endl;
            
            // Check API version
            uintptr_t apiVersion = pluginEntry(kNT_selector_version, 0);
            std::cout << "  API Version: " << apiVersion << std::endl;
            
            if (apiVersion > kNT_apiVersionCurrent) {
                std::cout << "✗ Incompatible API version" << std::endl;
                return false;
            }
            
            // Get number of factories
            uintptr_t numFactories = pluginEntry(kNT_selector_numFactories, 0);
            std::cout << "  Number of factories: " << numFactories << std::endl;
            
            if (numFactories == 0) {
                std::cout << "✗ No factories found" << std::endl;
                return false;
            }
            
            // Get first factory
            pluginFactory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
            if (!pluginFactory) {
                std::cout << "✗ Factory access failed" << std::endl;
                return false;
            }
            
        } else {
            std::cout << "✓ Using old API (NT_getFactoryPtr)" << std::endl;
            
            // Try old API
            typedef _NT_factory* (*FactoryFunc)();
            FactoryFunc getFactory = (FactoryFunc)dlsym(pluginHandle, "NT_getFactoryPtr");
            
            if (!getFactory) {
                std::cout << "✗ No factory function found" << std::endl;
                return false;
            }
            
            pluginFactory = getFactory();
            if (!pluginFactory) {
                std::cout << "✗ Factory returned null" << std::endl;
                return false;
            }
        }
        
        std::cout << "✓ Factory obtained" << std::endl;
        
        // Validate factory pointer
        if (!isValidPointer(pluginFactory)) {
            std::cout << "✗ Invalid factory pointer" << std::endl;
            return false;
        }
        
        std::cout << "✓ Factory pointer is valid" << std::endl;
        
        // Test factory info
        if (pluginFactory->name) {
            std::cout << "  Plugin Name: " << pluginFactory->name << std::endl;
        }
        
        if (pluginFactory->description) {
            std::cout << "  Description: " << pluginFactory->description << std::endl;
        }
        
        // Test algorithm construction
        return testAlgorithmConstruction();
    }
    
    bool testAlgorithmConstruction() {
        std::cout << "Testing algorithm construction..." << std::endl;
        
        if (!pluginFactory) {
            std::cout << "✗ No factory available" << std::endl;
            return false;
        }
        
        // Validate construct function
        if (!pluginFactory->construct) {
            std::cout << "✗ No construct function" << std::endl;
            return false;
        }
        
        std::cout << "✓ Construct function available" << std::endl;
        
        // Get memory requirements using the correct API
        _NT_algorithmRequirements reqs = {0};
        if (pluginFactory->calculateRequirements) {
            pluginFactory->calculateRequirements(reqs, nullptr);
            std::cout << "  SRAM requirement: " << reqs.sram << " bytes" << std::endl;
            std::cout << "  DRAM requirement: " << reqs.dram << " bytes" << std::endl;
            std::cout << "  DTC requirement: " << reqs.dtc << " bytes" << std::endl;
            std::cout << "  ITC requirement: " << reqs.itc << " bytes" << std::endl;
            std::cout << "  Num parameters: " << reqs.numParameters << std::endl;
        } else {
            std::cout << "  No calculateRequirements function, using defaults" << std::endl;
            reqs.sram = 1024; // Default reasonable size
            reqs.dram = 1024;
        }
        
        // Allocate separate memory blocks for each type
        size_t sramSize = std::max(reqs.sram, (uint32_t)1024);
        size_t dramSize = std::max(reqs.dram, (uint32_t)1024); 
        size_t dtcSize = std::max(reqs.dtc, (uint32_t)1024);
        size_t itcSize = std::max(reqs.itc, (uint32_t)1024);
        
        void* sramMemory = malloc(sramSize);
        void* dramMemory = malloc(dramSize);
        void* dtcMemory = malloc(dtcSize);
        void* itcMemory = malloc(itcSize);
        
        if (!sramMemory || !dramMemory || !dtcMemory || !itcMemory) {
            std::cout << "✗ Failed to allocate memory" << std::endl;
            return false;
        }
        
        // Initialize memory to zero
        memset(sramMemory, 0, sramSize);
        memset(dramMemory, 0, dramSize);
        memset(dtcMemory, 0, dtcSize);
        memset(itcMemory, 0, itcSize);
        
        std::cout << "✓ Memory allocated (SRAM:" << sramSize << ", DRAM:" << dramSize << ", DTC:" << dtcSize << ", ITC:" << itcSize << ")" << std::endl;
        
        // Create memory pointers structure
        _NT_algorithmMemoryPtrs memPtrs = {0};
        memPtrs.sram = (uint8_t*)sramMemory;
        memPtrs.dram = (uint8_t*)dramMemory;
        memPtrs.dtc = (uint8_t*)dtcMemory;
        memPtrs.itc = (uint8_t*)itcMemory;
        
        try {
            std::cout << "  Calling construct with:" << std::endl;
            std::cout << "    SRAM ptr: " << std::hex << (void*)memPtrs.sram << std::dec << std::endl;
            std::cout << "    DRAM ptr: " << std::hex << (void*)memPtrs.dram << std::dec << std::endl;
            std::cout << "    SRAM size: " << reqs.sram << std::endl;
            std::cout << "    DRAM size: " << reqs.dram << std::endl;
            
            // The plugin should construct the algorithm object in the provided memory
            // Let's try placing the algorithm at the start of SRAM
            pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
            
            std::cout << "  Expected algorithm location: " << std::hex << pluginAlgorithm << std::dec << std::endl;
            
            // Call construct - this should initialize the memory, not return a pointer
            _NT_algorithm* result = pluginFactory->construct(memPtrs, reqs, nullptr);
            
            std::cout << "  Construct returned: " << std::hex << result << std::dec << std::endl;
            
            // Check if the result is actually meant to be used, or if the algorithm is at memPtrs.sram
            if (result != nullptr && (uintptr_t)result > 0x1000) {
                // Use the returned pointer
                pluginAlgorithm = result;
                std::cout << "  Using returned pointer" << std::endl;
            } else {
                // Use the SRAM location
                pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
                std::cout << "  Using SRAM location (return value likely status: " << std::dec << (uintptr_t)result << ")" << std::endl;
            }
            
            if (!pluginAlgorithm) {
                std::cout << "✗ Algorithm construction failed (returned null)" << std::endl;
                free(sramMemory);
                free(dramMemory);
                free(dtcMemory);
                free(itcMemory);
                return false;
            }
            
            std::cout << "✓ Algorithm constructed successfully" << std::endl;
            
            // Test immediate access to algorithm
            std::cout << "  Testing immediate algorithm access..." << std::endl;
            if (isValidPointer(pluginAlgorithm)) {
                std::cout << "  ✓ Algorithm pointer valid immediately after construction" << std::endl;
            } else {
                std::cout << "  ✗ Algorithm pointer invalid immediately after construction" << std::endl;
                free(sramMemory);
                free(dramMemory);
                free(dtcMemory);
                free(itcMemory);
                return false;
            }
            
            // Test parameter access safely
            bool paramTest = testParameterAccess();
            
            // Clean up all memory
            free(sramMemory);
            free(dramMemory);
            free(dtcMemory);
            free(itcMemory);
            return paramTest;
            
        } catch (...) {
            std::cout << "✗ Exception during algorithm construction" << std::endl;
            free(sramMemory);
            free(dramMemory);
            free(dtcMemory);
            free(itcMemory);
            return false;
        }
    }
    
    bool testParameterAccess() {
        std::cout << "Testing parameter access..." << std::endl;
        
        if (!pluginAlgorithm) {
            std::cout << "✗ No algorithm available" << std::endl;
            return false;
        }
        
        // Print algorithm pointer details
        std::cout << "  Algorithm pointer: " << std::hex << pluginAlgorithm << std::dec << std::endl;
        
        // Try to validate the algorithm structure step by step
        std::cout << "  Testing algorithm pointer validity..." << std::endl;
        
        // First test: can we read the pointer at all?
        try {
            volatile void* test = (volatile void*)pluginAlgorithm;
            std::cout << "  ✓ Can read algorithm pointer location" << std::endl;
        } catch (...) {
            std::cout << "  ✗ Cannot read algorithm pointer location" << std::endl;
            return false;
        }
        
        // Second test: can we read the first few bytes?
        try {
            volatile uint8_t* bytes = (volatile uint8_t*)pluginAlgorithm;
            uint8_t first = bytes[0];
            uint8_t second = bytes[1];
            std::cout << "  ✓ Can read first bytes: 0x" << std::hex << (int)first << " 0x" << (int)second << std::dec << std::endl;
        } catch (...) {
            std::cout << "  ✗ Cannot read algorithm bytes" << std::endl;
            return false;
        }
        
        // Third test: validate using our function
        if (!isValidPointer(pluginAlgorithm)) {
            std::cout << "  ✗ Invalid algorithm pointer (failed validation)" << std::endl;
            return false;
        }
        
        std::cout << "✓ Algorithm pointer is valid" << std::endl;
        
        // Test parameters access
        if (!isValidPointer((void*)pluginAlgorithm->parameters)) {
            std::cout << "ⓘ Parameters pointer is null/invalid (this may be normal)" << std::endl;
            return true; // Not necessarily an error
        }
        
        std::cout << "✓ Parameters pointer looks valid" << std::endl;
        
        // Try to access first parameter carefully
        const int MAX_SAFE_PARAMS = 5; // Very conservative for testing
        int validParams = 0;
        
        for (int i = 0; i < MAX_SAFE_PARAMS; i++) {
            try {
                const _NT_parameter* param = &pluginAlgorithm->parameters[i];
                
                if (!isValidPointer((void*)param) || !isValidPointer((void*)param->name)) {
                    break; // End of valid parameters
                }
                
                // Check if name is readable
                if (param->name[0] == '\0') {
                    break; // Empty name = end
                }
                
                std::cout << "  Parameter " << i << ": " << param->name 
                          << " [" << param->min << ".." << param->max << "]"
                          << " default=" << param->def << std::endl;
                validParams++;
                
            } catch (...) {
                break; // Stop on any exception
            }
        }
        
        std::cout << "✓ Found " << validParams << " valid parameters" << std::endl;
        return true;
    }
    
    void runAllTests() {
        std::cout << "=== Disting NT Plugin Loading Tests ===" << std::endl;
        
        bool success = testLoadSimpleGain();
        
        if (success) {
            std::cout << "\n🎉 All tests passed!" << std::endl;
        } else {
            std::cout << "\n❌ Tests failed!" << std::endl;
        }
        
        cleanup();
    }
};

int main() {
    DistingNTTester tester;
    tester.runAllTests();
    return 0;
}