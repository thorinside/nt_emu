#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>

// Include the API header
#include "../emulator/include/distingnt/api.h"

// Mock the rack module context for testing
struct MockModule {
    enum MenuMode {
        MENU_OFF,
        MENU_PAGE_SELECT,
        MENU_PARAM_SELECT,
        MENU_VALUE_EDIT
    };
    
    MenuMode menuMode = MENU_OFF;
    int currentPageIndex = 0;
    int currentParamIndex = 0;
    
    std::vector<_NT_parameterPage> parameterPages;
    std::vector<_NT_parameter> parameters;
    std::array<int, 256> routingMatrix;
    
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginInstanceMemory = nullptr;
    
    bool loadPlugin(const std::string& path) {
        pluginHandle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!pluginHandle) {
            std::cout << "Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(pluginHandle, "pluginEntry");
        if (!pluginEntry) {
            std::cout << "No pluginEntry function" << std::endl;
            dlclose(pluginHandle);
            pluginHandle = nullptr;
            return false;
        }
        
        pluginFactory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
        if (!pluginFactory) {
            std::cout << "No factory" << std::endl;
            dlclose(pluginHandle);
            pluginHandle = nullptr;
            return false;
        }
        
        // Display GUID as four character ASCII
        char guidStr[5] = {0};
        guidStr[0] = (pluginFactory->guid >> 0) & 0xFF;
        guidStr[1] = (pluginFactory->guid >> 8) & 0xFF;
        guidStr[2] = (pluginFactory->guid >> 16) & 0xFF;
        guidStr[3] = (pluginFactory->guid >> 24) & 0xFF;
        std::cout << "Plugin GUID: " << guidStr << " - " << pluginFactory->name << std::endl;
        
        // Test construction
        if (pluginFactory->calculateRequirements && pluginFactory->construct) {
            _NT_algorithmRequirements reqs;
            memset(&reqs, 0, sizeof(reqs));
            pluginFactory->calculateRequirements(reqs, nullptr);
            
            pluginInstanceMemory = malloc(reqs.sram + reqs.dram + reqs.dtc + reqs.itc + 1024);
            if (pluginInstanceMemory) {
                memset(pluginInstanceMemory, 0, reqs.sram + reqs.dram + reqs.dtc + reqs.itc + 1024);
                
                _NT_algorithmMemoryPtrs memPtrs;
                memset(&memPtrs, 0, sizeof(memPtrs));
                memPtrs.sram = (uint8_t*)pluginInstanceMemory;
                memPtrs.dram = memPtrs.sram + reqs.sram;
                memPtrs.dtc = memPtrs.dram + reqs.dram;
                memPtrs.itc = memPtrs.dtc + reqs.dtc;
                
                pluginAlgorithm = pluginFactory->construct(memPtrs, reqs, nullptr);
                if (pluginAlgorithm) {
                    extractParameterData();
                    return true;
                }
            }
        }
        
        dlclose(pluginHandle);
        pluginHandle = nullptr;
        return false;
    }
    
    void extractParameterData() {
        parameterPages.clear();
        parameters.clear();
        
        if (!pluginAlgorithm || !pluginFactory || !pluginFactory->calculateRequirements) {
            std::cout << "Invalid plugin state for parameter extraction" << std::endl;
            return;
        }
        
        try {
            _NT_algorithmRequirements reqs;
            memset(&reqs, 0, sizeof(reqs));
            pluginFactory->calculateRequirements(reqs, nullptr);
            
            std::cout << "Expected parameters: " << reqs.numParameters << std::endl;
            
            // Extract parameters
            const _NT_parameter* parametersPtr = pluginAlgorithm->parameters;
            if (parametersPtr && reqs.numParameters > 0) {
                for (uint32_t i = 0; i < reqs.numParameters; i++) {
                    const _NT_parameter* param = &parametersPtr[i];
                    
                    _NT_parameter paramCopy;
                    paramCopy.name = param->name;
                    paramCopy.min = param->min;
                    paramCopy.max = param->max;
                    paramCopy.def = param->def;
                    paramCopy.unit = param->unit;
                    paramCopy.scaling = param->scaling;
                    paramCopy.enumStrings = param->enumStrings;
                    
                    parameters.push_back(paramCopy);
                    std::cout << "Parameter " << i << ": " << param->name << " [" << param->min << "-" << param->max << "]" << std::endl;
                }
            }
            
            // Extract parameter pages  
            const _NT_parameterPages* parameterPagesPtr = pluginAlgorithm->parameterPages;
            if (parameterPagesPtr && parameterPagesPtr->numPages > 0) {
                for (uint32_t pageIdx = 0; pageIdx < parameterPagesPtr->numPages; pageIdx++) {
                    const _NT_parameterPage* page = &parameterPagesPtr->pages[pageIdx];
                    
                    _NT_parameterPage pageCopy;
                    pageCopy.name = page->name;
                    pageCopy.numParams = page->numParams;
                    pageCopy.params = page->params;
                    
                    parameterPages.push_back(pageCopy);
                    std::cout << "Page " << pageIdx << ": " << page->name << " (" << page->numParams << " params)" << std::endl;
                }
            }
            
            // Create default page if needed
            if (!parameters.empty() && parameterPages.empty()) {
                _NT_parameterPage defaultPage;
                defaultPage.name = "Parameters";
                defaultPage.numParams = parameters.size();
                defaultPage.params = nullptr;
                parameterPages.push_back(defaultPage);
                std::cout << "Created default page" << std::endl;
            }
            
        } catch (...) {
            std::cout << "Exception during parameter extraction" << std::endl;
        }
    }
    
    void formatParameterValue(char* str, const _NT_parameter& param, int value) {
        float scaledValue = value;
        switch (param.scaling) {
            case kNT_scaling10: scaledValue = value / 10.0f; break;
            case kNT_scaling100: scaledValue = value / 100.0f; break;
            case kNT_scaling1000: scaledValue = value / 1000.0f; break;
        }
        
        switch (param.unit) {
            case kNT_unitEnum:
                if (param.enumStrings && value >= param.min && value <= param.max) {
                    strncpy(str, param.enumStrings[value - param.min], 31);
                    str[31] = '\0';
                } else {
                    sprintf(str, "%d", value);
                }
                break;
            case kNT_unitPercent:
                sprintf(str, "%d%%", value);
                break;
            case kNT_unitVolts:
                sprintf(str, "%.2f V", scaledValue);
                break;
            case kNT_unitHz:
                sprintf(str, "%.1f Hz", scaledValue);
                break;
            case kNT_unitMs:
                sprintf(str, "%.1f ms", scaledValue);
                break;
            case kNT_unitDb:
                sprintf(str, "%.1f dB", scaledValue);
                break;
            default:
                sprintf(str, "%d", value);
                break;
        }
    }
    
    void testParameterFormatting() {
        std::cout << "\n=== Parameter Formatting Test ===" << std::endl;
        
        for (size_t i = 0; i < parameters.size() && i < 5; i++) { // Test first 5 parameters
            const _NT_parameter& param = parameters[i];
            char valueStr[64];
            
            // Test default value
            formatParameterValue(valueStr, param, param.def);
            std::cout << "Parameter " << i << " (" << param.name << "): default=" << valueStr << std::endl;
            
            // Test min and max values
            formatParameterValue(valueStr, param, param.min);
            std::cout << "  min=" << valueStr;
            
            formatParameterValue(valueStr, param, param.max);
            std::cout << ", max=" << valueStr << std::endl;
        }
    }
    
    ~MockModule() {
        if (pluginInstanceMemory) free(pluginInstanceMemory);
        if (pluginHandle) dlclose(pluginHandle);
    }
};

int main() {
    std::cout << "=== VCV Parameter Integration Test ===" << std::endl;
    
    MockModule module;
    
    // Test scaling demo plugin
    std::cout << "\n--- Testing scaling_demo.dylib ---" << std::endl;
    if (module.loadPlugin("./scaling_demo.dylib")) {
        std::cout << "✓ Loaded successfully" << std::endl;
        std::cout << "Parameters extracted: " << module.parameters.size() << std::endl;
        std::cout << "Pages extracted: " << module.parameterPages.size() << std::endl;
        
        module.testParameterFormatting();
    } else {
        std::cout << "✗ Failed to load scaling_demo.dylib" << std::endl;
    }
    
    // Test gain plugin
    std::cout << "\n--- Testing gain.dylib ---" << std::endl;
    MockModule module2;
    if (module2.loadPlugin("./gain.dylib")) {
        std::cout << "✓ Loaded successfully" << std::endl;
        std::cout << "Parameters extracted: " << module2.parameters.size() << std::endl;
        std::cout << "Pages extracted: " << module2.parameterPages.size() << std::endl;
        
        module2.testParameterFormatting();
    } else {
        std::cout << "✗ Failed to load gain.dylib" << std::endl;
    }
    
    std::cout << "\n✓ Integration test completed" << std::endl;
    return 0;
}