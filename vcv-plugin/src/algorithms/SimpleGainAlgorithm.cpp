#include "Algorithm.hpp"
#include <cmath>

class SimpleGainAlgorithm : public BaseAlgorithm {
private:
    float gain = 1.0f;
    
public:
    SimpleGainAlgorithm() : BaseAlgorithm("Simple Gain", "Basic gain control algorithm") {}
    
    void step(float* buses, int numFramesBy4) override {
        // Apply gain to input bus 0 and output to bus 4
        for (int frame = 0; frame < numFramesBy4 * 4; frame += 4) {
            for (int sample = 0; sample < 4; sample++) {
                int index = frame + sample;
                
                // Get input from bus 0
                float input = buses[index]; // Bus 0 contains input audio
                
                // Apply gain
                float output = input * gain;
                
                // Send to output bus 4 
                buses[4 * 4 + index] = output; // Bus 4 for audio output
            }
        }
    }
    
    void onParameterChanged(int param, float value) override {
        if (param == 0) { // Pot L controls gain
            gain = value * 2.0f; // 0-2x gain range
        }
    }
    
    void drawCustomDisplay(NVGcontext* vg, int width, int height) override {
        // Draw gain value
        char gainText[32];
        snprintf(gainText, sizeof(gainText), "Gain: %.2f", gain);
        
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgFontSize(vg, 10);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgText(vg, width / 2, height - 10, gainText, nullptr);
        
        // Draw gain meter
        int meterWidth = width - 20;
        int meterHeight = 8;
        int meterX = 10;
        int meterY = height - 25;
        
        // Meter background
        nvgBeginPath(vg);
        nvgRect(vg, meterX, meterY, meterWidth, meterHeight);
        nvgStrokeColor(vg, nvgRGB(128, 128, 128));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);
        
        // Meter fill
        float fillWidth = meterWidth * (gain / 2.0f); // Max gain is 2.0
        nvgBeginPath(vg);
        nvgRect(vg, meterX, meterY, fillWidth, meterHeight);
        nvgFillColor(vg, nvgRGB(0, 255, 0));
        nvgFill(vg);
    }
};

// Factory function to create the algorithm
std::unique_ptr<IDistingAlgorithm> createSimpleGainAlgorithm() {
    return std::unique_ptr<IDistingAlgorithm>(new SimpleGainAlgorithm());
}