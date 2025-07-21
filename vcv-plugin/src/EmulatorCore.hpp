#pragma once
#include <rack.hpp>
#include <memory>
#include <vector>
#include <string>
#include <array>

// Import Disting NT API types
#include "../../emulator/include/distingnt/api.h"

using namespace rack;

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
    std::array<uint8_t, (256 * 64) / 8> pixels{};
    bool dirty = true;
    
    void clear() {
        pixels.fill(0);
        dirty = true;
    }
    
    void setPixel(int x, int y, bool on) {
        if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
        
        int byte_idx = (y * 256 + x) / 8;
        int bit_idx = 7 - (x % 8);
        
        if (on) {
            pixels[byte_idx] |= (1 << bit_idx);
        } else {
            pixels[byte_idx] &= ~(1 << bit_idx);
        }
        dirty = true;
    }
    
    bool getPixel(int x, int y) const {
        if (x < 0 || x >= 256 || y < 0 || y >= 64) return false;
        
        int byte_idx = (y * 256 + x) / 8;
        int bit_idx = 7 - (x % 8);
        
        return (pixels[byte_idx] & (1 << bit_idx)) != 0;
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
    
    // Display
    VCVDisplayBuffer& getDisplayBuffer() { return display_buffer_; }
    const VCVDisplayBuffer& getDisplayBuffer() const { return display_buffer_; }
    void updateDisplay();
    
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
    
    float sample_rate_ = 44100.0f;
    bool initialized_ = false;
    
    // Disting NT globals
    _NT_globals nt_globals_;
    std::vector<float> work_buffer_;
    
    // Built-in algorithms
    void registerBuiltinAlgorithm(const std::string& name, _NT_factory* factory);
    bool initializeAlgorithm(VCVPluginInstance& instance);
    void cleanupAlgorithm(VCVPluginInstance& instance);
    
    // Hardware state change detection
    bool hasHardwareChanged() const;
    void processHardwareChanges();
    
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
    
    // Call algorithm step function
    if (current_algorithm_->factory->step) {
        current_algorithm_->factory->step(current_algorithm_->algorithm, buses, numFramesBy4);
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

inline void EmulatorCore::processHardwareChanges() {
    if (!hasHardwareChanged() || !current_algorithm_ || !current_algorithm_->factory) {
        return;
    }
    
    // Process parameter changes (pots)
    for (int i = 0; i < 3; i++) {
        if (hardware_state_.pots[i] != previous_hardware_state_.pots[i]) {
            if (current_algorithm_->factory->parameterChanged) {
                current_algorithm_->factory->parameterChanged(current_algorithm_->algorithm, i);
            }
        }
    }
    
    // Process button changes
    for (int i = 0; i < 4; i++) {
        if (hardware_state_.buttons[i] != previous_hardware_state_.buttons[i]) {
            // Button state changed - could trigger custom UI or other functions
            display_buffer_.dirty = true;
        }
    }
    
    // Process encoder changes
    for (int i = 0; i < 2; i++) {
        if (hardware_state_.encoder_deltas[i] != 0) {
            // Handle encoder rotation
            display_buffer_.dirty = true;
        }
        
        if (hardware_state_.encoder_pressed[i] != previous_hardware_state_.encoder_pressed[i]) {
            // Handle encoder press/release
            display_buffer_.dirty = true;
        }
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
        bool suppress_default = current_algorithm_->factory->draw(current_algorithm_->algorithm);
        if (!suppress_default) {
            // Draw default parameter display if algorithm doesn't suppress it
            // TODO: Implement default parameter display
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