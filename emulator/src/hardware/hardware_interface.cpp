#include "hardware_interface.h"
#include <imgui.h>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

HardwareInterface::HardwareInterface() 
{
    // Initialize states
    memset(&state_, 0, sizeof(state_));
    memset(&previous_state_, 0, sizeof(previous_state_));
    
    // Initialize UI display values
    for (int i = 0; i < 3; i++) {
        pot_display_values_[i] = 0.0f;
        pot_pressed_states_[i] = false;
    }
    for (int i = 0; i < 4; i++) {
        button_display_states_[i] = false;
    }
    for (int i = 0; i < 2; i++) {
        encoder_display_values_[i] = 0;
        encoder_pressed_states_[i] = false;
    }
}

HardwareInterface::~HardwareInterface() {
}

void HardwareInterface::render() {
    renderControlsWindow();
}

void HardwareInterface::update() {
    // Copy current state to previous
    previous_state_ = state_;
    
    // Copy UI values to state
    for (int i = 0; i < 3; i++) {
        state_.pots[i] = pot_display_values_[i];
        state_.pot_pressed[i] = pot_pressed_states_[i];
    }
    for (int i = 0; i < 4; i++) {
        state_.buttons[i] = button_display_states_[i];
    }
    for (int i = 0; i < 2; i++) {
        state_.encoder_values[i] = encoder_display_values_[i];
        state_.encoder_pressed[i] = encoder_pressed_states_[i];
    }
    
    // Update API state
    ApiShim::getState().hardware = state_;
    
    // Check for changes and call callbacks
    updateCallbacks();
}

void HardwareInterface::renderControlsWindow() {
    ImGui::Begin("Disting NT Hardware", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Expert Sleepers Disting NT - Hardware Controls");
    ImGui::Separator();
    
    // Top row: 3 Potentiometers - arranged horizontally like the real hardware
    ImGui::Text("Potentiometers:");
    ImGui::BeginGroup();
    {
        ImGui::BeginGroup();
        renderPotentiometer(0, "Pot 1");
        ImGui::EndGroup();
        
        ImGui::SameLine(120);
        ImGui::BeginGroup();
        renderPotentiometer(1, "Pot 2");
        ImGui::EndGroup();
        
        ImGui::SameLine(240);
        ImGui::BeginGroup();
        renderPotentiometer(2, "Pot 3");
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Bottom section: Left buttons, Encoders, Right buttons
    ImGui::BeginGroup();
    {
        // Left buttons column
        ImGui::BeginGroup();
        ImGui::Text("  ");  // Spacing
        renderButton(0, "◀");
        ImGui::Spacing();
        renderButton(1, "◀");
        ImGui::EndGroup();
        
        // Encoders in the center
        ImGui::SameLine(100);
        ImGui::BeginGroup();
        {
            ImGui::BeginGroup();
            renderEncoder(0, "Enc 1");
            ImGui::EndGroup();
            
            ImGui::SameLine(100);
            ImGui::BeginGroup();
            renderEncoder(1, "Enc 2");
            ImGui::EndGroup();
        }
        ImGui::EndGroup();
        
        // Right buttons column
        ImGui::SameLine(280);
        ImGui::BeginGroup();
        ImGui::Text("  ");  // Spacing
        renderButton(2, "▶");
        ImGui::Spacing();
        renderButton(3, "▶");
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    
    ImGui::Separator();
    
    // Current values display (compact)
    ImGui::Text("Values:");
    ImGui::Text("Pots: %.2f, %.2f, %.2f | Encoders: %d, %d", 
               state_.pots[0], state_.pots[1], state_.pots[2],
               state_.encoder_values[0], state_.encoder_values[1]);
    ImGui::Text("Buttons: %s%s%s%s", 
               state_.buttons[0] ? "ON" : "OFF",
               state_.buttons[1] ? "ON" : "OFF",
               state_.buttons[2] ? "ON" : "OFF",
               state_.buttons[3] ? "ON" : "OFF");
    
    ImGui::End();
}

void HardwareInterface::renderPotentiometer(int index, const char* label) {
    if (index < 0 || index >= 3) return;
    
    ImGui::PushID(index);
    
    // Use custom knob widget
    renderKnob(label, &pot_display_values_[index], &pot_pressed_states_[index]);
    
    // Show current value below knob
    ImGui::Text("%.3f", pot_display_values_[index]);
    
    ImGui::PopID();
}

void HardwareInterface::renderButton(int index, const char* label) {
    if (index < 0 || index >= 4) return;
    
    ImGui::PushID(100 + index);
    
    // Button
    bool pressed = ImGui::Button(label, ImVec2(60, 30));
    if (pressed) {
        button_display_states_[index] = !button_display_states_[index];
    }
    
    // Show state
    if (button_display_states_[index]) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "ON");
    } else {
        ImGui::Text("OFF");
    }
    
    ImGui::PopID();
}

void HardwareInterface::renderEncoder(int index, const char* label) {
    if (index < 0 || index >= 2) return;
    
    ImGui::PushID(200 + index);
    
    // Use custom encoder knob widget
    renderEncoderKnob(label, &encoder_display_values_[index], &encoder_pressed_states_[index]);
    
    // Show current value and reset button
    ImGui::Text("%d", encoder_display_values_[index]);
    if (ImGui::Button("Reset", ImVec2(50, 20))) {
        encoder_display_values_[index] = 0;
    }
    
    ImGui::PopID();
}

void HardwareInterface::updateCallbacks() {
    // Check for potentiometer changes
    for (int i = 0; i < 3; i++) {
        if (state_.pots[i] != previous_state_.pots[i]) {
            if (parameter_change_callback_) {
                parameter_change_callback_(i, state_.pots[i]);
            }
        }
    }
    
    // Check for button changes
    for (int i = 0; i < 4; i++) {
        if (state_.buttons[i] != previous_state_.buttons[i]) {
            if (button_callback_) {
                button_callback_(i, state_.buttons[i]);
            }
        }
    }
    
    // Check for encoder changes
    for (int i = 0; i < 2; i++) {
        if (state_.encoder_values[i] != previous_state_.encoder_values[i]) {
            if (encoder_callback_) {
                encoder_callback_(state_.encoder_values[i] - previous_state_.encoder_values[i]);
            }
        }
    }
}

bool HardwareInterface::renderKnob(const char* label, float* value, bool* pressed, float min_val, float max_val, float radius) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size(radius * 2.2f, radius * 2.2f + 40); // Extra space for label and value
    
    // Reserve space
    ImGui::InvisibleButton(label, canvas_size);
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    
    // Handle mouse interaction for turning the knob
    bool value_changed = false;
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        float delta = -mouse_delta.y * 0.01f; // Vertical mouse movement controls knob
        *value = std::clamp(*value + delta * (max_val - min_val), min_val, max_val);
        value_changed = true;
    }
    
    // Handle clicking to press the knob
    if (ImGui::IsItemClicked()) {
        *pressed = !*pressed;
    }
    
    // Calculate knob center
    ImVec2 center(canvas_pos.x + radius + 10, canvas_pos.y + radius + 10);
    
    // Draw knob background circle
    ImU32 knob_color = *pressed ? IM_COL32(100, 255, 100, 255) : 
                      is_hovered ? IM_COL32(80, 80, 80, 255) : 
                      IM_COL32(60, 60, 60, 255);
    
    draw_list->AddCircleFilled(center, radius, knob_color);
    draw_list->AddCircle(center, radius, IM_COL32(200, 200, 200, 255), 0, 2.0f);
    
    // Draw knob indicator (white line from center to edge)
    float angle = (*value - min_val) / (max_val - min_val) * 2.0f * M_PI * 0.75f - M_PI * 0.875f; // 270 degree range
    ImVec2 line_end(
        center.x + cos(angle) * (radius * 0.8f),
        center.y + sin(angle) * (radius * 0.8f)
    );
    draw_list->AddLine(center, line_end, IM_COL32(255, 255, 255, 255), 3.0f);
    
    // Draw a small center dot
    draw_list->AddCircleFilled(center, 3.0f, IM_COL32(255, 255, 255, 255));
    
    // Draw label above
    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 label_pos(center.x - text_size.x * 0.5f, canvas_pos.y - 5);
    draw_list->AddText(label_pos, IM_COL32(255, 255, 255, 255), label);
    
    // Show pressed state
    if (*pressed) {
        ImVec2 pressed_pos(center.x - 25, center.y + radius + 15);
        draw_list->AddText(pressed_pos, IM_COL32(100, 255, 100, 255), "PRESSED");
    }
    
    return value_changed;
}

bool HardwareInterface::renderEncoderKnob(const char* label, int* value, bool* pressed, int min_val, int max_val, float radius) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size(radius * 2.2f, radius * 2.2f + 40);
    
    // Reserve space
    ImGui::InvisibleButton(label, canvas_size);
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    
    // Handle mouse interaction for turning the encoder
    bool value_changed = false;
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        int delta = (int)(-mouse_delta.y * 0.1f); // More sensitive for encoders
        *value = std::clamp(*value + delta, min_val, max_val);
        value_changed = true;
    }
    
    // Handle clicking to press the encoder
    if (ImGui::IsItemClicked()) {
        *pressed = !*pressed;
    }
    
    // Calculate encoder center
    ImVec2 center(canvas_pos.x + radius + 10, canvas_pos.y + radius + 10);
    
    // Draw encoder background (slightly different style from pot)
    ImU32 encoder_color = *pressed ? IM_COL32(100, 255, 100, 255) : 
                         is_hovered ? IM_COL32(90, 90, 90, 255) : 
                         IM_COL32(70, 70, 70, 255);
    
    draw_list->AddCircleFilled(center, radius, encoder_color);
    draw_list->AddCircle(center, radius, IM_COL32(220, 220, 220, 255), 0, 2.0f);
    
    // Draw encoder detents (small lines around the edge)
    for (int i = 0; i < 12; i++) {
        float detent_angle = (i / 12.0f) * 2.0f * M_PI;
        ImVec2 outer_pos(
            center.x + cos(detent_angle) * radius,
            center.y + sin(detent_angle) * radius
        );
        ImVec2 inner_pos(
            center.x + cos(detent_angle) * (radius * 0.9f),
            center.y + sin(detent_angle) * (radius * 0.9f)
        );
        draw_list->AddLine(outer_pos, inner_pos, IM_COL32(180, 180, 180, 255), 1.0f);
    }
    
    // Draw current position indicator (more prominent for encoder)
    float normalized_value = (*value - min_val) / (float)(max_val - min_val);
    float angle = normalized_value * 2.0f * M_PI - M_PI * 0.5f; // Full circle
    ImVec2 indicator_pos(
        center.x + cos(angle) * (radius * 0.6f),
        center.y + sin(angle) * (radius * 0.6f)
    );
    draw_list->AddCircleFilled(indicator_pos, 4.0f, IM_COL32(255, 255, 0, 255)); // Yellow dot
    
    // Draw center
    draw_list->AddCircleFilled(center, 4.0f, IM_COL32(255, 255, 255, 255));
    
    // Draw label above
    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 label_pos(center.x - text_size.x * 0.5f, canvas_pos.y - 5);
    draw_list->AddText(label_pos, IM_COL32(255, 255, 255, 255), label);
    
    // Show pressed state
    if (*pressed) {
        ImVec2 pressed_pos(center.x - 25, center.y + radius + 15);
        draw_list->AddText(pressed_pos, IM_COL32(100, 255, 100, 255), "PRESSED");
    }
    
    return value_changed;
}

// External state setters for main window integration
void HardwareInterface::setPotValue(int index, float value) {
    if (index >= 0 && index < 3) {
        pot_display_values_[index] = std::clamp(value, 0.0f, 1.0f);
    }
}

void HardwareInterface::setPotPressed(int index, bool pressed) {
    if (index >= 0 && index < 3) {
        pot_pressed_states_[index] = pressed;
    }
}

void HardwareInterface::setButtonState(int index, bool state) {
    if (index >= 0 && index < 4) {
        button_display_states_[index] = state;
    }
}

void HardwareInterface::setEncoderValue(int index, int value) {
    if (index >= 0 && index < 2) {
        encoder_display_values_[index] = std::clamp(value, -100, 100);
    }
}

void HardwareInterface::setEncoderPressed(int index, bool pressed) {
    if (index >= 0 && index < 2) {
        encoder_pressed_states_[index] = pressed;
    }
}

// State query methods
float HardwareInterface::getPotValue(int index) const {
    if (index >= 0 && index < 3) {
        return pot_display_values_[index];
    }
    return 0.0f;
}

bool HardwareInterface::isPotPressed(int index) const {
    if (index >= 0 && index < 3) {
        return pot_pressed_states_[index];
    }
    return false;
}

bool HardwareInterface::getButtonState(int index) const {
    if (index >= 0 && index < 4) {
        return button_display_states_[index];
    }
    return false;
}

int HardwareInterface::getEncoderValue(int index) const {
    if (index >= 0 && index < 2) {
        return encoder_display_values_[index];
    }
    return 0;
}

bool HardwareInterface::isEncoderPressed(int index) const {
    if (index >= 0 && index < 2) {
        return encoder_pressed_states_[index];
    }
    return false;
}