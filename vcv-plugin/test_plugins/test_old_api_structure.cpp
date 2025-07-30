#include <iostream>
#include <cassert>
#include <dlfcn.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <iomanip>

// Include the current API header
#include "../emulator/include/distingnt/api.h"

// Define the OLD API structures based on the simple_gain.cpp source
struct _NT_factory_old {
    void* refCon;
    unsigned int (*getAPIVersion)(struct _NT_factory_old* self);
    int (*getValue)(struct _NT_factory_old* self, enum _NT_selector selector, void* value, unsigned int maxLength);
    struct _NT_staticRequirements (*getStaticRequirements)(struct _NT_factory_old* self);
    int (*initialise)(struct _NT_factory_old* self, void* sharedMemory);
    struct _NT_algorithm* (*construct)(struct _NT_factory_old* self, void* memory);
    void (*destruct)(struct _NT_factory_old* self, struct _NT_algorithm* algorithm);
    void (*terminate)(struct _NT_factory_old* self);
    struct _NT_memoryRequirements (*getRequirements)(struct _NT_factory_old* self);
};

struct _NT_algorithm_old {
    void* refCon;
    unsigned int (*getAPIVersion)(struct _NT_algorithm_old* self);
    int (*getValue)(struct _NT_algorithm_old* self, enum _NT_selector selector, void* value, unsigned int maxLength);
    struct _NT_memoryRequirements (*getRequirements)(struct _NT_algorithm_old* self);
    void (*setParameterValue)(struct _NT_algorithm_old* self, unsigned int parameterIndex, float value);
    float (*getParameterValue)(struct _NT_algorithm_old* self, unsigned int parameterIndex);
    void (*step)(struct _NT_algorithm_old* self, float** buffers, unsigned int numSamples);
    void (*draw)(struct _NT_algorithm_old* self);
    // ... MIDI handlers and other functions follow
};

// Define old requirement structures (based on what simple_gain expects)
struct _NT_staticRequirements_old {
    unsigned int memorySize;
    unsigned int numRequirements;
    void* requirements;
};

struct _NT_memoryRequirements_old {
    unsigned int memorySize;
    unsigned int numRequirements;
    void* requirements;
};

class OldAPITester {
private:
    void* pluginHandle = nullptr;
    _NT_factory_old* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginInstanceMemory = nullptr;
    void* pluginSharedMemory = nullptr;
    
public:
    ~OldAPITester() {
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
        
        for (size_t i = 0; i < size && i < 128; i += 16) {
            std::cout << "      " << std::hex << std::setw(8) << std::setfill('0') << (uintptr_t)(bytes + i) << ": ";
            
            for (size_t j = 0; j < 16 && i + j < size; j++) {
                try {
                    volatile uint8_t byte = bytes[i + j];
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                } catch (...) {
                    std::cout << "?? ";
                }
            }
            
            for (size_t j = size - i; j < 16 && j < 16; j++) {
                std::cout << "   ";
            }
            
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
    
    bool loadPlugin() {
        std::cout << "=== Loading simple_gain.dylib with OLD API ===" << std::endl;
        
        std::string pluginPath = "./simple_gain.dylib";
        pluginHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!pluginHandle) {
            std::cout << "✗ Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        std::cout << "✓ Plugin loaded successfully" << std::endl;
        
        // Get the old API factory function
        typedef _NT_factory_old* (*FactoryFunc)();
        FactoryFunc getFactory = (FactoryFunc)dlsym(pluginHandle, "NT_getFactoryPtr");
        
        if (!getFactory) {
            std::cout << "✗ No NT_getFactoryPtr function found" << std::endl;
            return false;
        }
        
        pluginFactory = getFactory();
        if (!pluginFactory) {
            std::cout << "✗ Factory returned null" << std::endl;
            return false;
        }
        
        std::cout << "✓ Old factory obtained: " << std::hex << pluginFactory << std::dec << std::endl;
        return true;
    }
    
    bool testOldFactoryStructure() {
        std::cout << "=== Testing Old Factory Structure ===" << std::endl;
        
        if (!pluginFactory) {
            std::cout << "✗ No factory available" << std::endl;
            return false;
        }
        
        std::cout << "  sizeof(_NT_factory_old): " << sizeof(_NT_factory_old) << std::endl;
        std::cout << "  sizeof(_NT_factory): " << sizeof(_NT_factory) << std::endl;
        
        // Dump the factory structure using old format
        hexDump(pluginFactory, sizeof(_NT_factory_old), "Old factory structure");
        
        // Test reading fields using old structure
        std::cout << "  Reading old factory fields:" << std::endl;
        try {
            std::cout << "    refCon: " << std::hex << pluginFactory->refCon << std::dec << std::endl;
            std::cout << "    getAPIVersion: " << std::hex << (void*)pluginFactory->getAPIVersion << std::dec << std::endl;
            std::cout << "    getValue: " << std::hex << (void*)pluginFactory->getValue << std::dec << std::endl;
            std::cout << "    construct: " << std::hex << (void*)pluginFactory->construct << std::dec << std::endl;
            
            // Test calling functions
            if (pluginFactory->getAPIVersion) {
                unsigned int apiVersion = pluginFactory->getAPIVersion(pluginFactory);
                std::cout << "    API Version: " << apiVersion << std::endl;
            }
            
            if (pluginFactory->getValue) {
                char buffer[256];
                int result = pluginFactory->getValue(pluginFactory, kNT_selector_factoryName, buffer, sizeof(buffer));
                if (result == 0) {
                    std::cout << "    Algorithm Name: \"" << buffer << "\"" << std::endl;
                }
            }
            
        } catch (...) {
            std::cout << "✗ Exception reading old factory fields" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool testOldAlgorithmConstruction() {
        std::cout << "=== Testing Old Algorithm Construction ===" << std::endl;
        
        if (!pluginFactory || !pluginFactory->construct) {
            std::cout << "✗ No construct function available" << std::endl;
            return false;
        }
        
        // Get memory requirements using old API
        _NT_memoryRequirements_old reqs = {0, 0, nullptr};
        if (pluginFactory->getRequirements) {
            try {
                _NT_algorithmRequirements reqs = {0};
                pluginFactory->getRequirements(pluginFactory);
                std::cout << "  Memory requirements: (function called)" << std::endl;
                
                // Allocate memory
                if (oldReqs.memorySize > 0) {
                    pluginInstanceMemory = malloc(oldReqs.memorySize);
                    if (!pluginInstanceMemory) {
                        std::cout << "✗ Failed to allocate memory" << std::endl;
                        return false;
                    }
                    
                    memset(pluginInstanceMemory, 0, oldReqs.memorySize);
                    std::cout << "✓ Memory allocated: " << oldReqs.memorySize << " bytes at " << std::hex << pluginInstanceMemory << std::dec << std::endl;
                    
                    // Construct the algorithm using old API
                    pluginAlgorithm = pluginFactory->construct(pluginFactory, pluginInstanceMemory);
                    
                    if (!pluginAlgorithm) {
                        std::cout << "✗ Algorithm construction failed" << std::endl;
                        return false;
                    }
                    
                    std::cout << "✓ Algorithm constructed: " << std::hex << pluginAlgorithm << std::dec << std::endl;
                    
                    // Dump the algorithm structure
                    hexDump(pluginAlgorithm, 128, "Old algorithm structure");
                    
                    return testOldAlgorithmStructure();
                    
                } else {
                    std::cout << "ⓘ Plugin requires no instance memory" << std::endl;
                }
                
            } catch (...) {
                std::cout << "✗ Exception during old algorithm construction" << std::endl;
                return false;
            }
        }
        
        return false;
    }
    
    bool testOldAlgorithmStructure() {
        std::cout << "=== Testing Old Algorithm Structure ===" << std::endl;
        
        if (!pluginAlgorithm) {
            std::cout << "✗ No algorithm available" << std::endl;
            return false;
        }
        
        std::cout << "  sizeof(_NT_algorithm_old): " << sizeof(_NT_algorithm_old) << std::endl;
        std::cout << "  sizeof(_NT_algorithm): " << sizeof(_NT_algorithm) << std::endl;
        
        // Cast the algorithm to the old structure format
        _NT_algorithm_old* oldAlgorithm = (_NT_algorithm_old*)pluginAlgorithm;
        
        try {
            std::cout << "  Reading old algorithm fields:" << std::endl;
            std::cout << "    refCon: " << std::hex << oldAlgorithm->refCon << std::dec << std::endl;
            std::cout << "    getAPIVersion: " << std::hex << (void*)oldAlgorithm->getAPIVersion << std::dec << std::endl;
            std::cout << "    step: " << std::hex << (void*)oldAlgorithm->step << std::dec << std::endl;
            
            // Now here's the key insight: the old algorithm structure does NOT have a "parameters" field!
            // The current _NT_algorithm structure expects:
            //   - const _NT_parameter *parameters;          // offset 0
            //   - const _NT_parameterPages *parameterPages; // offset 8
            //   - const int16_t *vIncludingCommon;          // offset 16
            //   - const int16_t *v;                         // offset 24
            //
            // But the old structure has:
            //   - void* refCon;                             // offset 0
            //   - function pointer                          // offset 8
            //   - function pointer                          // offset 16
            //   - function pointer                          // offset 24
            //
            // So when our VCV code tries to read `pluginAlgorithm->parameters` (at offset 0),
            // it's actually reading the `refCon` pointer from the old structure!
            
            std::cout << "\n  🎯 ROOT CAUSE ANALYSIS:" << std::endl;
            std::cout << "    Our VCV code expects _NT_algorithm.parameters at offset 0" << std::endl;
            std::cout << "    But old API has _NT_algorithm_old.refCon at offset 0" << std::endl;
            std::cout << "    The refCon value is: " << std::hex << oldAlgorithm->refCon << std::dec << std::endl;
            
            if (oldAlgorithm->refCon == (void*)0x3f800000) {
                std::cout << "    ⚠ BINGO! The refCon is 0x3f800000 - this is what causes the crash!" << std::endl;
            }
            
            // The crash happens because:
            // 1. VCV code calls extractParameterData()
            // 2. It tries to read pluginAlgorithm->parameters (expecting new API structure)
            // 3. But it's actually reading pluginAlgorithm_old->refCon (old API structure)
            // 4. If refCon happens to be 0x3f800000, accessing that as parameters array crashes
            
            return true;
            
        } catch (...) {
            std::cout << "✗ Exception reading old algorithm structure" << std::endl;
            return false;
        }
    }
    
    void runAllTests() {
        std::cout << "=== Old API Structure Analysis ===" << std::endl;
        std::cout << "Testing simple_gain.dylib with correct OLD API structures" << std::endl;
        
        bool success = true;
        
        success &= loadPlugin();
        if (!success) { cleanup(); return; }
        
        success &= testOldFactoryStructure();
        if (!success) { cleanup(); return; }
        
        success &= testOldAlgorithmConstruction();
        // Algorithm construction handles its own reporting
        
        cleanup();
        
        std::cout << "\n=== CONCLUSION ===" << std::endl;
        std::cout << "The crash occurs because simple_gain.dylib uses OLD API structures" << std::endl;
        std::cout << "but our VCV module expects NEW API structures." << std::endl;
        std::cout << "Structure field misalignment causes memory corruption." << std::endl;
    }
};

int main() {
    OldAPITester tester;
    tester.runAllTests();
    return 0;
}