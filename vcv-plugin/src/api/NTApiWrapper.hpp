#pragma once

#include "../nt_api_interface.h"
#include "../../../emulator/src/core/fonts.h"
#include "../json_bridge.h"
#include <memory>

namespace NTApi {
    // Helper functions (internal use)
    void setNTPixel(int x, int y, int colour);
    void drawNTLine(int x0, int y0, int x1, int y1, int colour);
    
    // Thread-local storage management for JSON bridge
    void setCurrentJsonParse(std::unique_ptr<JsonParseBridge> bridge);
    void clearCurrentJsonParse();
    JsonParseBridge* getCurrentJsonParse();
    void setCurrentJsonStream(std::unique_ptr<JsonStreamBridge> bridge);
    void clearCurrentJsonStream();
    JsonStreamBridge* getCurrentJsonStream();
}

// External C API functions (for plugin compatibility)
    extern "C" {
        // Drawing API
        __attribute__((visibility("default"))) void NT_drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size);
        __attribute__((visibility("default"))) void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour);
        __attribute__((visibility("default"))) void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour);
        
        // Algorithm/Parameter API
        __attribute__((visibility("default"))) int32_t NT_algorithmIndex(const _NT_algorithm* algorithm);
        __attribute__((visibility("default"))) uint32_t NT_parameterOffset(void);
        __attribute__((visibility("default"))) void NT_setParameterFromUi(uint32_t algorithmIndex, uint32_t parameter, int16_t value);
        __attribute__((visibility("default"))) void NT_setParameterFromAudio(uint32_t algorithmIndex, uint32_t parameter, int16_t value);
        __attribute__((visibility("default"))) uint32_t NT_getCpuCycleCount(void);
        __attribute__((visibility("default"))) void NT_setParameterRange(_NT_parameter* ptr, float init, float min, float max, float step);
        
        // String utilities
        __attribute__((visibility("default"))) int NT_intToString(char* buffer, int32_t value);
        __attribute__((visibility("default"))) int NT_floatToString(char* buffer, float value, int decimalPlaces);
        
        // JSON Bridge functions (mangled C++ symbols for compatibility)
        __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse4boolERb(void* self, bool& value);
        __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse6numberERf(void* self, float& value);
        __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse6numberERi(void* self, int& value);
        __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse6stringERNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE(void* self, std::string& str);
        __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse7booleanERb(void* self, bool& value);
        __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse9matchNameEPKc(void* self, const char* name);
        
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10addBooleanEb(void* self, bool value);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10closeArrayEv(void* self);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10openObjectEv(void* self);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream11closeObjectEv(void* self);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream13addMemberNameEPKc(void* self, const char* name);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream7addNullEv(void* self);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addFourCCEj(void* self, uint32_t fourcc);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addNumberEf(void* self, float value);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addNumberEi(void* self, int value);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addStringEPKc(void* self, const char* str);
        __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9openArrayEv(void* self);
    }