#include "api_shim.h"
#include <iostream>
#include <cstring>
#include <random>
#include <algorithm>
#include <cmath>

ApiState ApiShim::state_;

extern "C" {
    void NT_parameterChanged(unsigned int parameterIndex) {
        ApiShim::parameterChanged(parameterIndex);
    }
    
    float NT_getParameterValueMapped(unsigned int parameterIndex) {
        return ApiShim::getParameterValueMapped(parameterIndex);
    }
    
    float NT_getParameterValueMappedNormalised(unsigned int parameterIndex) {
        return ApiShim::getParameterValueMappedNormalised(parameterIndex);
    }
    
    void NT_setParameterValueMapped(unsigned int parameterIndex, float value) {
        ApiShim::setParameterValueMapped(parameterIndex, value);
    }
    
    void NT_setParameterValueMappedNormalised(unsigned int parameterIndex, float value) {
        ApiShim::setParameterValueMappedNormalised(parameterIndex, value);
    }
    
    void NT_lockParameter(unsigned int parameterIndex) {
        ApiShim::lockParameter(parameterIndex);
    }
    
    void NT_unlockParameter(unsigned int parameterIndex) {
        ApiShim::unlockParameter(parameterIndex);
    }
    
    int NT_parameterIsLocked(unsigned int parameterIndex) {
        return ApiShim::parameterIsLocked(parameterIndex);
    }
    
    void NT_drawText(int x, int y, const char* text, enum _NT_textSize size, enum _NT_textAlign align) {
        ApiShim::drawText(x, y, text, size, align);
    }
    
    void NT_drawShapeI(enum _NT_shape shape, int x, int y, int w, int h) {
        ApiShim::drawShapeI(shape, x, y, w, h);
    }
    
    void NT_drawShapeF(enum _NT_shape shape, float x, float y, float w, float h) {
        ApiShim::drawShapeF(shape, x, y, w, h);
    }
    
    void NT_getDisplayDimensions(unsigned int* width, unsigned int* height) {
        ApiShim::getDisplayDimensions(width, height);
    }
    
    void NT_sendMIDIControllerChange(struct _NT_controllerChange* controllerChange, enum _NT_midiDestination destination) {
        ApiShim::sendMIDIControllerChange(controllerChange, destination);
    }
    
    void NT_sendMIDINoteOn(struct _NT_noteOn* noteOn, enum _NT_midiDestination destination) {
        ApiShim::sendMIDINoteOn(noteOn, destination);
    }
    
    void NT_sendMIDINoteOff(struct _NT_noteOff* noteOff, enum _NT_midiDestination destination) {
        ApiShim::sendMIDINoteOff(noteOff, destination);
    }
    
    float NT_getSampleRate() {
        return ApiShim::getSampleRate();
    }
    
    unsigned int NT_getSamplesPerBlock() {
        return ApiShim::getSamplesPerBlock();
    }
    
    void NT_log(const char* text) {
        ApiShim::log(text);
    }
    
    unsigned int NT_random(unsigned int max) {
        return ApiShim::random(max);
    }
    
    float NT_randomF() {
        return ApiShim::randomF();
    }
    
    // Stub implementations for other API functions
    void NT_sendMIDIPitchBend(struct _NT_pitchBend* pitchBend, enum _NT_midiDestination destination) { (void)pitchBend; (void)destination; }
    void NT_sendMIDIProgramChange(struct _NT_programChange* programChange, enum _NT_midiDestination destination) { (void)programChange; (void)destination; }
    void NT_sendMIDIChannelPressure(struct _NT_channelPressure* channelPressure, enum _NT_midiDestination destination) { (void)channelPressure; (void)destination; }
    void NT_sendMIDIPolyKeyPressure(struct _NT_polyKeyPressure* polyKeyPressure, enum _NT_midiDestination destination) { (void)polyKeyPressure; (void)destination; }
    void NT_sendMIDISystemExclusive(struct _NT_systemExclusive* systemExclusive, enum _NT_midiDestination destination) { (void)systemExclusive; (void)destination; }
    void NT_sendMIDIClockTick(enum _NT_midiDestination destination) { (void)destination; }
    void NT_sendMIDIClockStart(enum _NT_midiDestination destination) { (void)destination; }
    void NT_sendMIDIClockStop(enum _NT_midiDestination destination) { (void)destination; }
    void NT_sendMIDIClockContinue(enum _NT_midiDestination destination) { (void)destination; }
    void NT_sendMIDIActiveSense(enum _NT_midiDestination destination) { (void)destination; }
    void NT_sendMIDISystemReset(enum _NT_midiDestination destination) { (void)destination; }
    float NT_getTemperatureC() { return 25.0f; }
    void NT_copyFromFlash(void* destination, const void* source, unsigned int length) { memcpy(destination, source, length); }
    int NT_strlenUTF8(const char* text) { return strlen(text); }
    int NT_getTextWidthUTF8(const char* text, enum _NT_textSize size) { return strlen(text) * (8 + size * 4); }
}

void ApiShim::initialize() {
    state_.display.clear();
    state_.hardware = {};
    state_.parameter_values.clear();
    state_.parameter_locked.clear();
    state_.current_algorithm = nullptr;
}

void ApiShim::setAlgorithm(_NT_algorithm* algorithm) {
    state_.current_algorithm = algorithm;
    
    if (algorithm) {
        // Get number of parameters and resize vectors
        int numParams = 0;
        algorithm->getValue(algorithm, kNT_selector_numParameters, &numParams, sizeof(numParams));
        
        state_.parameter_values.resize(numParams);
        state_.parameter_locked.resize(numParams, false);
        
        // Initialize parameters to default values
        for (int i = 0; i < numParams; i++) {
            state_.parameter_values[i].asFloat = 0.0f;
        }
    }
}

void ApiShim::parameterChanged(unsigned int parameterIndex) {
    // Notify system that parameter changed
    // Logging disabled to prevent console spam during audio processing
    // std::cout << "Parameter " << parameterIndex << " changed" << std::endl;
    (void)parameterIndex;  // Suppress unused parameter warning
}

float ApiShim::getParameterValueMapped(unsigned int parameterIndex) {
    if (parameterIndex >= state_.parameter_values.size()) return 0.0f;
    return state_.parameter_values[parameterIndex].asFloat;
}

float ApiShim::getParameterValueMappedNormalised(unsigned int parameterIndex) {
    // For now, assume parameters are already normalized [0,1]
    return getParameterValueMapped(parameterIndex);
}

void ApiShim::setParameterValueMapped(unsigned int parameterIndex, float value) {
    if (parameterIndex >= state_.parameter_values.size()) return;
    state_.parameter_values[parameterIndex].asFloat = value;
}

void ApiShim::setParameterValueMappedNormalised(unsigned int parameterIndex, float value) {
    setParameterValueMapped(parameterIndex, value);
}

void ApiShim::lockParameter(unsigned int parameterIndex) {
    if (parameterIndex >= state_.parameter_locked.size()) return;
    state_.parameter_locked[parameterIndex] = true;
}

void ApiShim::unlockParameter(unsigned int parameterIndex) {
    if (parameterIndex >= state_.parameter_locked.size()) return;
    state_.parameter_locked[parameterIndex] = false;
}

int ApiShim::parameterIsLocked(unsigned int parameterIndex) {
    if (parameterIndex >= state_.parameter_locked.size()) return 0;
    return state_.parameter_locked[parameterIndex] ? 1 : 0;
}

void ApiShim::drawText(int x, int y, const char* text, enum _NT_textSize size, enum _NT_textAlign align) {
    if (!text) return;
    
    int text_width = strlen(text) * getCharWidth('W', size);
    int start_x = x;
    
    // Apply alignment
    switch (align) {
        case kNT_textAlign_centre:
            start_x = x - text_width / 2;
            break;
        case kNT_textAlign_right:
            start_x = x - text_width;
            break;
        case kNT_textAlign_left:
        default:
            start_x = x;
            break;
    }
    
    // Draw each character
    int char_x = start_x;
    for (const char* c = text; *c; c++) {
        drawChar(char_x, y, *c, size);
        char_x += getCharWidth(*c, size);
    }
}

void ApiShim::drawShapeI(enum _NT_shape shape, int x, int y, int w, int h) {
    switch (shape) {
        case kNT_shape_filled_rectangle:
            for (int dy = 0; dy < h; dy++) {
                for (int dx = 0; dx < w; dx++) {
                    state_.display.setPixel(x + dx, y + dy, true);
                }
            }
            break;
            
        case kNT_shape_outline_rectangle:
            // Top and bottom lines
            for (int dx = 0; dx < w; dx++) {
                state_.display.setPixel(x + dx, y, true);
                state_.display.setPixel(x + dx, y + h - 1, true);
            }
            // Left and right lines
            for (int dy = 0; dy < h; dy++) {
                state_.display.setPixel(x, y + dy, true);
                state_.display.setPixel(x + w - 1, y + dy, true);
            }
            break;
            
        case kNT_shape_horizontal_line:
            for (int dx = 0; dx < w; dx++) {
                state_.display.setPixel(x + dx, y, true);
            }
            break;
            
        case kNT_shape_vertical_line:
            for (int dy = 0; dy < h; dy++) {
                state_.display.setPixel(x, y + dy, true);
            }
            break;
            
        case kNT_shape_filled_circle: {
            int cx = x + w/2, cy = y + h/2;
            int radius = std::min(w, h) / 2;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (dx*dx + dy*dy <= radius*radius) {
                        state_.display.setPixel(cx + dx, cy + dy, true);
                    }
                }
            }
            break;
        }
        
        default:
            // For other shapes, just draw a rectangle for now
            drawShapeI(kNT_shape_outline_rectangle, x, y, w, h);
            break;
    }
}

void ApiShim::drawShapeF(enum _NT_shape shape, float x, float y, float w, float h) {
    drawShapeI(shape, (int)x, (int)y, (int)w, (int)h);
}

void ApiShim::getDisplayDimensions(unsigned int* width, unsigned int* height) {
    if (width) *width = 256;
    if (height) *height = 64;
}

void ApiShim::sendMIDIControllerChange(struct _NT_controllerChange* controllerChange, enum _NT_midiDestination destination) {
    if (state_.midi_cc_callback) {
        state_.midi_cc_callback(*controllerChange, destination);
    }
}

void ApiShim::sendMIDINoteOn(struct _NT_noteOn* noteOn, enum _NT_midiDestination destination) {
    if (state_.midi_note_on_callback) {
        state_.midi_note_on_callback(*noteOn, destination);
    }
}

void ApiShim::sendMIDINoteOff(struct _NT_noteOff* noteOff, enum _NT_midiDestination destination) {
    if (state_.midi_note_off_callback) {
        state_.midi_note_off_callback(*noteOff, destination);
    }
}

float ApiShim::getSampleRate() {
    return 48000.0f;  // Updated to 48kHz for compatibility
}

unsigned int ApiShim::getSamplesPerBlock() {
    return 4;  // disting NT processes in 4-sample blocks
}

void ApiShim::log(const char* text) {
    // Plugin logging - can be disabled to prevent console spam
    // Enable this line for plugin debugging:
    // std::cout << "[Plugin] " << text << std::endl;
    (void)text;  // Suppress unused parameter warning
}

unsigned int ApiShim::random(unsigned int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    if (max == 0) return 0;
    std::uniform_int_distribution<> dis(0, max - 1);
    return dis(gen);
}

float ApiShim::randomF() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

void ApiShim::drawChar(int x, int y, char c, enum _NT_textSize size) {
    // Simple 8x8 bitmap font for now
    int char_width = getCharWidth(c, size);
    int char_height = getTextHeight(size);
    
    // Very basic character drawing - just draw a rectangle for now
    // In a real implementation, you'd have bitmap font data
    if (c != ' ') {
        for (int dy = 0; dy < char_height; dy++) {
            for (int dx = 0; dx < char_width; dx++) {
                // Simple pattern based on character
                if ((dx + dy + c) % 3 == 0) {
                    state_.display.setPixel(x + dx, y + dy, true);
                }
            }
        }
    }
}

int ApiShim::getCharWidth(char c, enum _NT_textSize size) {
    (void)c;  // For now, all characters have same width
    switch (size) {
        case kNT_textSize_8: return 6;
        case kNT_textSize_12: return 8;
        case kNT_textSize_16: return 10;
        case kNT_textSize_24: return 14;
        default: return 8;
    }
}

int ApiShim::getTextHeight(enum _NT_textSize size) {
    switch (size) {
        case kNT_textSize_8: return 8;
        case kNT_textSize_12: return 12;
        case kNT_textSize_16: return 16;
        case kNT_textSize_24: return 24;
        default: return 12;
    }
}