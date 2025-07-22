#include <distingnt/api.h>
#include <iostream>
#include <cstring>
#include <cstdlib>

// Screen buffer (256x64, 2 pixels per byte)
uint8_t NT_screen[128 * 64];

// Global NT structure
float workBuffer[4096];  // Work buffer for plugins
const _NT_globals NT_globals = {
    .sampleRate = 96000,                    // 96 kHz sample rate
    .maxFramesPerStep = 4,                  // 4 frames max per step
    .workBuffer = workBuffer,               // Work buffer pointer
    .workBufferSizeBytes = sizeof(workBuffer) // Buffer size
};

// Stub implementations for testing - the emulator will provide the real ones
extern "C" {
    void NT_drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size) {
        std::cout << "NT_drawText(" << x << ", " << y << ", \"" << str << "\", " << colour << ", " << align << ", " << size << ")" << std::endl;
    }
    
    void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour) {
        std::cout << "NT_drawShapeI(" << shape << ", " << x0 << ", " << y0 << ", " << x1 << ", " << y1 << ", " << colour << ")" << std::endl;
    }
    
    void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour) {
        std::cout << "NT_drawShapeF(" << shape << ", " << x0 << ", " << y0 << ", " << x1 << ", " << y1 << ", " << colour << ")" << std::endl;
    }
    
    void NT_getDisplayDimensions(unsigned int* width, unsigned int* height) {
        if (width) *width = 256;
        if (height) *height = 64;
    }
    
    float NT_getSampleRate() { return 96000.0f; }
    unsigned int NT_getSamplesPerBlock() { return 4; }
    void NT_log(const char* text) { std::cout << "[Plugin] " << text << std::endl; }
    
    // Stub out all other API functions
    void NT_parameterChanged(unsigned int) {}
    float NT_getParameterValueMapped(unsigned int) { return 0.0f; }
    float NT_getParameterValueMappedNormalised(unsigned int) { return 0.0f; }
    void NT_setParameterValueMapped(unsigned int, float) {}
    void NT_setParameterValueMappedNormalised(unsigned int, float) {}
    void NT_lockParameter(unsigned int) {}
    void NT_unlockParameter(unsigned int) {}
    int NT_parameterIsLocked(unsigned int) { return 0; }
    
    void NT_sendMIDIControllerChange(struct _NT_controllerChange*, enum _NT_midiDestination) {}
    void NT_sendMIDINoteOn(struct _NT_noteOn*, enum _NT_midiDestination) {}
    void NT_sendMIDINoteOff(struct _NT_noteOff*, enum _NT_midiDestination) {}
    void NT_sendMIDIPitchBend(struct _NT_pitchBend*, enum _NT_midiDestination) {}
    void NT_sendMIDIProgramChange(struct _NT_programChange*, enum _NT_midiDestination) {}
    void NT_sendMIDIChannelPressure(struct _NT_channelPressure*, enum _NT_midiDestination) {}
    void NT_sendMIDIPolyKeyPressure(struct _NT_polyKeyPressure*, enum _NT_midiDestination) {}
    void NT_sendMIDISystemExclusive(struct _NT_systemExclusive*, enum _NT_midiDestination) {}
    void NT_sendMIDIClockTick(enum _NT_midiDestination) {}
    void NT_sendMIDIClockStart(enum _NT_midiDestination) {}
    void NT_sendMIDIClockStop(enum _NT_midiDestination) {}
    void NT_sendMIDIClockContinue(enum _NT_midiDestination) {}
    void NT_sendMIDIActiveSense(enum _NT_midiDestination) {}
    void NT_sendMIDISystemReset(enum _NT_midiDestination) {}
    
    float NT_getTemperatureC() { return 25.0f; }
    void NT_copyFromFlash(void* dest, const void* src, unsigned int len) { memcpy(dest, src, len); }
    unsigned int NT_random(unsigned int max) { return max > 0 ? rand() % max : 0; }
    float NT_randomF() { return (float)rand() / RAND_MAX; }
    int NT_strlenUTF8(const char* text) { return strlen(text); }
    int NT_getTextWidthUTF8(const char* text, enum _NT_textSize size) { return strlen(text) * (8 + size * 4); }
    
    // UI and parameter functions
    int32_t NT_algorithmIndex(const _NT_algorithm *algorithm) { (void)algorithm; return 0; }
    uint32_t NT_parameterOffset(void) { return 0; }
    void NT_setParameterFromUi(uint32_t algorithmIndex, uint32_t parameter, int16_t value) { 
        (void)algorithmIndex; (void)parameter; (void)value; 
    }
}