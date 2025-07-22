#include <iostream>
#include <cassert>
#include <dlfcn.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <iomanip>

// Include the API header
#include "../emulator/include/distingnt/api.h"

class APIComprehensiveTester {
private:
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginInstanceMemory = nullptr;
    void* pluginSharedMemory = nullptr;
    
public:
    ~APIComprehensiveTester() {
        cleanup();
    }
    
    void cleanup() {
        if (pluginInstanceMemory) {
            free(pluginInstanceMemory);
            pluginInstanceMemory = nullptr;
        }
        if (pluginSharedMemory) {
            free(pluginSharedMemory);
            pluginSharedMemory = nullptr;
        }
        if (pluginHandle) {
            dlclose(pluginHandle);
            pluginHandle = nullptr;
        }
        pluginFactory = nullptr;
        pluginAlgorithm = nullptr;
    }
    
    void hexDump(void* ptr, size_t size, const char* label = "") {
        if (!ptr) {
            std::cout << "    " << label << ": NULL pointer" << std::endl;
            return;
        }
        
        uint8_t* bytes = (uint8_t*)ptr;
        std::cout << "    " << label << " (" << size << " bytes at " << std::hex << ptr << std::dec << "):" << std::endl;
        
        for (size_t i = 0; i < size && i < 256; i += 16) { // Limit to 256 bytes
            std::cout << "      " << std::hex << std::setw(8) << std::setfill('0') << (uintptr_t)(bytes + i) << ": ";
            
            // Print hex values
            for (size_t j = 0; j < 16 && i + j < size; j++) {
                try {
                    volatile uint8_t byte = bytes[i + j];
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                } catch (...) {
                    std::cout << "?? ";
                }
            }
            
            // Pad to consistent width
            for (size_t j = size - i; j < 16 && j < 16; j++) {
                std::cout << "   ";
            }
            
            // Print ASCII representation
            std::cout << " |";
            for (size_t j = 0; j < 16 && i + j < size; j++) {
                try {
                    volatile char c = (char)bytes[i + j];
                    if (c >= 32 && c <= 126) {
                        std::cout << c;
                    } else {
                        std::cout << '.';
                    }
                } catch (...) {
                    std::cout << '?';
                }
            }
            std::cout << "|" << std::endl;
        }
        std::cout << std::dec << std::setfill(' ');
    }
    
    bool testPluginLoading() {
        std::cout << "=== Testing Plugin Loading ===" << std::endl;
        
        std::string pluginPath = "./simple_gain.dylib";
        pluginHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!pluginHandle) {
            std::cout << "✗ Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        std::cout << "✓ Plugin loaded successfully" << std::endl;
        
        // Check what symbols are available
        std::vector<std::string> symbolsToCheck = {
            "pluginEntry", "NT_getFactoryPtr", "_NT_getFactoryPtr",
            "pluginInit", "pluginExit", "initialize", "terminate"
        };
        
        std::cout << "  Available symbols:" << std::endl;
        for (const auto& symbol : symbolsToCheck) {
            void* sym = dlsym(pluginHandle, symbol.c_str());
            std::cout << "    " << symbol << ": " << (sym ? "✓" : "✗") << " " << std::hex << sym << std::dec << std::endl;
        }
        
        return true;
    }
    
    bool testAPIVersions() {
        std::cout << "=== Testing API Versions ===" << std::endl;
        
        // Try new API first
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector selector, uint32_t data);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(pluginHandle, "pluginEntry");
        
        if (pluginEntry) {
            std::cout << "✓ New API (pluginEntry) available" << std::endl;
            
            // Test different selectors
            std::cout << "  Testing selectors:" << std::endl;
            
            try {
                uintptr_t version = pluginEntry(kNT_selector_version, 0);
                std::cout << "    version: " << version << std::endl;
                
                uintptr_t numFactories = pluginEntry(kNT_selector_numFactories, 0);
                std::cout << "    numFactories: " << numFactories << std::endl;
                
                // Test with different factory indices
                for (uint32_t i = 0; i < std::min((uintptr_t)3, numFactories); i++) {
                    uintptr_t factoryPtr = pluginEntry(kNT_selector_factoryInfo, i);
                    std::cout << "    factory[" << i << "]: " << std::hex << factoryPtr << std::dec << std::endl;
                    
                    if (i == 0) {
                        pluginFactory = (_NT_factory*)factoryPtr;
                    }
                }
            } catch (...) {
                std::cout << "✗ Exception calling pluginEntry" << std::endl;
                return false;
            }
            
        } else {
            std::cout << "✓ Using old API (NT_getFactoryPtr)" << std::endl;
            
            typedef _NT_factory* (*FactoryFunc)();
            FactoryFunc getFactory = (FactoryFunc)dlsym(pluginHandle, "NT_getFactoryPtr");
            
            if (!getFactory) {
                std::cout << "✗ No factory function found" << std::endl;
                return false;
            }
            
            try {
                pluginFactory = getFactory();
                std::cout << "  Factory pointer: " << std::hex << pluginFactory << std::dec << std::endl;
            } catch (...) {
                std::cout << "✗ Exception calling NT_getFactoryPtr" << std::endl;
                return false;
            }
        }
        
        return pluginFactory != nullptr;
    }
    
    bool testFactoryStructure() {
        std::cout << "=== Testing Factory Structure ===" << std::endl;
        
        if (!pluginFactory) {
            std::cout << "✗ No factory available" << std::endl;
            return false;
        }
        
        std::cout << "  Factory address: " << std::hex << pluginFactory << std::dec << std::endl;
        
        // Dump the entire factory structure
        hexDump(pluginFactory, sizeof(_NT_factory), "Factory structure");
        
        // Test individual fields
        std::cout << "  Testing factory fields:" << std::endl;
        
        try {
            std::cout << "    guid: 0x" << std::hex << pluginFactory->guid << std::dec << std::endl;
            
            if (pluginFactory->name) {
                std::cout << "    name: \"" << pluginFactory->name << "\"" << std::endl;
            } else {
                std::cout << "    name: NULL" << std::endl;
            }
            
            if (pluginFactory->description) {
                std::cout << "    description: \"" << pluginFactory->description << "\"" << std::endl;
            } else {
                std::cout << "    description: NULL" << std::endl;
            }
            
            std::cout << "    numSpecifications: " << pluginFactory->numSpecifications << std::endl;
            std::cout << "    specifications: " << std::hex << pluginFactory->specifications << std::dec << std::endl;
            
            // Test function pointers
            std::cout << "  Function pointers:" << std::endl;
            std::cout << "    calculateStaticRequirements: " << std::hex << (void*)pluginFactory->calculateStaticRequirements << std::dec << std::endl;
            std::cout << "    initialise: " << std::hex << (void*)pluginFactory->initialise << std::dec << std::endl;
            std::cout << "    calculateRequirements: " << std::hex << (void*)pluginFactory->calculateRequirements << std::dec << std::endl;
            std::cout << "    construct: " << std::hex << (void*)pluginFactory->construct << std::dec << std::endl;
            std::cout << "    parameterChanged: " << std::hex << (void*)pluginFactory->parameterChanged << std::dec << std::endl;
            std::cout << "    step: " << std::hex << (void*)pluginFactory->step << std::dec << std::endl;
            std::cout << "    draw: " << std::hex << (void*)pluginFactory->draw << std::dec << std::endl;
            
        } catch (...) {
            std::cout << "✗ Exception reading factory fields" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool testStaticMemoryPhase() {
        std::cout << "=== Testing Static Memory Phase ===" << std::endl;
        
        if (!pluginFactory) {
            std::cout << "✗ No factory available" << std::endl;
            return false;
        }
        
        // Test static requirements
        if (pluginFactory->calculateStaticRequirements) {
            std::cout << "✓ calculateStaticRequirements available" << std::endl;
            
            try {
                _NT_staticRequirements staticReqs = {0};
                pluginFactory->calculateStaticRequirements(staticReqs);
                
                std::cout << "  Static DRAM requirement: " << staticReqs.dram << " bytes" << std::endl;
                
                // If we need static memory, allocate it
                if (staticReqs.dram > 0) {
                    if (posix_memalign(&pluginSharedMemory, 16, staticReqs.dram) != 0) {
                        std::cout << "✗ Failed to allocate static memory" << std::endl;
                        return false;
                    }
                    
                    memset(pluginSharedMemory, 0, staticReqs.dram);
                    std::cout << "✓ Static memory allocated: " << staticReqs.dram << " bytes at " << std::hex << pluginSharedMemory << std::dec << std::endl;
                    
                    // Test initialization
                    if (pluginFactory->initialise) {
                        std::cout << "✓ initialise available" << std::endl;
                        
                        try {
                            _NT_staticMemoryPtrs staticPtrs = {0};
                            staticPtrs.dram = (uint8_t*)pluginSharedMemory;
                            
                            pluginFactory->initialise(staticPtrs, staticReqs);
                            std::cout << "✓ Plugin initialized successfully" << std::endl;
                            
                            // Dump initialized memory
                            hexDump(pluginSharedMemory, std::min((size_t)staticReqs.dram, (size_t)256), "Initialized static memory");
                            
                        } catch (...) {
                            std::cout << "✗ Exception during initialization" << std::endl;
                            return false;
                        }
                    } else {
                        std::cout << "ⓘ No initialise function" << std::endl;
                    }
                }
                
            } catch (...) {
                std::cout << "✗ Exception in calculateStaticRequirements" << std::endl;
                return false;
            }
        } else {
            std::cout << "ⓘ No calculateStaticRequirements function" << std::endl;
        }
        
        return true;
    }
    
    bool testAlgorithmMemoryPhase() {
        std::cout << "=== Testing Algorithm Memory Phase ===" << std::endl;
        
        if (!pluginFactory) {
            std::cout << "✗ No factory available" << std::endl;
            return false;
        }
        
        // Test algorithm requirements
        if (pluginFactory->calculateRequirements) {
            std::cout << "✓ calculateRequirements available" << std::endl;
            
            try {
                _NT_algorithmRequirements algoReqs = {0};
                
                // Test with different specifications
                std::vector<int32_t*> testSpecs = {
                    nullptr,
                    new int32_t[4]{0, 1, 2, 3}
                };
                
                for (size_t specIndex = 0; specIndex < testSpecs.size(); specIndex++) {
                    std::cout << "  Testing with specifications[" << specIndex << "]:" << std::endl;
                    
                    pluginFactory->calculateRequirements(algoReqs, testSpecs[specIndex]);
                    
                    std::cout << "    numParameters: " << algoReqs.numParameters << std::endl;
                    std::cout << "    SRAM: " << algoReqs.sram << " bytes" << std::endl;
                    std::cout << "    DRAM: " << algoReqs.dram << " bytes" << std::endl;
                    std::cout << "    DTC: " << algoReqs.dtc << " bytes" << std::endl;
                    std::cout << "    ITC: " << algoReqs.itc << " bytes" << std::endl;
                    
                    if (specIndex == 0) {
                        // Use the first result for algorithm construction
                        return testAlgorithmConstruction(algoReqs, testSpecs[specIndex]);
                    }
                }
                
                // Cleanup
                for (auto spec : testSpecs) {
                    delete[] spec;
                }
                
            } catch (...) {
                std::cout << "✗ Exception in calculateRequirements" << std::endl;
                return false;
            }
        } else {
            std::cout << "✗ No calculateRequirements function" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool testAlgorithmConstruction(_NT_algorithmRequirements& reqs, const int32_t* specifications) {
        std::cout << "=== Testing Algorithm Construction ===" << std::endl;
        
        if (!pluginFactory->construct) {
            std::cout << "✗ No construct function" << std::endl;
            return false;
        }
        
        // Calculate total memory needed
        size_t totalMem = std::max((size_t)(reqs.sram + reqs.dram + reqs.dtc + reqs.itc), (size_t)4096);
        
        if (posix_memalign(&pluginInstanceMemory, 16, totalMem) != 0) {
            std::cout << "✗ Failed to allocate algorithm memory" << std::endl;
            return false;
        }
        
        // Initialize memory with a pattern to detect corruption
        uint8_t* mem = (uint8_t*)pluginInstanceMemory;
        for (size_t i = 0; i < totalMem; i++) {
            mem[i] = (uint8_t)(i & 0xFF);
        }
        
        std::cout << "✓ Algorithm memory allocated: " << totalMem << " bytes at " << std::hex << pluginInstanceMemory << std::dec << std::endl;
        
        // Set up memory pointers with proper alignment and spacing
        _NT_algorithmMemoryPtrs memPtrs{};
        uint8_t* ptr = (uint8_t*)pluginInstanceMemory;
        
        size_t sramSize = std::max(reqs.sram, (uint32_t)1024);
        size_t dramSize = std::max(reqs.dram, (uint32_t)1024);
        size_t dtcSize = std::max(reqs.dtc, (uint32_t)1024);
        size_t itcSize = std::max(reqs.itc, (uint32_t)1024);
        
        memPtrs.sram = ptr; ptr += sramSize;
        memPtrs.dram = ptr; ptr += dramSize;
        memPtrs.dtc = ptr; ptr += dtcSize;
        memPtrs.itc = ptr;
        
        std::cout << "  Memory layout:" << std::endl;
        std::cout << "    SRAM: " << std::hex << memPtrs.sram << std::dec << " (size: " << sramSize << ")" << std::endl;
        std::cout << "    DRAM: " << std::hex << memPtrs.dram << std::dec << " (size: " << dramSize << ")" << std::endl;
        std::cout << "    DTC:  " << std::hex << memPtrs.dtc << std::dec << " (size: " << dtcSize << ")" << std::endl;
        std::cout << "    ITC:  " << std::hex << memPtrs.itc << std::dec << " (size: " << itcSize << ")" << std::endl;
        
        // Dump memory state before construction
        hexDump(pluginInstanceMemory, 256, "Memory before construction");
        
        try {
            std::cout << "  Calling construct..." << std::endl;
            _NT_algorithm* result = pluginFactory->construct(memPtrs, reqs, specifications);
            
            std::cout << "  construct() returned: " << std::hex << result << std::dec;
            if (result) {
                std::cout << " (looks like pointer)";
            } else {
                std::cout << " (likely status code: " << (uintptr_t)result << ")";
            }
            std::cout << std::endl;
            
            // Dump memory state after construction
            hexDump(pluginInstanceMemory, 256, "Memory after construction");
            
            // Determine where the algorithm is
            if (result != nullptr && (uintptr_t)result > 0x1000) {
                pluginAlgorithm = result;
                std::cout << "  ✓ Using returned pointer" << std::endl;
            } else {
                pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
                std::cout << "  ✓ Using SRAM location" << std::endl;
            }
            
            std::cout << "  Algorithm at: " << std::hex << pluginAlgorithm << std::dec << std::endl;
            
            // Dump the algorithm structure
            hexDump(pluginAlgorithm, sizeof(_NT_algorithm), "Algorithm structure");
            
            return testAlgorithmAccess();
            
        } catch (...) {
            std::cout << "✗ Exception during construct" << std::endl;
            hexDump(pluginInstanceMemory, 256, "Memory after exception");
            return false;
        }
    }
    
    bool testAlgorithmAccess() {
        std::cout << "=== Testing Algorithm Access ===" << std::endl;
        
        if (!pluginAlgorithm) {
            std::cout << "✗ No algorithm available" << std::endl;
            return false;
        }
        
        try {
            // Test reading each field of the algorithm structure
            std::cout << "  Testing algorithm fields:" << std::endl;
            
            // Test parameters field - THIS IS WHERE THE CRASH OCCURS
            std::cout << "    Reading parameters field..." << std::endl;
            
            // Use the safest possible approach - read byte by byte if needed
            volatile uint8_t* algorithmBytes = (volatile uint8_t*)pluginAlgorithm;
            
            // The parameters field is at offset 0 in the _NT_algorithm struct
            volatile const _NT_parameter** parametersFieldPtr = (volatile const _NT_parameter**)algorithmBytes;
            
            std::cout << "    Parameters field address: " << std::hex << parametersFieldPtr << std::dec << std::endl;
            
            // Try to read the pointer value
            const _NT_parameter* parametersPtr = nullptr;
            try {
                parametersPtr = (const _NT_parameter*)*parametersFieldPtr;
                std::cout << "    Parameters pointer value: " << std::hex << parametersPtr << std::dec << std::endl;
                
                // Check if this is the corrupted value
                if (parametersPtr == (const _NT_parameter*)0x3f800000) {
                    std::cout << "    ⚠ FOUND THE CORRUPTED VALUE! This is what causes the crash." << std::endl;
                    
                    // Let's investigate how this value got there
                    hexDump((void*)parametersFieldPtr, 64, "Area around parameters field");
                    
                    return false; // Don't continue - we found the problem
                }
                
            } catch (...) {
                std::cout << "    ✗ Exception reading parameters field" << std::endl;
                return false;
            }
            
            // Test other fields
            try {
                std::cout << "    parameterPages: " << std::hex << pluginAlgorithm->parameterPages << std::dec << std::endl;
                std::cout << "    vIncludingCommon: " << std::hex << pluginAlgorithm->vIncludingCommon << std::dec << std::endl;
                std::cout << "    v: " << std::hex << pluginAlgorithm->v << std::dec << std::endl;
            } catch (...) {
                std::cout << "    ✗ Exception reading other algorithm fields" << std::endl;
                return false;
            }
            
            std::cout << "✓ Algorithm access test completed successfully" << std::endl;
            return true;
            
        } catch (...) {
            std::cout << "✗ Exception during algorithm access" << std::endl;
            return false;
        }
    }
    
    void runAllTests() {
        std::cout << "=== Comprehensive API Testing ===" << std::endl;
        std::cout << "Testing plugin: simple_gain.dylib" << std::endl;
        std::cout << "sizeof(_NT_factory): " << sizeof(_NT_factory) << std::endl;
        std::cout << "sizeof(_NT_algorithm): " << sizeof(_NT_algorithm) << std::endl;
        std::cout << "sizeof(_NT_parameter): " << sizeof(_NT_parameter) << std::endl;
        
        bool success = true;
        
        success &= testPluginLoading();
        if (!success) { cleanup(); return; }
        
        success &= testAPIVersions();
        if (!success) { cleanup(); return; }
        
        success &= testFactoryStructure();
        if (!success) { cleanup(); return; }
        
        success &= testStaticMemoryPhase();
        if (!success) { cleanup(); return; }
        
        success &= testAlgorithmMemoryPhase();
        // Note: algorithm phase handles its own success/failure
        
        cleanup();
        
        if (success) {
            std::cout << "\n🎉 All API tests completed!" << std::endl;
        } else {
            std::cout << "\n❌ Some API tests failed - root cause analysis needed" << std::endl;
        }
    }
};

int main() {
    APIComprehensiveTester tester;
    tester.runAllTests();
    return 0;
}