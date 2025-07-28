#pragma once
#include <app/SvgKnob.hpp>
#include <componentlibrary.hpp>

using namespace rack;
using namespace window;

struct PressablePot : app::SvgKnob {
    // Simple press state - no atomic, no complex initialization
    bool pressed = false;
    
    // Rotation range: -160° to +160° (320° total)  
    static constexpr float MIN_ANGLE = -160.0f * M_PI / 180.0f;  // -160° in radians
    static constexpr float MAX_ANGLE = 160.0f * M_PI / 180.0f;   // +160° in radians
    
    PressablePot() {
        // Use standard knob for visual consistency
        setSvg(Svg::load(asset::system("res/ComponentLibrary/RoundBlackKnob.svg")));
        
        // Configure rotation limits
        minAngle = MIN_ANGLE;
        maxAngle = MAX_ANGLE;
        speed = 1.0f;
        snap = false;
        smooth = true;
    }
    
    // Event handlers
    void onButton(const event::Button& e) override;
    void onDragMove(const event::DragMove& e) override;
    
    // State management
    void setPressed(bool state) { pressed = state; }
    bool isPressed() const { return pressed; }
    
    // Press event callbacks (can be overridden by users)
    virtual void onPress() {}
    virtual void onRelease() {}
};