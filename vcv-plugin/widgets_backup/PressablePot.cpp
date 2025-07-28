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
    
    // Visual update handled by parent widget
}

void PressablePot::draw(const DrawArgs& args) {
    // Modify color based on press state
    nvgSave(args.vg);
    
    if (isPressed()) {
        // Lighter appearance when pressed
        nvgGlobalAlpha(args.vg, 1.2f);
        nvgGlobalTint(args.vg, nvgRGBA(255, 255, 255, 40));
    } else {
        // Normal dark grey appearance
        nvgGlobalAlpha(args.vg, 1.0f);
    }
    
    // Draw the knob using parent implementation
    app::SvgKnob::draw(args);
    
    // Draw position indicator line
    drawPositionIndicator(args);
    
    nvgRestore(args.vg);
}

void PressablePot::setPressed(bool state) {
    pressed.store(state);
    // Visual update handled by parent widget
}

float PressablePot::getRotationAngle() const {
    engine::ParamQuantity* pq = const_cast<PressablePot*>(this)->getParamQuantity();
    if (!pq) return 0.0f;
    
    // Get normalized value (0.0 to 1.0)
    float normalizedValue = pq->getScaledValue();
    
    // Convert to angle: 0.0 -> MIN_ANGLE, 1.0 -> MAX_ANGLE
    return MIN_ANGLE + normalizedValue * ANGLE_RANGE;
}

float PressablePot::getNormalizedValue() const {
    engine::ParamQuantity* pq = const_cast<PressablePot*>(this)->getParamQuantity();
    if (!pq) return 0.5f;  // Default to center
    
    return pq->getScaledValue();
}

void PressablePot::drawPositionIndicator(const DrawArgs& args) {
    float angle = getRotationAngle();
    
    // Calculate line endpoints
    float centerX = box.size.x * 0.5f;
    float centerY = box.size.y * 0.5f;
    float radius = std::min(centerX, centerY) * 0.7f;
    
    float lineStartX = centerX + std::sin(angle) * radius * 0.3f;
    float lineStartY = centerY - std::cos(angle) * radius * 0.3f;
    float lineEndX = centerX + std::sin(angle) * radius;
    float lineEndY = centerY - std::cos(angle) * radius;
    
    // Draw position line
    nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
    nvgStrokeWidth(args.vg, 2.0f);
    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg, lineStartX, lineStartY);
    nvgLineTo(args.vg, lineEndX, lineEndY);
    nvgStroke(args.vg);
}