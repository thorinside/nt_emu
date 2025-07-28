#pragma once
#include <app/SvgKnob.hpp>
#include <componentlibrary.hpp>
#include <atomic>

using namespace rack;
using namespace window;

struct PressablePot : app::SvgKnob {
    // Press state management
    std::atomic<bool> pressed{false};
    float pressValue = 0.0f;
    
    // Visual state
    // (Using parent SvgKnob rendering, no additional widgets needed)
    
    // Rotation range: -160° to +160° (320° total)
    static constexpr float MIN_ANGLE = -160.0f * M_PI / 180.0f;  // -160° in radians
    static constexpr float MAX_ANGLE = 160.0f * M_PI / 180.0f;   // +160° in radians
    static constexpr float ANGLE_RANGE = MAX_ANGLE - MIN_ANGLE;
    
    PressablePot() {
        // Use standard Befaco knob for visual consistency
        setSvg(Svg::load(asset::system("res/ComponentLibrary/BefacoKnob.svg")));
        
        // Configure rotation limits
        minAngle = MIN_ANGLE;
        maxAngle = MAX_ANGLE;
        speed = 1.0f;
        snap = false;
        smooth = true;
        
        // No additional child widgets needed - using parent SvgKnob rendering
    }
    
    ~PressablePot() {
        // Cleanup handled by parent widget destruction
    }
    
    // Event handlers
    void onButton(const event::Button& e) override;
    void onDragMove(const event::DragMove& e) override;
    void draw(const DrawArgs& args) override;
    
    // State management
    void setPressed(bool state);
    bool isPressed() const { return pressed.load(); }
    float getRotationAngle() const;
    float getNormalizedValue() const;
    
    // Press event callback (can be overridden by users)
    virtual void onPress() {}
    virtual void onRelease() {}
    
    // Drawing helper
    void drawPositionIndicator(const DrawArgs& args);
};