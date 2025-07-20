#include "audio_settings_dialog.h"
#include <algorithm>
#include <cmath>

// Static member definitions
const std::vector<int> AudioSettingsDialog::buffer_sizes_ = {32, 64, 128, 256, 512, 1024, 2048};
const std::vector<double> AudioSettingsDialog::sample_rates_ = {22050.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};

AudioSettingsDialog::AudioSettingsDialog() 
    : is_open_(false)
    , selected_input_device_index_(-1)
    , selected_output_device_index_(-1)
    , devices_need_refresh_(true)
    , selected_buffer_size_index_(1) // Default to 64 samples
    , selected_sample_rate_index_(2) // Default to 48kHz
{
}

AudioSettingsDialog::~AudioSettingsDialog() {
}

void AudioSettingsDialog::setCurrentConfiguration(const AudioConfiguration& config) {
    temp_config_ = config;
    original_config_ = config;
    
    // Update UI state to match configuration
    selected_buffer_size_index_ = findBufferSizeIndex(config.buffer_size);
    selected_sample_rate_index_ = findSampleRateIndex(config.sample_rate);
    
    devices_need_refresh_ = true;
}

void AudioSettingsDialog::refreshDeviceList() {
    if (!devices_need_refresh_) return;
    
    input_devices_ = AudioDeviceManager::getInputDevices();
    output_devices_ = AudioDeviceManager::getOutputDevices();
    
    // Find current device indices
    selected_input_device_index_ = findDeviceIndex(input_devices_, temp_config_.input_device_id);
    selected_output_device_index_ = findDeviceIndex(output_devices_, temp_config_.output_device_id);
    
    // Add "Default Device" options at the beginning
    AudioDeviceInfo default_input;
    default_input.device_id = -1;
    default_input.name = "Default Input Device";
    default_input.max_input_channels = 1;
    input_devices_.insert(input_devices_.begin(), default_input);
    
    AudioDeviceInfo default_output;
    default_output.device_id = -1;
    default_output.name = "Default Output Device";
    default_output.max_output_channels = 1;
    output_devices_.insert(output_devices_.begin(), default_output);
    
    // Adjust indices for the inserted default devices
    if (selected_input_device_index_ >= 0) selected_input_device_index_++;
    else selected_input_device_index_ = 0; // Select default device
    
    if (selected_output_device_index_ >= 0) selected_output_device_index_++;
    else selected_output_device_index_ = 0; // Select default device
    
    devices_need_refresh_ = false;
}

void AudioSettingsDialog::render() {
    if (!is_open_) return;
    
    // Center the dialog
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_Appearing);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::Begin("Audio Settings", &is_open_, window_flags)) {
        refreshDeviceList();
        
        // Calculate space for buttons at bottom
        float button_height = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2;
        float available_height = ImGui::GetContentRegionAvail().y - button_height;
        
        // Scrollable content area
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, available_height), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            renderDeviceSelection();
            ImGui::Separator();
            
            renderBufferConfiguration();
            ImGui::Separator();
            
            renderChannelMapping();
            ImGui::Separator();
            
            renderErrorMessages();
        }
        ImGui::EndChild();
        
        // Fixed buttons at bottom
        ImGui::Separator();
        renderButtons();
    }
    ImGui::End();
}

void AudioSettingsDialog::renderDeviceSelection() {
    ImGui::Text("Audio Devices");
    ImGui::Spacing();
    
    // Input device selection
    ImGui::Text("Input Device:");
    ImGui::SetNextItemWidth(-1); // Use full width
    
    if (ImGui::BeginCombo("##InputDevice", 
                         selected_input_device_index_ >= 0 && selected_input_device_index_ < static_cast<int>(input_devices_.size())
                         ? input_devices_[selected_input_device_index_].name.c_str() 
                         : "No device selected")) {
        
        for (int i = 0; i < static_cast<int>(input_devices_.size()); i++) {
            const auto& device = input_devices_[i];
            bool is_selected = (selected_input_device_index_ == i);
            
            std::string label = device.name;
            if (device.device_id >= 0) {
                label += " [" + device.host_api_name + "]";
                label += " (" + std::to_string(device.max_input_channels) + " ch)";
            }
            
            if (ImGui::Selectable(label.c_str(), is_selected)) {
                selected_input_device_index_ = i;
                temp_config_.input_device_id = device.device_id;
                error_message_.clear();
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Output device selection
    ImGui::Spacing();
    ImGui::Text("Output Device:");
    ImGui::SetNextItemWidth(-1); // Use full width
    
    if (ImGui::BeginCombo("##OutputDevice", 
                         selected_output_device_index_ >= 0 && selected_output_device_index_ < static_cast<int>(output_devices_.size())
                         ? output_devices_[selected_output_device_index_].name.c_str() 
                         : "No device selected")) {
        
        for (int i = 0; i < static_cast<int>(output_devices_.size()); i++) {
            const auto& device = output_devices_[i];
            bool is_selected = (selected_output_device_index_ == i);
            
            std::string label = device.name;
            if (device.device_id >= 0) {
                label += " [" + device.host_api_name + "]";
                label += " (" + std::to_string(device.max_output_channels) + " ch)";
            }
            
            if (ImGui::Selectable(label.c_str(), is_selected)) {
                selected_output_device_index_ = i;
                temp_config_.output_device_id = device.device_id;
                error_message_.clear();
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Refresh devices button
    ImGui::Spacing();
    if (ImGui::Button("Refresh Devices")) {
        devices_need_refresh_ = true;
        error_message_.clear();
    }
}

void AudioSettingsDialog::renderBufferConfiguration() {
    ImGui::Text("Audio Configuration");
    ImGui::Spacing();
    
    // Buffer size selection
    ImGui::Text("Buffer Size:");
    ImGui::SetNextItemWidth(-1); // Use full width
    
    if (ImGui::BeginCombo("##BufferSize", getBufferSizeLabel(temp_config_.buffer_size))) {
        for (int i = 0; i < static_cast<int>(buffer_sizes_.size()); i++) {
            bool is_selected = (selected_buffer_size_index_ == i);
            
            if (ImGui::Selectable(getBufferSizeLabel(buffer_sizes_[i]), is_selected)) {
                selected_buffer_size_index_ = i;
                temp_config_.buffer_size = buffer_sizes_[i];
                error_message_.clear();
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::TextDisabled("(Lower = less latency, higher CPU usage)");
    
    // Sample rate selection
    ImGui::Spacing();
    ImGui::Text("Sample Rate:");
    ImGui::SetNextItemWidth(-1); // Use full width
    
    if (ImGui::BeginCombo("##SampleRate", getSampleRateLabel(temp_config_.sample_rate))) {
        for (int i = 0; i < static_cast<int>(sample_rates_.size()); i++) {
            bool is_selected = (selected_sample_rate_index_ == i);
            
            if (ImGui::Selectable(getSampleRateLabel(sample_rates_[i]), is_selected)) {
                selected_sample_rate_index_ = i;
                temp_config_.sample_rate = sample_rates_[i];
                error_message_.clear();
            }
            
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Voltage monitoring toggle
    ImGui::Spacing();
    ImGui::Checkbox("Enable CV Voltage Monitoring", &temp_config_.voltage_monitoring_enabled);
    ImGui::TextDisabled("(Disable for better performance)");
}

void AudioSettingsDialog::renderChannelMapping() {
    ImGui::Text("Channel Mapping");
    
    // Get current device info for channel validation
    bool has_input_device = selected_input_device_index_ >= 0 && selected_input_device_index_ < static_cast<int>(input_devices_.size());
    bool has_output_device = selected_output_device_index_ >= 0 && selected_output_device_index_ < static_cast<int>(output_devices_.size());
    
    int max_input_channels = has_input_device ? input_devices_[selected_input_device_index_].max_input_channels : 12;
    int max_output_channels = has_output_device ? output_devices_[selected_output_device_index_].max_output_channels : 6;
    
    // Input channel mapping
    if (ImGui::CollapsingHeader("Input Channel Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Map physical input channels to CV inputs (1-12):");
        ImGui::Spacing();
        
        for (int i = 0; i < 12; i++) {
            ImGui::PushID(i);
            
            // Use full width for labels and combos
            ImGui::Text("CV Input %d:", i + 1);
            ImGui::SameLine(120); // Fixed label width
            ImGui::SetNextItemWidth(-1); // Use remaining width
            
            int& channel = temp_config_.input_channel_mapping[i];
            std::string channel_text = "Channel " + std::to_string(channel + 1);
            
            if (ImGui::BeginCombo("##InputChannel", channel_text.c_str())) {
                for (int ch = 0; ch < max_input_channels; ch++) {
                    bool is_selected = (channel == ch);
                    std::string label = "Channel " + std::to_string(ch + 1);
                    
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        channel = ch;
                    }
                    
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            ImGui::PopID();
        }
    }
    
    // Output channel mapping
    if (ImGui::CollapsingHeader("Output Channel Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Map CV outputs (1-6) to physical output channels:");
        ImGui::Spacing();
        
        for (int i = 0; i < 6; i++) {
            ImGui::PushID(i + 100); // Offset to avoid ID conflicts with inputs
            
            // Use full width for labels and combos
            ImGui::Text("CV Output %d:", i + 1);
            ImGui::SameLine(120); // Fixed label width
            ImGui::SetNextItemWidth(-1); // Use remaining width
            
            int& channel = temp_config_.output_channel_mapping[i];
            std::string channel_text = "Channel " + std::to_string(channel + 1);
            
            if (ImGui::BeginCombo("##OutputChannel", channel_text.c_str())) {
                for (int ch = 0; ch < max_output_channels; ch++) {
                    bool is_selected = (channel == ch);
                    std::string label = "Channel " + std::to_string(ch + 1);
                    
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        channel = ch;
                    }
                    
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            ImGui::PopID();
        }
    }
}

void AudioSettingsDialog::renderErrorMessages() {
    if (!error_message_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255)); // Red text
        ImGui::TextWrapped("Error: %s", error_message_.c_str());
        ImGui::PopStyleColor();
    }
}

void AudioSettingsDialog::renderButtons() {
    ImGui::Separator();
    
    float button_width = 100.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float total_width = button_width * 3 + spacing * 2;
    float window_width = ImGui::GetWindowSize().x;
    float offset = (window_width - total_width) * 0.5f;
    
    ImGui::SetCursorPosX(offset);
    
    if (ImGui::Button("Apply", ImVec2(button_width, 0))) {
        applyConfiguration();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
        cancelConfiguration();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Defaults", ImVec2(button_width, 0))) {
        resetToDefaults();
    }
}

bool AudioSettingsDialog::validateConfiguration() {
    error_message_.clear();
    
    // Validate device configuration using AudioDeviceManager
    if (!AudioDeviceManager::validateDeviceConfiguration(
            temp_config_.input_device_id, 
            temp_config_.output_device_id,
            temp_config_.buffer_size, 
            temp_config_.sample_rate)) {
        error_message_ = AudioDeviceManager::getLastError();
        return false;
    }
    
    return true;
}

void AudioSettingsDialog::applyConfiguration() {
    if (validateConfiguration()) {
        if (on_apply) {
            on_apply(temp_config_);
        }
        hide();
    }
}

void AudioSettingsDialog::cancelConfiguration() {
    temp_config_ = original_config_;
    error_message_.clear();
    
    if (on_cancel) {
        on_cancel();
    }
    hide();
}

void AudioSettingsDialog::resetToDefaults() {
    temp_config_ = AudioConfiguration(); // Use default constructor
    setCurrentConfiguration(temp_config_);
    error_message_.clear();
}

int AudioSettingsDialog::findDeviceIndex(const std::vector<AudioDeviceInfo>& devices, int device_id) {
    for (int i = 0; i < static_cast<int>(devices.size()); i++) {
        if (devices[i].device_id == device_id) {
            return i;
        }
    }
    return -1; // Not found
}

int AudioSettingsDialog::findBufferSizeIndex(int buffer_size) {
    auto it = std::find(buffer_sizes_.begin(), buffer_sizes_.end(), buffer_size);
    if (it != buffer_sizes_.end()) {
        return static_cast<int>(std::distance(buffer_sizes_.begin(), it));
    }
    return 1; // Default to 64 samples
}

int AudioSettingsDialog::findSampleRateIndex(double sample_rate) {
    auto it = std::find_if(sample_rates_.begin(), sample_rates_.end(),
                          [sample_rate](double rate) {
                              return std::abs(rate - sample_rate) < 0.1;
                          });
    if (it != sample_rates_.end()) {
        return static_cast<int>(std::distance(sample_rates_.begin(), it));
    }
    return 2; // Default to 48kHz
}

const char* AudioSettingsDialog::getBufferSizeLabel(int buffer_size) {
    static std::string label;
    label = std::to_string(buffer_size) + " samples";
    return label.c_str();
}

const char* AudioSettingsDialog::getSampleRateLabel(double sample_rate) {
    static std::string label;
    if (sample_rate >= 1000.0) {
        label = std::to_string(static_cast<int>(sample_rate / 1000.0)) + "." + 
                std::to_string(static_cast<int>(sample_rate) % 1000 / 100) + " kHz";
    } else {
        label = std::to_string(static_cast<int>(sample_rate)) + " Hz";
    }
    return label.c_str();
}