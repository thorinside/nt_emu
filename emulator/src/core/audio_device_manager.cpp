#include "audio_device_manager.h"
#include <iostream>
#include <algorithm>

// Static member initialization
std::vector<AudioDeviceInfo> AudioDeviceManager::cached_devices_;
bool AudioDeviceManager::devices_enumerated_ = false;
bool AudioDeviceManager::portaudio_initialized_ = false;
std::string AudioDeviceManager::last_error_;

bool AudioDeviceManager::initialize() {
    if (portaudio_initialized_) {
        return true;
    }
    
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        last_error_ = std::string("PortAudio initialization failed: ") + Pa_GetErrorText(err);
        std::cerr << last_error_ << std::endl;
        return false;
    }
    
    portaudio_initialized_ = true;
    enumerateDevices();
    
    std::cout << "AudioDeviceManager initialized successfully" << std::endl;
    std::cout << "Found " << cached_devices_.size() << " audio devices" << std::endl;
    
    return true;
}

void AudioDeviceManager::terminate() {
    if (portaudio_initialized_) {
        Pa_Terminate();
        portaudio_initialized_ = false;
        devices_enumerated_ = false;
        cached_devices_.clear();
    }
}

void AudioDeviceManager::enumerateDevices() {
    if (!portaudio_initialized_) {
        last_error_ = "PortAudio not initialized";
        return;
    }
    
    cached_devices_.clear();
    
    int device_count = Pa_GetDeviceCount();
    if (device_count < 0) {
        last_error_ = std::string("Failed to get device count: ") + Pa_GetErrorText(device_count);
        std::cerr << last_error_ << std::endl;
        return;
    }
    
    int default_input = Pa_GetDefaultInputDevice();
    int default_output = Pa_GetDefaultOutputDevice();
    
    std::cout << "Enumerating " << device_count << " audio devices:" << std::endl;
    
    for (int i = 0; i < device_count; i++) {
        const PaDeviceInfo* pa_info = Pa_GetDeviceInfo(i);
        if (!pa_info) {
            std::cerr << "Failed to get info for device " << i << std::endl;
            continue;
        }
        
        AudioDeviceInfo device_info;
        device_info.device_id = i;
        device_info.name = pa_info->name ? pa_info->name : "Unknown Device";
        device_info.host_api_name = getHostApiName(pa_info->hostApi);
        device_info.max_input_channels = pa_info->maxInputChannels;
        device_info.max_output_channels = pa_info->maxOutputChannels;
        device_info.default_sample_rate = pa_info->defaultSampleRate;
        device_info.is_default_input = (i == default_input);
        device_info.is_default_output = (i == default_output);
        
        cached_devices_.push_back(device_info);
        
        std::cout << "Device " << i << ": " << device_info.name 
                  << " [" << device_info.host_api_name << "]"
                  << " (in:" << device_info.max_input_channels 
                  << " out:" << device_info.max_output_channels 
                  << " sr:" << device_info.default_sample_rate << ")"
                  << (device_info.is_default_input ? " [DEFAULT_IN]" : "")
                  << (device_info.is_default_output ? " [DEFAULT_OUT]" : "")
                  << std::endl;
    }
    
    devices_enumerated_ = true;
}

std::string AudioDeviceManager::getHostApiName(PaHostApiIndex host_api) {
    const PaHostApiInfo* host_info = Pa_GetHostApiInfo(host_api);
    if (host_info && host_info->name) {
        return std::string(host_info->name);
    }
    return "Unknown Host API";
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getInputDevices() {
    if (!devices_enumerated_) {
        enumerateDevices();
    }
    
    std::vector<AudioDeviceInfo> input_devices;
    for (const auto& device : cached_devices_) {
        if (device.max_input_channels > 0) {
            input_devices.push_back(device);
        }
    }
    
    return input_devices;
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getOutputDevices() {
    if (!devices_enumerated_) {
        enumerateDevices();
    }
    
    std::vector<AudioDeviceInfo> output_devices;
    for (const auto& device : cached_devices_) {
        if (device.max_output_channels > 0) {
            output_devices.push_back(device);
        }
    }
    
    return output_devices;
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getAllDevices() {
    if (!devices_enumerated_) {
        enumerateDevices();
    }
    
    return cached_devices_;
}

AudioDeviceInfo AudioDeviceManager::getDeviceInfo(int device_id) {
    if (!devices_enumerated_) {
        enumerateDevices();
    }
    
    auto it = std::find_if(cached_devices_.begin(), cached_devices_.end(),
                          [device_id](const AudioDeviceInfo& device) {
                              return device.device_id == device_id;
                          });
    
    if (it != cached_devices_.end()) {
        return *it;
    }
    
    // Return empty device info if not found
    return AudioDeviceInfo();
}

bool AudioDeviceManager::isValidDevice(int device_id) {
    if (device_id < 0) return false;
    
    if (!devices_enumerated_) {
        enumerateDevices();
    }
    
    return std::any_of(cached_devices_.begin(), cached_devices_.end(),
                      [device_id](const AudioDeviceInfo& device) {
                          return device.device_id == device_id;
                      });
}

int AudioDeviceManager::getDefaultInputDevice() {
    return Pa_GetDefaultInputDevice();
}

int AudioDeviceManager::getDefaultOutputDevice() {
    return Pa_GetDefaultOutputDevice();
}

bool AudioDeviceManager::validateDeviceConfiguration(int input_device, int output_device, 
                                                    int buffer_size, double sample_rate) {
    if (!portaudio_initialized_) {
        last_error_ = "PortAudio not initialized";
        return false;
    }
    
    // Validate buffer size
    if (buffer_size <= 0 || buffer_size > 8192) {
        last_error_ = "Invalid buffer size: " + std::to_string(buffer_size);
        return false;
    }
    
    // Validate sample rate
    if (sample_rate < 8000.0 || sample_rate > 192000.0) {
        last_error_ = "Invalid sample rate: " + std::to_string(sample_rate);
        return false;
    }
    
    // Validate input device if specified
    if (input_device >= 0) {
        if (!isValidDevice(input_device)) {
            last_error_ = "Invalid input device ID: " + std::to_string(input_device);
            return false;
        }
        
        AudioDeviceInfo input_info = getDeviceInfo(input_device);
        if (input_info.max_input_channels == 0) {
            last_error_ = "Device " + std::to_string(input_device) + " has no input channels";
            return false;
        }
        
        // Test input device configuration
        if (!testDeviceConfiguration(input_device, true, 1, sample_rate, buffer_size)) {
            last_error_ = "Input device configuration test failed";
            return false;
        }
    }
    
    // Validate output device if specified
    if (output_device >= 0) {
        if (!isValidDevice(output_device)) {
            last_error_ = "Invalid output device ID: " + std::to_string(output_device);
            return false;
        }
        
        AudioDeviceInfo output_info = getDeviceInfo(output_device);
        if (output_info.max_output_channels == 0) {
            last_error_ = "Device " + std::to_string(output_device) + " has no output channels";
            return false;
        }
        
        // Test output device configuration
        if (!testDeviceConfiguration(output_device, false, 1, sample_rate, buffer_size)) {
            last_error_ = "Output device configuration test failed";
            return false;
        }
    }
    
    return true;
}

bool AudioDeviceManager::testDeviceConfiguration(int device_id, bool is_input, 
                                                int channels, double sample_rate, int buffer_size) {
    (void)buffer_size; // Unused parameter
    PaStreamParameters params;
    params.device = device_id;
    params.channelCount = channels;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = 0; // Use device default
    params.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_IsFormatSupported(
        is_input ? &params : nullptr,
        is_input ? nullptr : &params,
        sample_rate
    );
    
    if (err != paFormatIsSupported) {
        last_error_ = std::string("Device format not supported: ") + Pa_GetErrorText(err);
        return false;
    }
    
    return true;
}