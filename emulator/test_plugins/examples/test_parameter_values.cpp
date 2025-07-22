#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <distingnt/api.h>
#include <iomanip>

class ParameterValueTester {
private:
    std::string getUnitName(uint8_t unit) {
        switch (unit) {
            case kNT_unitNone: return "None";
            case kNT_unitEnum: return "Enum";
            case kNT_unitDb: return "dB";
            case kNT_unitDb_minInf: return "dB (-inf)";
            case kNT_unitPercent: return "%";
            case kNT_unitHz: return "Hz";
            case kNT_unitSemitones: return "semitones";
            case kNT_unitCents: return "cents";
            case kNT_unitMs: return "ms";
            case kNT_unitSeconds: return "s";
            case kNT_unitFrames: return "frames";
            case kNT_unitMIDINote: return "MIDI note";
            case kNT_unitMillivolts: return "mV";
            case kNT_unitVolts: return "V";
            case kNT_unitBPM: return "BPM";
            case kNT_unitAudioInput: return "Audio Input";
            case kNT_unitCvInput: return "CV Input";
            case kNT_unitAudioOutput: return "Audio Output";
            case kNT_unitCvOutput: return "CV Output";
            case kNT_unitOutputMode: return "Output Mode";
            default: return "Unknown";
        }
    }
    
    double applyScaling(int16_t value, uint8_t scaling) {
        switch (scaling) {
            case kNT_scalingNone: return value;
            case kNT_scaling10: return value / 10.0;
            case kNT_scaling100: return value / 100.0;
            case kNT_scaling1000: return value / 1000.0;
            default: return value;
        }
    }
    
    std::string formatValue(int16_t rawValue, const _NT_parameter& param) {
        std::string result;
        
        // Handle enum special case
        if (param.unit == kNT_unitEnum && param.enumStrings) {
            if (rawValue >= 0) {
                int enumIndex = 0;
                while (param.enumStrings[enumIndex] != nullptr && enumIndex <= rawValue) {
                    if (enumIndex == rawValue) {
                        return "\"" + std::string(param.enumStrings[rawValue]) + "\"";
                    }
                    enumIndex++;
                }
            }
            return "[Invalid enum: " + std::to_string(rawValue) + "]";
        }
        
        // Apply scaling
        double scaledValue = applyScaling(rawValue, param.scaling);
        
        // Format with appropriate precision
        if (param.scaling == kNT_scalingNone) {
            result = std::to_string((int)scaledValue);
        } else {
            result = std::to_string(scaledValue);
            // Remove trailing zeros
            result.erase(result.find_last_not_of('0') + 1, std::string::npos);
            result.erase(result.find_last_not_of('.') + 1, std::string::npos);
        }
        
        // Add unit
        std::string unit = getUnitName(param.unit);
        if (unit != "None" && unit != "Unknown" && 
            unit != "Audio Input" && unit != "CV Input" &&
            unit != "Audio Output" && unit != "CV Output" &&
            unit != "Output Mode" && unit != "Enum") {
            result += " " + unit;
        }
        
        return result;
    }
    
public:
    bool testParameterValues(const std::string& pluginName) {
        std::cout << "\n=== Parameter Value Testing: " << pluginName << " ===" << std::endl;
        
        // Load and construct plugin
        std::string pluginPath = pluginName + ".dylib";
        void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cout << "✗ Failed to load " << pluginName << std::endl;
            return false;
        }
        
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(handle, "pluginEntry");
        if (!pluginEntry) {
            dlclose(handle);
            return false;
        }
        
        _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
        if (!factory || !factory->calculateRequirements || !factory->construct) {
            dlclose(handle);
            return false;
        }
        
        // Display basic info
        char guidStr[5] = {0};
        guidStr[0] = (factory->guid >> 0) & 0xFF;
        guidStr[1] = (factory->guid >> 8) & 0xFF;
        guidStr[2] = (factory->guid >> 16) & 0xFF;
        guidStr[3] = (factory->guid >> 24) & 0xFF;
        std::cout << "Plugin: " << guidStr << " - " << factory->name << std::endl;
        
        _NT_algorithmRequirements reqs;
        memset(&reqs, 0, sizeof(reqs));
        factory->calculateRequirements(reqs, nullptr);
        
        if (reqs.numParameters == 0) {
            std::cout << "No parameters to test" << std::endl;
            dlclose(handle);
            return true;
        }
        
        // Construct algorithm
        void* memory = malloc(reqs.sram + reqs.dram + reqs.dtc + reqs.itc + 1024);
        if (!memory) {
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
            free(memory);
            dlclose(handle);
            return false;
        }
        
        // Test parameter values
        try {
            const _NT_parameter* parameters = algorithm->parameters;
            if (!parameters) {
                std::cout << "⚠ No parameters available" << std::endl;
                free(memory);
                dlclose(handle);
                return true;
            }
            
            // Create a fake parameter value array for testing
            int16_t* testValues = (int16_t*)malloc(reqs.numParameters * sizeof(int16_t));
            if (testValues) {
                // Initialize with default values
                for (uint32_t i = 0; i < reqs.numParameters; i++) {
                    testValues[i] = parameters[i].def;
                }
                
                // Set the v pointer (normally done by the system)
                algorithm->v = testValues;
                
                std::cout << "\n--- Parameter Value Analysis ---" << std::endl;
                for (uint32_t i = 0; i < reqs.numParameters; i++) {
                    const _NT_parameter& param = parameters[i];
                    
                    std::cout << "\nParameter [" << i << "]: \"" << (param.name ? param.name : "NULL") << "\"" << std::endl;
                    
                    // Show default value formatted
                    std::cout << "  Default: " << formatValue(param.def, param) 
                             << " (raw: " << param.def << ")" << std::endl;
                    
                    // Show min/max formatted
                    std::cout << "  Range: " << formatValue(param.min, param) 
                             << " to " << formatValue(param.max, param) << std::endl;
                    
                    // Show current value
                    std::cout << "  Current: " << formatValue(testValues[i], param) 
                             << " (raw: " << testValues[i] << ")" << std::endl;
                    
                    // Test parameter change callback if available
                    if (factory->parameterChanged) {
                        std::cout << "  Testing parameterChanged callback..." << std::endl;
                        
                        // Test with different values
                        std::vector<int16_t> testCases = {param.min, param.max, param.def};
                        if (param.max > param.min) {
                            testCases.push_back((param.min + param.max) / 2); // midpoint
                        }
                        
                        for (int16_t testVal : testCases) {
                            if (testVal >= param.min && testVal <= param.max) {
                                try {
                                    testValues[i] = testVal;
                                    factory->parameterChanged(algorithm, i);
                                    std::cout << "    ✓ Value " << formatValue(testVal, param) << " - OK" << std::endl;
                                } catch (...) {
                                    std::cout << "    ✗ Value " << formatValue(testVal, param) << " - Exception!" << std::endl;
                                }
                            }
                        }
                        
                        // Restore default
                        testValues[i] = param.def;
                    }
                }
                
                free(testValues);
            }
            
        } catch (...) {
            std::cout << "✗ Exception during parameter value testing" << std::endl;
            free(memory);
            dlclose(handle);
            return false;
        }
        
        free(memory);
        dlclose(handle);
        return true;
    }
    
    void runAllValueTests() {
        std::cout << "=== Parameter Value Test Suite ===" << std::endl;
        
        std::vector<std::string> pluginNames = {
            "gain", "monosynth", "fourteen"
        };
        
        int successCount = 0;
        for (const auto& name : pluginNames) {
            if (testParameterValues(name)) {
                successCount++;
            }
        }
        
        std::cout << "\n=== Value Test Summary ===" << std::endl;
        std::cout << "Tested " << successCount << "/" << pluginNames.size() << " plugins successfully" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    ParameterValueTester tester;
    
    if (argc == 2) {
        std::string pluginName = argv[1];
        tester.testParameterValues(pluginName);
    } else {
        tester.runAllValueTests();
    }
    
    return 0;
}