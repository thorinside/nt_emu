#pragma once
#include <app/Knob.hpp>
#include <app/SvgKnob.hpp>
#include <componentlibrary.hpp>

using namespace rack;

struct InfiniteEncoder : app::SvgKnob {
    // State tracking for discrete behavior
    float lastAngle = 0.f;
    int accumulatedSteps = 0;
    const float STEPS_PER_REVOLUTION = 24.f; // Typical encoder resolution
    
    InfiniteEncoder() {
        // Use the same SVG as BefacoTinyKnob for visual consistency
        setSvg(Svg::load(asset::system("res/ComponentLibrary/BefacoTinyKnob.svg")));
        
        // Configure for infinite rotation behavior
        speed = 1.0f;
        snap = false;
        smooth = false; // Disable smoothing for crisp discrete behavior
        minAngle = -INFINITY;
        maxAngle = INFINITY;
    }
    
    void onDragMove(const event::DragMove& e) override {
        engine::ParamQuantity* pq = getParamQuantity();
        if (pq) {
            // Calculate rotation delta in radians
            float sensitivity = 0.002f; // Adjust sensitivity
            float deltaAngle = -e.mouseDelta.y * sensitivity;
            
            // Accumulate angle change
            lastAngle += deltaAngle;
            
            // Convert to discrete steps
            float totalSteps = lastAngle * STEPS_PER_REVOLUTION / (2.f * M_PI);
            int newSteps = (int)std::round(totalSteps);
            int stepDelta = newSteps - accumulatedSteps;
            
            if (stepDelta != 0) {
                // Generate discrete step value for the parameter
                float currentValue = pq->getValue();
                float newValue = currentValue + stepDelta;
                
                // Set the new value
                pq->setValue(newValue);
                
                // Update our step counter
                accumulatedSteps = newSteps;
            }
        }
    }
    
    void onDragStart(const event::DragStart& e) override {
        // Don't call parent to avoid normal knob behavior
        engine::ParamQuantity* pq = getParamQuantity();
        if (pq) {
            // Initialize based on current parameter value
            accumulatedSteps = (int)pq->getValue();
            lastAngle = (accumulatedSteps * 2.f * M_PI) / STEPS_PER_REVOLUTION;
        }
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        // Don't call parent to avoid normal knob behavior
        // Keep our accumulated position for next interaction
    }
};