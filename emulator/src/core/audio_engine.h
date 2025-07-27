#pragma once

#include <portaudio.h>
#include <distingnt/api.h>
#include <array>
#include <functional>
#include <memory>
#include <cmath>
#include <vector>
#include <string>
#include "../utils/config.h"

class AudioEngine {
public:
    static constexpr int SAMPLES_PER_BLOCK = 4;   // disting NT processes 4 samples at a time
    static constexpr int NUM_BUSES = 28;
    
    AudioEngine();
    ~AudioEngine();
    
    bool initialize();
    bool initialize(const AudioConfiguration& config);
    void terminate();
    
    bool start();
    void stop();
    
    bool isRunning() const { return stream_ != nullptr && !Pa_IsStreamStopped(stream_); }
    
    void setAlgorithm(_NT_algorithm* algorithm) { algorithm_ = algorithm; }
    void setFactory(_NT_factory* factory) { factory_ = factory; }
    _NT_algorithm* getAlgorithm() const { return algorithm_; }
    _NT_factory* getFactory() const { return factory_; }
    
    // Device configuration
    bool configureDevices(const AudioConfiguration& config);
    AudioConfiguration getCurrentConfiguration() const { return current_config_; }
    std::string getDeviceStatusString() const;
    
    // Audio routing
    void setInputBus(int bus, bool enabled) { 
        if (bus >= 0 && bus < NUM_BUSES) input_enabled_[bus] = enabled; 
    }
    void setOutputBus(int bus, bool enabled) { 
        if (bus >= 0 && bus < NUM_BUSES) output_enabled_[bus] = enabled; 
    }
    
    // Monitoring
    float getCpuLoad() const;
    double getStreamTime() const;
    
    std::string getLastError() const { return last_error_; }
    
private:
    PaStream* stream_ = nullptr;
    _NT_algorithm* algorithm_ = nullptr;
    _NT_factory* factory_ = nullptr;
    
    // Configuration state
    AudioConfiguration current_config_;
    std::string last_error_;
    
    // Audio buffers - 28 buses, each with enough samples for processing
    std::array<std::array<float, SAMPLES_PER_BLOCK>, NUM_BUSES> audio_buses_;
    std::array<bool, NUM_BUSES> input_enabled_;
    std::array<bool, NUM_BUSES> output_enabled_;
    
    // Multi-channel audio buffers
    std::vector<float> multi_channel_input_buffer_;
    std::vector<float> multi_channel_output_buffer_;
    int input_channel_count_;
    int output_channel_count_;
    
    // Voltage monitoring state
    std::array<float, 12> input_peak_detectors_;     // Peak hold values for 12 CV inputs
    std::array<float, 6> output_peak_detectors_;     // Peak hold values for 6 CV outputs
    float peak_decay_rate_;                          // How fast peaks decay
    int voltage_update_counter_;                     // Update voltage state every N buffers
    static constexpr int VOLTAGE_UPDATE_INTERVAL = 16; // Update every 16 buffers (~21ms at 64 samples)
    
    static int audioCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData);
    
    void processAudio(const float* input, float* output, unsigned long frames);
    void clearBuses();
    void copyInputToBuses(const float* input, unsigned long frames);
    void copyBusesToOutput(float* output, unsigned long frames);
    
    // Multi-channel audio processing
    void mapMultiChannelInput(const float* input, unsigned long frames);
    void mapMultiChannelOutput(float* output, unsigned long frames);
    
    // Device management
    bool initializeStream(const AudioConfiguration& config);
    bool validateDeviceConfiguration(const AudioConfiguration& config);
    void setDefaultConfiguration();
    
    // Voltage monitoring methods
    void updateVoltageMonitoring(unsigned long frames);
    float calculateRMS(const float* buffer, unsigned long frameCount);
    float calculatePeak(const float* buffer, unsigned long frameCount);
    void updatePeakDetector(float& detector, float new_value);
};