#include "NTApiWrapper.hpp"
#include "VirtualSdCard.hpp"
#include <logger.hpp>
#include <cstdio>
#include <cstring>
#include <thread>

// External NT_screen buffer
extern uint8_t NT_screen[128 * 64];

// External VCV-specific drawText function
extern "C" void vcv_drawText(int x, int y, const char* text, NT::FontType font, int color);

namespace NTApi {
    // Thread-local storage for JSON bridge instances
    thread_local std::unique_ptr<JsonStreamBridge> g_currentStream;
    thread_local std::unique_ptr<JsonParseBridge> g_currentParse;
    
    // Thread-local storage management functions
    void setCurrentJsonParse(std::unique_ptr<JsonParseBridge> bridge) {
        if (bridge) {
            g_currentParse = std::move(bridge);
            INFO("setCurrentJsonParse: Bridge set successfully, address: %p", g_currentParse.get());
        } else {
            WARN("setCurrentJsonParse: Attempted to set null bridge");
        }
    }
    
    void clearCurrentJsonParse() {
        if (g_currentParse) {
            INFO("clearCurrentJsonParse: Clearing bridge at address: %p", g_currentParse.get());
            g_currentParse.reset();
        }
    }
    
    JsonParseBridge* getCurrentJsonParse() {
        return g_currentParse.get();
    }
    
    void setCurrentJsonStream(std::unique_ptr<JsonStreamBridge> bridge) {
        if (bridge) {
            g_currentStream = std::move(bridge);
            INFO("setCurrentJsonStream: Bridge set successfully, address: %p", g_currentStream.get());
        } else {
            WARN("setCurrentJsonStream: Attempted to set null bridge");
        }
    }
    
    void clearCurrentJsonStream() {
        if (g_currentStream) {
            INFO("clearCurrentJsonStream: Clearing bridge at address: %p", g_currentStream.get());
            g_currentStream.reset();
        }
    }
    
    JsonStreamBridge* getCurrentJsonStream() {
        return g_currentStream.get();
    }
    
    // Coordinate clipping constants
    static constexpr int SCREEN_WIDTH = 256;
    static constexpr int SCREEN_HEIGHT = 64;
    
    // Clip coordinates to screen bounds
    void clipPoint(int& x, int& y) {
        x = std::max(0, std::min(SCREEN_WIDTH - 1, x));
        y = std::max(0, std::min(SCREEN_HEIGHT - 1, y));
    }
    
    // Cohen-Sutherland line clipping outcodes
    enum OutCode {
        INSIDE = 0,  // 0000
        LEFT = 1,    // 0001
        RIGHT = 2,   // 0010
        BOTTOM = 4,  // 0100
        TOP = 8      // 1000
    };
    
    // Compute outcode for point (x, y)
    int computeOutCode(int x, int y) {
        int code = INSIDE;
        if (x < 0) code |= LEFT;
        else if (x >= SCREEN_WIDTH) code |= RIGHT;
        if (y < 0) code |= BOTTOM;
        else if (y >= SCREEN_HEIGHT) code |= TOP;
        return code;
    }
    
    // Cohen-Sutherland line clipping algorithm
    bool clipLine(int& x0, int& y0, int& x1, int& y1) {
        int outcode0 = computeOutCode(x0, y0);
        int outcode1 = computeOutCode(x1, y1);
        bool accept = false;
        
        while (true) {
            if (!(outcode0 | outcode1)) {
                // Both endpoints inside - trivially accept
                accept = true;
                break;
            } else if (outcode0 & outcode1) {
                // Both endpoints outside same region - trivially reject
                break;
            } else {
                // Failed both tests, so calculate the line segment to clip
                int outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;
                
                // Find intersection point
                int x, y;
                if (outcodeOut & TOP) {
                    x = x0 + (x1 - x0) * (SCREEN_HEIGHT - 1 - y0) / (y1 - y0);
                    y = SCREEN_HEIGHT - 1;
                } else if (outcodeOut & BOTTOM) {
                    x = x0 + (x1 - x0) * (0 - y0) / (y1 - y0);
                    y = 0;
                } else if (outcodeOut & RIGHT) {
                    y = y0 + (y1 - y0) * (SCREEN_WIDTH - 1 - x0) / (x1 - x0);
                    x = SCREEN_WIDTH - 1;
                } else if (outcodeOut & LEFT) {
                    y = y0 + (y1 - y0) * (0 - x0) / (x1 - x0);
                    x = 0;
                }
                
                // Update endpoint and outcode
                if (outcodeOut == outcode0) {
                    x0 = x;
                    y0 = y;
                    outcode0 = computeOutCode(x0, y0);
                } else {
                    x1 = x;
                    y1 = y;
                    outcode1 = computeOutCode(x1, y1);
                }
            }
        }
        
        return accept;
    }
    
    // Helper function to set a single pixel in NT_screen buffer with clipping
    void setNTPixel(int x, int y, int colour) {
        // Clip coordinates to screen bounds
        if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
            return; // Out of bounds - just return without drawing
        }
        
        int byte_idx = y * 128 + x / 2;
        colour = colour & 0x0F;  // Ensure 4-bit value
        
        if (x & 1) {
            // Odd x: low nibble (bits 0-3)
            NT_screen[byte_idx] = (NT_screen[byte_idx] & 0xF0) | colour;
        } else {
            // Even x: high nibble (bits 4-7)
            NT_screen[byte_idx] = (NT_screen[byte_idx] & 0x0F) | (colour << 4);
        }
    }
    
    // Bresenham's line algorithm with clipping
    void drawNTLine(int x0, int y0, int x1, int y1, int colour) {
        // First clip the line to screen bounds
        if (!clipLine(x0, y0, x1, y1)) {
            return; // Line is completely outside screen bounds
        }
        
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        
        while (true) {
            setNTPixel(x0, y0, colour);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    }
    
    // Helper functions implementation stays in namespace
}

// Global wrapper functions for backward compatibility
JsonParseBridge* getCurrentJsonParse() {
    return NTApi::getCurrentJsonParse();
}

void setCurrentJsonParse(std::unique_ptr<JsonParseBridge> bridge) {
    NTApi::setCurrentJsonParse(std::move(bridge));
}

void clearCurrentJsonParse() {
    NTApi::clearCurrentJsonParse();
}

JsonStreamBridge* getCurrentJsonStream() {
    return NTApi::getCurrentJsonStream();
}

void setCurrentJsonStream(std::unique_ptr<JsonStreamBridge> bridge) {
    NTApi::setCurrentJsonStream(std::move(bridge));
}

void clearCurrentJsonStream() {
    NTApi::clearCurrentJsonStream();
}

extern "C" {
    // JSON bridge constructor/destructor implementations (dummy implementations for plugin compatibility)
    __attribute__((visibility("default"))) void* _ZN14_NT_jsonStreamC1EPv(void* refCon) {
        // Return a dummy pointer - the actual implementation uses thread_local g_currentStream
        return reinterpret_cast<void*>(0x1);  // Non-null dummy pointer
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStreamD1Ev(void* self) {
        // Nothing to do - thread_local handles cleanup
    }
    
    __attribute__((visibility("default"))) void* _ZN13_NT_jsonParseC1EPvi(void* refCon, int idx) {
        // Return a dummy pointer - the actual implementation uses thread_local g_currentParse
        return reinterpret_cast<void*>(0x2);  // Non-null dummy pointer
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParseD1Ev(void* self) {
        // Nothing to do - thread_local handles cleanup
    }

    // C API functions with direct implementations
    __attribute__((visibility("default"))) void NT_drawText(int x, int y, const char* str, int colour, _NT_textAlignment align, _NT_textSize size) {
        static int callCount = 0;
        if (callCount++ < 5) {
            INFO("VCV NT_drawText called: '%s' at (%d,%d), colour=%d, call %d", str ? str : "NULL", x, y, colour, callCount);
        }
        
        if (!str || x < 0 || x >= 256 || y < 0 || y >= 64) return;
        
        // Convert NT text size to FontType
        NT::FontType fontType;
        switch (size) {
            case kNT_textTiny: fontType = NT::FontType::TINY; break;
            case kNT_textLarge: fontType = NT::FontType::LARGE; break;
            case kNT_textNormal:
            default: fontType = NT::FontType::NORMAL; break;
        }
        
        // Get accurate text width using shared font system
        int text_width = NT::getTextWidth(str, fontType);
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
        
        // Use unified font rendering system with color
        vcv_drawText(start_x, y, str, fontType, colour);
    }
    
    __attribute__((visibility("default"))) void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour) {
        static int shapeCallCount = 0;
        if (shapeCallCount++ < 10) {
            INFO("VCV NT_drawShapeI called: shape=%d, (%d,%d) to (%d,%d), color=%d, call %d", 
                 (int)shape, x0, y0, x1, y1, colour, shapeCallCount);
        }
        
        switch (shape) {
            case kNT_point:
                NTApi::setNTPixel(x0, y0, colour);
                break;
                
            case kNT_line:
                NTApi::drawNTLine(x0, y0, x1, y1, colour);
                break;
                
            case kNT_box: {
                // Unfilled rectangle (box outline)
                NTApi::drawNTLine(x0, y0, x1, y0, colour); // Top
                NTApi::drawNTLine(x1, y0, x1, y1, colour); // Right
                NTApi::drawNTLine(x1, y1, x0, y1, colour); // Bottom
                NTApi::drawNTLine(x0, y1, x0, y0, colour); // Left
                break;
            }
            
            case kNT_rectangle: {
                // Filled rectangle with clipping
                // Clip rectangle bounds to screen
                int clipX0 = std::max(0, std::min(x0, x1));
                int clipY0 = std::max(0, std::min(y0, y1));
                int clipX1 = std::min(NTApi::SCREEN_WIDTH - 1, std::max(x0, x1));
                int clipY1 = std::min(NTApi::SCREEN_HEIGHT - 1, std::max(y0, y1));
                
                // Draw horizontal lines for filled rectangle
                for (int y = clipY0; y <= clipY1; y++) {
                    NTApi::drawNTLine(clipX0, y, clipX1, y, colour);
                }
                break;
            }
            
            default:
                // Unknown shape - draw a point
                NTApi::setNTPixel(x0, y0, colour);
                break;
        }
    }
    
    __attribute__((visibility("default"))) void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour) {
        NT_drawShapeI(shape, (int)x0, (int)y0, (int)x1, (int)y1, (int)colour);
    }
    
    __attribute__((visibility("default"))) int32_t NT_algorithmIndex(const _NT_algorithm* algorithm) {
        // For now, return 0 as we only support one algorithm at a time in VCV
        return 0;
    }
    
    __attribute__((visibility("default"))) uint32_t NT_parameterOffset(void) {
        // For now, return 0 as VCV handles parameter indexing differently
        return 0;
    }
    
    // Forward declaration
    extern "C" void emulatorHandleSetParameterFromUi(uint32_t parameter, int16_t value);
    
    __attribute__((visibility("default"))) void NT_setParameterFromUi(uint32_t algorithmIndex, uint32_t parameter, int16_t value) {
        INFO("NT_setParameterFromUi called: alg=%d, param=%d, value=%d", algorithmIndex, parameter, value);
        emulatorHandleSetParameterFromUi(parameter, value);
    }
    
    __attribute__((visibility("default"))) void NT_setParameterFromAudio(uint32_t algorithmIndex, uint32_t parameter, int16_t value) {
        // TODO: Connect to VCV parameter system
        // This should update VCV module parameters from audio thread
        INFO("NT_setParameterFromAudio called: alg=%d, param=%d, value=%d", algorithmIndex, parameter, value);
    }

    // Forward declaration for grayed out handler
    extern "C" void emulatorHandleSetParameterGrayedOut(uint32_t parameter, bool gray);

    __attribute__((visibility("default"))) void NT_setParameterGrayedOut(uint32_t algorithmIndex, uint32_t parameter, bool gray) {
        INFO("NT_setParameterGrayedOut called: alg=%d, param=%d, gray=%d", algorithmIndex, parameter, gray ? 1 : 0);
        emulatorHandleSetParameterGrayedOut(parameter, gray);
    }

    __attribute__((visibility("default"))) uint32_t NT_getCpuCycleCount(void) {
        // Simple implementation for profiling - could use mach_absolute_time() on macOS
        return 0;
    }
    
    __attribute__((visibility("default"))) void NT_setParameterRange(_NT_parameter* ptr, float init, float min, float max, float step) {
        if (!ptr) return;
        
        // Convert float ranges to int16_t
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
    
    __attribute__((visibility("default"))) int NT_intToString(char* buffer, int32_t value) {
        if (!buffer) return 0;
        return snprintf(buffer, 32, "%d", value);
    }
    
    __attribute__((visibility("default"))) int NT_floatToString(char* buffer, float value, int decimalPlaces) {
        if (!buffer) return 0;
        char format[16];
        snprintf(format, sizeof(format), "%%.%df", decimalPlaces);
        return snprintf(buffer, 32, format, value);
    }
    
    // JSON Bridge functions (mangled C++ symbols for compatibility)
    __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse4boolERb(void* self, bool& value) {
        JsonParseBridge* bridge = NTApi::getCurrentJsonParse();
        if (bridge) {
            return bridge->boolean(value);
        }
        value = false;
        return false;
    }
    
    __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse6numberERf(void* self, float& value) {
        JsonParseBridge* bridge = NTApi::getCurrentJsonParse();
        if (bridge) {
            return bridge->number(value);
        }
        value = 0.0f;
        return false;
    }
    
    __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse6numberERi(void* self, int& value) {
        JsonParseBridge* bridge = NTApi::getCurrentJsonParse();
        if (bridge) {
            return bridge->number(value);
        }
        value = 0;
        return false;
    }
    
    __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse6stringERNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE(void* self, std::string& str) {
        JsonParseBridge* bridge = NTApi::getCurrentJsonParse();
        if (bridge) {
            const char* cstr = nullptr;
            bool result = bridge->string(cstr);
            if (result && cstr) {
                str = cstr;
            } else {
                str = "";
            }
            return result;
        }
        str = "";
        return false;
    }
    
    __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse7booleanERb(void* self, bool& value) {
        JsonParseBridge* bridge = NTApi::getCurrentJsonParse();
        if (bridge) {
            return bridge->boolean(value);
        }
        value = false;
        return false;
    }
    
    __attribute__((visibility("default"))) bool _ZN13_NT_jsonParse9matchNameEPKc(void* self, const char* name) {
        JsonParseBridge* bridge = NTApi::getCurrentJsonParse();
        if (bridge) {
            return bridge->matchName(name);
        }
        return false;
    }
    
    // JSON Stream functions
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10addBooleanEb(void* self, bool value) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addBoolean(value);
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10closeArrayEv(void* self) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->closeArray();
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10openObjectEv(void* self) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->openObject();
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream11closeObjectEv(void* self) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->closeObject();
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream13addMemberNameEPKc(void* self, const char* name) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addMemberName(name);
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream7addNullEv(void* self) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addNull();
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addFourCCEj(void* self, uint32_t fourcc) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addFourCC(fourcc);
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addNumberEf(void* self, float value) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addNumber(value);
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addNumberEi(void* self, int value) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addNumber(value);
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addStringEPKc(void* self, const char* str) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->addString(str);
        }
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9openArrayEv(void* self) {
        JsonStreamBridge* bridge = NTApi::getCurrentJsonStream();
        if (bridge) {
            bridge->openArray();
        }
    }

    // WAV API functions
    __attribute__((visibility("default"))) bool NT_isSdCardMounted(void) {
        return VirtualSdCard::getInstance().isMounted();
    }

    __attribute__((visibility("default"))) uint32_t NT_getNumSampleFolders(void) {
        return VirtualSdCard::getInstance().getNumSampleFolders();
    }

    __attribute__((visibility("default"))) void NT_getSampleFolderInfo(uint32_t folder, _NT_wavFolderInfo& info) {
        VirtualSdCard::getInstance().getSampleFolderInfo(folder, info);
    }

    __attribute__((visibility("default"))) void NT_getSampleFileInfo(uint32_t folder, uint32_t sample, _NT_wavInfo& info) {
        VirtualSdCard::getInstance().getSampleFileInfo(folder, sample, info);
    }

    __attribute__((visibility("default"))) bool NT_readSampleFrames(const _NT_wavRequest& request) {
        return VirtualSdCard::getInstance().readSampleFrames(request);
    }
}