#pragma once

#include <portaudio.h>
#include <vector>
#include <string>

struct AudioDeviceInfo {
    int device_id;
    std::string name;
    std::string host_api_name;
    int max_input_channels;
    int max_output_channels;
    double default_sample_rate;
    bool is_default_input;
    bool is_default_output;
    
    AudioDeviceInfo() : device_id(-1), max_input_channels(0), max_output_channels(0), 
                       default_sample_rate(48000.0), is_default_input(false), is_default_output(false) {}
};

class AudioDeviceManager {
public:
    static bool initialize();
    static void terminate();
    
    static std::vector<AudioDeviceInfo> getInputDevices();
    static std::vector<AudioDeviceInfo> getOutputDevices();
    static std::vector<AudioDeviceInfo> getAllDevices();
    
    static AudioDeviceInfo getDeviceInfo(int device_id);
    static bool isValidDevice(int device_id);
    
    static bool validateDeviceConfiguration(int input_device, int output_device, 
                                          int buffer_size, double sample_rate);
    
    static int getDefaultInputDevice();
    static int getDefaultOutputDevice();
    
    static std::string getLastError() { return last_error_; }
    
private:
    static std::vector<AudioDeviceInfo> cached_devices_;
    static bool devices_enumerated_;
    static bool portaudio_initialized_;
    static std::string last_error_;
    
    static void enumerateDevices();
    static std::string getHostApiName(PaHostApiIndex host_api);
    static bool testDeviceConfiguration(int device_id, bool is_input, 
                                      int channels, double sample_rate, int buffer_size);
};