#include "emulator.h"
#include <iostream>

Emulator::Emulator() {
    plugin_loader_ = std::make_unique<PluginLoader>();
    audio_engine_ = std::make_unique<AudioEngine>();
    display_ = std::make_unique<Display>();
    hardware_interface_ = std::make_shared<HardwareInterface>();
    config_ = std::make_unique<Config>();
}

Emulator::~Emulator() {
    shutdown();
}

bool Emulator::initialize() {
    if (initialized_) return true;
    
    // Initialize API shim
    ApiShim::initialize();
    
    // Load configuration
    if (!config_->load()) {
        std::cout << "Using default configuration" << std::endl;
    }
    
    // Initialize audio engine with configuration
    if (!audio_engine_->initialize(config_->getAudioConfig())) {
        std::cerr << "Failed to initialize audio engine: " << audio_engine_->getLastError() << std::endl;
        
        // Try fallback to defaults
        std::cout << "Attempting fallback to default audio configuration..." << std::endl;
        AudioConfiguration default_config;
        if (!audio_engine_->initialize(default_config)) {
            std::cerr << "Failed to initialize audio engine with default configuration" << std::endl;
            return false;
        }
    }
    
    // Setup callbacks
    setupCallbacks();
    
    initialized_ = true;
    std::cout << "Emulator initialized successfully" << std::endl;
    
    // Auto-start audio for immediate voltage monitoring
    if (startAudio()) {
        std::cout << "Audio auto-started for voltage monitoring" << std::endl;
    } else {
        std::cout << "Warning: Could not auto-start audio" << std::endl;
    }
    
    return true;
}

void Emulator::shutdown() {
    if (!initialized_) return;
    
    stopAudio();
    unloadPlugin();
    
    // Save configuration before shutdown
    if (config_ && audio_engine_) {
        config_->setAudioConfig(audio_engine_->getCurrentConfiguration());
        if (!config_->save()) {
            std::cerr << "Failed to save configuration: " << config_->getLastError() << std::endl;
        }
    }
    
    if (audio_engine_) {
        audio_engine_->terminate();
    }
    
    initialized_ = false;
    std::cout << "Emulator shutdown complete" << std::endl;
}

bool Emulator::loadPlugin(const std::string& path) {
    if (!initialized_) {
        std::cerr << "Emulator not initialized" << std::endl;
        return false;
    }
    
    // Stop audio while loading
    bool was_running = isAudioRunning();
    if (was_running) {
        stopAudio();
    }
    
    // Load the plugin
    if (!plugin_loader_->loadPlugin(path)) {
        return false;
    }
    
    // Set up the plugin with our systems
    auto* algorithm = plugin_loader_->getAlgorithm();
    if (algorithm) {
        ApiShim::setAlgorithm(algorithm);
        audio_engine_->setAlgorithm(algorithm);
        
        std::cout << "Plugin loaded: " << path << std::endl;
        
        // Restart audio if it was running
        if (was_running) {
            startAudio();
        }
        
        return true;
    }
    
    return false;
}

void Emulator::unloadPlugin() {
    if (plugin_loader_) {
        stopAudio();
        
        audio_engine_->setAlgorithm(nullptr);
        ApiShim::setAlgorithm(nullptr);
        
        plugin_loader_->unloadPlugin();
        
        // Clear display
        display_->clear();
        
        std::cout << "Plugin unloaded" << std::endl;
    }
}

bool Emulator::isPluginLoaded() const {
    return plugin_loader_ && plugin_loader_->isLoaded();
}

bool Emulator::startAudio() {
    if (!audio_engine_) return false;
    
    if (audio_engine_->start()) {
        std::cout << "Audio started" << std::endl;
        return true;
    }
    
    return false;
}

void Emulator::stopAudio() {
    if (audio_engine_) {
        audio_engine_->stop();
        std::cout << "Audio stopped" << std::endl;
    }
}

bool Emulator::isAudioRunning() const {
    return audio_engine_ && audio_engine_->isRunning();
}

void Emulator::update() {
    if (!initialized_) return;
    
    // Update hardware interface
    hardware_interface_->update();
    
    // Update display from plugin
    updateDisplayInternal();
    
    // Check for hot reload
    checkForReload();
}

void Emulator::render() {
    if (!initialized_) return;
    
    // Render all UI components
    hardware_interface_->render();
    display_->render();
}

void Emulator::checkForReload() {
    if (plugin_loader_ && plugin_loader_->needsReload()) {
        std::cout << "Plugin file changed, reloading..." << std::endl;
        
        std::string path = plugin_loader_->getPath();
        bool was_running = isAudioRunning();
        
        unloadPlugin();
        
        if (loadPlugin(path)) {
            std::cout << "Plugin reloaded successfully" << std::endl;
            if (was_running) {
                startAudio();
            }
        } else {
            std::cerr << "Failed to reload plugin" << std::endl;
        }
    }
}

std::string Emulator::getPluginPath() const {
    if (plugin_loader_) {
        return plugin_loader_->getPath();
    }
    return "";
}

float Emulator::getAudioCpuLoad() const {
    if (audio_engine_) {
        return audio_engine_->getCpuLoad();
    }
    return 0.0f;
}

std::shared_ptr<HardwareInterface> Emulator::getHardwareInterface() const {
    return hardware_interface_;
}

AudioEngine* Emulator::getAudioEngine() const {
    return audio_engine_.get();
}

Config* Emulator::getConfig() const {
    return config_.get();
}

void Emulator::setupCallbacks() {
    if (!hardware_interface_) return;
    
    // Set up parameter change callback
    hardware_interface_->setParameterChangeCallback(
        [this](int parameter, float value) {
            onParameterChange(parameter, value);
        }
    );
    
    // Set up button callback
    hardware_interface_->setButtonCallback(
        [this](int button, bool pressed) {
            onButtonPress(button, pressed);
        }
    );
    
    // Set up encoder callback
    hardware_interface_->setEncoderCallback(
        [this](int delta) {
            onEncoderChange(delta);
        }
    );
}

void Emulator::updateDisplay() {
    updateDisplayInternal();
}

void Emulator::updateDisplayInternal() {
    if (!display_ || !plugin_loader_ || !plugin_loader_->isLoaded()) {
        return;
    }
    
    auto* algorithm = plugin_loader_->getAlgorithm();
    auto* factory = plugin_loader_->getFactory();
    if (!algorithm || !factory || !factory->draw) {
        return;
    }
    
    // Clear display buffer
    ApiShim::getState().display.clear();
    
    // Let plugin draw to the display
    try {
        factory->draw(algorithm);
        
        // Update our display from API state
        display_->updateFromApiState();
    } catch (...) {
        std::cerr << "Plugin draw error" << std::endl;
    }
}

void Emulator::onParameterChange(int parameter, float value) {
    if (!plugin_loader_ || !plugin_loader_->isLoaded()) {
        return;
    }
    
    auto* algorithm = plugin_loader_->getAlgorithm();
    if (!algorithm) {
        return;
    }
    
    // Set parameter value using API function
    try {
        int16_t int_value = (int16_t)value;
        NT_setParameterFromUi(NT_algorithmIndex(algorithm), parameter + NT_parameterOffset(), int_value);
        std::cout << "Parameter " << parameter << " set to " << value << std::endl;
    } catch (...) {
        std::cerr << "Error setting parameter " << parameter << std::endl;
    }
}

void Emulator::onButtonPress(int button, bool pressed) {
    (void)button;
    (void)pressed;
    
    // For now, just log button presses
    // In a real implementation, this might trigger plugin events
    std::cout << "Button " << button << " " << (pressed ? "pressed" : "released") << std::endl;
}

void Emulator::onEncoderChange(int delta) {
    (void)delta;
    
    // For now, just log encoder changes
    // In a real implementation, this might navigate parameters or menus
    std::cout << "Encoder changed by " << delta << std::endl;
}