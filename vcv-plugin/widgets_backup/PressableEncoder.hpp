#pragma once
#include <app/SvgKnob.hpp>
#include <componentlibrary.hpp>
#include <atomic>

using namespace rack;
using namespace window;

struct PressableEncoder : app::SvgKnob {
    // Press state management
    std::atomic<bool> pressed{false};
    
    // Infinite rotation state
    float lastAngle = 0.0f;
    int stepOutput = 0;
    int accumulatedSteps = 0;
    
    // Encoder configuration
    static constexpr float STEPS_PER_REVOLUTION = 24.0f;  // Typical encoder resolution
    static constexpr float SENSITIVITY = 0.002f;           // Mouse sensitivity
    
    // Visual state
    // (Using parent SvgKnob rendering, no additional widgets needed)
    
    PressableEncoder() {
        // Use the same SVG as BefacoTinyKnob for visual consistency
        setSvg(Svg::load(asset::system("res/ComponentLibrary/BefacoTinyKnob.svg")));
        
        // Configure for infinite rotation behavior
        speed = 1.0f;
        snap = false;
        smooth = false;  // Disable smoothing for crisp discrete behavior
        minAngle = -INFINITY;
        maxAngle = INFINITY;
        
        // No additional child widgets needed - using parent SvgKnob rendering
    }
    
    ~PressableEncoder() {
        // Cleanup handled by parent widget destruction
    }
    
    // Event handlers
    void onButton(const event::Button& e) override;
    void onDragMove(const event::DragMove& e) override;
    void onDragStart(const event::DragStart& e) override;
    void onDragEnd(const event::DragEnd& e) override;
    void draw(const DrawArgs& args) override;
    
    // State management
    void setPressed(bool state);
    bool isPressed() const { return pressed.load(); }
    int calculateSteps(float deltaAngle);
    void resetSteps();
    
    // Press event callbacks (can be overridden by users)
    virtual void onPress() {}
    virtual void onRelease() {}
    virtual void onStep(int stepDelta) {}  // Called when encoder steps are generated
    
    // Drawing helper
    void drawRotationIndicator(const DrawArgs& args);
};