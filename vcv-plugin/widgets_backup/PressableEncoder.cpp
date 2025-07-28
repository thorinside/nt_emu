#include "PressableEncoder.hpp"
#include <GLFW/glfw3.h>

using namespace rack;

void PressableEncoder::onButton(const event::Button& e) {
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

void PressableEncoder::onDragMove(const event::DragMove& e) {
    engine::ParamQuantity* pq = getParamQuantity();
    if (pq) {
        // Calculate rotation delta in radians
        float deltaAngle = -e.mouseDelta.y * SENSITIVITY;
        
        // Accumulate angle change
        lastAngle += deltaAngle;
        
        // Convert to discrete steps
        int stepDelta = calculateSteps(deltaAngle);
        
        if (stepDelta != 0) {
            // Generate discrete step value for the parameter
            float currentValue = pq->getValue();
            float newValue = currentValue + stepDelta;
            
            // Set the new value (parameter will handle bounds)
            pq->setValue(newValue);
            
            // Update our step counter
            accumulatedSteps += stepDelta;
            stepOutput = stepDelta;  // Store last step delta for external use
            
            // Notify callback
            onStep(stepDelta);
        } else {
            stepOutput = 0;  // No step this frame
        }
    }
    
    // Visual update handled by parent widget
}

void PressableEncoder::onDragStart(const event::DragStart& e) {
    // Don't call parent to avoid normal knob behavior
    engine::ParamQuantity* pq = getParamQuantity();
    if (pq) {
        // Initialize based on current parameter value
        accumulatedSteps = (int)pq->getValue();
        lastAngle = (accumulatedSteps * 2.0f * M_PI) / STEPS_PER_REVOLUTION;
        stepOutput = 0;
    }
}

void PressableEncoder::onDragEnd(const event::DragEnd& e) {
    // Don't call parent to avoid normal knob behavior
    // Keep our accumulated position for next interaction
    stepOutput = 0;  // Clear step output when done dragging
}

void PressableEncoder::draw(const DrawArgs& args) {
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
    
    // Draw rotation feedback (small indicator)
    drawRotationIndicator(args);
    
    nvgRestore(args.vg);
}

void PressableEncoder::setPressed(bool state) {
    pressed.store(state);
    // Visual update handled by parent widget
}

int PressableEncoder::calculateSteps(float deltaAngle) {
    // Convert angle delta to steps
    float stepDelta = (deltaAngle * STEPS_PER_REVOLUTION) / (2.0f * M_PI);
    return (int)std::round(stepDelta);
}

void PressableEncoder::resetSteps() {
    accumulatedSteps = 0;
    lastAngle = 0.0f;
    stepOutput = 0;
    
    engine::ParamQuantity* pq = getParamQuantity();
    if (pq) {
        pq->setValue(0);
    }
}

void PressableEncoder::drawRotationIndicator(const DrawArgs& args) {
    // Draw a small indicator showing recent rotation activity
    if (stepOutput != 0) {
        float centerX = box.size.x * 0.5f;
        float centerY = box.size.y * 0.5f;
        float radius = std::min(centerX, centerY) * 0.8f;
        
        // Draw direction indicator
        nvgStrokeColor(args.vg, stepOutput > 0 ? nvgRGBA(0, 255, 0, 180) : nvgRGBA(255, 100, 0, 180));
        nvgStrokeWidth(args.vg, 3.0f);
        nvgBeginPath(args.vg);
        
        // Draw a small arc showing rotation direction
        float startAngle = stepOutput > 0 ? -0.3f : 0.3f;
        float endAngle = stepOutput > 0 ? 0.3f : -0.3f;
        
        nvgArc(args.vg, centerX, centerY, radius * 0.7f, startAngle, endAngle, NVG_CW);
        nvgStroke(args.vg);
    }
}