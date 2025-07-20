#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <functional>
#include "../core/audio_device_manager.h"
#include "../utils/config.h"

class AudioSettingsDialog {
public:
    AudioSettingsDialog();
    ~AudioSettingsDialog();
    
    void show() { is_open_ = true; refreshDeviceList(); }
    void hide() { is_open_ = false; }
    bool isOpen() const { return is_open_; }
    
    void render();  // Called from main render loop
    
    // Configuration management
    void setCurrentConfiguration(const AudioConfiguration& config);
    AudioConfiguration getCurrentConfiguration() const { return temp_config_; }
    
    // Callbacks
    std::function<void(const AudioConfiguration&)> on_apply;
    std::function<void()> on_cancel;
    
private:
    bool is_open_;
    AudioConfiguration temp_config_;
    AudioConfiguration original_config_;
    
    // Device lists
    std::vector<AudioDeviceInfo> input_devices_;
    std::vector<AudioDeviceInfo> output_devices_;
    
    // UI state
    int selected_input_device_index_;
    int selected_output_device_index_;
    std::string error_message_;
    bool devices_need_refresh_;
    
    // Buffer size options
    static const std::vector<int> buffer_sizes_;
    int selected_buffer_size_index_;
    
    // Sample rate options  
    static const std::vector<double> sample_rates_;
    int selected_sample_rate_index_;
    
    // UI rendering methods
    void renderDeviceSelection();
    void renderBufferConfiguration();
    void renderChannelMapping();
    void renderErrorMessages();
    void renderButtons();
    
    // Helper methods
    void refreshDeviceList();
    bool validateConfiguration();
    void resetToDefaults();
    void applyConfiguration();
    void cancelConfiguration();
    
    int findDeviceIndex(const std::vector<AudioDeviceInfo>& devices, int device_id);
    int findBufferSizeIndex(int buffer_size);
    int findSampleRateIndex(double sample_rate);
    
    const char* getBufferSizeLabel(int buffer_size);
    const char* getSampleRateLabel(double sample_rate);
};