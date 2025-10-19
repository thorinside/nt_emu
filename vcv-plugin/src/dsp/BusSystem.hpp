#pragma once
#include <rack.hpp>
#include <cstring>

using namespace rack;

// Forward declaration
struct EmulatorModule;

class BusSystem {
private:
    // 28 buses, each with 4 samples for block processing
    // Layout: [Bus0_S0-S3][Bus1_S0-S3]...[Bus27_S0-S3] for plugin compatibility
    alignas(16) float buses[28 * 4];
    int sampleIndex = 0;
    
public:
    void init() {
        clear();
    }
    
    void clear() {
        memset(buses, 0, sizeof(buses));
    }

    void clearOutputBuses() {
        // Clear only buses 12-27 (internal processing and outputs)
        // Keep buses 0-11 (inputs) intact
        memset(buses + 12 * 4, 0, 16 * 4 * sizeof(float));
    }

    float* getBuses() {
        return buses;
    }
    
    template<typename ModuleType>
    void routeInputs(ModuleType* module) {
        if (!module) return;
        
        int currentSample = getCurrentSampleIndex();
        
        // Route 12 inputs to buses 0-11 (0-based indexing)
        for (int i = 0; i < 12; i++) {
            if (module->inputs[ModuleType::AUDIO_INPUT_1 + i].isConnected()) {
                float voltage = module->inputs[ModuleType::AUDIO_INPUT_1 + i].getVoltage();
                setBus(i, currentSample, voltage);  // Use 0-based bus indexing
            } else {
                setBus(i, currentSample, 0.0f);
            }
        }
        
        // Buses 12-27 are used for internal algorithm processing
        // They will be populated by the algorithms themselves
    }
    
    template<typename ModuleType>
    void routeOutputs(ModuleType* module) {
        if (!module) return;
        
        int currentSample = getCurrentSampleIndex();
        
        // Route buses 12-19 to 8 outputs
        for (int i = 0; i < 8; i++) {
            float busValue = getBus(12 + i, currentSample);
            module->outputs[ModuleType::AUDIO_OUTPUT_1 + i].setVoltage(busValue);
        }
        
        // Advance to next sample in the block
        nextSample();
    }
    
    // Get specific bus for reading
    float getBus(int busIndex, int sampleOffset = 0) {
        if (busIndex >= 0 && busIndex < 28 && sampleOffset >= 0 && sampleOffset < 4) {
            // Data layout: [Bus0_S0-S3][Bus1_S0-S3]...[Bus27_S0-S3]
            return buses[busIndex * 4 + sampleOffset];
        }
        return 0.0f;
    }
    
    // Set specific bus value
    void setBus(int busIndex, int sampleOffset, float value) {
        if (busIndex >= 0 && busIndex < 28 && sampleOffset >= 0 && sampleOffset < 4) {
            // Data layout: [Bus0_S0-S3][Bus1_S0-S3]...[Bus27_S0-S3]
            buses[busIndex * 4 + sampleOffset] = value;
        }
    }
    
    // Increment sample index (for block processing)
    void nextSample() {
        sampleIndex = (sampleIndex + 1) % 4;
    }
    
    int getCurrentSampleIndex() const {
        return sampleIndex;
    }
};