#pragma once

#include "plugin_loader.h"
#include "audio_engine.h"
#include "api_shim.h"
#include <memory>
#include <string>

class EmulatorConsole {
public:
    EmulatorConsole();
    ~EmulatorConsole();
    
    bool initialize();
    void shutdown();
    
    // Plugin management
    bool loadPlugin(const std::string& path);
    void unloadPlugin();
    bool isPluginLoaded() const;
    
    // Audio control
    bool startAudio();
    void stopAudio();
    bool isAudioRunning() const;
    
    // Main update loop
    void update();
    
    // Hot reload
    void checkForReload();
    
    // Status information
    std::string getPluginPath() const;
    float getAudioCpuLoad() const;
    
    // Hardware control
    void setPotValue(int pot, float value);
    void setButtonState(int button, bool pressed);
    void setEncoderValue(int encoder, int value);
    
private:
    std::unique_ptr<PluginLoader> plugin_loader_;
    std::unique_ptr<AudioEngine> audio_engine_;
    
    bool initialized_ = false;
    
    void updateDisplay();
    void onParameterChange(int parameter, float value);
};