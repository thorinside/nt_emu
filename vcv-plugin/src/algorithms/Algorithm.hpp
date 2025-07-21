#pragma once
#include <rack.hpp>
#include <string>

using namespace rack;

// Algorithm interface matching Disting NT plugin API
struct IDistingAlgorithm {
    virtual ~IDistingAlgorithm() = default;
    
    // Core processing
    virtual void prepare(float sampleRate) = 0;
    virtual void step(float* buses, int numFramesBy4) = 0;
    
    // Parameter and control handling
    virtual void parameterChanged(int param, float value) = 0;
    virtual void buttonPressed(int button) = 0;
    
    // Display rendering
    virtual void drawDisplay(NVGcontext* vg, int width, int height) = 0;
    
    // Metadata
    virtual const char* getName() = 0;
    virtual const char* getDescription() { return ""; }
    
    // State serialization
    virtual json_t* dataToJson() { return nullptr; }
    virtual void dataFromJson(json_t* root) {}
};

// Base algorithm class with common functionality
class BaseAlgorithm : public IDistingAlgorithm {
protected:
    float sampleRate = 44100.0f;
    std::string name;
    std::string description;
    
    // Common parameter storage
    float parameters[16] = {0.0f}; // Up to 16 parameters per algorithm
    
public:
    BaseAlgorithm(const std::string& algorithmName, const std::string& algorithmDescription = "")
        : name(algorithmName), description(algorithmDescription) {}
    
    void prepare(float sr) override {
        sampleRate = sr;
        onPrepare();
    }
    
    const char* getName() override {
        return name.c_str();
    }
    
    const char* getDescription() override {
        return description.c_str();
    }
    
    void parameterChanged(int param, float value) override {
        if (param >= 0 && param < 16) {
            parameters[param] = value;
            onParameterChanged(param, value);
        }
    }
    
    // Default implementations that can be overridden
    virtual void onPrepare() {}
    virtual void onParameterChanged(int param, float value) {}
    
    void buttonPressed(int button) override {
        onButtonPressed(button);
    }
    
    virtual void onButtonPressed(int button) {}
    
    // Default display - shows algorithm name
    void drawDisplay(NVGcontext* vg, int width, int height) override {
        // Clear background
        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, width, height);
        nvgFillColor(vg, nvgRGB(0, 0, 0));
        nvgFill(vg);
        
        // Draw algorithm name
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgFontSize(vg, 12);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, width / 2, height / 2, getName(), nullptr);
        
        // Call custom draw implementation
        drawCustomDisplay(vg, width, height);
    }
    
    virtual void drawCustomDisplay(NVGcontext* vg, int width, int height) {}
    
    // Basic JSON serialization
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        
        // Save parameters
        json_t* paramsJ = json_array();
        for (int i = 0; i < 16; i++) {
            json_array_append_new(paramsJ, json_real(parameters[i]));
        }
        json_object_set_new(rootJ, "parameters", paramsJ);
        
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        // Load parameters
        json_t* paramsJ = json_object_get(rootJ, "parameters");
        if (paramsJ) {
            for (int i = 0; i < 16; i++) {
                json_t* paramJ = json_array_get(paramsJ, i);
                if (paramJ) {
                    parameters[i] = json_number_value(paramJ);
                }
            }
        }
    }
};

// Factory function for creating algorithms
std::unique_ptr<IDistingAlgorithm> createSimpleGainAlgorithm();