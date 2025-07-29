#pragma once
#include <componentlibrary.hpp>
#include "../EmulatorCore.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <rack.hpp>

using namespace rack;

// Forward declaration to avoid circular dependency
struct EmulatorModule;

struct EmulatorPressablePot : componentlibrary::RoundBigBlackKnob {
    EmulatorCore* emulatorCore = nullptr;
    int potIndex = 0;  // 0, 1, or 2 for left, center, right pot
    float lastValue = 0.5f;  // Track last value for change detection
    std::function<void(int, float)> onPotChanged = nullptr;  // Callback for pot changes
    
    EmulatorPressablePot() : componentlibrary::RoundBigBlackKnob() {
        // Constructor automatically sets up the correct SVGs via parent class
    }
    
    void onDragStart(const event::DragStart& e) override {
        INFO("EmulatorPressablePot::onDragStart pot %d - value before parent: %.3f", 
             potIndex, getParamQuantity() ? getParamQuantity()->getValue() : -1.0f);
        
        // Call parent first
        componentlibrary::RoundBigBlackKnob::onDragStart(e);
        
        INFO("EmulatorPressablePot::onDragStart pot %d - value after parent: %.3f", 
             potIndex, getParamQuantity() ? getParamQuantity()->getValue() : -1.0f);
        
        // Press when drag starts
        if (emulatorCore) {
            emulatorCore->pressPot(potIndex);
        }
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        // Release when drag ends
        if (emulatorCore) {
            emulatorCore->releasePot(potIndex);
        }
        
        // Call parent after
        componentlibrary::RoundBigBlackKnob::onDragEnd(e);
    }
    
    void onChange(const event::Change& e) override {
        // Call parent first
        componentlibrary::RoundBigBlackKnob::onChange(e);
        
        // Check for value change and call callback
        if (onPotChanged && getParamQuantity()) {
            float currentValue = getParamQuantity()->getValue();
            INFO("Pot %d onChange: current=%.3f, last=%.3f, diff=%.6f", 
                 potIndex, currentValue, lastValue, std::abs(currentValue - lastValue));
            if (std::abs(currentValue - lastValue) > 0.001f) {
                lastValue = currentValue;
                onPotChanged(potIndex, currentValue);
            }
        }
    }
    
    void onDragMove(const event::DragMove& e) override {
        // Call parent first
        componentlibrary::RoundBigBlackKnob::onDragMove(e);
        
        // Also check for changes during drag
        if (onPotChanged && getParamQuantity()) {
            float currentValue = getParamQuantity()->getValue();
            INFO("Pot %d onDragMove: current=%.3f, last=%.3f, paramQuantity=%p", 
                 potIndex, currentValue, lastValue, getParamQuantity());
            if (std::abs(currentValue - lastValue) > 0.001f) {
                lastValue = currentValue;
                onPotChanged(potIndex, currentValue);
            }
        }
    }
    
    void onHover(const event::Hover& e) override {
        // Call parent first
        componentlibrary::RoundBigBlackKnob::onHover(e);
        
        // Always check for value changes when hovering (mouse wheel)
        if (onPotChanged) {
            float currentValue = getParamQuantity()->getValue();
            if (std::abs(currentValue - lastValue) > 0.001f) {
                INFO("Pot %d onHover: current=%.3f, last=%.3f", potIndex, currentValue, lastValue);
                lastValue = currentValue;
                onPotChanged(potIndex, currentValue);
            }
        }
    }
    
    void step() override {
        // Call parent first
        componentlibrary::RoundBigBlackKnob::step();
        
        // Check for any value changes every frame (fallback)
        if (onPotChanged) {
            float currentValue = getParamQuantity()->getValue();
            if (std::abs(currentValue - lastValue) > 0.001f) {
                INFO("Pot %d step: current=%.3f, last=%.3f", potIndex, currentValue, lastValue);
                lastValue = currentValue;
                onPotChanged(potIndex, currentValue);
            }
        }
    }
    
    void setEmulatorCore(EmulatorCore* core, int index) {
        emulatorCore = core;
        potIndex = index;
        
        // Initialize lastValue to current parameter value if available
        if (getParamQuantity()) {
            lastValue = getParamQuantity()->getValue();
            INFO("EmulatorPressablePot %d initialized with value %.3f", potIndex, lastValue);
        }
    }
    
    void setPotChangedCallback(std::function<void(int, float)> callback) {
        onPotChanged = callback;
    }
};