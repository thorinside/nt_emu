#include <iostream>
#include <cassert>
#include <dlfcn.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <iomanip>

// Include the API header
#include "../emulator/include/distingnt/api.h"

class ParameterExtractorTester {
private:
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginInstanceMemory = nullptr;
    
    // Simulate the VCV module's parameter storage
    std::vector<_NT_parameter> parameters;
    
public:
    ~ParameterExtractorTester() {
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
        parameters.clear();
    }
    
    bool isValidPointer(void* ptr, size_t size = sizeof(void*)) {
        if (!ptr) return false;
        
        // Check if pointer looks reasonable (not too high or too low)
        uintptr_t addr = (uintptr_t)ptr;
        if (addr < 0x1000) return false;
        
        // ARM64 specific checks - avoid pointer authentication issues
        #if defined(__aarch64__) || defined(_M_ARM64)
            // On ARM64, check for reasonable address ranges
            // User space addresses typically don't exceed this range
            if (addr > 0x800000000000ULL) return false;
        #else
            // x86_64 check
            if (addr > 0x7FFFFFFFFFFF) return false;
        #endif
        
        // Try a simple read test with better exception handling
        try {
            volatile char test = *((volatile char*)ptr);
            (void)test; // Prevent optimization
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void hexDump(void* ptr, size_t size) {
        if (!ptr) {
            std::cout << "    NULL pointer" << std::endl;
            return;
        }
        
        uint8_t* bytes = (uint8_t*)ptr;
        std::cout << "    Hex dump (" << size << " bytes):" << std::endl;
        
        for (size_t i = 0; i < size; i += 16) {
            std::cout << "    " << std::hex << std::setw(8) << std::setfill('0') << (uintptr_t)(bytes + i) << ": ";
            
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
            for (size_t j = size - i; j < 16; j++) {
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
    
    // Replicate the exact extractParameterData logic from VCV module
    void testExtractParameterData() {
        std::cout << "=== Testing extractParameterData() function ===" << std::endl;
        
        // First, validate pluginAlgorithm pointer with extensive checks
        if (!pluginAlgorithm || !isValidPointer(pluginAlgorithm)) {
            std::cout << "✗ pluginAlgorithm is null or invalid" << std::endl;
            return;
        }
        
        std::cout << "✓ pluginAlgorithm pointer is valid: " << std::hex << pluginAlgorithm << std::dec << std::endl;
        
        // Clear existing data first
        parameters.clear();
        
        try {
            // Check if pluginAlgorithm is pointing to valid memory structure
            // Try to read the first few bytes to validate memory
            std::cout << "  Testing algorithm structure access..." << std::endl;
            
            volatile uint8_t* algorithmBytes = (volatile uint8_t*)pluginAlgorithm;
            uint8_t testByte = algorithmBytes[0]; // This should not crash if memory is valid
            (void)testByte; // Prevent optimization
            
            std::cout << "  ✓ Can read algorithm first byte: 0x" << std::hex << (int)testByte << std::dec << std::endl;
            
            // Dump the first part of the algorithm structure
            std::cout << "  Algorithm structure dump:" << std::endl;
            hexDump(pluginAlgorithm, sizeof(_NT_algorithm));
            
            // Now try to access the parameters field
            std::cout << "  Accessing parameters field..." << std::endl;
            std::cout << "  pluginAlgorithm->parameters = " << std::hex << pluginAlgorithm->parameters << std::dec << std::endl;
            
            // Validate parameters pointer - it might be NULL which is valid
            if (pluginAlgorithm->parameters == nullptr) {
                std::cout << "  ℹ parameters pointer is NULL (plugin may have no parameters)" << std::endl;
                return;
            }
            
            if (!isValidPointer((void*)pluginAlgorithm->parameters)) {
                std::cout << "  ✗ parameters pointer is invalid (" << std::hex << pluginAlgorithm->parameters << std::dec << ")" << std::endl;
                return;
            }
            
            std::cout << "  ✓ parameters pointer looks valid" << std::endl;
            
            // Dump the parameters array
            std::cout << "  Parameters array dump:" << std::endl;
            hexDump((void*)pluginAlgorithm->parameters, 16 * sizeof(_NT_parameter));
            
            // Use a much simpler approach - avoid complex pointer arithmetic
            // Just try to access the first few parameters directly
            const int MAX_SAFE_PARAMS = 16; // Very conservative limit
            
            for (int paramIndex = 0; paramIndex < MAX_SAFE_PARAMS; paramIndex++) {
                std::cout << "  Testing parameter " << paramIndex << "..." << std::endl;
                
                // Check if this parameter slot is valid
                const _NT_parameter* currentParam = &pluginAlgorithm->parameters[paramIndex];
                
                std::cout << "    Parameter address: " << std::hex << currentParam << std::dec << std::endl;
                
                if (!isValidPointer((void*)currentParam)) {
                    std::cout << "    ✗ Parameter pointer is invalid - end of parameters" << std::endl;
                    break;
                }
                
                std::cout << "    ✓ Parameter pointer is valid" << std::endl;
                
                // Dump this parameter structure
                std::cout << "    Parameter " << paramIndex << " structure dump:" << std::endl;
                hexDump((void*)currentParam, sizeof(_NT_parameter));
                
                // Check if name pointer is valid
                std::cout << "    currentParam->name = " << std::hex << currentParam->name << std::dec << std::endl;
                
                if (!isValidPointer((void*)currentParam->name)) {
                    std::cout << "    ✗ Parameter name pointer is invalid - end of parameters" << std::endl;
                    break;
                }
                
                // Validate the parameter name by checking first character
                try {
                    std::cout << "    Testing name string access..." << std::endl;
                    volatile char firstChar = currentParam->name[0];
                    
                    if (firstChar == '\0') {
                        std::cout << "    ℹ Empty name - end of parameters" << std::endl;
                        break;
                    }
                    
                    // Try to read the full name string safely
                    std::string name;
                    for (int i = 0; i < 64; i++) { // Max 64 chars
                        volatile char c = currentParam->name[i];
                        if (c == '\0') break;
                        name += c;
                    }
                    
                    std::cout << "    ✓ Parameter name: \"" << name << "\"" << std::endl;
                    
                } catch (...) {
                    std::cout << "    ✗ Failed to read parameter name - end of parameters" << std::endl;
                    break;
                }
                
                // Try to access other fields
                try {
                    std::cout << "    Parameter values: min=" << currentParam->min 
                             << ", max=" << currentParam->max 
                             << ", def=" << currentParam->def << std::endl;
                             
                    // Validate parameter bounds
                    if (currentParam->min > currentParam->max) {
                        std::cout << "    ⚠ Invalid parameter bounds: min > max" << std::endl;
                        break;
                    }
                    
                    // Copy the parameter safely
                    _NT_parameter paramCopy;
                    paramCopy.name = currentParam->name;
                    paramCopy.min = currentParam->min;
                    paramCopy.max = currentParam->max;
                    paramCopy.def = currentParam->def;
                    paramCopy.unit = currentParam->unit;
                    paramCopy.scaling = currentParam->scaling;
                    paramCopy.enumStrings = currentParam->enumStrings;
                    
                    parameters.push_back(paramCopy);
                    
                    std::cout << "    ✓ Parameter " << paramIndex << " copied successfully" << std::endl;
                    
                } catch (...) {
                    std::cout << "    ✗ Exception accessing parameter fields - end of parameters" << std::endl;
                    break;
                }
            }
            
            std::cout << "✓ Parameter extraction completed. Found " << parameters.size() << " parameters." << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "✗ Exception in extractParameterData: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "✗ Unknown exception in extractParameterData" << std::endl;
        }
    }
    
    bool loadAndConstructPlugin() {
        std::cout << "=== Loading and constructing simple_gain.dylib ===" << std::endl;
        
        // Try to load the simple gain plugin
        std::string pluginPath = "./simple_gain.dylib";
        
        pluginHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!pluginHandle) {
            std::cout << "✗ Failed to load plugin: " << dlerror() << std::endl;
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
        
        std::cout << "✓ Factory obtained: " << std::hex << pluginFactory << std::dec << std::endl;
        
        // Test factory info
        if (pluginFactory->name) {
            std::cout << "  Plugin Name: " << pluginFactory->name << std::endl;
        }
        
        if (pluginFactory->description) {
            std::cout << "  Description: " << pluginFactory->description << std::endl;
        }
        
        // Construct algorithm
        return constructAlgorithm();
    }
    
    bool constructAlgorithm() {
        std::cout << "=== Constructing Algorithm ===" << std::endl;
        
        if (!pluginFactory) {
            std::cout << "✗ No factory available" << std::endl;
            return false;
        }
        
        // Validate construct function
        if (!pluginFactory->construct) {
            std::cout << "✗ No construct function" << std::endl;
            return false;
        }
        
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
        
        // Calculate total memory needed
        size_t totalMem = reqs.sram + reqs.dram + reqs.dtc + reqs.itc;
        if (totalMem == 0) totalMem = 4096; // Minimum allocation
        
        if (posix_memalign(&pluginInstanceMemory, 16, totalMem) != 0) {
            std::cout << "✗ Failed to allocate memory" << std::endl;
            return false;
        }
        
        // Initialize memory to zero
        memset(pluginInstanceMemory, 0, totalMem);
        
        std::cout << "✓ Memory allocated: " << totalMem << " bytes at " << std::hex << pluginInstanceMemory << std::dec << std::endl;
        
        // Set up memory pointers
        _NT_algorithmMemoryPtrs memPtrs{};
        uint8_t* ptr = (uint8_t*)pluginInstanceMemory;
        memPtrs.sram = ptr; ptr += std::max(reqs.sram, (uint32_t)1024);
        memPtrs.dram = ptr; ptr += std::max(reqs.dram, (uint32_t)1024);
        memPtrs.dtc = ptr; ptr += std::max(reqs.dtc, (uint32_t)1024);
        memPtrs.itc = ptr;
        
        std::cout << "  SRAM ptr: " << std::hex << memPtrs.sram << std::dec << std::endl;
        std::cout << "  DRAM ptr: " << std::hex << memPtrs.dram << std::dec << std::endl;
        std::cout << "  DTC ptr: " << std::hex << memPtrs.dtc << std::dec << std::endl;
        std::cout << "  ITC ptr: " << std::hex << memPtrs.itc << std::dec << std::endl;
        
        // construct() returns status code, algorithm is placed at SRAM location
        _NT_algorithm* result = pluginFactory->construct(memPtrs, reqs, nullptr);
        
        std::cout << "  construct() returned: " << std::hex << result << std::dec << std::endl;
        
        // Check if construct returned a valid pointer or if algorithm is at SRAM location
        if (result != nullptr && (uintptr_t)result > 0x1000) {
            // Use returned pointer
            pluginAlgorithm = result;
            std::cout << "  ✓ Using returned pointer" << std::endl;
        } else {
            // Algorithm should be constructed at SRAM location
            pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
            std::cout << "  ✓ Using SRAM location (construct returned status: " << (unsigned)(uintptr_t)result << ")" << std::endl;
        }
        
        // Validate the algorithm pointer
        if (!pluginAlgorithm) {
            std::cout << "✗ Algorithm pointer is NULL" << std::endl;
            return false;
        }
        
        if (!isValidPointer(pluginAlgorithm)) {
            std::cout << "✗ Algorithm pointer validation failed: " << std::hex << pluginAlgorithm << std::dec << std::endl;
            return false;
        }
        
        // Try to validate the algorithm structure by reading first few bytes
        try {
            volatile uint8_t* testPtr = (volatile uint8_t*)pluginAlgorithm;
            uint8_t testByte = testPtr[0];
            (void)testByte;
            std::cout << "✓ Algorithm memory validation: first byte readable" << std::endl;
        } catch (...) {
            std::cout << "✗ Algorithm memory validation failed - cannot read first byte" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void runAllTests() {
        std::cout << "=== Comprehensive Parameter Extraction Tests ===" << std::endl;
        
        if (!loadAndConstructPlugin()) {
            std::cout << "✗ Plugin loading/construction failed" << std::endl;
            return;
        }
        
        // Now test the parameter extraction function
        testExtractParameterData();
        
        cleanup();
    }
};

int main() {
    ParameterExtractorTester tester;
    tester.runAllTests();
    return 0;
}