#pragma once
#include <componentlibrary.hpp>
#include "../EmulatorCore.hpp"
#include <GLFW/glfw3.h>

using namespace rack;

struct DistingNTPressableEncoder : app::SvgKnob {
    EmulatorCore* emulatorCore = nullptr;
    int encoderIndex = 0;  // 0 or 1 for left or right encoder
    
    // Encoder state tracking
    float lastValue = 0.0f;
    float accumulatedDelta = 0.0f;
    
    // Step sensitivity - how much continuous change equals one discrete step
    // Hardware encoders typically have 24 steps per revolution
    // Make it quite sensitive so small movements generate steps
    static constexpr float STEP_THRESHOLD = 0.05f;  // ~20 steps per unit of parameter change
    
    DistingNTPressableEncoder() {
        // Configure for infinite rotation first
        minAngle = -INFINITY;
        maxAngle = INFINITY;
        snap = false;
        smooth = false;
        speed = 0.2f;  // Faster speed for encoder (smaller value = faster rotation)
        
        // Set the rotating foreground SVG (with the dot)
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/DistingNTEncoder_fg.svg")));
        
        // Add background SVG behind the rotating part
        auto bg = new widget::SvgWidget();
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DistingNTEncoder.svg")));
        // Add background to frame buffer, below transform widget
        fb->addChildBelow(bg, tw);
    }
    
    void onDragStart(const event::DragStart& e) override {
        // Call parent first
        app::SvgKnob::onDragStart(e);
        
        // Press when drag starts
        if (emulatorCore) {
            emulatorCore->pressEncoder(encoderIndex);
        }
        
        // Initialize value tracking
        if (auto pq = getParamQuantity()) {
            lastValue = pq->getValue();
            accumulatedDelta = 0.0f;  // Reset accumulated delta on new drag
        }
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        // Release when drag ends
        if (emulatorCore) {
            emulatorCore->releaseEncoder(encoderIndex);
        }
        
        // Call parent after
        app::SvgKnob::onDragEnd(e);
    }
    
    void step() override {
        // Call parent first - this is what actually updates the visual rotation
        app::SvgKnob::step();
        
        // Track value changes and generate discrete steps
        if (auto pq = getParamQuantity()) {
            float currentValue = pq->getValue();
            float delta = currentValue - lastValue;
            
            // Debug extreme values
            if (std::abs(currentValue) > 1000.0f) {
                INFO("DistingNTPressableEncoder[%d]: Large param value = %f", encoderIndex, currentValue);
            }
            
            if (delta != 0.0f) {
                // Accumulate the delta
                accumulatedDelta += delta;
                lastValue = currentValue;
                
                // Generate discrete steps when threshold is reached
                int stepsGenerated = 0;
                while (std::abs(accumulatedDelta) >= STEP_THRESHOLD) {
                    int stepDirection = (accumulatedDelta > 0) ? 1 : -1;
                    
                    // Send step to emulator
                    if (emulatorCore) {
                        emulatorCore->turnEncoder(encoderIndex, stepDirection);
                    }
                    
                    // Subtract the processed step, keeping remainder
                    accumulatedDelta -= stepDirection * STEP_THRESHOLD;
                    stepsGenerated++;
                    
                    // Keep the parameter value reasonable to prevent drift
                    if (pq && std::abs(currentValue) > 10.0f) {
                        pq->setValue(0.0f);
                        lastValue = 0.0f;
                        currentValue = 0.0f;
                    }
                    
                    // Safety check to prevent infinite loop
                    if (stepsGenerated > 100) {
                        WARN("DistingNTPressableEncoder: Too many steps in one frame! accumulated=%f", accumulatedDelta);
                        accumulatedDelta = 0.0f;
                        break;
                    }
                }
            }
        }
    }
    
    void setEmulatorCore(EmulatorCore* core, int index) {
        emulatorCore = core;
        encoderIndex = index;
    }
};