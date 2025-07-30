#include <iostream>
#include <cstdint>
#include "../emulator/include/distingnt/api.h"

struct _NT_algorithm_old {
    void* refCon;                                               // offset 0 (OLD API)
    unsigned int (*getAPIVersion)(struct _NT_algorithm* self);  // offset 8
    // ... other function pointers
};

int main() {
    std::cout << "=== Structure Mismatch Analysis ===" << std::endl;
    
    // Create an algorithm using current API structure
    _NT_algorithm current_algo;
    current_algo.parameters = nullptr;
    current_algo.parameterPages = nullptr;
    current_algo.vIncludingCommon = nullptr;
    current_algo.v = nullptr;
    
    std::cout << "Current API _NT_algorithm structure:" << std::endl;
    std::cout << "  parameters: " << std::hex << current_algo.parameters << std::dec << std::endl;
    std::cout << "  parameterPages: " << std::hex << current_algo.parameterPages << std::dec << std::endl;
    
    // Now simulate what simple_gain.cpp does: algorithm.refCon = plugin
    // This will actually write to the parameters field!
    void* fake_plugin = (void*)0x3f800000;  // Simulate the crash address
    
    // Cast to old structure and set refCon
    _NT_algorithm_old* old_view = (_NT_algorithm_old*)&current_algo;
    old_view->refCon = fake_plugin;
    
    std::cout << "\nAfter setting 'refCon' (which is actually parameters field):" << std::endl;
    std::cout << "  parameters: " << std::hex << current_algo.parameters << std::dec << std::endl;
    std::cout << "  parameterPages: " << std::hex << current_algo.parameterPages << std::dec << std::endl;
    
    // This proves that setting refCon corrupts the parameters field!
    if (current_algo.parameters == (const _NT_parameter*)0x3f800000) {
        std::cout << "\n🎯 BINGO! Setting 'refCon' corrupted the parameters field to 0x3f800000!" << std::endl;
        std::cout << "This is exactly what causes the crash in extractParameterData()!" << std::endl;
    }
    
    return 0;
}