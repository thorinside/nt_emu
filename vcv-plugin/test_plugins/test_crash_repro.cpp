#include <iostream>
#include <cassert>
#include <dlfcn.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <iomanip>

// Include the API header
#include "../emulator/include/distingnt/api.h"

// Test different memory access patterns that could lead to 0x3f800000
class CrashReproTester {
private:
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginInstanceMemory = nullptr;
    
public:
    ~CrashReproTester() {
        cleanup();
    }
    
    void cleanup() {
        if (pluginInstanceMemory) {
            free(pluginInstanceMemory);
            pluginInstanceMemory = nullptr;
        }
        if (pluginHandle) {
            dlclose(pluginHandle);
            pluginHandle = nullptr;
        }
        pluginFactory = nullptr;
        pluginAlgorithm = nullptr;
    }
    
    void testMemoryArithmetic() {
        std::cout << "=== Testing Memory Arithmetic Patterns ===" << std::endl;
        
        // Test various values that could lead to 0x3f800000
        uint64_t crashAddr = 0x000000003f800000ULL;
        std::cout << "Crash address: 0x" << std::hex << crashAddr << std::dec << std::endl;
        std::cout << "Crash address decimal: " << crashAddr << std::endl;
        
        // This looks like it could be array index * 24 (sizeof parameter struct)
        uint64_t paramSize = 24; // sizeof(_NT_parameter) should be 24
        uint64_t possibleIndex = crashAddr / paramSize;
        std::cout << "If param size is " << paramSize << ", index would be: " << possibleIndex << std::endl;
        
        // Test other potential calculations
        for (int base = 0; base < 32; base++) {
            uint64_t testValue = (uint64_t)base << 26; // 26-bit shift
            if (testValue == crashAddr) {
                std::cout << "Crash addr = " << base << " << 26" << std::endl;
            }
        }
        
        // Check if it's a multiple of common sizes
        std::cout << "Potential causes:" << std::endl;
        if (crashAddr % 24 == 0) {
            std::cout << "  Multiple of 24 (parameter struct size): index " << (crashAddr / 24) << std::endl;
        }
        if (crashAddr % 0x1000 == 0) {
            std::cout << "  Multiple of 0x1000 (page size): " << (crashAddr / 0x1000) << " pages" << std::endl;
        }
        
        // Test specific value - 0x3f800000 = 63 * 0x1000000
        if (crashAddr == 63 * 0x1000000) {
            std::cout << "  Crash addr = 63 * 0x1000000" << std::endl;
        }
    }
    
    void testCorruptedParameterAccess() {
        std::cout << "=== Testing Corrupted Parameter Access ===" << std::endl;
        
        if (!loadAndConstructPlugin()) {
            std::cout << "✗ Plugin loading failed" << std::endl;
            return;
        }
        
        std::cout << "✓ Plugin loaded and constructed" << std::endl;
        std::cout << "  pluginAlgorithm: " << std::hex << pluginAlgorithm << std::dec << std::endl;
        
        // Test if we can reproduce the crash by manipulating the parameters pointer
        if (pluginAlgorithm) {
            std::cout << "  Original parameters ptr: " << std::hex << pluginAlgorithm->parameters << std::dec << std::endl;
            
            // Create a backup
            const _NT_parameter* originalParams = pluginAlgorithm->parameters;
            
            // Test different corrupted parameter pointers that could lead to 0x3f800000 on access
            std::vector<uintptr_t> testPointers = {
                0,                          // NULL - should be handled
                0x1000,                     // Low address
                0x3f800000,                 // The crash address itself
                0x3f800000 - 24,           // One parameter struct before crash
                0x3f800000 - 0x1000,       // One page before
                (uintptr_t)pluginAlgorithm + 0x3f800000, // Algorithm + offset
            };
            
            for (auto testPtr : testPointers) {
                std::cout << "  Testing parameters = 0x" << std::hex << testPtr << std::dec << std::endl;
                
                // Temporarily set the corrupted pointer
                pluginAlgorithm->parameters = (const _NT_parameter*)testPtr;
                
                try {
                    // Try the parameter access pattern from extractParameterData
                    if (testPtr == 0) {
                        std::cout << "    NULL pointer - should be handled gracefully" << std::endl;
                        continue;
                    }
                    
                    // Test reading the first parameter (this is where the crash likely occurs)
                    const _NT_parameter* param0 = &pluginAlgorithm->parameters[0];
                    std::cout << "    param0 address: " << std::hex << param0 << std::dec << std::endl;
                    
                    if ((uintptr_t)param0 == 0x3f800000) {
                        std::cout << "    ⚠ This would crash at the reported address!" << std::endl;
                    }
                    
                    // Try to read the name field (likely crash point)
                    if (param0 && (uintptr_t)param0 > 0x1000 && (uintptr_t)param0 < 0x800000000000ULL) {
                        try {
                            volatile const char* name = param0->name;
                            std::cout << "    name ptr: " << std::hex << name << std::dec << std::endl;
                            
                            // This is likely where the actual crash occurs
                            if (name && (uintptr_t)name > 0x1000 && (uintptr_t)name < 0x800000000000ULL) {
                                volatile char firstChar = name[0];
                                std::cout << "    first char readable: 0x" << std::hex << (int)firstChar << std::dec << std::endl;
                            }
                        } catch (...) {
                            std::cout << "    ✗ Exception reading parameter name" << std::endl;
                        }
                    } else {
                        std::cout << "    Parameter pointer out of valid range" << std::endl;
                    }
                    
                } catch (...) {
                    std::cout << "    ✗ Exception during parameter access" << std::endl;
                }
                
                // Restore original
                pluginAlgorithm->parameters = originalParams;
            }
        }
    }
    
    void testArrayIndexCalculation() {
        std::cout << "=== Testing Array Index Calculations ===" << std::endl;
        
        // Test what index would cause us to land at 0x3f800000
        // The crash appears to be in the parameter loop at line 0x50c0
        
        uint64_t crashAddr = 0x3f800000ULL;
        size_t paramSize = sizeof(_NT_parameter);
        
        std::cout << "sizeof(_NT_parameter): " << paramSize << std::endl;
        
        // If we have a base pointer and add index * paramSize, what index gives us crashAddr?
        for (uintptr_t base = 0x1000; base < 0x100000000ULL; base += 0x1000) {
            if (base > crashAddr) break;
            
            uintptr_t offset = crashAddr - base;
            if (offset % paramSize == 0) {
                size_t index = offset / paramSize;
                if (index > 1000 && index < 100000000) { // Reasonable suspicious range
                    std::cout << "  Base 0x" << std::hex << base 
                             << ", index " << std::dec << index 
                             << " -> 0x" << std::hex << (base + index * paramSize) << std::dec << std::endl;
                }
            }
        }
    }
    
    bool loadAndConstructPlugin() {
        // Try to load the simple gain plugin
        std::string pluginPath = "./simple_gain.dylib";
        
        pluginHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!pluginHandle) {
            std::cout << "✗ Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        // Try old API
        typedef _NT_factory* (*FactoryFunc)();
        FactoryFunc getFactory = (FactoryFunc)dlsym(pluginHandle, "NT_getFactoryPtr");
        
        if (!getFactory) {
            std::cout << "✗ No factory function found" << std::endl;
            return false;
        }
        
        pluginFactory = getFactory();
        if (!pluginFactory || !pluginFactory->construct) {
            std::cout << "✗ Invalid factory" << std::endl;
            return false;
        }
        
        // Construct algorithm
        size_t totalMem = 4096; // Minimum allocation
        
        if (posix_memalign(&pluginInstanceMemory, 16, totalMem) != 0) {
            std::cout << "✗ Failed to allocate memory" << std::endl;
            return false;
        }
        
        memset(pluginInstanceMemory, 0, totalMem);
        
        // Set up memory pointers
        _NT_algorithmMemoryPtrs memPtrs{};
        uint8_t* ptr = (uint8_t*)pluginInstanceMemory;
        memPtrs.sram = ptr; ptr += 1024;
        memPtrs.dram = ptr; ptr += 1024;
        memPtrs.dtc = ptr; ptr += 1024;
        memPtrs.itc = ptr;
        
        _NT_algorithmRequirements reqs = {0};
        reqs.sram = 1024;
        reqs.dram = 1024;
        
        // construct() returns status code, algorithm is placed at SRAM location
        _NT_algorithm* result = pluginFactory->construct(memPtrs, reqs, nullptr);
        pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
        
        return pluginAlgorithm != nullptr;
    }
    
    void runAllTests() {
        std::cout << "=== Crash Reproduction Tests ===" << std::endl;
        
        testMemoryArithmetic();
        testArrayIndexCalculation();
        testCorruptedParameterAccess();
    }
};

int main() {
    CrashReproTester tester;
    tester.runAllTests();
    return 0;
}