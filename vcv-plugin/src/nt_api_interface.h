#ifndef _NT_API_INTERFACE_H
#define _NT_API_INTERFACE_H

#include "../../emulator/include/distingnt/api.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * API version for the function pointer table interface
 */
#define NT_API_VERSION 1

/*
 * Function pointer table structure that provides explicit API access
 * This approach avoids macOS dynamic symbol resolution issues by providing
 * direct function pointers instead of relying on global symbol lookup.
 */
typedef struct {
    // API version for compatibility checking
    uint32_t version;
    
    // Display buffer access
    uint8_t* (*getScreenBuffer)(void);
    
    // Drawing functions
    void (*drawText)(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size);
    void (*drawShapeI)(_NT_shape shape, int x0, int y0, int x1, int y1, int colour);
    void (*drawShapeF)(_NT_shape shape, float x0, float y0, float x1, float y1, float colour);
    
    // String formatting helpers
    int (*intToString)(char* buffer, int32_t value);
    int (*floatToString)(char* buffer, float value, int decimalPlaces);
    
    // Parameter manipulation
    int32_t (*algorithmIndex)(const _NT_algorithm* algorithm);
    void (*setParameterFromAudio)(uint32_t algorithmIndex, uint32_t parameter, int16_t value);
    void (*setParameterFromUi)(uint32_t algorithmIndex, uint32_t parameter, int16_t value);
    uint32_t (*parameterOffset)(void);
    
    // MIDI functions
    void (*sendMidiByte)(uint32_t destination, uint8_t b0);
    void (*sendMidi2ByteMessage)(uint32_t destination, uint8_t b0, uint8_t b1);
    void (*sendMidi3ByteMessage)(uint32_t destination, uint8_t b0, uint8_t b1, uint8_t b2);
    void (*sendMidiSysEx)(uint32_t destination, const uint8_t* data, uint32_t count, bool end);
    
    // Utility functions
    uint32_t (*getCpuCycleCount)(void);
    void (*setParameterRange)(_NT_parameter* ptr, float init, float min, float max, float step);
    
    // Global data access
    const _NT_globals* globals;
    
} NT_API_Interface;

/*
 * Function signature for plugins to receive the API interface
 * This function should be implemented by NT plugins that want to use the API table
 */
typedef void (*NT_setAPI_func)(const NT_API_Interface* api);

#ifdef __cplusplus
}
#endif

#endif // _NT_API_INTERFACE_H