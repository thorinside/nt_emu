#pragma once

#include <distingnt/api.h>
#include <array>
#include <vector>
#include <functional>
#include <atomic>

struct DisplayBuffer {
    std::array<uint8_t, (256 * 64) / 8> pixels{};
    bool dirty = false;
    
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

struct HardwareState {
    std::array<float, 3> pots{0.0f, 0.0f, 0.0f};       // 3 potentiometers
    std::array<bool, 3> pot_pressed{false, false, false}; // Pots are pressable
    std::array<bool, 4> buttons{false, false, false, false}; // 4 buttons
    std::array<int, 2> encoder_values{0, 0};            // 2 encoders
    std::array<bool, 2> encoder_pressed{false, false};  // Encoders are pressable
};

struct VoltageState {
    std::array<std::atomic<float>, 12> input_voltages;   // Thread-safe voltage readings for 12 inputs
    std::array<std::atomic<float>, 6> output_voltages;   // Thread-safe voltage readings for 6 outputs
    std::atomic<bool> monitoring_enabled;                // Enable/disable monitoring
    
    VoltageState() : monitoring_enabled(true) {
        // Initialize all voltages to zero
        for (int i = 0; i < 12; i++) {
            input_voltages[i].store(0.0f);
        }
        for (int i = 0; i < 6; i++) {
            output_voltages[i].store(0.0f);
        }
    }
};

struct ApiState {
    DisplayBuffer display;
    HardwareState hardware;
    VoltageState voltage;                                // Real-time CV voltage monitoring
    std::vector<_NT_parameterValue> parameter_values;
    std::vector<bool> parameter_locked;
    
    // Callbacks for MIDI output
    std::function<void(const struct _NT_controllerChange&, enum _NT_midiDestination)> midi_cc_callback;
    std::function<void(const struct _NT_noteOn&, enum _NT_midiDestination)> midi_note_on_callback;
    std::function<void(const struct _NT_noteOff&, enum _NT_midiDestination)> midi_note_off_callback;
    
    // Current algorithm for parameter access
    _NT_algorithm* current_algorithm = nullptr;
};

class ApiShim {
public:
    static void initialize();
    static void setAlgorithm(_NT_algorithm* algorithm);
    static ApiState& getState() { return state_; }
    
    // Parameter functions
    static void parameterChanged(unsigned int parameterIndex);
    static float getParameterValueMapped(unsigned int parameterIndex);
    static float getParameterValueMappedNormalised(unsigned int parameterIndex);
    static void setParameterValueMapped(unsigned int parameterIndex, float value);
    static void setParameterValueMappedNormalised(unsigned int parameterIndex, float value);
    static void lockParameter(unsigned int parameterIndex);
    static void unlockParameter(unsigned int parameterIndex);
    static int parameterIsLocked(unsigned int parameterIndex);
    
    // Drawing functions
    static void drawText(int x, int y, const char* text, enum _NT_textSize size, enum _NT_textAlign align);
    static void drawShapeI(enum _NT_shape shape, int x, int y, int w, int h);
    static void drawShapeF(enum _NT_shape shape, float x, float y, float w, float h);
    static void getDisplayDimensions(unsigned int* width, unsigned int* height);
    
    // MIDI functions
    static void sendMIDIControllerChange(struct _NT_controllerChange* controllerChange, enum _NT_midiDestination destination);
    static void sendMIDINoteOn(struct _NT_noteOn* noteOn, enum _NT_midiDestination destination);
    static void sendMIDINoteOff(struct _NT_noteOff* noteOff, enum _NT_midiDestination destination);
    
    // Utility functions
    static float getSampleRate();
    static unsigned int getSamplesPerBlock();
    static void log(const char* text);
    static unsigned int random(unsigned int max);
    static float randomF();
    
private:
    static ApiState state_;
    
    static void drawChar(int x, int y, char c, enum _NT_textSize size);
    static int getCharWidth(char c, enum _NT_textSize size);
    static int getTextHeight(enum _NT_textSize size);
};