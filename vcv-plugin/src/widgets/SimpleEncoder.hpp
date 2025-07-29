#pragma once
#include <app/SvgKnob.hpp>
#include <componentlibrary.hpp>
#include "../EmulatorCore.hpp"
#include <functional>

using namespace rack;

struct SimpleEncoder : app::SvgKnob {
    EmulatorCore* emulatorCore = nullptr;
    int encoderIndex = 0;
    std::function<void(int, int)> onEncoderChanged = nullptr;  // Callback for encoder changes
    
    // Mouse tracking for discrete steps
    float lastMouseY = 0.0f;
    static constexpr float PIXELS_PER_STEP = 4.0f; // 5x more sensitive for drag
    
    // Visual rotation
    float displayAngle = 0.0f;
    static constexpr float RADIANS_PER_STEP = 2.0f * M_PI / 24.0f; // 24 steps per rotation
    
    // Mouse wheel accumulator for reduced sensitivity
    float scrollAccumulator = 0.0f;
    static constexpr float SCROLL_SENSITIVITY = 0.0167f; // 1/60 sensitivity - 3x less than before
    
    SimpleEncoder() {
        // Configure for infinite rotation
        minAngle = -INFINITY;
        maxAngle = INFINITY;
        snap = false;
        smooth = false;
        
        // Load the rotating foreground SVG (with the dot)
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/DistingNTEncoder_fg.svg")));
        
        // Add background SVG behind the rotating part
        auto bg = new widget::SvgWidget();
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DistingNTEncoder.svg")));
        // Add background to frame buffer, below transform widget
        fb->addChildBelow(bg, tw);
    }
    
    void onDragStart(const event::DragStart& e) override {
        // Don't need to track position, mouseDelta gives us what we need
        
        if (emulatorCore) {
            emulatorCore->pressEncoder(encoderIndex);
        }
        
        e.consume(this);
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        if (emulatorCore) {
            emulatorCore->releaseEncoder(encoderIndex);
        }
    }
    
    void onDragMove(const event::DragMove& e) override {
        float deltaY = e.mouseDelta.y;
        
        // Generate steps based on vertical mouse movement
        int steps = (int)(deltaY / PIXELS_PER_STEP);
        if (steps != 0) {
            if (onEncoderChanged) {
                onEncoderChanged(encoderIndex, -steps);  // Invert for natural feel
            } else if (emulatorCore) {
                emulatorCore->turnEncoder(encoderIndex, -steps);  // Fallback
            }
            
            // Update visual rotation
            displayAngle += steps * RADIANS_PER_STEP;
            
            // Update the knob's parameter to trigger visual update
            if (getParamQuantity()) {
                getParamQuantity()->setValue(displayAngle);
            }
        }
    }
    
    void onHoverScroll(const event::HoverScroll& e) override {
        if (e.scrollDelta.y != 0) {
            // Accumulate scroll with reduced sensitivity
            scrollAccumulator += e.scrollDelta.y * SCROLL_SENSITIVITY;
            
            // Generate steps when accumulator reaches threshold
            int steps = (int)scrollAccumulator;
            if (steps != 0) {
                if (onEncoderChanged) {
                    onEncoderChanged(encoderIndex, steps);
                } else if (emulatorCore) {
                    emulatorCore->turnEncoder(encoderIndex, steps);  // Fallback
                }
                
                // Update visual rotation
                displayAngle += steps * RADIANS_PER_STEP;
                
                // Update the knob's parameter to trigger visual update
                if (getParamQuantity()) {
                    getParamQuantity()->setValue(displayAngle);
                }
                
                // Keep the remainder
                scrollAccumulator -= steps;
            }
            
            e.consume(this);
        }
    }
    
    void setEmulatorCore(EmulatorCore* core, int index) {
        emulatorCore = core;
        encoderIndex = index;
    }
    
    void setEncoderChangedCallback(std::function<void(int, int)> callback) {
        onEncoderChanged = callback;
    }
};