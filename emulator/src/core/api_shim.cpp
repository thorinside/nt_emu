#include "api_shim.h"
#include <iostream>
#include <cstring>
#include <random>
#include <algorithm>
#include <cmath>

ApiState ApiShim::state_;

// NT_screen buffer as per API specification - definition (not declaration)
uint8_t NT_screen[128 * 64];

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
    
    void NT_drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size) {
        ApiShim::drawText(x, y, str, colour, align, size);
    }
    
    void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour) {
        ApiShim::drawShapeI(shape, x0, y0, x1, y1, colour);
    }
    
    void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour) {
        ApiShim::drawShapeF(shape, x0, y0, x1, y1, colour);
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
    
    // Initialize NT_screen buffer and sync with display buffer
    memset(NT_screen, 0, sizeof(NT_screen));
    memcpy(NT_screen, state_.display.pixels.data(), sizeof(NT_screen));
}

void ApiShim::setAlgorithm(_NT_algorithm* algorithm) {
    state_.current_algorithm = algorithm;
    
    if (algorithm) {
        // The algorithm's parameter arrays are managed by the plugin system
        // We don't need to track them separately in the shim
        // The algorithm->v pointer provides direct access to parameter values
        
        // Clear our local parameter tracking (if we need it for specific shim functions)
        state_.parameter_values.clear();
        state_.parameter_locked.clear();
    }
}

void ApiShim::parameterChanged(unsigned int parameterIndex) {
    // Notify system that parameter changed
    // Logging disabled to prevent console spam during audio processing
    // std::cout << "Parameter " << parameterIndex << " changed" << std::endl;
    (void)parameterIndex;  // Suppress unused parameter warning
}

float ApiShim::getParameterValueMapped(unsigned int parameterIndex) {
    // These parameter functions are stubs - the real parameter management
    // happens in the plugin system, not in the drawing API shim
    (void)parameterIndex;
    return 0.0f;
}

float ApiShim::getParameterValueMappedNormalised(unsigned int parameterIndex) {
    (void)parameterIndex;
    return 0.0f;
}

void ApiShim::setParameterValueMapped(unsigned int parameterIndex, float value) {
    (void)parameterIndex;
    (void)value;
}

void ApiShim::setParameterValueMappedNormalised(unsigned int parameterIndex, float value) {
    (void)parameterIndex;
    (void)value;
}

void ApiShim::lockParameter(unsigned int parameterIndex) {
    (void)parameterIndex;
}

void ApiShim::unlockParameter(unsigned int parameterIndex) {
    (void)parameterIndex;
}

int ApiShim::parameterIsLocked(unsigned int parameterIndex) {
    (void)parameterIndex;
    return 0;
}

void ApiShim::setPixel(int x, int y, uint8_t color) {
    static int pixelCallCount = 0;
    if (pixelCallCount++ < 5) {
        std::cout << "[ApiShim] setPixel called: (" << x << "," << y << ") = " << (int)color << ", call " << pixelCallCount << std::endl;
    }
    
    state_.display.setPixel(x, y, color);
    
    // Sync with NT_screen buffer
    if (x >= 0 && x < 256 && y >= 0 && y < 64) {
        int byte_idx = y * 128 + x / 2;
        color = color & 0x0F;
        
        if (x & 1) {
            // Odd x: low nibble
            NT_screen[byte_idx] = (NT_screen[byte_idx] & 0xF0) | color;
        } else {
            // Even x: high nibble  
            NT_screen[byte_idx] = (NT_screen[byte_idx] & 0x0F) | (color << 4);
        }
    }
}

void ApiShim::drawLine(int x0, int y0, int x1, int y1, int colour) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        setPixel(x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void ApiShim::drawCircle(int cx, int cy, int radius, int colour, bool filled) {
    // Midpoint circle algorithm
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        if (filled) {
            // Draw horizontal lines for filled circle
            for (int i = -x; i <= x; i++) {
                setPixel(cx + i, cy + y, colour);
                setPixel(cx + i, cy - y, colour);
            }
            for (int i = -y; i <= y; i++) {
                setPixel(cx + i, cy + x, colour);
                setPixel(cx + i, cy - x, colour);
            }
        } else {
            // Draw circle outline using 8-way symmetry
            setPixel(cx + x, cy + y, colour);
            setPixel(cx + y, cy + x, colour);
            setPixel(cx - y, cy + x, colour);
            setPixel(cx - x, cy + y, colour);
            setPixel(cx - x, cy - y, colour);
            setPixel(cx - y, cy - x, colour);
            setPixel(cx + y, cy - x, colour);
            setPixel(cx + x, cy - y, colour);
        }
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void ApiShim::drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size) {
    static int textCallCount = 0;
    if (textCallCount++ < 3) {
        std::cout << "[ApiShim] NT_drawText called: '" << (str ? str : "NULL") << "' at (" << x << "," << y << "), call " << textCallCount << std::endl;
    }
    
    if (!str) return;
    
    int text_width = strlen(str) * getCharWidth('W', size);
    int start_x = x;
    
    // Apply alignment
    switch (align) {
        case kNT_textCentre:
            start_x = x - text_width / 2;
            break;
        case kNT_textRight:
            start_x = x - text_width;
            break;
        case kNT_textLeft:
        default:
            start_x = x;
            break;
    }
    
    // Draw each character
    int char_x = start_x;
    for (const char* c = str; *c; c++) {
        drawChar(char_x, y, *c, size, colour);
        char_x += getCharWidth(*c, size);
    }
}

void ApiShim::drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour) {
    switch (shape) {
        case kNT_point:
            setPixel(x0, y0, colour);
            break;
            
        case kNT_line:
            drawLine(x0, y0, x1, y1, colour);
            break;
            
        case kNT_box: {
            // Unfilled rectangle (box)
            drawLine(x0, y0, x1, y0, colour); // Top
            drawLine(x1, y0, x1, y1, colour); // Right
            drawLine(x1, y1, x0, y1, colour); // Bottom
            drawLine(x0, y1, x0, y0, colour); // Left
            break;
        }
        
        case kNT_rectangle: {
            // Filled rectangle
            int minX = std::min(x0, x1);
            int maxX = std::max(x0, x1);
            int minY = std::min(y0, y1);
            int maxY = std::max(y0, y1);
            
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    setPixel(x, y, colour);
                }
            }
            break;
        }
        
        case kNT_circle: {
            // Unfilled circle - use center and radius from coordinates
            int cx = (x0 + x1) / 2;
            int cy = (y0 + y1) / 2;
            int radius = std::min(abs(x1 - x0), abs(y1 - y0)) / 2;
            drawCircle(cx, cy, radius, colour, false);
            break;
        }
        
        default:
            // Unknown shape - draw a point
            setPixel(x0, y0, colour);
            break;
    }
}

void ApiShim::drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour) {
    // For now, convert to integer coordinates (basic antialiasing would be added later)
    int int_colour = (int)(colour * 15.0f / 15.0f); // Scale float color to 4-bit
    int_colour = std::max(0, std::min(15, int_colour));
    
    drawShapeI(shape, (int)round(x0), (int)round(y0), (int)round(x1), (int)round(y1), int_colour);
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

void ApiShim::drawChar(int x, int y, char c, _NT_textSize size, int colour) {
    // Basic 5x7 bitmap font (simplified Tom Thumb style)
    static const uint8_t font5x7[95][7] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
        {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00}, // !
        {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
        {0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00, 0x00}, // #
        {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}, // $
        {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}, // %
        {0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D}, // &
        {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
        {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}, // (
        {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}, // )
        {0x00, 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00}, // *
        {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}, // +
        {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08}, // ,
        {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // -
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00}, // .
        {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00}, // /
        {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
        {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
        {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // 3
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
        {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
        {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
        {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9
        {0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x08}, // :
        {0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x08}, // ;
        {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}, // <
        {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00}, // =
        {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}, // >
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, // ?
        {0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0E}, // @
        {0x0E, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11}, // A
        {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
        {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // C
        {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, // D
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // F
        {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
        {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
        {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // I
        {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, // J
        {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
        {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
        {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, // N
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
        {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
        {0x0E, 0x11, 0x11, 0x15, 0x12, 0x0E, 0x01}, // Q
        {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
        {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // S
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // V
        {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, // W
        {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
        {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04}, // Y
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z
        {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}, // [
        {0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00}, // backslash
        {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}, // ]
        {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00}, // ^
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}, // _
        {0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}, // `
        {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F}, // a
        {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E}, // b
        {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E}, // c
        {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F}, // d
        {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E}, // e
        {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08}, // f
        {0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E}, // g
        {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11}, // h
        {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E}, // i
        {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C}, // j
        {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12}, // k
        {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // l
        {0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11}, // m
        {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11}, // n
        {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E}, // o
        {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10}, // p
        {0x00, 0x00, 0x0D, 0x13, 0x0F, 0x01, 0x01}, // q
        {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10}, // r
        {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E}, // s
        {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06}, // t
        {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D}, // u
        {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04}, // v
        {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A}, // w
        {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11}, // x
        {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E}, // y
        {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F}, // z
        {0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02}, // {
        {0x04, 0x04, 0x04, 0x00, 0x04, 0x04, 0x04}, // |
        {0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08}, // }
        {0x00, 0x00, 0x00, 0x0C, 0x12, 0x06, 0x00}  // ~
    };
    
    int char_width = getCharWidth(c, size);
    int char_height = getTextHeight(size);
    
    if (c >= ' ' && c < ' ' + 95) {
        const uint8_t* bitmap = font5x7[c - ' '];
        
        for (int row = 0; row < 7 && row < char_height; row++) {
            uint8_t line = bitmap[row];
            for (int col = 0; col < 5 && col < char_width; col++) {
                if (line & (1 << (4 - col))) {
                    setPixel(x + col, y + row, colour);
                }
            }
        }
    }
}

int ApiShim::getCharWidth(char c, _NT_textSize size) {
    (void)c;  // For now, all characters have same width
    switch (size) {
        case kNT_textTiny: return 4;    // 3x5 tiny font  
        case kNT_textNormal: return 6;  // 5x7 normal font
        case kNT_textLarge: return 12;  // Large font
        default: return 6;
    }
}

int ApiShim::getTextHeight(_NT_textSize size) {
    switch (size) {
        case kNT_textTiny: return 5;    // 3x5 tiny font
        case kNT_textNormal: return 7;  // 5x7 normal font  
        case kNT_textLarge: return 21;  // Large font
        default: return 7;
    }
}