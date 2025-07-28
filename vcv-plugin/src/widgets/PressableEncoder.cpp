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