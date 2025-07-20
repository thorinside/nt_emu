#pragma once

#include "plugin_loader.h"
#include "audio_engine.h"
#include "api_shim.h"
#include "../hardware/display.h"
#include "../hardware/hardware_interface.h"
#include "../utils/config.h"
#include <memory>
#include <string>

class Emulator {
public:
    Emulator();
    ~Emulator();
    
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
    void render();
    
    // Plugin display update (public for GUI access)
    void updateDisplay();
    
    // Hot reload
    void checkForReload();
    
    // Status information
    std::string getPluginPath() const;
    float getAudioCpuLoad() const;
    
    // Component access
    std::shared_ptr<HardwareInterface> getHardwareInterface() const;
    AudioEngine* getAudioEngine() const;
    Config* getConfig() const;
    
    // Hardware event handlers (public for GUI access)
    void onParameterChange(int parameter, float value);
    void onButtonPress(int button, bool pressed);
    void onEncoderChange(int delta);
    
private:
    std::unique_ptr<PluginLoader> plugin_loader_;
    std::unique_ptr<AudioEngine> audio_engine_;
    std::unique_ptr<Display> display_;
    std::shared_ptr<HardwareInterface> hardware_interface_;
    std::unique_ptr<Config> config_;
    
    bool initialized_ = false;
    
    void setupCallbacks();
    void updateDisplayInternal();
};