#include "nt_api_interface.h"
#include "../../emulator/include/distingnt/api.h"
#include <cstring>
#include <cstdio>
#include <atomic>

// External NT_screen buffer (defined in DistingNT.cpp)
extern uint8_t NT_screen[128 * 64];

// Global NT_globals structure (we need to populate this)
static _NT_globals g_nt_globals = {
    .sampleRate = 48000,
    .maxFramesPerStep = 256,
    .workBuffer = nullptr,
    .workBufferSizeBytes = 0
};

// Thread-safe access flag for display buffer
static std::atomic<bool> displayDirty{false};

// External reference to current module (forward declared)
struct EmulatorModule;
extern EmulatorModule* g_currentModule;

// Forward declarations of API implementation functions
extern "C" {
    // These are already implemented in DistingNT.cpp
    void NT_drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size);
    void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour);
    void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour);
}

// API implementation functions that need to be connected to the actual VCV implementation

static uint8_t* api_getScreenBuffer(void) {
    return NT_screen;
}

static void api_drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size) {
    NT_drawText(x, y, str, colour, align, size);
    displayDirty.store(true);
}

static void api_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour) {
    NT_drawShapeI(shape, x0, y0, x1, y1, colour);
    displayDirty.store(true);
}

static void api_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour) {
    NT_drawShapeF(shape, x0, y0, x1, y1, colour);
    displayDirty.store(true);
}

static int api_intToString(char* buffer, int32_t value) {
    if (!buffer) return 0;
    return snprintf(buffer, 32, "%d", value);
}

static int api_floatToString(char* buffer, float value, int decimalPlaces) {
    if (!buffer) return 0;
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimalPlaces);
    return snprintf(buffer, 32, format, value);
}

// Stub implementations for parameter/MIDI functions (to be implemented when needed)
static int32_t api_algorithmIndex(const _NT_algorithm* algorithm) {
    // TODO: Implement actual algorithm index lookup
    return -1;
}

static void api_setParameterFromAudio(uint32_t algorithmIndex, uint32_t parameter, int16_t value) {
    // TODO: Connect to VCV parameter system
}

// Forward declaration
extern "C" void emulatorHandleSetParameterFromUi(uint32_t parameter, int16_t value);

static void api_setParameterFromUi(uint32_t algorithmIndex, uint32_t parameter, int16_t value) {
    emulatorHandleSetParameterFromUi(parameter, value);
}

static uint32_t api_parameterOffset(void) {
    // TODO: Implement parameter offset
    return 0;
}

// Forward declarations of NT_* functions (implemented in NtEmu.cpp)
extern "C" {
    void NT_sendMidiByte(uint32_t destination, uint8_t b0);
    void NT_sendMidi2ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1);
    void NT_sendMidi3ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1, uint8_t b2);
    void NT_sendMidiSysEx(uint32_t destination, const uint8_t* data, uint32_t count, bool end);
}

static void api_sendMidiByte(uint32_t destination, uint8_t b0) {
    NT_sendMidiByte(destination, b0);
}

static void api_sendMidi2ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1) {
    NT_sendMidi2ByteMessage(destination, b0, b1);
}

static void api_sendMidi3ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1, uint8_t b2) {
    NT_sendMidi3ByteMessage(destination, b0, b1, b2);
}

static void api_sendMidiSysEx(uint32_t destination, const uint8_t* data, uint32_t count, bool end) {
    NT_sendMidiSysEx(destination, data, count, end);
}

static uint32_t api_getCpuCycleCount(void) {
    // TODO: Implement CPU cycle counting for profiling
    return 0;
}

static void api_setParameterRange(_NT_parameter* ptr, float init, float min, float max, float step) {
    if (!ptr) return;
    
    // Simple implementation - convert float ranges to int16_t
    ptr->min = (int16_t)min;
    ptr->max = (int16_t)max;
    ptr->def = (int16_t)init;
    
    // Determine scaling based on step size
    if (step >= 1.0f) {
        ptr->scaling = kNT_scalingNone;
    } else if (step >= 0.1f) {
        ptr->scaling = kNT_scaling10;
    } else if (step >= 0.01f) {
        ptr->scaling = kNT_scaling100;
    } else {
        ptr->scaling = kNT_scaling1000;
    }
}

// Global API interface instance
static NT_API_Interface g_api_interface = {
    .version = NT_API_VERSION,
    
    // Display buffer access
    .getScreenBuffer = api_getScreenBuffer,
    
    // Drawing functions
    .drawText = api_drawText,
    .drawShapeI = api_drawShapeI,
    .drawShapeF = api_drawShapeF,
    
    // String formatting helpers
    .intToString = api_intToString,
    .floatToString = api_floatToString,
    
    // Parameter manipulation
    .algorithmIndex = api_algorithmIndex,
    .setParameterFromAudio = api_setParameterFromAudio,
    .setParameterFromUi = api_setParameterFromUi,
    .parameterOffset = api_parameterOffset,
    
    // MIDI functions
    .sendMidiByte = api_sendMidiByte,
    .sendMidi2ByteMessage = api_sendMidi2ByteMessage,
    .sendMidi3ByteMessage = api_sendMidi3ByteMessage,
    .sendMidiSysEx = api_sendMidiSysEx,
    
    // Utility functions
    .getCpuCycleCount = api_getCpuCycleCount,
    .setParameterRange = api_setParameterRange,
    
    // Global data access
    .globals = &g_nt_globals
};

// Main API provider function
extern "C" __attribute__((visibility("default"))) const NT_API_Interface* getNT_API(void) {
    return &g_api_interface;
}

// Function to check if display needs updating (can be called from VCV render loop)
extern "C" bool NT_isDisplayDirty(void) {
    return displayDirty.exchange(false);
}

// Function to update globals (called from VCV module)
extern "C" void NT_updateGlobals(uint32_t sampleRate, uint32_t maxFrames, float* workBuffer, uint32_t workBufferSize) {
    g_nt_globals.sampleRate = sampleRate;
    g_nt_globals.maxFramesPerStep = maxFrames;
    g_nt_globals.workBuffer = workBuffer;
    g_nt_globals.workBufferSizeBytes = workBufferSize;
}

// No module registry needed - using simple global approach