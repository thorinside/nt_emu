#pragma once

#include <string>
#include <array>
#include <nlohmann/json.hpp>

struct AudioConfiguration {
    int input_device_id = -1;        // -1 = default device
    int output_device_id = -1;       // -1 = default device
    int buffer_size = 64;            // Samples per buffer
    double sample_rate = 48000.0;    // Hz
    std::array<int, 12> input_channel_mapping;   // Physical channel to CV input mapping
    std::array<int, 6> output_channel_mapping;   // CV output to physical channel mapping
    bool voltage_monitoring_enabled = true;
    
    AudioConfiguration() {
        // Initialize channel mappings to 1:1 mapping
        for (int i = 0; i < 12; i++) {
            input_channel_mapping[i] = i;
        }
        for (int i = 0; i < 6; i++) {
            output_channel_mapping[i] = i;
        }
    }
    
    // JSON serialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AudioConfiguration, 
        input_device_id, output_device_id, buffer_size, sample_rate,
        input_channel_mapping, output_channel_mapping, voltage_monitoring_enabled)
};

class Config {
public:
    Config();
    ~Config();
    
    bool load();
    bool save();
    bool load(const std::string& filename);
    bool save(const std::string& filename);
    
    // Audio configuration
    AudioConfiguration& getAudioConfig() { return audio_config_; }
    const AudioConfiguration& getAudioConfig() const { return audio_config_; }
    void setAudioConfig(const AudioConfiguration& config);
    
    // Legacy methods for backward compatibility
    int getSampleRate() const { return static_cast<int>(audio_config_.sample_rate); }
    int getBufferSize() const { return audio_config_.buffer_size; }
    
    std::string getLastError() const { return last_error_; }
    
private:
    AudioConfiguration audio_config_;
    std::string config_file_path_;
    std::string last_error_;
    
    std::string getDefaultConfigPath();
    void setDefaults();
};