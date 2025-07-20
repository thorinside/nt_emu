#pragma once

#include <memory>
#include <array>
#include <chrono>
#include <imgui.h>
#include "audio_settings_dialog.h"

// Forward declarations
class Emulator;
class Display;
class HardwareInterface;

class DistingNTMainWindow {
public:
    DistingNTMainWindow();
    ~DistingNTMainWindow() = default;
    
    void setEmulator(std::shared_ptr<Emulator> emulator) { 
        emulator_ = emulator; 
        setupHardwareCallbacks();
    }
    void setHardwareInterface(std::shared_ptr<HardwareInterface> hardware_interface) { 
        hardware_interface_ = hardware_interface; 
        setupHardwareCallbacks();
    }
    void render();
    
private:
    std::shared_ptr<Emulator> emulator_;
    std::shared_ptr<HardwareInterface> hardware_interface_;
    
    // Audio settings dialog
    std::unique_ptr<AudioSettingsDialog> audio_settings_dialog_;
    
    // Plugin loading dialog state
    bool show_plugin_dialog_ = false;
    char plugin_path_buffer_[512] = "";
    
    // Interactive state (synchronized with HardwareInterface)
    std::array<float, 3> pot_values_{0.5f, 0.5f, 0.5f};        // 0.0-1.0
    std::array<bool, 3> pot_pressed_{false, false, false};
    std::array<bool, 4> button_states_{false, false, false, false};
    std::array<int, 2> encoder_values_{0, 0};
    std::array<bool, 2> encoder_pressed_{false, false};
    
    // Visual feedback state
    std::array<float, 3> pot_brightness_{1.0f, 1.0f, 1.0f};
    std::array<float, 4> button_brightness_{1.0f, 1.0f, 1.0f, 1.0f};
    std::array<float, 2> encoder_brightness_{1.0f, 1.0f};
    
    // Display update timing for 30fps
    std::chrono::steady_clock::time_point last_display_update_;
    static constexpr int DISPLAY_FPS = 30;
    static constexpr auto DISPLAY_INTERVAL = std::chrono::milliseconds(1000 / DISPLAY_FPS);
    
    // Rendering methods
    void renderMenuBar();
    void renderHeader();
    void renderDisplay();
    void renderControlsSection();
    void renderCVSection();
    void renderCVJack(ImVec2 pos, float size, float voltage, bool is_input, int number);
    
    // Audio configuration callbacks
    void handleAudioConfigurationApplied(const AudioConfiguration& config);
    void handleAudioConfigurationCancelled();
    
    // Plugin loading dialog
    void renderPluginDialog();
    
    // Interactive control methods
    bool renderInteractivePot(int index, ImVec2 center, float radius);
    bool renderInteractiveButton(int index, ImVec2 center, float radius);
    bool renderInteractiveEncoder(int index, ImVec2 center, float radius);
    void renderPotVisual(ImVec2 center, float radius, float value, bool pressed, float brightness);
    void renderButtonVisual(ImVec2 center, float radius, bool pressed, float brightness);
    void renderEncoderVisual(ImVec2 center, float radius, int value, bool pressed, float brightness);
    void updateBrightness();
    void syncStateToHardware();
    
    // Plugin display methods
    void updatePluginDisplay();
    bool shouldUpdateDisplay();
    
    // Hardware integration setup
    void setupHardwareCallbacks();
    
    // Utility methods
    ImU32 getVoltageColor(float voltage);
    float getInputVoltage(int index);
    float getOutputVoltage(int index);
    
    // Layout constants - matching actual hardware proportions
    static constexpr float WINDOW_WIDTH = 600.0f;
    static constexpr float WINDOW_HEIGHT = 800.0f;  // Taller to match hardware ratio
    static constexpr float HEADER_HEIGHT = 60.0f;
    static constexpr float DISPLAY_HEIGHT = 100.0f;
    static constexpr float KNOBS_HEIGHT = 120.0f;
    static constexpr float ENCODERS_HEIGHT = 80.0f;
    static constexpr float CV_SECTION_HEIGHT = 300.0f;
    static constexpr float BOTTOM_MARGIN = 40.0f;
};