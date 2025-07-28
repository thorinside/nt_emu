#pragma once
#include <app/SvgKnob.hpp>
#include <componentlibrary.hpp>

using namespace rack;
using namespace window;

struct PressableEncoder : app::SvgKnob {
    // Simple press state - no atomic, no complex initialization
    bool pressed = false;
    
    // Infinite rotation state
    float lastAngle = 0.0f;
    int stepOutput = 0;
    int accumulatedSteps = 0;
    
    // Encoder configuration
    static constexpr float STEPS_PER_REVOLUTION = 24.0f;  // Typical encoder resolution
    static constexpr float SENSITIVITY = 0.002f;           // Mouse sensitivity
    
    PressableEncoder() {
        // Use smaller knob for encoder
        setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1PSRed.svg")));
        
        // Configure for infinite rotation behavior
        speed = 1.0f;
        snap = false;
        smooth = false;  // Disable smoothing for crisp discrete behavior
        minAngle = -INFINITY;
        maxAngle = INFINITY;
    }
    
    // Event handlers
    void onButton(const event::Button& e) override;
    void onDragMove(const event::DragMove& e) override;
    void onDragStart(const event::DragStart& e) override;
    void onDragEnd(const event::DragEnd& e) override;
    
    // State management
    void setPressed(bool state) { pressed = state; }
    bool isPressed() const { return pressed; }
    int calculateSteps(float deltaAngle);
    void resetSteps();
    
    // Press event callbacks (can be overridden by users)
    virtual void onPress() {}
    virtual void onRelease() {}
    virtual void onStep(int stepDelta) {}  // Called when encoder steps are generated
};