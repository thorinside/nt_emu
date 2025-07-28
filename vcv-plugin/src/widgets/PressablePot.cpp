#include "PressablePot.hpp"
#include <GLFW/glfw3.h>

using namespace rack;

void PressablePot::onButton(const event::Button& e) {
    // Handle left mouse button press/release
    if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
        if (e.action == GLFW_PRESS) {
            setPressed(true);
            onPress();  // Callback for derived classes
            e.consume(this);
        } else if (e.action == GLFW_RELEASE) {
            setPressed(false);
            onRelease();  // Callback for derived classes
            e.consume(this);
        }
    }
    
    // Always call parent to handle normal knob behavior
    app::SvgKnob::onButton(e);
}

void PressablePot::onDragMove(const event::DragMove& e) {
    // Call parent first to handle rotation
    app::SvgKnob::onDragMove(e);
}