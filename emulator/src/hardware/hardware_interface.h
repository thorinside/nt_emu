#pragma once

#include "../core/api_shim.h"
#include <imgui.h>
#include <functional>

class HardwareInterface {
public:
    HardwareInterface();
    ~HardwareInterface();
    
    void render();
    void update();
    
    // Get current hardware state
    const HardwareState& getState() const { return state_; }
    
    // Callbacks for when controls change
    void setParameterChangeCallback(std::function<void(int, float)> callback) {
        parameter_change_callback_ = callback;
    }
    
    void setButtonCallback(std::function<void(int, bool)> callback) {
        button_callback_ = callback;
    }
    
    void setEncoderCallback(std::function<void(int)> callback) {
        encoder_callback_ = callback;
    }
    
    // External state setters for main window integration
    void setPotValue(int index, float value);
    void setPotPressed(int index, bool pressed);
    void setButtonState(int index, bool state);
    void setEncoderValue(int index, int value);
    void setEncoderPressed(int index, bool pressed);
    
    // State query methods
    float getPotValue(int index) const;
    bool isPotPressed(int index) const;
    bool getButtonState(int index) const;
    int getEncoderValue(int index) const;
    bool isEncoderPressed(int index) const;
    
private:
    HardwareState state_;
    HardwareState previous_state_;
    
    // UI state
    float pot_display_values_[3];
    bool pot_pressed_states_[3];
    bool button_display_states_[4];
    int encoder_display_values_[2];
    bool encoder_pressed_states_[2];
    
    // Callbacks
    std::function<void(int, float)> parameter_change_callback_;
    std::function<void(int, bool)> button_callback_;
    std::function<void(int)> encoder_callback_;
    
    void renderControlsWindow();
    void renderPotentiometer(int index, const char* label);
    void renderButton(int index, const char* label);
    void renderEncoder(int index, const char* label);
    
    // Custom knob rendering
    bool renderKnob(const char* label, float* value, bool* pressed, float min_val = 0.0f, float max_val = 1.0f, float radius = 30.0f);
    bool renderEncoderKnob(const char* label, int* value, bool* pressed, int min_val = -100, int max_val = 100, float radius = 35.0f);
    
    void updateCallbacks();
};