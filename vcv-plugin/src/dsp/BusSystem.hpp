#pragma once
#include <rack.hpp>
#include <cstring>

using namespace rack;

// Forward declaration
struct DistingNT;

class BusSystem {
private:
    // 28 buses, each with 4 samples for block processing
    alignas(16) float buses[28][4];
    int sampleIndex = 0;
    
public:
    void init() {
        clear();
    }
    
    void clear() {
        memset(buses, 0, sizeof(buses));
        sampleIndex = 0;
    }
    
    float* getBuses() {
        return &buses[0][0];
    }
    
    template<typename ModuleType>
    void routeInputs(ModuleType* module) {
        if (!module) return;
        
        int currentSample = getCurrentSampleIndex();
        
        // Route 12 inputs to buses 0-11 (matches hardware specification)
        for (int i = 0; i < 12; i++) {
            if (module->inputs[ModuleType::AUDIO_INPUT_1 + i].isConnected()) {
                float voltage = module->inputs[ModuleType::AUDIO_INPUT_1 + i].getVoltage();
                // Convert from ±5V to ±1.0 range for Disting NT processing
                setBus(i, currentSample, voltage / 5.0f);
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
        
        // Route buses 20-25 to 6 outputs (matches hardware specification)
        for (int i = 0; i < 6; i++) {
            float busValue = getBus(20 + i, currentSample);
            // Convert from ±1.0 range back to ±5V for VCV Rack
            float voltage = busValue * 5.0f;
            module->outputs[ModuleType::AUDIO_OUTPUT_1 + i].setVoltage(voltage);
        }
        
        // Advance to next sample in the block
        nextSample();
    }
    
    // Get specific bus for reading
    float getBus(int busIndex, int sampleOffset = 0) {
        if (busIndex >= 0 && busIndex < 28 && sampleOffset >= 0 && sampleOffset < 4) {
            return buses[busIndex][sampleOffset];
        }
        return 0.0f;
    }
    
    // Set specific bus value
    void setBus(int busIndex, int sampleOffset, float value) {
        if (busIndex >= 0 && busIndex < 28 && sampleOffset >= 0 && sampleOffset < 4) {
            buses[busIndex][sampleOffset] = value;
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