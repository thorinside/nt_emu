#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <distingnt/api.h>
#include <iomanip>

class ParameterExtractor {
private:
    std::string getUnitName(uint8_t unit) {
        switch (unit) {
            case kNT_unitNone: return "None";
            case kNT_unitEnum: return "Enum";
            case kNT_unitDb: return "dB";
            case kNT_unitDb_minInf: return "dB (-inf)";
            case kNT_unitPercent: return "Percent";
            case kNT_unitHz: return "Hz";
            case kNT_unitSemitones: return "Semitones";
            case kNT_unitCents: return "Cents";
            case kNT_unitMs: return "ms";
            case kNT_unitSeconds: return "Seconds";
            case kNT_unitFrames: return "Frames";
            case kNT_unitMIDINote: return "MIDI Note";
            case kNT_unitMillivolts: return "mV";
            case kNT_unitVolts: return "V";
            case kNT_unitBPM: return "BPM";
            case kNT_unitAudioInput: return "Audio Input";
            case kNT_unitCvInput: return "CV Input";
            case kNT_unitAudioOutput: return "Audio Output";
            case kNT_unitCvOutput: return "CV Output";
            case kNT_unitOutputMode: return "Output Mode";
            default: return "Unknown (" + std::to_string(unit) + ")";
        }
    }
    
    std::string getScalingName(uint8_t scaling) {
        switch (scaling) {
            case kNT_scalingNone: return "None";
            case kNT_scaling10: return "x10";
            case kNT_scaling100: return "x100";
            case kNT_scaling1000: return "x1000";
            default: return "Unknown (" + std::to_string(scaling) + ")";
        }
    }
    
    void printEnumStrings(const _NT_parameter& param) {
        if (param.unit == kNT_unitEnum && param.enumStrings) {
            std::cout << "      Enum Values:" << std::endl;
            int enumIndex = 0;
            while (param.enumStrings[enumIndex] != nullptr) {
                std::cout << "        [" << enumIndex << "] \"" 
                         << param.enumStrings[enumIndex] << "\"" << std::endl;
                enumIndex++;
                if (enumIndex > 100) break; // Safety limit
            }
        }
    }
    
public:
    bool extractParameters(const std::string& pluginName) {
        std::cout << "\n=== Parameter Analysis: " << pluginName << " ===" << std::endl;
        
        // Load plugin
        std::string pluginPath = pluginName + ".dylib";
        void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cout << "✗ Failed to load " << pluginName << ": " << dlerror() << std::endl;
            return false;
        }
        
        // Get plugin entry
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(handle, "pluginEntry");
        if (!pluginEntry) {
            std::cout << "✗ No pluginEntry function" << std::endl;
            dlclose(handle);
            return false;
        }
        
        // Get factory
        _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
        if (!factory) {
            std::cout << "✗ No factory" << std::endl;
            dlclose(handle);
            return false;
        }
        
        // Display GUID
        char guidStr[5] = {0};
        guidStr[0] = (factory->guid >> 0) & 0xFF;
        guidStr[1] = (factory->guid >> 8) & 0xFF;
        guidStr[2] = (factory->guid >> 16) & 0xFF;
        guidStr[3] = (factory->guid >> 24) & 0xFF;
        std::cout << "Plugin: " << guidStr << " - " << factory->name << std::endl;
        
        // Test construction to get parameter structures
        if (!factory->calculateRequirements || !factory->construct) {
            std::cout << "⚠ Missing required construction functions" << std::endl;
            dlclose(handle);
            return false;
        }
        
        _NT_algorithmRequirements reqs;
        memset(&reqs, 0, sizeof(reqs));
        factory->calculateRequirements(reqs, nullptr);
        
        std::cout << "Parameter Count: " << reqs.numParameters << std::endl;
        
        if (reqs.numParameters == 0) {
            std::cout << "No parameters to analyze" << std::endl;
            dlclose(handle);
            return true;
        }
        
        // Construct algorithm to access parameter structures
        void* memory = malloc(reqs.sram + reqs.dram + reqs.dtc + reqs.itc + 1024);
        if (!memory) {
            std::cout << "✗ Failed to allocate memory" << std::endl;
            dlclose(handle);
            return false;
        }
        
        memset(memory, 0, reqs.sram + reqs.dram + reqs.dtc + reqs.itc + 1024);
        
        _NT_algorithmMemoryPtrs memPtrs;
        memset(&memPtrs, 0, sizeof(memPtrs));
        memPtrs.sram = (uint8_t*)memory;
        memPtrs.dram = memPtrs.sram + reqs.sram;
        memPtrs.dtc = memPtrs.dram + reqs.dram;
        memPtrs.itc = memPtrs.dtc + reqs.dtc;
        
        _NT_algorithm* algorithm = factory->construct(memPtrs, reqs, nullptr);
        if (!algorithm) {
            std::cout << "✗ Algorithm construction failed" << std::endl;
            free(memory);
            dlclose(handle);
            return false;
        }
        
        // Extract and analyze parameters
        try {
            const _NT_parameter* parameters = algorithm->parameters;
            if (!parameters) {
                std::cout << "⚠ No parameters array available" << std::endl;
                free(memory);
                dlclose(handle);
                return true;
            }
            
            std::cout << "\n--- Parameter Details ---" << std::endl;
            for (uint32_t i = 0; i < reqs.numParameters; i++) {
                const _NT_parameter& param = parameters[i];
                
                std::cout << "Parameter [" << i << "]: \"" << (param.name ? param.name : "NULL") << "\"" << std::endl;
                std::cout << "    Range: " << param.min << " to " << param.max 
                         << " (default: " << param.def << ")" << std::endl;
                std::cout << "    Unit: " << getUnitName(param.unit) << std::endl;
                std::cout << "    Scaling: " << getScalingName(param.scaling) << std::endl;
                
                // Print enum strings if applicable
                printEnumStrings(param);
                
                // Show current value if algorithm has v array
                if (algorithm->v) {
                    std::cout << "    Current Value: " << algorithm->v[i] << std::endl;
                }
                
                std::cout << std::endl;
            }
            
            // Extract and analyze parameter pages
            analyzeParameterPages(algorithm);
            
        } catch (...) {
            std::cout << "✗ Exception while analyzing parameters" << std::endl;
            free(memory);
            dlclose(handle);
            return false;
        }
        
        free(memory);
        dlclose(handle);
        return true;
    }
    
private:
    void analyzeParameterPages(const _NT_algorithm* algorithm) {
        if (!algorithm->parameterPages) {
            std::cout << "--- No Parameter Pages ---" << std::endl;
            return;
        }
        
        std::cout << "--- Parameter Pages ---" << std::endl;
        const _NT_parameterPages* pages = algorithm->parameterPages;
        
        std::cout << "Total Pages: " << pages->numPages << std::endl;
        
        if (!pages->pages) {
            std::cout << "⚠ Pages array is null" << std::endl;
            return;
        }
        
        try {
            for (uint32_t pageIdx = 0; pageIdx < pages->numPages; pageIdx++) {
                const _NT_parameterPage& page = pages->pages[pageIdx];
                
                std::cout << "\nPage [" << pageIdx << "]: \"" << (page.name ? page.name : "NULL") << "\"" << std::endl;
                std::cout << "  Parameter Count: " << (int)page.numParams << std::endl;
                
                if (page.params && page.numParams > 0) {
                    std::cout << "  Parameter Indices: ";
                    for (uint8_t paramIdx = 0; paramIdx < page.numParams; paramIdx++) {
                        if (paramIdx > 0) std::cout << ", ";
                        std::cout << (int)page.params[paramIdx];
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << "  ⚠ No parameter indices available" << std::endl;
                }
            }
        } catch (...) {
            std::cout << "✗ Exception while analyzing parameter pages" << std::endl;
        }
    }
    
public:
    void runAllParameterTests() {
        std::cout << "=== Comprehensive Parameter Analysis Suite ===" << std::endl;
        
        std::vector<std::string> pluginNames = {
            "gain", "multiple", "monosynth", "fourteen"
        };
        
        int successCount = 0;
        for (const auto& name : pluginNames) {
            if (extractParameters(name)) {
                successCount++;
            }
        }
        
        std::cout << "\n=== Summary ===" << std::endl;
        std::cout << "Analyzed " << successCount << "/" << pluginNames.size() << " plugins successfully" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    ParameterExtractor extractor;
    
    if (argc == 2) {
        // Test single plugin
        std::string pluginName = argv[1];
        extractor.extractParameters(pluginName);
    } else {
        // Test all plugins
        extractor.runAllParameterTests();
    }
    
    return 0;
}