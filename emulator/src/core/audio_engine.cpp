#include "audio_engine.h"
#include "api_shim.h"
#include "audio_device_manager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

AudioEngine::AudioEngine() 
    : input_channel_count_(1)
    , output_channel_count_(1)
{
    // Initialize bus states
    input_enabled_.fill(false);
    output_enabled_.fill(false);
    
    // Enable all CV inputs (buses 0-11) and outputs (buses 12-17) by default
    for (int i = 0; i < 12; i++) {
        input_enabled_[i] = true;
    }
    for (int i = 12; i < 18; i++) {
        output_enabled_[i] = true;
    }
    
    // Note: Voltage monitoring will track all buses regardless of enabled state
    // This allows monitoring of plugin-generated signals even on disabled buses
    
    // Initialize voltage monitoring
    input_peak_detectors_.fill(0.0f);
    output_peak_detectors_.fill(0.0f);
    peak_decay_rate_ = 0.992f;
    voltage_update_counter_ = 0;
    
    // Set default configuration
    setDefaultConfiguration();
    
    clearBuses();
}

AudioEngine::~AudioEngine() {
    terminate();
}

bool AudioEngine::initialize() {
    // Use default configuration
    return initialize(current_config_);
}

bool AudioEngine::initialize(const AudioConfiguration& config) {
    if (!AudioDeviceManager::initialize()) {
        last_error_ = "Failed to initialize audio device manager";
        return false;
    }
    
    return configureDevices(config);
}

bool AudioEngine::configureDevices(const AudioConfiguration& config) {
    // Stop current stream if running
    if (stream_) {
        stop();
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
    
    // Validate configuration
    if (!validateDeviceConfiguration(config)) {
        return false;
    }
    
    // Update current configuration
    current_config_ = config;
    
    // Initialize stream with new configuration
    return initializeStream(config);
}

void AudioEngine::setDefaultConfiguration() {
    current_config_ = AudioConfiguration(); // Use default constructor
    current_config_.input_device_id = -1;   // Default input device
    current_config_.output_device_id = -1;  // Default output device
}

bool AudioEngine::validateDeviceConfiguration(const AudioConfiguration& config) {
    if (!AudioDeviceManager::validateDeviceConfiguration(
            config.input_device_id, config.output_device_id,
            config.buffer_size, config.sample_rate)) {
        last_error_ = AudioDeviceManager::getLastError();
        return false;
    }
    return true;
}

bool AudioEngine::initializeStream(const AudioConfiguration& config) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        last_error_ = std::string("PortAudio initialization failed: ") + Pa_GetErrorText(err);
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    
    // Determine actual device IDs
    int input_device = config.input_device_id >= 0 ? config.input_device_id : Pa_GetDefaultInputDevice();
    int output_device = config.output_device_id >= 0 ? config.output_device_id : Pa_GetDefaultOutputDevice();
    
    // Input parameters
    inputParameters.device = input_device;
    if (inputParameters.device == paNoDevice) {
        last_error_ = "No input device available";
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(inputParameters.device);
    if (!inputInfo) {
        last_error_ = "Failed to get input device info";
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    // Configure multi-channel input
    input_channel_count_ = std::min(inputInfo->maxInputChannels, 12); // Limit to 12 channels
    inputParameters.channelCount = input_channel_count_;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = inputInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    
    // Output parameters
    outputParameters.device = output_device;
    if (outputParameters.device == paNoDevice) {
        last_error_ = "No output device available";
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    const PaDeviceInfo* outputInfo = Pa_GetDeviceInfo(outputParameters.device);
    if (!outputInfo) {
        last_error_ = "Failed to get output device info";
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    // Configure multi-channel output
    output_channel_count_ = std::min(outputInfo->maxOutputChannels, 6); // Limit to 6 channels
    outputParameters.channelCount = output_channel_count_;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = outputInfo->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    
    // Resize multi-channel buffers
    multi_channel_input_buffer_.resize(config.buffer_size * input_channel_count_);
    multi_channel_output_buffer_.resize(config.buffer_size * output_channel_count_);
    
    // Open stream
    err = Pa_OpenStream(&stream_,
                        &inputParameters,
                        &outputParameters,
                        config.sample_rate,
                        config.buffer_size,
                        paClipOff,
                        audioCallback,
                        this);
    
    if (err != paNoError) {
        last_error_ = std::string("Failed to open audio stream: ") + Pa_GetErrorText(err);
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    std::cout << "Audio engine configured successfully" << std::endl;
    std::cout << "Input device: " << inputInfo->name << " (" << input_channel_count_ << " channels)" << std::endl;
    std::cout << "Output device: " << outputInfo->name << " (" << output_channel_count_ << " channels)" << std::endl;
    std::cout << "Sample rate: " << config.sample_rate << " Hz" << std::endl;
    std::cout << "Buffer size: " << config.buffer_size << " frames" << std::endl;
    
    return true;
}

std::string AudioEngine::getDeviceStatusString() const {
    if (!stream_) {
        return "No audio stream";
    }
    
    std::string status = "Running: ";
    status += std::to_string(input_channel_count_) + " in, ";
    status += std::to_string(output_channel_count_) + " out, ";
    status += std::to_string(static_cast<int>(current_config_.sample_rate)) + " Hz, ";
    status += std::to_string(current_config_.buffer_size) + " samples";
    
    return status;
}

void AudioEngine::terminate() {
    stop();
    
    if (stream_) {
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
    
    Pa_Terminate();
}

bool AudioEngine::start() {
    if (!stream_) {
        std::cerr << "Audio stream not initialized" << std::endl;
        return false;
    }
    
    PaError err = Pa_StartStream(stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start audio stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    std::cout << "Audio stream started" << std::endl;
    return true;
}

void AudioEngine::stop() {
    if (stream_ && !Pa_IsStreamStopped(stream_)) {
        Pa_StopStream(stream_);
        std::cout << "Audio stream stopped" << std::endl;
    }
}

float AudioEngine::getCpuLoad() const {
    if (!stream_) return 0.0f;
    return Pa_GetStreamCpuLoad(stream_);
}

double AudioEngine::getStreamTime() const {
    if (!stream_) return 0.0;
    return Pa_GetStreamTime(stream_);
}

int AudioEngine::audioCallback(const void* inputBuffer, void* outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void* userData) {
    (void)timeInfo;
    (void)statusFlags;
    
    auto* engine = static_cast<AudioEngine*>(userData);
    auto* input = static_cast<const float*>(inputBuffer);
    auto* output = static_cast<float*>(outputBuffer);
    
    engine->processAudio(input, output, framesPerBuffer);
    
    return paContinue;
}

void AudioEngine::processAudio(const float* input, float* output, unsigned long frames) {
    // Clear output buffer
    std::fill(output, output + frames * output_channel_count_, 0.0f);
    
    if (!algorithm_) {
        // No plugin loaded - just pass through silence
        return;
    }
    
    // Process audio in blocks of SAMPLES_PER_BLOCK
    for (unsigned long frame = 0; frame < frames; frame += SAMPLES_PER_BLOCK) {
        unsigned long samples_to_process = std::min(static_cast<unsigned long>(SAMPLES_PER_BLOCK), frames - frame);
        
        // Clear all buses
        clearBuses();
        
        // Map multi-channel input to CV input buses
        if (input) {
            const float* frame_input = input + (frame * input_channel_count_);
            mapMultiChannelInput(frame_input, samples_to_process);
        }
        
        // Create flat bus buffer for the plugin - buses arranged consecutively
        std::array<std::array<float, SAMPLES_PER_BLOCK>, NUM_BUSES> plugin_buses = audio_buses_;
        std::array<float, NUM_BUSES * SAMPLES_PER_BLOCK> flat_bus_buffer;
        
        // Copy bus data to flat buffer and zero disabled inputs
        for (int i = 0; i < NUM_BUSES; i++) {
            // For input buses (0-11), only pass enabled ones to plugin
            if (i < 12 && !input_enabled_[i]) {
                plugin_buses[i].fill(0.0f);
            }
            std::copy(plugin_buses[i].begin(), plugin_buses[i].end(), 
                     flat_bus_buffer.begin() + i * SAMPLES_PER_BLOCK);
        }
        
        // Process through plugin
        try {
            if (factory_ && factory_->step) {
                factory_->step(algorithm_, flat_bus_buffer.data(), samples_to_process);
                
                // Copy processed data back from flat buffer to plugin_buses
                for (int i = 0; i < NUM_BUSES; i++) {
                    std::copy(flat_bus_buffer.begin() + i * SAMPLES_PER_BLOCK,
                             flat_bus_buffer.begin() + (i + 1) * SAMPLES_PER_BLOCK,
                             plugin_buses[i].begin());
                }
            }
        } catch (...) {
            std::cerr << "Plugin processing error" << std::endl;
            // Clear plugin buses on error
            for (auto& bus : plugin_buses) {
                bus.fill(0.0f);
            }
        }
        
        // Copy plugin output buses back to main audio buses for output mapping
        for (int i = 12; i < NUM_BUSES; i++) {
            audio_buses_[i] = plugin_buses[i];
        }
        
        // Map CV output buses to multi-channel output
        if (output) {
            float* frame_output = output + (frame * output_channel_count_);
            mapMultiChannelOutput(frame_output, samples_to_process);
        }
    }
    
    // Update voltage monitoring after processing all frames
    updateVoltageMonitoring(frames);
}

void AudioEngine::clearBuses() {
    for (auto& bus : audio_buses_) {
        bus.fill(0.0f);
    }
}

void AudioEngine::copyInputToBuses(const float* input, unsigned long frames) {
    if (!input) return;
    
    for (int bus = 0; bus < NUM_BUSES; bus++) {
        if (input_enabled_[bus]) {
            for (unsigned long i = 0; i < frames && i < SAMPLES_PER_BLOCK; i++) {
                audio_buses_[bus][i] = input[i];
            }
        }
    }
}

void AudioEngine::copyBusesToOutput(float* output, unsigned long frames) {
    if (!output) return;
    
    // Mix all enabled output buses
    for (unsigned long i = 0; i < frames; i++) {
        float sample = 0.0f;
        int active_buses = 0;
        
        for (int bus = 0; bus < NUM_BUSES; bus++) {
            if (output_enabled_[bus] && i < SAMPLES_PER_BLOCK) {
                sample += audio_buses_[bus][i];
                active_buses++;
            }
        }
        
        // Prevent clipping if multiple buses are active
        if (active_buses > 1) {
            sample /= active_buses;
        }
        
        output[i] = std::clamp(sample, -1.0f, 1.0f);
    }
}

// Voltage monitoring implementation
void AudioEngine::updateVoltageMonitoring(unsigned long frames) {
    (void)frames; // Unused parameter
    // Only update voltage monitoring every N buffers to avoid UI thread pressure
    voltage_update_counter_++;
    if (voltage_update_counter_ < VOLTAGE_UPDATE_INTERVAL) return;
    voltage_update_counter_ = 0;
    
    auto& voltage_state = ApiShim::getState().voltage;
    if (!voltage_state.monitoring_enabled.load()) {
        return;
    }
    
    // Update input voltage monitoring for all input buses
    // Map the first 12 buses to the 12 CV inputs (monitor regardless of enabled state)
    for (int i = 0; i < 12; i++) {
        if (i < NUM_BUSES) {
            float rms = calculateRMS(audio_buses_[i].data(), SAMPLES_PER_BLOCK);
            updatePeakDetector(input_peak_detectors_[i], rms);
            voltage_state.input_voltages[i].store(input_peak_detectors_[i] * 10.0f); // Scale to ±10V
        } else {
            input_peak_detectors_[i] *= peak_decay_rate_; // Decay unused channels
            voltage_state.input_voltages[i].store(input_peak_detectors_[i] * 10.0f);
        }
    }
    
    // Update output voltage monitoring for all output buses
    // Map buses 12-17 to the 6 CV outputs (monitor regardless of enabled state)
    for (int i = 0; i < 6; i++) {
        int bus_index = 12 + i; // Use buses 12-17 for outputs
        if (bus_index < NUM_BUSES) {
            float rms = calculateRMS(audio_buses_[bus_index].data(), SAMPLES_PER_BLOCK);
            updatePeakDetector(output_peak_detectors_[i], rms);
            voltage_state.output_voltages[i].store(output_peak_detectors_[i] * 10.0f); // Scale to ±10V
        } else {
            output_peak_detectors_[i] *= peak_decay_rate_; // Decay unused channels
            voltage_state.output_voltages[i].store(output_peak_detectors_[i] * 10.0f);
        }
    }
}

float AudioEngine::calculateRMS(const float* buffer, unsigned long frameCount) {
    if (!buffer || frameCount == 0) return 0.0f;
    
    float sum = 0.0f;
    for (unsigned long i = 0; i < frameCount; i++) {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / frameCount);
}

float AudioEngine::calculatePeak(const float* buffer, unsigned long frameCount) {
    if (!buffer || frameCount == 0) return 0.0f;
    
    float peak = 0.0f;
    for (unsigned long i = 0; i < frameCount; i++) {
        peak = std::max(peak, std::abs(buffer[i]));
    }
    return peak;
}

void AudioEngine::updatePeakDetector(float& detector, float new_value) {
    // Peak detection with exponential decay
    const float attack_rate = 0.99f;   // Fast attack
    const float decay_rate = 0.992f;   // Slow decay for readable display
    
    if (new_value > detector) {
        detector = detector * attack_rate + new_value * (1.0f - attack_rate);
    } else {
        detector *= decay_rate;
    }
    
    // Prevent underflow
    if (detector < 0.001f) detector = 0.0f;
}

// Multi-channel audio processing
void AudioEngine::mapMultiChannelInput(const float* input, unsigned long frames) {
    if (!input || frames == 0) return;
    
    // Clear all input buses first
    for (int bus = 0; bus < 12; bus++) {
        if (bus < NUM_BUSES) {
            audio_buses_[bus].fill(0.0f);
        }
    }
    
    // Map physical input channels to CV input buses using configuration
    // Always map all inputs for voltage monitoring, regardless of enabled state
    for (int cv_input = 0; cv_input < 12; cv_input++) {
        int physical_channel = current_config_.input_channel_mapping[cv_input];
        if (physical_channel >= 0 && physical_channel < input_channel_count_) {
            // Copy from interleaved multi-channel buffer to CV bus
            for (unsigned long frame = 0; frame < frames && frame < SAMPLES_PER_BLOCK; frame++) {
                unsigned long input_index = frame * input_channel_count_ + physical_channel;
                if (input_index < frames * input_channel_count_) {
                    audio_buses_[cv_input][frame] = input[input_index];
                }
            }
        }
    }
}

void AudioEngine::mapMultiChannelOutput(float* output, unsigned long frames) {
    if (!output || frames == 0) return;
    
    // Clear multi-channel output buffer
    std::fill(multi_channel_output_buffer_.begin(), multi_channel_output_buffer_.end(), 0.0f);
    
    // Map CV output buses to physical output channels using configuration
    for (int cv_output = 0; cv_output < 6; cv_output++) {
        int bus_index = 12 + cv_output; // CV outputs are on buses 12-17
        if (bus_index >= NUM_BUSES || !output_enabled_[bus_index]) continue;
        
        int physical_channel = current_config_.output_channel_mapping[cv_output];
        if (physical_channel >= 0 && physical_channel < output_channel_count_) {
            // Copy from CV bus to interleaved multi-channel buffer
            for (unsigned long frame = 0; frame < frames && frame < SAMPLES_PER_BLOCK; frame++) {
                unsigned long output_index = frame * output_channel_count_ + physical_channel;
                if (output_index < multi_channel_output_buffer_.size()) {
                    output[output_index] = audio_buses_[bus_index][frame];
                }
            }
        }
    }
}