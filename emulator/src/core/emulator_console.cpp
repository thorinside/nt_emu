#include "emulator_console.h"
#include <iostream>

EmulatorConsole::EmulatorConsole() {
    plugin_loader_ = std::make_unique<PluginLoader>();
    audio_engine_ = std::make_unique<AudioEngine>();
}

EmulatorConsole::~EmulatorConsole() {
    shutdown();
}

bool EmulatorConsole::initialize() {
    if (initialized_) return true;
    
    // Initialize API shim
    ApiShim::initialize();
    
    // Initialize audio engine
    if (!audio_engine_->initialize()) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "Emulator initialized successfully" << std::endl;
    return true;
}

void EmulatorConsole::shutdown() {
    if (!initialized_) return;
    
    stopAudio();
    unloadPlugin();
    
    if (audio_engine_) {
        audio_engine_->terminate();
    }
    
    initialized_ = false;
    std::cout << "Emulator shutdown complete" << std::endl;
}

bool EmulatorConsole::loadPlugin(const std::string& path) {
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
    auto* factory = plugin_loader_->getFactory();
    if (algorithm) {
        ApiShim::setAlgorithm(algorithm);
        audio_engine_->setAlgorithm(algorithm);
        audio_engine_->setFactory(factory);
        
        std::cout << "Plugin loaded: " << path << std::endl;
        
        // Restart audio if it was running
        if (was_running) {
            startAudio();
        }
        
        return true;
    }
    
    return false;
}

void EmulatorConsole::unloadPlugin() {
    if (plugin_loader_) {
        stopAudio();
        
        audio_engine_->setAlgorithm(nullptr);
        audio_engine_->setFactory(nullptr);
        ApiShim::setAlgorithm(nullptr);
        
        plugin_loader_->unloadPlugin();
        
        std::cout << "Plugin unloaded" << std::endl;
    }
}

bool EmulatorConsole::isPluginLoaded() const {
    return plugin_loader_ && plugin_loader_->isLoaded();
}

bool EmulatorConsole::startAudio() {
    if (!audio_engine_) return false;
    
    if (audio_engine_->start()) {
        std::cout << "Audio started" << std::endl;
        return true;
    }
    
    return false;
}

void EmulatorConsole::stopAudio() {
    if (audio_engine_) {
        audio_engine_->stop();
        std::cout << "Audio stopped" << std::endl;
    }
}

bool EmulatorConsole::isAudioRunning() const {
    return audio_engine_ && audio_engine_->isRunning();
}

void EmulatorConsole::update() {
    if (!initialized_) return;
    
    // Update display from plugin
    updateDisplay();
    
    // Check for hot reload
    checkForReload();
}

void EmulatorConsole::checkForReload() {
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

std::string EmulatorConsole::getPluginPath() const {
    if (plugin_loader_) {
        return plugin_loader_->getPath();
    }
    return "";
}

float EmulatorConsole::getAudioCpuLoad() const {
    if (audio_engine_) {
        return audio_engine_->getCpuLoad();
    }
    return 0.0f;
}

void EmulatorConsole::setPotValue(int pot, float value) {
    if (pot >= 0 && pot < 3) {
        ApiShim::getState().hardware.pots[pot] = value;
        onParameterChange(pot, value);
    }
}

void EmulatorConsole::setButtonState(int button, bool pressed) {
    if (button >= 0 && button < 4) {
        ApiShim::getState().hardware.buttons[button] = pressed;
    }
}

void EmulatorConsole::setEncoderValue(int encoder, int value) {
    if (encoder >= 0 && encoder < 2) {
        ApiShim::getState().hardware.encoder_values[encoder] = value;
    }
}

void EmulatorConsole::updateDisplay() {
    if (!plugin_loader_ || !plugin_loader_->isLoaded()) {
        return;
    }
    
    auto* algorithm = plugin_loader_->getAlgorithm();
    if (!algorithm || !algorithm->draw) {
        return;
    }
    
    // Clear display buffer
    ApiShim::getState().display.clear();
    
    // Let plugin draw to the display
    try {
        algorithm->draw(algorithm);
        
        // In console mode, we could output display state as text
        // For now, just silently update the buffer
    } catch (...) {
        std::cerr << "Plugin draw error" << std::endl;
    }
}

void EmulatorConsole::onParameterChange(int parameter, float value) {
    if (!plugin_loader_ || !plugin_loader_->isLoaded()) {
        return;
    }
    
    auto* algorithm = plugin_loader_->getAlgorithm();
    if (!algorithm || !algorithm->setParameterValue) {
        return;
    }
    
    // Set parameter value in plugin
    _NT_parameterValue param_value;
    param_value.asFloat = value;
    
    try {
        algorithm->setParameterValue(algorithm, parameter, param_value);
        std::cout << "Parameter " << parameter << " set to " << value << std::endl;
    } catch (...) {
        std::cerr << "Error setting parameter " << parameter << std::endl;
    }
}