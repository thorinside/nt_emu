#include "main_window.h"
#include <imgui.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include "../core/emulator.h"
#include "../core/api_shim.h"
#include "../utils/file_dialog.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DistingNTMainWindow::DistingNTMainWindow() {
    // Initialize audio settings dialog
    audio_settings_dialog_ = std::make_unique<AudioSettingsDialog>();
    
    // Set up callbacks
    audio_settings_dialog_->on_apply = [this](const AudioConfiguration& config) {
        handleAudioConfigurationApplied(config);
    };
    
    audio_settings_dialog_->on_cancel = [this]() {
        handleAudioConfigurationCancelled();
    };
    
    // Initialize display timing
    last_display_update_ = std::chrono::steady_clock::now();
}

void DistingNTMainWindow::setupHardwareCallbacks() {
    if (!hardware_interface_ || !emulator_) return;
    
    // Set up hardware callbacks to forward to emulator
    hardware_interface_->setParameterChangeCallback([this](int param, float value) {
        if (emulator_ && emulator_->isPluginLoaded()) {
            emulator_->onParameterChange(param, value);
        }
    });

    hardware_interface_->setButtonCallback([this](int button, bool pressed) {
        if (emulator_ && emulator_->isPluginLoaded()) {
            emulator_->onButtonPress(button, pressed);
        }
    });

    hardware_interface_->setEncoderCallback([this](int delta) {
        if (emulator_ && emulator_->isPluginLoaded()) {
            emulator_->onEncoderChange(delta);
        }
    });
}

void DistingNTMainWindow::render() {
    // Set window style for authentic hardware look
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));  // Dark charcoal
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));   // White text
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(60, 60, 60, 255));    // Dark border
    
    // Fixed size window matching hardware proportions
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("Expert Sleepers Disting NT", nullptr, window_flags)) {
        renderMenuBar();
        renderHeader();
        
        // Update plugin display at 30fps
        if (shouldUpdateDisplay()) {
            updatePluginDisplay();
        }
        
        renderDisplay();
        renderControlsSection();
        renderCVSection();
    }
    ImGui::End();
    
    // Render audio settings dialog
    if (audio_settings_dialog_) {
        audio_settings_dialog_->render();
    }
    
    // Render plugin loading dialog
    renderPluginDialog();
    
    ImGui::PopStyleColor(3);
}

void DistingNTMainWindow::renderHeader() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 content_pos = ImGui::GetCursorScreenPos();
    
    // White mounting holes in corners (like actual hardware)
    float hole_radius = 6.0f;
    ImU32 hole_color = IM_COL32(255, 255, 255, 255);
    draw_list->AddCircleFilled(ImVec2(content_pos.x + 15, content_pos.y + 15), hole_radius, hole_color);
    draw_list->AddCircleFilled(ImVec2(content_pos.x + WINDOW_WIDTH - 15, content_pos.y + 15), hole_radius, hole_color);
    
    // Small round encoders/buttons next to display (like reference)
    float small_encoder_radius = 12.0f;
    ImVec2 left_encoder(content_pos.x + 80, content_pos.y + 25);
    ImVec2 right_encoder(content_pos.x + WINDOW_WIDTH - 80, content_pos.y + 25);
    
    // Left encoder (dark with center dot)
    draw_list->AddCircleFilled(left_encoder, small_encoder_radius, IM_COL32(40, 40, 40, 255));
    draw_list->AddCircle(left_encoder, small_encoder_radius, IM_COL32(70, 70, 70, 255), 0, 1.5f);
    draw_list->AddCircleFilled(left_encoder, 3.0f, IM_COL32(60, 60, 60, 255));
    
    // Right encoder (dark with center dot) 
    draw_list->AddCircleFilled(right_encoder, small_encoder_radius, IM_COL32(40, 40, 40, 255));
    draw_list->AddCircle(right_encoder, small_encoder_radius, IM_COL32(70, 70, 70, 255), 0, 1.5f);
    draw_list->AddCircleFilled(right_encoder, 3.0f, IM_COL32(60, 60, 60, 255));
    
    // Expert Sleepers logo (centered, matching reference)
    ImVec2 logo_pos(content_pos.x + WINDOW_WIDTH/2 - 15, content_pos.y + 5);
    ImU32 logo_color = IM_COL32(255, 255, 255, 255);
    
    // Draw stylized "Z" with proper proportions
    draw_list->AddText(ImGui::GetFont(), 36.0f, logo_pos, logo_color, "Z");
    
    // Multiple horizontal lines above and below Z (matching reference pattern)
    float line_width = 35.0f;
    float line_spacing = 2.0f;
    ImVec2 line_start(logo_pos.x - 8, logo_pos.y - 6);
    
    for (int i = 0; i < 4; i++) {
        draw_list->AddLine(
            ImVec2(line_start.x, line_start.y - i * line_spacing),
            ImVec2(line_start.x + line_width, line_start.y - i * line_spacing),
            logo_color, 1.0f
        );
        draw_list->AddLine(
            ImVec2(line_start.x, line_start.y + 30 + i * line_spacing),
            ImVec2(line_start.x + line_width, line_start.y + 30 + i * line_spacing),
            logo_color, 1.0f
        );
    }
    
    // Reserve space for header
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + HEADER_HEIGHT);
}

void DistingNTMainWindow::renderDisplay() {
    if (!emulator_) return;
    
    ImVec2 content_pos = ImGui::GetCursorScreenPos();
    // Display size matching NT hardware (256x64 scaled up)
    const float scale = 1.75f;  // Scale factor for visibility
    ImVec2 display_size(256 * scale, 64 * scale);  // Native NT display resolution
    ImVec2 display_pos(content_pos.x + (WINDOW_WIDTH - display_size.x) / 2, content_pos.y + 10);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Black display background
    draw_list->AddRectFilled(
        display_pos,
        ImVec2(display_pos.x + display_size.x, display_pos.y + display_size.y),
        IM_COL32(0, 0, 0, 255)
    );
    
    // Render plugin display content
    if (emulator_->isPluginLoaded()) {
        // Get the display buffer from API state
        const auto& display_buffer = ApiShim::getState().display;
        
        // Render each pixel from the plugin's display buffer
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 256; x++) {
                if (display_buffer.getPixel(x, y)) {
                    ImVec2 pixel_pos(
                        display_pos.x + x * scale,
                        display_pos.y + y * scale
                    );
                    
                    ImVec2 pixel_end(
                        pixel_pos.x + scale,
                        pixel_pos.y + scale
                    );
                    
                    draw_list->AddRectFilled(
                        pixel_pos,
                        pixel_end,
                        IM_COL32(0, 255, 255, 255)  // Cyan pixels like real hardware
                    );
                }
            }
        }
    } else {
        // Show default text when no plugin is loaded
        ImVec2 text_pos(display_pos.x + display_size.x/2 - 85, display_pos.y + 20);
        draw_list->AddText(ImGui::GetFont(), 16.0f, text_pos, IM_COL32(0, 255, 255, 255), "expert sleepers");
        draw_list->AddText(
            ImGui::GetFont(), 18.0f,
            ImVec2(text_pos.x + 40, text_pos.y + 25), 
            IM_COL32(0, 255, 255, 255), 
            "disting NT"
        );
    }
    
    // Thin white border around display (matching hardware)
    draw_list->AddRect(
        display_pos,
        ImVec2(display_pos.x + display_size.x, display_pos.y + display_size.y),
        IM_COL32(180, 180, 180, 255),
        0.0f, 0, 1.0f
    );
    
    // Reserve space for display
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + DISPLAY_HEIGHT);
}

void DistingNTMainWindow::renderControlsSection() {
    ImVec2 content_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // USB connector on left side (matching reference)
    ImVec2 usb_pos(content_pos.x + 20, content_pos.y + 60);
    draw_list->AddRectFilled(
        usb_pos,
        ImVec2(usb_pos.x + 25, usb_pos.y + 10),
        IM_COL32(50, 50, 50, 255)
    );
    draw_list->AddRect(
        usb_pos,
        ImVec2(usb_pos.x + 25, usb_pos.y + 10),
        IM_COL32(80, 80, 80, 255),
        0.0f, 0, 1.0f
    );
    
    // 3 Large Interactive Potentiometers - centered between screen edges
    float knob_size = 32.0f;
    float knob_spacing = 120.0f;
    float total_knobs_width = knob_spacing * 2;  // 2 gaps between 3 knobs
    float knob_start_x = content_pos.x + (WINDOW_WIDTH - total_knobs_width) / 2;
    float knob_y = content_pos.y + 45;  // Moved back down to match reference
    
    bool any_pot_changed = false;
    for (int i = 0; i < 3; i++) {
        ImVec2 knob_center(knob_start_x + i * knob_spacing, knob_y);
        any_pot_changed |= renderInteractivePot(i, knob_center, knob_size);
    }
    
    if (any_pot_changed) {
        syncStateToHardware();
    }
    
    // Reserve space for knobs (reduced spacing)
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 70);  // Reduced from KNOBS_HEIGHT to 70
    
    // 2 Encoders and buttons section - centered between screen edges
    ImVec2 encoder_content_pos = ImGui::GetCursorScreenPos();
    float encoder_size = 22.0f;
    float encoder_y = encoder_content_pos.y + 10;  // Reduced from 25 to 10
    float encoder_spacing = 100.0f;
    ImVec2 encoder_center_start(encoder_content_pos.x + WINDOW_WIDTH/2 - encoder_spacing/2, encoder_y);
    
    // Side buttons (4 total, aligned with outer pots)
    float side_button_size = 8.0f;
    
    // Calculate pot positions to align buttons correctly
    float pot_knob_spacing = 120.0f;
    float pot_total_width = pot_knob_spacing * 2;
    float pot_start_x = encoder_content_pos.x + (WINDOW_WIDTH - pot_total_width) / 2;
    float leftmost_pot_x = pot_start_x;
    float rightmost_pot_x = pot_start_x + 2 * pot_knob_spacing;
    
    // Left side buttons (aligned with leftmost pot)
    ImVec2 left_buttons[2] = {
        {leftmost_pot_x, encoder_y - 15},  // Reduced from 20 to 15
        {leftmost_pot_x, encoder_y + 15}   // Reduced from 20 to 15
    };
    
    // Right side buttons (aligned with rightmost pot)
    ImVec2 right_buttons[2] = {
        {rightmost_pot_x, encoder_y - 15}, // Reduced from 20 to 15
        {rightmost_pot_x, encoder_y + 15}  // Reduced from 20 to 15
    };
    
    // Draw interactive side buttons
    bool any_button_changed = false;
    for (int i = 0; i < 2; i++) {
        // Left buttons (button indices 0 and 1)
        any_button_changed |= renderInteractiveButton(i, left_buttons[i], side_button_size);
        
        // Right buttons (button indices 2 and 3)
        any_button_changed |= renderInteractiveButton(i + 2, right_buttons[i], side_button_size);
    }
    
    if (any_button_changed) {
        syncStateToHardware();
    }
    
    // 2 Interactive Encoders (smaller, very dark like reference)
    bool any_encoder_changed = false;
    for (int i = 0; i < 2; i++) {
        ImVec2 encoder_center(encoder_center_start.x + i * encoder_spacing, encoder_center_start.y);
        any_encoder_changed |= renderInteractiveEncoder(i, encoder_center, encoder_size);
    }
    
    if (any_encoder_changed) {
        syncStateToHardware();
    }
    
    // Reserve space for encoders section (reduced spacing)
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 60);  // Reduced from ENCODERS_HEIGHT to 60
}

void DistingNTMainWindow::renderCVSection() {
    ImVec2 content_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float jack_size = 14.0f;  // Smaller to prevent overlap
    float jack_spacing_x = 50.0f;  // More space around jacks
    float jack_spacing_y = 45.0f;  // More space around jacks
    
    // Left side: 12 inputs in 3 rows of 4 (matching reference exactly)
    ImVec2 inputs_start(content_pos.x + 50, content_pos.y + 30);
    
    for (int i = 0; i < 12; i++) {
        int row = i / 4;  // 4 jacks per row
        int col = i % 4;
        ImVec2 jack_pos(
            inputs_start.x + col * jack_spacing_x,
            inputs_start.y + row * jack_spacing_y
        );
        
        float voltage = getInputVoltage(i);
        renderCVJack(jack_pos, jack_size, voltage, true, i + 1);
    }
    
    // Right side: 6 outputs in 3 rows of 2 (matching reference exactly)
    ImVec2 outputs_start(content_pos.x + WINDOW_WIDTH - 150, content_pos.y + 30);
    
    for (int i = 0; i < 6; i++) {
        int row = i / 2;  // 2 jacks per row
        int col = i % 2;
        ImVec2 jack_pos(
            outputs_start.x + col * jack_spacing_x,
            outputs_start.y + row * jack_spacing_y
        );
        
        float voltage = getOutputVoltage(i);
        renderCVJack(jack_pos, jack_size, voltage, false, i + 1);
    }
    
    // Bottom mounting holes (like reference)
    float hole_radius = 6.0f;
    ImU32 hole_color = IM_COL32(255, 255, 255, 255);
    draw_list->AddCircleFilled(
        ImVec2(content_pos.x + 15, content_pos.y + CV_SECTION_HEIGHT - 20), 
        hole_radius, hole_color
    );
    draw_list->AddCircleFilled(
        ImVec2(content_pos.x + WINDOW_WIDTH - 15, content_pos.y + CV_SECTION_HEIGHT - 20), 
        hole_radius, hole_color
    );
    
    // Voltage monitoring control (small debug control)
    ImGui::SetCursorScreenPos(ImVec2(content_pos.x + 20, content_pos.y + CV_SECTION_HEIGHT - 60));
    auto& voltage_state = ApiShim::getState().voltage;
    bool monitoring = voltage_state.monitoring_enabled.load();
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    if (ImGui::Checkbox("CV Monitoring", &monitoring)) {
        voltage_state.monitoring_enabled.store(monitoring);
    }
    ImGui::PopStyleVar();
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable/disable real-time CV voltage display\n(disable for better performance)");
    }
    
    // Bottom text: "expert sleepers disting NT" (matching reference)
    ImVec2 bottom_text_pos(content_pos.x + WINDOW_WIDTH/2 - 70, content_pos.y + CV_SECTION_HEIGHT - 35);
    draw_list->AddText(bottom_text_pos, IM_COL32(180, 180, 180, 255), "expert sleepers");
    draw_list->AddText(
        ImVec2(bottom_text_pos.x + 15, bottom_text_pos.y + 15), 
        IM_COL32(180, 180, 180, 255), 
        "disting NT"
    );
}

void DistingNTMainWindow::renderCVJack(ImVec2 pos, float size, float voltage, bool is_input, int number) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Add connection indicator (green ring around jack when connected)
    bool is_connected = std::abs(voltage) > 0.1f;
    if (is_connected) {
        // Draw connection ring around jack
        ImU32 connection_color = IM_COL32(0, 255, 0, 128); // Green glow
        draw_list->AddCircle(pos, size + 8, connection_color, 0, 3.0f);
    }
    
    // Outer threaded ring (metallic silver with threading)
    ImU32 outer_ring_color = IM_COL32(200, 200, 200, 255);
    draw_list->AddCircleFilled(pos, size + 2, outer_ring_color);
    
    // Threading lines (multiple concentric circles to simulate threads)
    for (int i = 1; i <= 3; i++) {
        draw_list->AddCircle(pos, size + 2 - i * 1.5f, IM_COL32(160, 160, 160, 255), 0, 0.5f);
    }
    
    // Main jack body (bright metallic)
    ImU32 jack_color = IM_COL32(220, 220, 220, 255);
    draw_list->AddCircleFilled(pos, size, jack_color);
    
    // Inner shadow ring
    draw_list->AddCircle(pos, size - 1, IM_COL32(180, 180, 180, 255), 0, 1.0f);
    
    // Center hole (dark)
    draw_list->AddCircleFilled(pos, size * 0.5f, IM_COL32(20, 20, 20, 255));
    
    // Inner metallic ring
    draw_list->AddCircle(pos, size * 0.5f, IM_COL32(100, 100, 100, 255), 0, 1.0f);
    
    // Voltage indicator ring (glowing effect)
    ImU32 voltage_color = getVoltageColor(voltage);
    if (voltage != 0.0f) {
        // Outer glow
        draw_list->AddCircle(pos, size + 6, voltage_color, 0, 4.0f);
        // Inner indicator
        draw_list->AddCircle(pos, size + 4, voltage_color, 0, 2.0f);
    }
    
    // Number label (smaller, positioned like reference)
    char label[4];
    snprintf(label, sizeof(label), "%d", number);
    ImVec2 label_pos(pos.x - 4, pos.y + size + 12);
    draw_list->AddText(ImGui::GetFont(), 10.0f, label_pos, IM_COL32(180, 180, 180, 255), label);
}

ImU32 DistingNTMainWindow::getVoltageColor(float voltage) {
    if (voltage > 0) {
        // Red shades for positive voltage
        int intensity = (int)(voltage * 25) + 100;
        intensity = std::min(255, std::max(100, intensity));
        return IM_COL32(intensity, 0, 0, 255);
    } else if (voltage < 0) {
        // Blue shades for negative voltage
        int intensity = (int)(-voltage * 25) + 100;
        intensity = std::min(255, std::max(100, intensity));
        return IM_COL32(0, 0, intensity, 255);
    } else {
        // Gray for zero voltage
        return IM_COL32(80, 80, 80, 255);
    }
}

float DistingNTMainWindow::getInputVoltage(int index) {
    if (index >= 0 && index < 12) {
        auto& voltage_state = ApiShim::getState().voltage;
        return voltage_state.input_voltages[index].load();
    }
    return 0.0f;
}

float DistingNTMainWindow::getOutputVoltage(int index) {
    if (index >= 0 && index < 6) {
        auto& voltage_state = ApiShim::getState().voltage;
        return voltage_state.output_voltages[index].load();
    }
    return 0.0f;
}

// Interactive control methods
bool DistingNTMainWindow::renderInteractivePot(int index, ImVec2 center, float radius) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    
    char id[32];
    snprintf(id, sizeof(id), "pot_%d", index);
    ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));
    
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool value_changed = false;
    
    // Handle mouse interaction
    if (ImGui::IsMouseHoveringRect(
        ImVec2(center.x - radius, center.y - radius),
        ImVec2(center.x + radius, center.y + radius))) {
            
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            // Calculate new pot value from mouse delta
            float delta = ImGui::GetIO().MouseDelta.y * -0.01f;
            pot_values_[index] = std::clamp(pot_values_[index] + delta, 0.0f, 1.0f);
            value_changed = true;
        }
        
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            pot_pressed_[index] = true;
            value_changed = true;
        }
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        pot_pressed_[index] = false;
        value_changed = true;
    }
    
    // Update brightness for visual feedback
    float target_brightness = 1.0f;
    if (pot_pressed_[index]) target_brightness = 1.4f;
    else if (is_hovered) target_brightness = 1.2f;
    
    pot_brightness_[index] = pot_brightness_[index] + (target_brightness - pot_brightness_[index]) * 0.1f;
    
    // Render visual with brightness adjustment
    renderPotVisual(center, radius, pot_values_[index], pot_pressed_[index], pot_brightness_[index]);
    
    return value_changed;
}

void DistingNTMainWindow::renderPotVisual(ImVec2 center, float radius, float value, bool pressed, float brightness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Brightness-adjusted colors
    ImU32 knob_color = IM_COL32(
        (int)(50 * brightness), (int)(50 * brightness), (int)(50 * brightness), 255
    );
    ImU32 ring_color = IM_COL32(
        (int)(75 * brightness), (int)(75 * brightness), (int)(75 * brightness), 255
    );
    
    // Outer shadow ring
    draw_list->AddCircleFilled(ImVec2(center.x, center.y), radius + 2, IM_COL32(25, 25, 25, 255));
    
    // Render knob body with brightness
    draw_list->AddCircleFilled(center, radius, knob_color);
    draw_list->AddCircle(center, radius, ring_color, 0, 2.0f);
    
    // Inner concentric rings for depth
    draw_list->AddCircle(center, radius - 5, IM_COL32((int)(40 * brightness), (int)(40 * brightness), (int)(40 * brightness), 255), 0, 1.0f);
    draw_list->AddCircle(center, radius - 10, IM_COL32((int)(35 * brightness), (int)(35 * brightness), (int)(35 * brightness), 255), 0, 1.0f);
    
    // Calculate indicator line angle with noon (12 o'clock) as 0° reference
    // 0.0 = -160° from noon, 0.5 = 0° (noon), 1.0 = +160° from noon
    float min_angle = -160.0f * M_PI / 180.0f;  // -160° from noon
    float max_angle = 160.0f * M_PI / 180.0f;   // +160° from noon
    float center_angle = 0.0f;                  // 0° (noon/12 o'clock)
    
    // Offset by -90° to convert from noon reference to standard math angles
    float angle_offset = -M_PI / 2.0f;
    
    // Map value 0.0-1.0 to the actual hardware range
    float angle;
    if (value <= 0.5f) {
        // Map 0.0-0.5 from -160° to 0° (noon)
        angle = min_angle + (center_angle - min_angle) * (value / 0.5f);
    } else {
        // Map 0.5-1.0 from 0° (noon) to +160°
        angle = center_angle + (max_angle - center_angle) * ((value - 0.5f) / 0.5f);
    }
    
    // Apply offset for ImGui coordinate system (where -90° is up)
    angle += angle_offset;
    
    // Render indicator line
    ImVec2 indicator_start(center.x + cos(angle) * 6, center.y + sin(angle) * 6);
    ImVec2 indicator_end(center.x + cos(angle) * (radius - 3), center.y + sin(angle) * (radius - 3));
    
    ImU32 indicator_color = IM_COL32((int)(255 * brightness), (int)(255 * brightness), (int)(255 * brightness), 255);
    draw_list->AddLine(indicator_start, indicator_end, indicator_color, 5.0f);
}

bool DistingNTMainWindow::renderInteractiveButton(int index, ImVec2 center, float radius) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    
    char id[32];
    snprintf(id, sizeof(id), "button_%d", index);
    ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));
    
    bool is_hovered = ImGui::IsItemHovered();
    bool is_active = ImGui::IsItemActive();
    bool clicked = ImGui::IsItemClicked();
    
    // Momentary button behavior - pressed only while mouse is down
    button_states_[index] = is_active;
    
    // Update brightness for visual feedback
    float target_brightness = 1.0f;
    if (button_states_[index]) target_brightness = 1.6f;
    else if (is_hovered) target_brightness = 1.3f;
    
    button_brightness_[index] = button_brightness_[index] + (target_brightness - button_brightness_[index]) * 0.15f;
    
    // Render visual with state and brightness
    renderButtonVisual(center, radius, button_states_[index], button_brightness_[index]);
    
    return clicked;
}

void DistingNTMainWindow::renderButtonVisual(ImVec2 center, float radius, bool pressed, float brightness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Brightness and state-adjusted colors
    int base_color = pressed ? 60 : 30;
    ImU32 button_color = IM_COL32(
        (int)(base_color * brightness), (int)(base_color * brightness), (int)(base_color * brightness), 255
    );
    ImU32 ring_color = IM_COL32(
        (int)(80 * brightness), (int)(80 * brightness), (int)(80 * brightness), 255
    );
    
    // Render button with pressed/unpressed appearance
    draw_list->AddCircleFilled(center, radius, button_color);
    draw_list->AddCircle(center, radius, ring_color, 0, pressed ? 2.0f : 1.0f);
    
    // Add inner highlight if pressed
    if (pressed) {
        ImU32 highlight_color = IM_COL32((int)(100 * brightness), (int)(100 * brightness), (int)(100 * brightness), 255);
        draw_list->AddCircle(center, radius - 2, highlight_color, 0, 1.0f);
    }
}

bool DistingNTMainWindow::renderInteractiveEncoder(int index, ImVec2 center, float radius) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    
    char id[32];
    snprintf(id, sizeof(id), "encoder_%d", index);
    ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));
    
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool value_changed = false;
    
    // Handle mouse wheel for encoder increment/decrement
    if (is_hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            encoder_values_[index] += (wheel > 0) ? 1 : -1;
            encoder_values_[index] = std::clamp(encoder_values_[index], -100, 100);
            value_changed = true;
        }
    }
    
    // Handle horizontal drag as alternative
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        static float accumulated_delta = 0.0f;
        accumulated_delta += mouse_delta.x;
        
        if (std::abs(accumulated_delta) > 10.0f) {
            encoder_values_[index] += (accumulated_delta > 0) ? 1 : -1;
            encoder_values_[index] = std::clamp(encoder_values_[index], -100, 100);
            accumulated_delta = 0.0f;
            value_changed = true;
        }
    }
    
    // Handle momentary press state (pressed only while mouse is down)
    encoder_pressed_[index] = is_active;
    
    // Update brightness for visual feedback
    float target_brightness = 1.0f;
    if (encoder_pressed_[index]) target_brightness = 1.4f;
    else if (is_hovered) target_brightness = 1.2f;
    
    encoder_brightness_[index] = encoder_brightness_[index] + (target_brightness - encoder_brightness_[index]) * 0.1f;
    
    // Render visual with brightness and value indication
    renderEncoderVisual(center, radius, encoder_values_[index], encoder_pressed_[index], encoder_brightness_[index]);
    
    return value_changed;
}

void DistingNTMainWindow::renderEncoderVisual(ImVec2 center, float radius, int value, bool pressed, float brightness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Brightness-adjusted colors
    int base_color = 25;
    ImU32 encoder_color = IM_COL32(
        (int)(base_color * brightness), (int)(base_color * brightness), (int)(base_color * brightness), 255
    );
    ImU32 ring_color = IM_COL32(
        (int)(45 * brightness), (int)(45 * brightness), (int)(45 * brightness), 255
    );
    
    // Outer shadow
    draw_list->AddCircleFilled(ImVec2(center.x, center.y), radius + 1, IM_COL32(20, 20, 20, 255));
    
    // Main encoder body (very dark, almost black)
    draw_list->AddCircleFilled(center, radius, encoder_color);
    draw_list->AddCircle(center, radius, ring_color, 0, 1.5f);
    
    // Inner detail rings
    draw_list->AddCircle(center, radius - 4, IM_COL32((int)(20 * brightness), (int)(20 * brightness), (int)(20 * brightness), 255), 0, 1.0f);
    draw_list->AddCircle(center, radius - 8, IM_COL32((int)(15 * brightness), (int)(15 * brightness), (int)(15 * brightness), 255), 0, 0.5f);
    
    // Value indicator (small notch or dot)
    if (value != 0) {
        float angle = (value / 100.0f) * M_PI; // Map -100 to 100 range to -π to π
        ImVec2 indicator_pos(center.x + cos(angle) * (radius - 6), center.y + sin(angle) * (radius - 6));
        ImU32 indicator_color = IM_COL32((int)(150 * brightness), (int)(150 * brightness), (int)(150 * brightness), 255);
        draw_list->AddCircleFilled(indicator_pos, 2.0f, indicator_color);
    }
}

void DistingNTMainWindow::updateBrightness() {
    // This method is called each frame to smoothly animate brightness changes
    // The actual brightness interpolation is handled in each render method
}

void DistingNTMainWindow::syncStateToHardware() {
    if (!hardware_interface_) return;
    
    // Keep track of previous values to detect changes
    static std::array<float, 3> prev_pot_values{-1.0f, -1.0f, -1.0f};
    static std::array<bool, 4> prev_button_states{false, false, false, false};
    static std::array<int, 2> prev_encoder_values{-999, -999};
    
    // Sync potentiometer values and press states
    for (int i = 0; i < 3; i++) {
        hardware_interface_->setPotValue(i, pot_values_[i]);
        hardware_interface_->setPotPressed(i, pot_pressed_[i]);
        
        // Send parameter change to emulator if value changed
        if (emulator_ && emulator_->isPluginLoaded() && prev_pot_values[i] != pot_values_[i]) {
            emulator_->onParameterChange(i, pot_values_[i]);
            prev_pot_values[i] = pot_values_[i];
        }
    }
    
    // Sync button states
    for (int i = 0; i < 4; i++) {
        hardware_interface_->setButtonState(i, button_states_[i]);
        
        // Send button press to emulator if state changed
        if (emulator_ && emulator_->isPluginLoaded() && prev_button_states[i] != button_states_[i]) {
            emulator_->onButtonPress(i, button_states_[i]);
            prev_button_states[i] = button_states_[i];
        }
    }
    
    // Sync encoder values and press states
    for (int i = 0; i < 2; i++) {
        hardware_interface_->setEncoderValue(i, encoder_values_[i]);
        hardware_interface_->setEncoderPressed(i, encoder_pressed_[i]);
        
        // Send encoder change to emulator if value changed
        if (emulator_ && emulator_->isPluginLoaded() && prev_encoder_values[i] != encoder_values_[i]) {
            int delta = encoder_values_[i] - prev_encoder_values[i];
            emulator_->onEncoderChange(delta);
            prev_encoder_values[i] = encoder_values_[i];
        }
    }
}

void DistingNTMainWindow::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Plugin...")) {
                show_plugin_dialog_ = true;
            }
            
            ImGui::Separator();
            
            if (emulator_ && emulator_->isPluginLoaded()) {
                if (ImGui::MenuItem("Unload Plugin")) {
                    emulator_->unloadPlugin();
                }
                
                ImGui::Separator();
                
                // Show current plugin info
                ImGui::TextDisabled("Current: %s", emulator_->getPluginPath().c_str());
            } else {
                ImGui::TextDisabled("No plugin loaded");
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Audio")) {
            if (emulator_) {
                bool audio_running = emulator_->isAudioRunning();
                
                if (ImGui::MenuItem(audio_running ? "Stop Audio" : "Start Audio")) {
                    if (audio_running) {
                        emulator_->stopAudio();
                    } else {
                        emulator_->startAudio();
                    }
                }
                
                ImGui::Separator();
            }
            
            if (ImGui::MenuItem("Audio Settings...")) {
                if (audio_settings_dialog_ && emulator_) {
                    // Get current configuration from audio engine
                    auto audio_engine = emulator_->getAudioEngine();
                    if (audio_engine) {
                        AudioConfiguration current_config = audio_engine->getCurrentConfiguration();
                        audio_settings_dialog_->setCurrentConfiguration(current_config);
                        audio_settings_dialog_->show();
                    }
                }
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Settings")) {
            
            if (ImGui::MenuItem("Exit")) {
                // Handle application exit
                std::exit(0);
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Could show an about dialog in the future
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void DistingNTMainWindow::handleAudioConfigurationApplied(const AudioConfiguration& config) {
    if (!emulator_) {
        std::cerr << "No emulator available for audio configuration" << std::endl;
        return;
    }
    
    auto audio_engine = emulator_->getAudioEngine();
    if (!audio_engine) {
        std::cerr << "No audio engine available for configuration" << std::endl;
        return;
    }
    
    // Apply the new configuration to the audio engine
    bool was_running = audio_engine->isRunning();
    
    if (!audio_engine->configureDevices(config)) {
        std::cerr << "Failed to configure audio devices: " << audio_engine->getLastError() << std::endl;
        return;
    }
    
    // Restart audio if it was running
    if (was_running) {
        audio_engine->start();
    }
    
    // Save configuration
    auto config_manager = emulator_->getConfig();
    if (config_manager) {
        config_manager->setAudioConfig(config);
        if (!config_manager->save()) {
            std::cerr << "Failed to save audio configuration: " << config_manager->getLastError() << std::endl;
        }
    }
    
    std::cout << "Audio configuration applied successfully" << std::endl;
    std::cout << "Status: " << audio_engine->getDeviceStatusString() << std::endl;
}

void DistingNTMainWindow::handleAudioConfigurationCancelled() {
    std::cout << "Audio configuration cancelled" << std::endl;
}

void DistingNTMainWindow::renderPluginDialog() {
    if (!show_plugin_dialog_) return;
    
    // Center the dialog
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Appearing);
    
    if (ImGui::Begin("Load Plugin", &show_plugin_dialog_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Enter plugin path (.dylib/.so/.dll):");
        ImGui::Spacing();
        
        // Plugin path input
        ImGui::SetNextItemWidth(-100); // Leave space for Browse button
        ImGui::InputText("##PluginPath", plugin_path_buffer_, sizeof(plugin_path_buffer_));
        
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            std::vector<std::string> filters;
#ifdef __APPLE__
            filters = {"dylib"};
#elif defined(_WIN32)
            filters = {"dll"};
#else
            filters = {"so"};
#endif
            
            std::string selectedPath = FileDialog::openFile(
                "Select Plugin",
                "", // Default path
                filters,
                "Plugin Files"
            );
            
            if (!selectedPath.empty()) {
                // Copy selected path to buffer
                strncpy(plugin_path_buffer_, selectedPath.c_str(), sizeof(plugin_path_buffer_) - 1);
                plugin_path_buffer_[sizeof(plugin_path_buffer_) - 1] = '\0';
            }
        }
        
        ImGui::Spacing();
        ImGui::Text("Examples:");
        ImGui::BulletText("macOS: /path/to/plugin.dylib");
        ImGui::BulletText("Linux: /path/to/plugin.so");
        ImGui::BulletText("Windows: C:\\path\\to\\plugin.dll");
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Buttons
        float button_width = 80.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float total_width = button_width * 2 + spacing;
        float window_width = ImGui::GetWindowSize().x;
        float offset = (window_width - total_width) * 0.5f;
        
        ImGui::SetCursorPosX(offset);
        
        if (ImGui::Button("Load", ImVec2(button_width, 0))) {
            if (strlen(plugin_path_buffer_) > 0 && emulator_) {
                std::cout << "Loading plugin: " << plugin_path_buffer_ << std::endl;
                if (emulator_->loadPlugin(plugin_path_buffer_)) {
                    std::cout << "Plugin loaded successfully!" << std::endl;
                    show_plugin_dialog_ = false;
                } else {
                    std::cout << "Failed to load plugin. Check the path and file format." << std::endl;
                }
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
            show_plugin_dialog_ = false;
        }
    }
    ImGui::End();
}

bool DistingNTMainWindow::shouldUpdateDisplay() {
    auto now = std::chrono::steady_clock::now();
    return (now - last_display_update_) >= DISPLAY_INTERVAL;
}

void DistingNTMainWindow::updatePluginDisplay() {
    if (!emulator_ || !emulator_->isPluginLoaded()) {
        return;
    }
    
    // Call emulator's updateDisplay method which handles plugin draw calls
    emulator_->updateDisplay();
    
    // Update timing
    last_display_update_ = std::chrono::steady_clock::now();
}