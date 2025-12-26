#pragma once
#include <rack.hpp>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <cmath>

// Import Disting NT API types
#include <distingnt/api.h>

// Forward declaration for PluginManager
class PluginManager;

using namespace rack;

// Simple MidiHandler implementation for VCV plugin
class MidiHandler {
public:
    void setMidiOutputCallback(std::function<void(const uint8_t*, size_t)> callback) {
        output_callback_ = callback;
    }
    
    void sendMidi3ByteMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
        if (output_callback_) {
            uint8_t msg[3] = {byte0, byte1, byte2};
            output_callback_(msg, 3);
        }
    }
    
    void sendMidi2ByteMessage(uint8_t byte0, uint8_t byte1) {
        if (output_callback_) {
            uint8_t msg[2] = {byte0, byte1};
            output_callback_(msg, 2);
        }
    }
    
    void sendMidiByte(uint8_t byte) {
        if (output_callback_) {
            output_callback_(&byte, 1);
        }
    }

private:
    std::function<void(const uint8_t*, size_t)> output_callback_;
};

// Hardware state adapted for VCV Rack
struct VCVHardwareState {
    std::array<float, 3> pots{0.5f, 0.5f, 0.5f};       // 3 potentiometers [0.0-1.0]
    std::array<bool, 4> buttons{false, false, false, false}; // 4 buttons
    std::array<int, 2> encoder_deltas{0, 0};            // Encoder deltas since last frame
    std::array<bool, 2> encoder_pressed{false, false};  // Encoder press states
    std::array<bool, 3> pot_pressed{false, false, false}; // Pot press states (encoders)
};

// Display buffer for 256x64 OLED
struct VCVDisplayBuffer {
    std::array<uint8_t, (256 * 64) / 2> pixels{};  // 4-bit grayscale, 2 pixels per byte
    bool dirty = true;
    
    void clear() {
        pixels.fill(0);
        dirty = true;
    }
    
    // Legacy method for backwards compatibility (threshold at 8)
    void setPixel(int x, int y, bool on) {
        setPixelGray(x, y, on ? 15 : 0);
    }
    
    // New method for 4-bit grayscale (0-15)
    void setPixelGray(int x, int y, uint8_t gray_value) {
        if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
        
        int byte_idx = y * 128 + x / 2;
        gray_value = gray_value & 0x0F;  // Ensure 4-bit value
        
        if (x & 1) {
            // Odd x: low nibble
            pixels[byte_idx] = (pixels[byte_idx] & 0xF0) | gray_value;
        } else {
            // Even x: high nibble
            pixels[byte_idx] = (pixels[byte_idx] & 0x0F) | (gray_value << 4);
        }
        dirty = true;
    }
    
    // Legacy method for backwards compatibility
    bool getPixel(int x, int y) const {
        return getPixelGray(x, y) > 7;  // Threshold at 8
    }
    
    // New method to get 4-bit grayscale value
    uint8_t getPixelGray(int x, int y) const {
        if (x < 0 || x >= 256 || y < 0 || y >= 64) return 0;
        
        int byte_idx = y * 128 + x / 2;
        
        if (x & 1) {
            // Odd x: low nibble
            return pixels[byte_idx] & 0x0F;
        } else {
            // Even x: high nibble
            return (pixels[byte_idx] >> 4) & 0x0F;
        }
    }
};

// Plugin instance wrapper for VCV
struct VCVPluginInstance {
    void* handle = nullptr;
    _NT_factory* factory = nullptr;
    _NT_algorithm* algorithm = nullptr;
    void* shared_memory = nullptr;
    void* instance_memory = nullptr;
    std::string name;
    bool is_loaded = false;
    
    // Memory requirements
    _NT_staticRequirements static_reqs{};
    _NT_algorithmRequirements algorithm_reqs{};
};

// UI state tracking for customUi events
struct CustomUiState {
    uint16_t lastButtons = 0;
    std::array<float, 3> lastPots{0.5f, 0.5f, 0.5f};
    bool hasCustomUi = false;
};

// Core emulator adapted for VCV Rack
class EmulatorCore {
public:
    EmulatorCore();
    ~EmulatorCore();
    
    bool initialize(float sampleRate);
    void shutdown();
    
    // Algorithm management
    bool loadBuiltinAlgorithms();
    void unloadAlgorithm();
    bool selectAlgorithm(int index);
    int getCurrentAlgorithmIndex() const { return current_algorithm_index_; }
    int getAlgorithmCount() const { return algorithms_.size(); }
    const std::string& getAlgorithmName(int index) const;
    
    // Audio processing
    void processAudio(float* buses, int numFramesBy4);
    
    // Hardware interface
    void updateHardwareState(const VCVHardwareState& state);
    const VCVHardwareState& getHardwareState() const { return hardware_state_; }
    void processHardwareChanges(_NT_factory* factory = nullptr, _NT_algorithm* algorithm = nullptr);
    
    // Display
    VCVDisplayBuffer& getDisplayBuffer() { return display_buffer_; }
    const VCVDisplayBuffer& getDisplayBuffer() const { return display_buffer_; }
    void updateDisplay();
    
    // MIDI
    MidiHandler& getMidiHandler() { return midi_handler_; }
    const MidiHandler& getMidiHandler() const { return midi_handler_; }
    
    // Parameter handling
    void setParameter(int param, float value);
    float getParameter(int param) const;
    
    // Button handling  
    void pressButton(int button);
    void releaseButton(int button);
    
    // Encoder handling
    void turnEncoder(int encoder, int delta);
    void pressEncoder(int encoder);
    void releaseEncoder(int encoder);
    void clearEncoderDeltas();
    
    // Pot handling (for pressable pots)
    void pressPot(int pot);
    void releasePot(int pot);
    
    // State persistence
    json_t* saveState();
    void loadState(json_t* rootJ);
    
private:
    std::vector<VCVPluginInstance> algorithms_;
    int current_algorithm_index_ = -1;
    VCVPluginInstance* current_algorithm_ = nullptr;
    
    VCVHardwareState hardware_state_;
    VCVHardwareState previous_hardware_state_;
    VCVDisplayBuffer display_buffer_;
    MidiHandler midi_handler_;
    
    float sample_rate_ = 44100.0f;
    bool initialized_ = false;
    
    // Disting NT globals
    _NT_globals nt_globals_;
    std::vector<float> work_buffer_;
    
    // Custom UI state tracking
    CustomUiState ui_state_;
    
    // Built-in algorithms
    void registerBuiltinAlgorithm(const std::string& name, _NT_factory* factory);
    bool initializeAlgorithm(VCVPluginInstance& instance);
    void cleanupAlgorithm(VCVPluginInstance& instance);
    
    // Hardware state change detection
    bool hasHardwareChanged() const;
    
    // Custom UI support  
    void sendCustomUiEvents(_NT_factory* factory, _NT_algorithm* algorithm);
    
    // Display management
    void clearDisplay();
    void renderAlgorithmDisplay();
};

// Implementations
inline EmulatorCore::EmulatorCore() {
    // Initialize NT globals structure
    nt_globals_.maxFramesPerStep = 4; // Process in 4-sample blocks
    work_buffer_.resize(1024); // 1KB work buffer
    nt_globals_.workBuffer = work_buffer_.data();
    nt_globals_.workBufferSizeBytes = work_buffer_.size() * sizeof(float);
}

inline EmulatorCore::~EmulatorCore() {
    shutdown();
}

inline bool EmulatorCore::initialize(float sampleRate) {
    if (initialized_) return true;
    
    sample_rate_ = sampleRate;
    nt_globals_.sampleRate = (uint32_t)sampleRate;
    
    // Load built-in algorithms
    if (!loadBuiltinAlgorithms()) {
        return false;
    }
    
    // Select first algorithm by default
    if (!algorithms_.empty()) {
        selectAlgorithm(0);
    }
    
    initialized_ = true;
    return true;
}

inline void EmulatorCore::shutdown() {
    for (auto& algo : algorithms_) {
        cleanupAlgorithm(algo);
    }
    algorithms_.clear();
    current_algorithm_ = nullptr;
    current_algorithm_index_ = -1;
    initialized_ = false;
}

inline bool EmulatorCore::loadBuiltinAlgorithms() {
    algorithms_.clear();
    
    // Add simple gain algorithm
    VCVPluginInstance instance;
    instance.name = "Simple Gain";
    instance.is_loaded = true;
    algorithms_.push_back(instance);
    
    return true;
}

inline bool EmulatorCore::selectAlgorithm(int index) {
    if (index < 0 || index >= (int)algorithms_.size()) {
        return false;
    }
    
    current_algorithm_index_ = index;
    current_algorithm_ = &algorithms_[index];
    
    if (current_algorithm_->algorithm) {
        // Algorithm is already constructed, just prepare it
        // Reset any state if needed
    }
    
    // Check for customUi support and call setupUi if available
    if (current_algorithm_ && current_algorithm_->factory && current_algorithm_->factory->setupUi) {
        try {
            INFO("EmulatorCore::selectAlgorithm calling setupUi for algorithm %d", index);
            // Initialize UI state with current hardware values
            ui_state_.lastPots[0] = hardware_state_.pots[0];
            ui_state_.lastPots[1] = hardware_state_.pots[1];
            ui_state_.lastPots[2] = hardware_state_.pots[2];
            
            // Get desired pot values from plugin
            float potValues[3] = {
                hardware_state_.pots[0], 
                hardware_state_.pots[1], 
                hardware_state_.pots[2]
            };
            current_algorithm_->factory->setupUi(current_algorithm_->algorithm, potValues);
            
            // Update hardware state with plugin's desired pot values
            hardware_state_.pots[0] = potValues[0];
            hardware_state_.pots[1] = potValues[1];
            hardware_state_.pots[2] = potValues[2];
            
            INFO("EmulatorCore::selectAlgorithm updated pot values to: %.3f %.3f %.3f", 
                 potValues[0], potValues[1], potValues[2]);
        } catch (...) {
            printf("Plugin crashed during setupUi\n");
            current_algorithm_ = nullptr;
            return false;
        }
    }
    
    display_buffer_.dirty = true;
    return true;
}

inline const std::string& EmulatorCore::getAlgorithmName(int index) const {
    static const std::string empty = "";
    if (index >= 0 && index < (int)algorithms_.size()) {
        return algorithms_[index].name;
    }
    return empty;
}

inline void EmulatorCore::processAudio(float* buses, int numFramesBy4) {
    if (!current_algorithm_ || !current_algorithm_->algorithm || !current_algorithm_->factory) {
        return;
    }
    
    // Process hardware changes
    processHardwareChanges();
    
    // Call algorithm step function with crash protection
    if (current_algorithm_->factory->step) {
        try {
            current_algorithm_->factory->step(current_algorithm_->algorithm, buses, numFramesBy4);
        } catch (const std::exception& e) {
            // Plugin crashed during audio processing - disable it
            printf("Plugin crashed during audio processing: %s\n", e.what());
            // Clear the current algorithm to prevent further crashes
            current_algorithm_ = nullptr;
        } catch (...) {
            // Plugin crashed during audio processing - disable it
            printf("Plugin crashed during audio processing: unknown error\n");
            // Clear the current algorithm to prevent further crashes
            current_algorithm_ = nullptr;
        }
    }
}

inline void EmulatorCore::updateHardwareState(const VCVHardwareState& state) {
    previous_hardware_state_ = hardware_state_;
    hardware_state_ = state;
}

inline bool EmulatorCore::hasHardwareChanged() const {
    // Check if any hardware state has changed
    for (int i = 0; i < 3; i++) {
        if (hardware_state_.pots[i] != previous_hardware_state_.pots[i]) return true;
        if (hardware_state_.pot_pressed[i] != previous_hardware_state_.pot_pressed[i]) return true;
    }
    
    for (int i = 0; i < 4; i++) {
        if (hardware_state_.buttons[i] != previous_hardware_state_.buttons[i]) return true;
    }
    
    for (int i = 0; i < 2; i++) {
        if (hardware_state_.encoder_deltas[i] != 0) return true;
        if (hardware_state_.encoder_pressed[i] != previous_hardware_state_.encoder_pressed[i]) return true;
    }
    
    return false;
}


inline void EmulatorCore::processHardwareChanges(_NT_factory* factory, _NT_algorithm* algorithm) {
    // Use passed parameters if available, otherwise fall back to built-in
    if (!factory || !algorithm) {
        if (current_algorithm_ && current_algorithm_->factory) {
            factory = current_algorithm_->factory;
            algorithm = current_algorithm_->algorithm;
        }
    }
    
    if (!factory || !algorithm) {
        return;
    }
    
    // Check if plugin supports customUi
    ui_state_.hasCustomUi = (factory->customUi != nullptr);
    
    // Send custom UI events if plugin supports it
    if (ui_state_.hasCustomUi) {
        sendCustomUiEvents(factory, algorithm);
        // Clear encoder deltas after sending to customUi
        clearEncoderDeltas();
    } else {
        // Process parameter changes (pots) for non-customUi plugins
        for (int i = 0; i < 3; i++) {
            if (hardware_state_.pots[i] != previous_hardware_state_.pots[i]) {
                if (factory->parameterChanged) {
                    try {
                        factory->parameterChanged(algorithm, i);
                    } catch (...) {
                        printf("Plugin crashed during parameterChanged\n");
                        return;
                    }
                }
            }
        }
        
        // Process button changes for non-customUi plugins
        for (int i = 0; i < 4; i++) {
            if (hardware_state_.buttons[i] != previous_hardware_state_.buttons[i]) {
                display_buffer_.dirty = true;
            }
        }
        
        // Process encoder changes for non-customUi plugins
        for (int i = 0; i < 2; i++) {
            if (hardware_state_.encoder_deltas[i] != 0) {
                display_buffer_.dirty = true;
            }
            
            if (hardware_state_.encoder_pressed[i] != previous_hardware_state_.encoder_pressed[i]) {
                display_buffer_.dirty = true;
            }
        }
        
        // Clear encoder deltas after processing
        clearEncoderDeltas();
    }
}

inline void EmulatorCore::updateDisplay() {
    if (!current_algorithm_ || !current_algorithm_->factory) {
        clearDisplay();
        return;
    }
    
    // Clear display
    clearDisplay();
    
    // Let algorithm draw if it has a draw function
    if (current_algorithm_->factory->draw) {
        try {
            bool suppress_default = current_algorithm_->factory->draw(current_algorithm_->algorithm);
            if (!suppress_default) {
                // Draw default parameter display if algorithm doesn't suppress it
                // TODO: Implement default parameter display
            }
        } catch (...) {
            printf("Plugin crashed during draw\n");
            current_algorithm_ = nullptr;
            return;
        }
    }
    
    display_buffer_.dirty = false;
}

inline void EmulatorCore::clearDisplay() {
    display_buffer_.clear();
}

inline json_t* EmulatorCore::saveState() {
    json_t* rootJ = json_object();
    
    json_object_set_new(rootJ, "algorithmIndex", json_integer(current_algorithm_index_));
    
    // Save algorithm-specific state if serialization is supported
    if (current_algorithm_ && current_algorithm_->factory && current_algorithm_->factory->serialise) {
        // TODO: Implement algorithm state serialization
    }
    
    return rootJ;
}

inline void EmulatorCore::loadState(json_t* rootJ) {
    json_t* algorithmJ = json_object_get(rootJ, "algorithmIndex");
    if (algorithmJ) {
        selectAlgorithm(json_integer_value(algorithmJ));
    }
    
    // Load algorithm-specific state if deserialization is supported
    if (current_algorithm_ && current_algorithm_->factory && current_algorithm_->factory->deserialise) {
        // TODO: Implement algorithm state deserialization
    }
}

// Missing method implementations
inline void EmulatorCore::setParameter(int param, float value) {
    // For now, just store in hardware state
    if (param >= 0 && param < 3) {
        hardware_state_.pots[param] = value;
    }
}

inline float EmulatorCore::getParameter(int param) const {
    if (param >= 0 && param < 3) {
        return hardware_state_.pots[param];
    }
    return 0.0f;
}

inline void EmulatorCore::pressButton(int button) {
    if (button >= 0 && button < 4) {
        hardware_state_.buttons[button] = true;
    }
}

inline void EmulatorCore::releaseButton(int button) {
    if (button >= 0 && button < 4) {
        hardware_state_.buttons[button] = false;
    }
}

inline void EmulatorCore::turnEncoder(int encoder, int delta) {
    if (encoder >= 0 && encoder < 2) {
        hardware_state_.encoder_deltas[encoder] = delta;
    }
}

inline void EmulatorCore::pressEncoder(int encoder) {
    if (encoder >= 0 && encoder < 2) {
        hardware_state_.encoder_pressed[encoder] = true;
    }
}

inline void EmulatorCore::releaseEncoder(int encoder) {
    if (encoder >= 0 && encoder < 2) {
        hardware_state_.encoder_pressed[encoder] = false;
    }
}

inline void EmulatorCore::clearEncoderDeltas() {
    hardware_state_.encoder_deltas[0] = 0;
    hardware_state_.encoder_deltas[1] = 0;
}

inline void EmulatorCore::pressPot(int pot) {
    if (pot >= 0 && pot < 3) {
        hardware_state_.pot_pressed[pot] = true;
    }
}

inline void EmulatorCore::releasePot(int pot) {
    if (pot >= 0 && pot < 3) {
        hardware_state_.pot_pressed[pot] = false;
    }
}

inline void EmulatorCore::registerBuiltinAlgorithm(const std::string& name, _NT_factory* factory) {
    // TODO: Implement algorithm registration
}

inline bool EmulatorCore::initializeAlgorithm(VCVPluginInstance& instance) {
    // TODO: Implement algorithm initialization
    return true;
}

inline void EmulatorCore::cleanupAlgorithm(VCVPluginInstance& instance) {
    // TODO: Implement algorithm cleanup
}

inline void EmulatorCore::renderAlgorithmDisplay() {
    // TODO: Implement algorithm-specific display rendering
}


inline void EmulatorCore::sendCustomUiEvents(_NT_factory* factory, _NT_algorithm* algorithm) {
    if (!ui_state_.hasCustomUi || !factory || !algorithm || !factory->customUi) {
        return;
    }
    
    // Build _NT_uiData structure
    _NT_uiData uiData = {};
    uint16_t controls = 0;
    
    // Copy current pot values
    for (int i = 0; i < 3; i++) {
        uiData.pots[i] = hardware_state_.pots[i];
        
        // Check for pot changes
        float potDiff = std::abs(hardware_state_.pots[i] - ui_state_.lastPots[i]);
        if (potDiff > 0.001f) {
            controls |= (kNT_potL << i);
            ui_state_.lastPots[i] = hardware_state_.pots[i];
        }
    }
    
    // Build current button state
    uint16_t currentButtons = 0;
    for (int i = 0; i < 4; i++) {
        if (hardware_state_.buttons[i]) {
            currentButtons |= (kNT_button1 << i);
        }
    }
    
    // Add pot button states
    for (int i = 0; i < 3; i++) {
        if (hardware_state_.pot_pressed[i]) {
            currentButtons |= (kNT_potButtonL << i);
        }
    }
    
    // Add encoder button states
    for (int i = 0; i < 2; i++) {
        if (hardware_state_.encoder_pressed[i]) {
            currentButtons |= (kNT_encoderButtonL << i);
        }
    }
    
    // Detect button changes (edge detection)
    uint16_t buttonChanges = currentButtons ^ ui_state_.lastButtons;
    controls |= buttonChanges;
    
    // Set encoder deltas and controls
    for (int i = 0; i < 2; i++) {
        uiData.encoders[i] = hardware_state_.encoder_deltas[i];
        if (hardware_state_.encoder_deltas[i] != 0) {
            controls |= (kNT_encoderL << i);
        }
    }
    
    // Set the control fields
    uiData.controls = controls;
    uiData.lastButtons = ui_state_.lastButtons;
    
    // Update stored button state
    ui_state_.lastButtons = currentButtons;
    
    // Call customUi if there are any changes
    if (controls != 0) {
        try {
            factory->customUi(algorithm, uiData);
        } catch (...) {
            INFO("Plugin crashed during customUi");
        }
    }
}