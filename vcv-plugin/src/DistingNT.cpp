#include "plugin.hpp"
#include "algorithms/Algorithm.hpp"
#include "dsp/BusSystem.hpp"
#include "EmulatorCore.hpp"
#include "EncoderParamQuantity.hpp"
#include "InfiniteEncoder.hpp"
#include "nt_api_interface.h"
#include "../../emulator/src/core/fonts.h"
// #include "../../emulator/src/core/api_shim.h" // Removed due to memory corruption
#include <componentlibrary.hpp>
#include <osdialog.h>
#include <map>
#include <cstring>

// NT_screen buffer definition for the VCV plugin
__attribute__((visibility("default"))) uint8_t NT_screen[128 * 64];

// NT_globals structure for the VCV plugin
static _NT_globals vcv_nt_globals = {
    .sampleRate = 48000,
    .maxFramesPerStep = 256,
    .workBuffer = nullptr,
    .workBufferSizeBytes = 0
};

__attribute__((visibility("default"))) const _NT_globals NT_globals = vcv_nt_globals;

// Simple C API wrapper functions for plugins to use (minimal implementation)
// External API provider function
extern "C" const NT_API_Interface* getNT_API(void);

extern "C" {

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
        // Call VCV-specific drawText function that accepts color parameter
        extern void vcv_drawText(int x, int y, const char* text, NT::FontType font, int color);
        vcv_drawText(start_x, y, str, fontType, colour);
    }
    
    // Helper function to set a single pixel in NT_screen buffer
    void setNTPixel(int x, int y, int colour) {
        if (x >= 0 && x < 256 && y >= 0 && y < 64) {
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
    }
    
    // Bresenham's line algorithm
    void drawNTLine(int x0, int y0, int x1, int y1, int colour) {
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
    
    __attribute__((visibility("default"))) void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour) {
        static int shapeCallCount = 0;
        if (shapeCallCount++ < 10) {
            INFO("VCV NT_drawShapeI called: shape=%d, (%d,%d) to (%d,%d), color=%d, call %d", 
                 (int)shape, x0, y0, x1, y1, colour, shapeCallCount);
        }
        
        switch (shape) {
            case kNT_point:
                setNTPixel(x0, y0, colour);
                break;
                
            case kNT_line:
                drawNTLine(x0, y0, x1, y1, colour);
                break;
                
            case kNT_box: {
                // Unfilled rectangle (box outline)
                drawNTLine(x0, y0, x1, y0, colour); // Top
                drawNTLine(x1, y0, x1, y1, colour); // Right
                drawNTLine(x1, y1, x0, y1, colour); // Bottom
                drawNTLine(x0, y1, x0, y0, colour); // Left
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
                        setNTPixel(x, y, colour);
                    }
                }
                break;
            }
            
            case kNT_circle: {
                // Unfilled circle - use center and radius from coordinates
                int cx = (x0 + x1) / 2;
                int cy = (y0 + y1) / 2;
                int radius = std::min(abs(x1 - x0), abs(y1 - y0)) / 2;
                
                // Midpoint circle algorithm
                int x = radius;
                int y = 0;
                int err = 0;
                
                while (x >= y) {
                    // Draw circle outline using 8-way symmetry
                    setNTPixel(cx + x, cy + y, colour);
                    setNTPixel(cx + y, cy + x, colour);
                    setNTPixel(cx - y, cy + x, colour);
                    setNTPixel(cx - x, cy + y, colour);
                    setNTPixel(cx - x, cy - y, colour);
                    setNTPixel(cx - y, cy - x, colour);
                    setNTPixel(cx + y, cy - x, colour);
                    setNTPixel(cx + x, cy - y, colour);
                    
                    if (err <= 0) {
                        y += 1;
                        err += 2*y + 1;
                    }
                    if (err > 0) {
                        x -= 1;
                        err -= 2*x + 1;
                    }
                }
                break;
            }
            
            default:
                // Unknown shape - draw a point
                setNTPixel(x0, y0, colour);
                break;
        }
    }
    
    __attribute__((visibility("default"))) void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour) {
        NT_drawShapeI(shape, (int)x0, (int)y0, (int)x1, (int)y1, (int)colour);
    }
    
    // Additional NT_* functions needed by plugins
    __attribute__((visibility("default"))) int32_t NT_algorithmIndex(const _NT_algorithm* algorithm) {
        // TODO: Implement proper algorithm index lookup
        // For now, return 0 as we only support one algorithm at a time in VCV
        return 0;
    }
    
    __attribute__((visibility("default"))) uint32_t NT_parameterOffset(void) {
        // TODO: Implement parameter offset for VCV parameter mapping
        // For now, return 0 as VCV handles parameter indexing differently
        return 0;
    }
    
    __attribute__((visibility("default"))) void NT_setParameterFromUi(uint32_t algorithmIndex, uint32_t parameter, int16_t value) {
        // TODO: Connect to VCV parameter system
        // This should update VCV module parameters when plugins call this function
        INFO("NT_setParameterFromUi called: alg=%d, param=%d, value=%d", algorithmIndex, parameter, value);
    }
    
    __attribute__((visibility("default"))) void NT_setParameterFromAudio(uint32_t algorithmIndex, uint32_t parameter, int16_t value) {
        // TODO: Connect to VCV parameter system  
        // This should update VCV module parameters from audio thread
        INFO("NT_setParameterFromAudio called: alg=%d, param=%d, value=%d", algorithmIndex, parameter, value);
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
    
    // MIDI functions (stubs for now)
    __attribute__((visibility("default"))) void NT_sendMidiByte(uint32_t destination, uint8_t b0) {
        // TODO: Connect to VCV MIDI system
    }
    
    __attribute__((visibility("default"))) void NT_sendMidi2ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1) {
        // TODO: Connect to VCV MIDI system
    }
    
    __attribute__((visibility("default"))) void NT_sendMidi3ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1, uint8_t b2) {
        // TODO: Connect to VCV MIDI system
    }
    
    __attribute__((visibility("default"))) void NT_sendMidiSysEx(uint32_t destination, const uint8_t* data, uint32_t count, bool end) {
        // TODO: Connect to VCV MIDI system
    }
}

// Include the serialisation header to get class definitions
#include "../../emulator/include/distingnt/serialisation.h"

// Minimal stub implementations for JSON classes
// These are needed by plugins that use serialisation but won't be called in VCV context
extern "C" {
    // _NT_jsonParse stub methods
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse10skipMemberEv() {
        // _NT_jsonParse::skipMember() stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse21numberOfArrayElementsERi() {
        // _NT_jsonParse::numberOfArrayElements(int&) stub  
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse21numberOfObjectMembersERi() {
        // _NT_jsonParse::numberOfObjectMembers(int&) stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse4nullEv() {
        // _NT_jsonParse::null() stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse6numberERf() {
        // _NT_jsonParse::number(float&) stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse6numberERi() {
        // _NT_jsonParse::number(int&) stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse6stringERPKc() {
        // _NT_jsonParse::string(const char*&) stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse7booleanERb() {
        // _NT_jsonParse::boolean(bool&) stub
    }
    
    __attribute__((visibility("default"))) void _ZN13_NT_jsonParse9matchNameEPKc() {
        // _NT_jsonParse::matchName(const char*) stub
    }
    
    // _NT_jsonStream stub methods
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10addBooleanEb() {
        // _NT_jsonStream::addBoolean(bool) stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10closeArrayEv() {
        // _NT_jsonStream::closeArray() stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream10openObjectEv() {
        // _NT_jsonStream::openObject() stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream11closeObjectEv() {
        // _NT_jsonStream::closeObject() stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream13addMemberNameEPKc() {
        // _NT_jsonStream::addMemberName(const char*) stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream7addNullEv() {
        // _NT_jsonStream::addNull() stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addFourCCEj() {
        // _NT_jsonStream::addFourCC(uint32_t) stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addNumberEf() {
        // _NT_jsonStream::addNumber(float) stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addNumberEi() {
        // _NT_jsonStream::addNumber(int) stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9addStringEPKc() {
        // _NT_jsonStream::addString(const char*) stub
    }
    
    __attribute__((visibility("default"))) void _ZN14_NT_jsonStream9openArrayEv() {
        // _NT_jsonStream::openArray() stub
    }
}

// For plugin loading
#ifdef ARCH_WIN
    #include <windows.h>
    #define PLUGIN_EXT ".dll"
#elif ARCH_MAC
    #include <dlfcn.h>
    #define PLUGIN_EXT ".dylib"
#else
    #include <dlfcn.h>
    #define PLUGIN_EXT ".so"
#endif

// Old API structures for compatibility with simple_gain plugin
struct _NT_memoryRequirements {
    uint32_t memorySize;
    uint32_t numRequirements;
    void* requirements;
};

// Old version of _NT_staticRequirements with memorySize field
struct _NT_staticRequirements_old {
    uint32_t memorySize;
    uint32_t numRequirements;
    void* requirements;
};

// Forward declaration
struct DistingNT;

// Context-aware encoder parameter quantity
struct ContextAwareEncoderQuantity : EncoderParamQuantity {
    DistingNT* distingModule = nullptr;
    bool isLeftEncoder = true;
    
    std::string getDisplayValueString() override;
};

// Custom output port widget with dynamic tooltips
struct DistingNTOutputPort : PJ301MPort {
    DistingNT* distingModule = nullptr;
    int outputIndex = 0;
    
    void onHover(const HoverEvent& e) override;
};

struct DistingNT : Module {
    enum ParamIds {
        // Pots
        POT_L_PARAM, POT_C_PARAM, POT_R_PARAM,
        
        // Buttons 
        BUTTON_1_PARAM, BUTTON_2_PARAM, BUTTON_3_PARAM, BUTTON_4_PARAM,
        
        // Encoders (simplified for widget placement)
        ENCODER_L_PARAM, ENCODER_R_PARAM,
        ENCODER_L_PRESS_PARAM, ENCODER_R_PRESS_PARAM,
        
        // Algorithm selection
        ALGORITHM_PARAM,
        
        NUM_PARAMS
    };
    
    enum InputIds {
        // 12 inputs (as per hardware specification)
        AUDIO_INPUT_1, AUDIO_INPUT_2, AUDIO_INPUT_3, AUDIO_INPUT_4,
        AUDIO_INPUT_5, AUDIO_INPUT_6, AUDIO_INPUT_7, AUDIO_INPUT_8,
        AUDIO_INPUT_9, AUDIO_INPUT_10, AUDIO_INPUT_11, AUDIO_INPUT_12,
        
        NUM_INPUTS
    };
    
    enum OutputIds {
        // 8 outputs (as per hardware specification)
        AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3, 
        AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
        AUDIO_OUTPUT_7, AUDIO_OUTPUT_8,
        
        NUM_OUTPUTS
    };
    
    enum LightIds {
        BUTTON_1_LIGHT, BUTTON_2_LIGHT, BUTTON_3_LIGHT, BUTTON_4_LIGHT,
        INPUT_LIGHT_1, INPUT_LIGHT_2, INPUT_LIGHT_3, INPUT_LIGHT_4,
        INPUT_LIGHT_5, INPUT_LIGHT_6, INPUT_LIGHT_7, INPUT_LIGHT_8,
        INPUT_LIGHT_9, INPUT_LIGHT_10, INPUT_LIGHT_11, INPUT_LIGHT_12,
        OUTPUT_LIGHT_1, OUTPUT_LIGHT_2, OUTPUT_LIGHT_3, 
        OUTPUT_LIGHT_4, OUTPUT_LIGHT_5, OUTPUT_LIGHT_6,
        OUTPUT_LIGHT_7, OUTPUT_LIGHT_8,
        MIDI_INPUT_LIGHT, MIDI_OUTPUT_LIGHT,
        NUM_LIGHTS
    };

    // Core components
    BusSystem busSystem;
    EmulatorCore emulatorCore;
    
    // MIDI ports
    midi::InputQueue midiInput;
    midi::Output midiOutput;
    
    // MIDI activity lights
    dsp::ClockDivider midiActivityDivider;
    float midiInputLight = 0.f;
    float midiOutputLight = 0.f;
    
    // Note: USB/Breakout routing is handled by hardware, not relevant in VCV emulation
    
    // Plugin loading
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginSharedMemory = nullptr;
    void* pluginInstanceMemory = nullptr;
    std::string pluginPath;
    std::string lastPluginFolder;
    
    // Member variables for two-phase plugin loading with specifications
    std::vector<int32_t> currentSpecifications;
    bool useCustomSpecifications = false;
    std::vector<int32_t> pluginSpecifications; // Persistent storage of loaded plugin specs
    
    // Display notification
    std::string loadingMessage;
    float loadingMessageTimer = 0.f;
    
    // Display state
    bool displayDirty = true;
    
    // Menu system state
    enum MenuMode {
        MENU_OFF,
        MENU_PAGE_SELECT,
        MENU_PARAM_SELECT,
        MENU_VALUE_EDIT
    };
    
    MenuMode menuMode = MENU_OFF;
    int currentPageIndex = 0;
    int currentParamIndex = 0;
    int parameterEditValue = 0;
    
    // Cached parameter data from plugin
    std::vector<_NT_parameterPage> parameterPages;
    std::vector<_NT_parameter> parameters;
    
    // Routing matrix (parameter index -> bus mapping)
    std::array<int16_t, 256> routingMatrix;
    
    // UI interaction tracking
    bool leftEncoderPressed = false;
    bool rightEncoderPressed = false;
    float lastLeftPotValue = 0.5f;
    float lastLeftEncoderValue = 0.f;
    float lastRightEncoderValue = 0.f;
    
    // Encoder state tracking for discrete step detection
    float lastEncoderLParam = 0.f;
    float lastEncoderRParam = 0.f;
    int encoderLSteps = 0;  // Accumulated steps to process
    int encoderRSteps = 0;  // Accumulated steps to process
    
    // Debug data for display
    
    // Bus routing maps
    std::array<int, 28> busInputMap;
    std::array<int, 28> busOutputMap;
    bool routingDirty = false;
    
    // Parameter-based routing structures
    struct InputRouting {
        int paramIndex;      // Which parameter controls this routing
        int busIndex;        // Target bus (0-27)
        bool isCV;           // Audio or CV input
    };
    
    struct OutputRouting {
        int paramIndex;           // Which parameter controls this routing
        int busIndex;             // Source bus (0-27)
        bool isCV;                // Audio or CV output
        bool hasOutputMode;       // Has associated output mode parameter
        int outputModeParamIndex; // Index of output mode parameter
    };
    
    // Maps from parameter index to routing info
    std::map<int, InputRouting> paramInputRouting;
    std::map<int, OutputRouting> paramOutputRouting;
    
    // Sample accumulation for 4-sample processing blocks
    int sampleCounter = 0;
    static constexpr int BLOCK_SIZE = 4;
    
    // Trigger detectors for buttons and encoders
    dsp::SchmittTrigger buttonTriggers[4];
    dsp::SchmittTrigger encoderLPressTrigger, encoderRPressTrigger;
    
    DistingNT() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        // Configure parameters
        configParam(POT_L_PARAM, 0.f, 1.f, 0.5f, "Pot L");
        configParam(POT_C_PARAM, 0.f, 1.f, 0.5f, "Pot C");  
        configParam(POT_R_PARAM, 0.f, 1.f, 0.5f, "Pot R");
        
        configParam(BUTTON_1_PARAM, 0.f, 1.f, 0.f, "Button 1");
        configParam(BUTTON_2_PARAM, 0.f, 1.f, 0.f, "Button 2");
        configParam(BUTTON_3_PARAM, 0.f, 1.f, 0.f, "Button 3");
        configParam(BUTTON_4_PARAM, 0.f, 1.f, 0.f, "Button 4");
        
        // Configure infinite encoders with special handling for infinite rotation
        configParam(ENCODER_L_PARAM, -INFINITY, INFINITY, 0.f, "Left Encoder");
        configParam(ENCODER_R_PARAM, -INFINITY, INFINITY, 0.f, "Right Encoder");
        
        // Disable snapping so encoders don't snap back to center
        getParamQuantity(ENCODER_L_PARAM)->snapEnabled = false;
        getParamQuantity(ENCODER_R_PARAM)->snapEnabled = false;
        
        // Replace with context-aware encoder quantities
        paramQuantities[ENCODER_L_PARAM] = new ContextAwareEncoderQuantity();
        paramQuantities[ENCODER_L_PARAM]->module = this;
        paramQuantities[ENCODER_L_PARAM]->paramId = ENCODER_L_PARAM;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_L_PARAM])->distingModule = this;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_L_PARAM])->isLeftEncoder = true;
        
        paramQuantities[ENCODER_R_PARAM] = new ContextAwareEncoderQuantity();
        paramQuantities[ENCODER_R_PARAM]->module = this;
        paramQuantities[ENCODER_R_PARAM]->paramId = ENCODER_R_PARAM;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_R_PARAM])->distingModule = this;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_R_PARAM])->isLeftEncoder = false;
        
        configParam(ENCODER_L_PRESS_PARAM, 0.f, 1.f, 0.f, "Encoder L Press");
        configParam(ENCODER_R_PRESS_PARAM, 0.f, 1.f, 0.f, "Encoder R Press");
        
        configParam(ALGORITHM_PARAM, 0.f, 1.f, 0.f, "Algorithm");
        
        // Configure 12 inputs
        for (int i = 0; i < 12; i++) {
            configInput(AUDIO_INPUT_1 + i, string::f("Input %d", i + 1));
        }
        
        // Configure 8 outputs  
        for (int i = 0; i < 8; i++) {
            configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
        }
        
        // Initialize bus system
        busSystem.init();
        
        // Initialize emulator core
        emulatorCore.initialize(APP->engine->getSampleRate());
        
        // Initialize routing matrices
        routingMatrix.fill(0);
        busInputMap.fill(-1);
        busOutputMap.fill(-1);
        
        // Initialize encoder step tracking to current parameter values
        // (This ensures first delta calculation works correctly)
        encoderLSteps = (int)params[ENCODER_L_PARAM].getValue();
        encoderRSteps = (int)params[ENCODER_R_PARAM].getValue();
        
        // Configure MIDI activity divider
        midiActivityDivider.setDivision(512);
        
        // Setup MIDI output callback
        setupMidiOutput();
    }
    
    // MIDI setup method
    void setupMidiOutput() {
        // Connect MidiHandler callback to VCV MIDI output
        emulatorCore.getMidiHandler().setMidiOutputCallback(
            [this](const uint8_t* data, size_t size) {
                // Convert raw bytes to VCV Message
                midi::Message msg;
                msg.setSize(size);
                for (size_t i = 0; i < size; i++) {
                    msg.bytes[i] = data[i];
                }
                
                // In VCV Rack, all MIDI goes to the selected VCV MIDI output
                // Hardware-level USB/Breakout routing is not applicable here
                
                // Apply channel filter if set
                if (midiOutput.channel >= 0) {
                    uint8_t status = data[0] & 0xF0;
                    // Only filter channel messages (0x80-0xEF)
                    if (status >= 0x80 && status < 0xF0) {
                        uint8_t channel = data[0] & 0x0F;
                        if (channel != midiOutput.channel) {
                            return;  // Skip if wrong channel
                        }
                    }
                }
                
                // Send to VCV MIDI output
                midiOutput.sendMessage(msg);
                
                // Flash activity light
                midiOutputLight = 1.f;
            }
        );
    }
    
    // Plugin helper methods (inline definitions to resolve forward references)
    bool isPluginLoaded() const {
        return pluginHandle && pluginFactory && pluginAlgorithm;
    }
    
    bool isValidPointer(void* ptr, size_t size = sizeof(void*)) {
        if (!ptr) return false;
        uintptr_t addr = (uintptr_t)ptr;
        if (addr < 0x1000) return false;
        #if defined(__aarch64__) || defined(_M_ARM64)
            if (addr > 0x800000000000ULL) return false;
        #else
            if (addr > 0x7FFFFFFFFFFF) return false;
        #endif
        try {
            volatile char test = *((volatile char*)ptr);
            (void)test;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void unloadPlugin() {
        menuMode = MENU_OFF;
        parameters.clear();
        parameterPages.clear();
        pluginAlgorithm = nullptr;
        pluginFactory = nullptr;
        if (pluginInstanceMemory) {
            free(pluginInstanceMemory);
            pluginInstanceMemory = nullptr;
        }
        if (pluginSharedMemory) {
            free(pluginSharedMemory);
            pluginSharedMemory = nullptr;
        }
        if (pluginHandle) {
            #ifdef ARCH_WIN
                FreeLibrary((HMODULE)pluginHandle);
            #else
                dlclose(pluginHandle);
            #endif
            pluginHandle = nullptr;
        }
        if (!pluginPath.empty()) {
            pluginPath.clear();
            loadingMessage = "Plugin unloaded";
            loadingMessageTimer = 1.5f;
            displayDirty = true;
        }
    }
    
    void reloadPlugin() {
        if (!isPluginLoaded() || pluginPath.empty()) {
            loadingMessage = "No plugin to reload";
            loadingMessageTimer = 1.5f;
            displayDirty = true;
            return;
        }
        
        // Store current state
        std::string currentPath = pluginPath;
        std::vector<int32_t> currentSpecs = currentSpecifications;
        
        // Unload current plugin
        unloadPlugin();
        
        // Reload from same path
        if (loadPlugin(currentPath)) {
            // Try to restore specifications if still valid
            if (!currentSpecs.empty() && currentSpecs.size() <= pluginSpecifications.size()) {
                currentSpecifications = currentSpecs;
                useCustomSpecifications = true;
            }
            loadingMessage = "Plugin reloaded";
        } else {
            loadingMessage = "Reload failed";
        }
        loadingMessageTimer = 1.5f;
        displayDirty = true;
    }
    
    // Plugin specification discovery (phase 1 of loading)
    struct PluginSpecificationInfo {
        std::string path;
        std::string name;
        std::string description;
        std::vector<_NT_specification> specifications;
        bool hasSpecifications() const { return !specifications.empty(); }
    };
    
    PluginSpecificationInfo discoverPluginSpecifications(const std::string& path) {
        PluginSpecificationInfo info;
        info.path = path;
        
        // Load the dynamic library temporarily
        void* tempHandle = nullptr;
        #ifdef ARCH_WIN
            tempHandle = LoadLibraryA(path.c_str());
        #else
            tempHandle = dlopen(path.c_str(), RTLD_LAZY);
        #endif
        
        if (!tempHandle) {
            INFO("Failed to load plugin for specification discovery: %s", dlerror());
            return info;
        }
        
        // Get pluginEntry function
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry;
        #ifdef ARCH_WIN
            pluginEntry = (PluginEntryFunc)GetProcAddress((HMODULE)tempHandle, "pluginEntry");
        #else
            pluginEntry = (PluginEntryFunc)dlsym(tempHandle, "pluginEntry");
        #endif
        
        if (pluginEntry) {
            try {
                // Get factory info
                _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
                if (factory) {
                    info.name = factory->name ? factory->name : "Unknown";
                    info.description = factory->description ? factory->description : "";
                    
                    // Copy specifications if they exist
                    if (factory->numSpecifications > 0 && factory->specifications) {
                        info.specifications.resize(factory->numSpecifications);
                        for (uint32_t i = 0; i < factory->numSpecifications; i++) {
                            info.specifications[i] = factory->specifications[i];
                        }
                        INFO("Discovered plugin '%s' with %u specifications", info.name.c_str(), factory->numSpecifications);
                    } else {
                        INFO("Discovered plugin '%s' with no specifications", info.name.c_str());
                    }
                }
            } catch (...) {
                WARN("Error during plugin specification discovery");
            }
        }
        
        // Clean up temporary handle
        #ifdef ARCH_WIN
            FreeLibrary((HMODULE)tempHandle);
        #else
            dlclose(tempHandle);
        #endif
        
        return info;
    }
    
    // MIDI input processing
    void processMidiMessage(const midi::Message& msg) {
        // Route to plugin if loaded
        if (isPluginLoaded() && pluginAlgorithm && pluginFactory && msg.bytes.size() > 0) {
            uint8_t status = msg.bytes[0] & 0xF0;
            
            // Route real-time messages (0xF0-0xFF)
            if (status >= 0xF0) {
                if (pluginFactory->midiRealtime) {
                    try {
                        pluginFactory->midiRealtime(pluginAlgorithm, msg.bytes[0]);
                    } catch (...) {
                        WARN("Plugin crashed during midiRealtime - unloading");
                        unloadPlugin();
                        return;
                    }
                }
            }
            // Route channel messages (Note On/Off, CC, Program Change, etc.)
            else if (msg.bytes.size() >= 2) {
                if (pluginFactory->midiMessage) {
                    try {
                        uint8_t byte0 = msg.bytes[0];
                        uint8_t byte1 = msg.bytes[1];
                        uint8_t byte2 = (msg.bytes.size() >= 3) ? msg.bytes[2] : 0;
                        
                        pluginFactory->midiMessage(pluginAlgorithm, byte0, byte1, byte2);
                    } catch (...) {
                        WARN("Plugin crashed during midiMessage - unloading");
                        unloadPlugin();
                        return;
                    }
                }
            }
        }
        
        // Note: Individual plugins handle their own channel filtering
        // Module-level MIDI learn could be added here in the future if needed
    }
    
    // Plugin loading methods
    bool loadPlugin(const std::string& path) {
        try {
            unloadPlugin();
            
            // Platform-specific loading
            #ifdef ARCH_WIN
                pluginHandle = LoadLibraryA(path.c_str());
            #else
                // First, ensure our own symbols are globally available
                // Get path to our own plugin
                Dl_info info;
                if (dladdr((void*)&NT_screen, &info) && info.dli_fname) {
                    INFO("Our plugin path: %s", info.dli_fname);
                    // Reopen our own plugin with RTLD_GLOBAL to export symbols
                    void* ourHandle = dlopen(info.dli_fname, RTLD_NOW | RTLD_GLOBAL);
                    INFO("Reopened our plugin with RTLD_GLOBAL: %p", ourHandle);
                } else {
                    INFO("Could not determine our plugin path");
                }
                
                // Check if we can find our own symbol now
                void* testSymbol = dlsym(RTLD_DEFAULT, "_NT_screen");
                INFO("Found _NT_screen symbol: %p", testSymbol);
                
                pluginHandle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
            #endif
            
            if (!pluginHandle) {
                std::string error;
                #ifdef ARCH_WIN
                    error = "Windows error"; // GetLastError() formatting
                #else
                    const char* dlerr = dlerror();
                    error = dlerr ? std::string(dlerr) : "Unknown error";
                #endif
                WARN("Failed to load plugin from path '%s': %s", path.c_str(), error.c_str());
                loadingMessage = "Error: Failed to load plugin - " + error;
                loadingMessageTimer = 4.0f;
                return false;
            }
            
            // Try new API first (pluginEntry)
            typedef uintptr_t (*PluginEntryFunc)(_NT_selector selector, uint32_t data);
            PluginEntryFunc pluginEntry;
            #ifdef ARCH_WIN
                pluginEntry = (PluginEntryFunc)GetProcAddress((HMODULE)pluginHandle, "pluginEntry");
            #else
                pluginEntry = (PluginEntryFunc)dlsym(pluginHandle, "pluginEntry");
            #endif
            
            if (!pluginEntry) {
                unloadPlugin();
                loadingMessage = "Error: Plugin missing pluginEntry symbol";
                loadingMessageTimer = 4.0f;
                return false;
            }
            
            // Check API version
            uintptr_t apiVersion = pluginEntry(kNT_selector_version, 0);
            if (apiVersion != kNT_apiVersionCurrent) {
                unloadPlugin();
                loadingMessage = "Error: API version mismatch";
                loadingMessageTimer = 4.0f;
                return false;
            }
            
            // Get number of factories
            uintptr_t numFactories = pluginEntry(kNT_selector_numFactories, 0);
            if (numFactories == 0) {
                unloadPlugin();
                loadingMessage = "Error: No factories found";
                loadingMessageTimer = 4.0f;
                return false;
            }
            
            // Get first factory
            pluginFactory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
            if (!pluginFactory) {
                unloadPlugin();
                loadingMessage = "Error: Factory access failed";
                loadingMessageTimer = 4.0f;
                return false;
            }
            
            // Handle memory allocation and algorithm construction
            if (pluginFactory->calculateStaticRequirements && pluginFactory->initialise) {
                _NT_staticRequirements staticReqs{};
                pluginFactory->calculateStaticRequirements(staticReqs);
                
                if (staticReqs.dram > 0) {
                    if (posix_memalign(&pluginSharedMemory, 16, staticReqs.dram) != 0) {
                        unloadPlugin();
                        loadingMessage = "Error: Memory allocation failed";
                        loadingMessageTimer = 4.0f;
                        return false;
                    }
                    
                    _NT_staticMemoryPtrs staticPtrs{};
                    staticPtrs.dram = (uint8_t*)pluginSharedMemory;
                    
                    pluginFactory->initialise(staticPtrs, staticReqs);
                }
            }
            
            // Get algorithm requirements and create instance
            if (pluginFactory->calculateRequirements && pluginFactory->construct) {
                _NT_algorithmRequirements reqs{};
                
                // Prepare specifications array - use custom values if provided, otherwise defaults
                std::vector<int32_t> specValues;
                const int32_t* specifications = nullptr;
                
                if (pluginFactory->numSpecifications > 0 && pluginFactory->specifications) {
                    if (useCustomSpecifications && currentSpecifications.size() >= pluginFactory->numSpecifications) {
                        INFO("Plugin has %u specifications, using custom values", pluginFactory->numSpecifications);
                        specValues = currentSpecifications; // Use custom specifications
                        specValues.resize(pluginFactory->numSpecifications); // Ensure correct size
                        for (uint32_t i = 0; i < pluginFactory->numSpecifications; i++) {
                            INFO("  Spec[%u]: %s = %d (custom, range: %d to %d)", 
                                 i, pluginFactory->specifications[i].name, 
                                 specValues[i], pluginFactory->specifications[i].min, pluginFactory->specifications[i].max);
                        }
                    } else {
                        INFO("Plugin has %u specifications, using default values", pluginFactory->numSpecifications);
                        specValues.resize(pluginFactory->numSpecifications);
                        for (uint32_t i = 0; i < pluginFactory->numSpecifications; i++) {
                            specValues[i] = pluginFactory->specifications[i].def;
                            INFO("  Spec[%u]: %s = %d (default, range: %d to %d)", 
                                 i, pluginFactory->specifications[i].name, 
                                 specValues[i], pluginFactory->specifications[i].min, pluginFactory->specifications[i].max);
                        }
                    }
                    specifications = specValues.data();
                    // Store specifications persistently for use during audio processing
                    pluginSpecifications = specValues;
                } else {
                    INFO("Plugin has no specifications");
                    pluginSpecifications.clear();
                }
                
                pluginFactory->calculateRequirements(reqs, specifications);
                
                // Calculate total memory needed
                size_t totalMem = reqs.sram + reqs.dram + reqs.dtc + reqs.itc;
                if (totalMem > 0) {
                    if (posix_memalign(&pluginInstanceMemory, 16, totalMem) != 0) {
                        unloadPlugin();
                        loadingMessage = "Error: Instance memory allocation failed";
                        loadingMessageTimer = 4.0f;
                        return false;
                    }
                    
                    // Set up memory pointers
                    _NT_algorithmMemoryPtrs memPtrs{};
                    uint8_t* ptr = (uint8_t*)pluginInstanceMemory;
                    memPtrs.sram = ptr; ptr += reqs.sram;
                    memPtrs.dram = ptr; ptr += reqs.dram;
                    memPtrs.dtc = ptr; ptr += reqs.dtc;
                    memPtrs.itc = ptr;
                    
                    // construct() returns status code, algorithm is placed at SRAM location
                    _NT_algorithm* result = pluginFactory->construct(memPtrs, reqs, specifications);
                    
                    // Check if construct returned a valid pointer or if algorithm is at SRAM location
                    if (result != nullptr && (uintptr_t)result > 0x1000) {
                        // Use returned pointer
                        pluginAlgorithm = result;
                        INFO("Algorithm construction: using returned pointer 0x%p", result);
                    } else {
                        // Algorithm should be constructed at SRAM location
                        pluginAlgorithm = (_NT_algorithm*)memPtrs.sram;
                        INFO("Algorithm construction: using SRAM location 0x%p (construct returned 0x%x)", 
                             pluginAlgorithm, (unsigned)(uintptr_t)result);
                    }
                    
                    // Validate the algorithm pointer more thoroughly
                    if (!pluginAlgorithm) {
                        unloadPlugin();
                        loadingMessage = "Error: Algorithm pointer is NULL";
                        loadingMessageTimer = 4.0f;
                        return false;
                    }
                    
                    // Connect the parameter values array to our routing matrix
                    // This is critical - the algorithm expects v[] to contain current parameter values
                    pluginAlgorithm->v = routingMatrix.data();
                    INFO("Connected parameter values array: pluginAlgorithm->v = 0x%p", pluginAlgorithm->v);
                    
                    if (!isValidPointer(pluginAlgorithm)) {
                        WARN("Algorithm pointer validation failed: 0x%p", pluginAlgorithm);
                        unloadPlugin();
                        loadingMessage = "Error: Invalid algorithm pointer";
                        loadingMessageTimer = 4.0f;
                        return false;
                    }
                    
                    // Try to validate the algorithm structure by reading first few bytes
                    try {
                        volatile uint8_t* testPtr = (volatile uint8_t*)pluginAlgorithm;
                        uint8_t testByte = testPtr[0];
                        (void)testByte;
                        INFO("Algorithm memory validation: first byte readable");
                    } catch (...) {
                        WARN("Algorithm memory validation failed - cannot read first byte");
                        unloadPlugin();
                        loadingMessage = "Error: Algorithm memory corruption";
                        loadingMessageTimer = 4.0f;
                        return false;
                    }
                }
            }
            
            // Try to set API interface for the loaded plugin (optional, for backward compatibility)
            NT_setAPI_func setAPIFunc = nullptr;
            #ifdef ARCH_WIN
                setAPIFunc = (NT_setAPI_func)GetProcAddress((HMODULE)pluginHandle, "NT_setAPI");
            #else
                setAPIFunc = (NT_setAPI_func)dlsym(pluginHandle, "NT_setAPI");
            #endif
            
            if (setAPIFunc) {
                // Plugin supports new API interface
                const NT_API_Interface* api = getNT_API();
                setAPIFunc(api);
                INFO("Plugin %s: API interface provided successfully", rack::system::getFilename(path).c_str());
            } else {
                // Plugin uses old direct symbol linking - this is still supported
                INFO("Plugin %s: Using legacy direct symbol linking", rack::system::getFilename(path).c_str());
            }
            
            // Success
            pluginPath = path;
            std::string filename = rack::system::getFilename(path);
            loadingMessage = string::f("Loaded: %s", filename.c_str());
            loadingMessageTimer = 2.0f;
            displayDirty = true;
            
            // Debug: Verify draw function pointer after reload
            INFO("Plugin loaded successfully: pluginFactory=%p, draw=%p", 
                 pluginFactory, pluginFactory ? pluginFactory->draw : nullptr);
            
            // Extract parameter information - but only if algorithm is properly constructed
            if (pluginAlgorithm && pluginInstanceMemory) {
                // Add a small delay to ensure memory is settled
                try {
                    extractParameterData();
                } catch (...) {
                    WARN("Exception in extractParameterData - skipping parameter extraction");
                    // Clear any partial data
                    parameters.clear();
                    parameterPages.clear();
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            WARN("Plugin load failed: %s", e.what());
            loadingMessage = string::f("Error: %s", e.what());
            loadingMessageTimer = 4.0f;
            unloadPlugin();
            return false;
        } catch (...) {
            WARN("Plugin load failed: Unknown error");
            loadingMessage = "Error: Unknown failure";
            loadingMessageTimer = 4.0f;
            unloadPlugin();
            return false;
        }
    }
    
    // Overloaded loadPlugin that accepts custom specifications
    bool loadPlugin(const std::string& path, const std::vector<int32_t>& customSpecifications) {
        INFO("Loading plugin with %zu custom specifications", customSpecifications.size());
        for (size_t i = 0; i < customSpecifications.size(); i++) {
            INFO("  Custom spec[%zu] = %d", i, customSpecifications[i]);
        }
        
        // Store specifications in member variables for use during loading
        currentSpecifications = customSpecifications;
        useCustomSpecifications = true;
        
        // Call the main loading method
        bool result = loadPlugin(path);
        
        // Reset custom specification mode
        useCustomSpecifications = false;
        currentSpecifications.clear();
        
        return result;
    }
    
    
    void extractParameterData() {
        // First, validate pluginAlgorithm pointer with extensive checks
        if (!pluginAlgorithm || !isValidPointer(pluginAlgorithm)) {
            WARN("extractParameterData: pluginAlgorithm is null or invalid");
            return;
        }
        
        // Clear existing data first
        parameterPages.clear();
        parameters.clear();
        
        try {
            // Get algorithm requirements to know expected parameter count
            _NT_algorithmRequirements reqs;
            memset(&reqs, 0, sizeof(reqs));
            if (pluginFactory && pluginFactory->calculateRequirements) {
                try {
                    // Prepare specifications array with default values if factory has specifications
                    std::vector<int32_t> specValues;
                    const int32_t* specifications = nullptr;
                    
                    if (pluginFactory->numSpecifications > 0 && pluginFactory->specifications) {
                        if (!pluginSpecifications.empty()) {
                            // Use the actual loaded specifications, not defaults
                            specValues = pluginSpecifications;
                            INFO("Using loaded specifications for parameter extraction");
                            for (uint32_t i = 0; i < specValues.size(); i++) {
                                INFO("  Loaded spec[%u] = %d", i, specValues[i]);
                            }
                        } else {
                            // Fallback to default values if no loaded specifications
                            specValues.resize(pluginFactory->numSpecifications);
                            for (uint32_t i = 0; i < pluginFactory->numSpecifications; i++) {
                                specValues[i] = pluginFactory->specifications[i].def;
                            }
                            INFO("Using default specifications for parameter extraction");
                        }
                        specifications = specValues.data();
                    }
                    
                    pluginFactory->calculateRequirements(reqs, specifications);
                    INFO("Plugin expects %u parameters", reqs.numParameters);
                } catch (...) {
                    WARN("Failed to get algorithm requirements");
                    return;
                }
            } else {
                WARN("No calculateRequirements function available");
                return;
            }
            
            // Validate algorithm structure access
            const _NT_parameter* parametersPtr = nullptr;
            const _NT_parameterPages* parameterPagesPtr = nullptr;
            
            try {
                // Safely read the parameters pointer
                volatile const _NT_parameter* volatile* ptrToPtr = 
                    (volatile const _NT_parameter* volatile*)&pluginAlgorithm->parameters;
                parametersPtr = (const _NT_parameter*)*ptrToPtr;
                
                // Safely read the parameter pages pointer
                volatile const _NT_parameterPages* volatile* pagesPtrToPtr = 
                    (volatile const _NT_parameterPages* volatile*)&pluginAlgorithm->parameterPages;
                parameterPagesPtr = (const _NT_parameterPages*)*pagesPtrToPtr;
            } catch (...) {
                WARN("extractParameterData: Cannot read algorithm structure fields");
                return;
            }
            
            // Extract parameters
            if (reqs.numParameters > 0) {
                if (parametersPtr == nullptr) {
                    INFO("extractParameterData: parameters pointer is NULL (plugin may have no parameters)");
                } else {
                    // Check for known corruption patterns
                    uintptr_t paramAddr = (uintptr_t)parametersPtr;
                    if (paramAddr == 0x3f800000 || 
                        (paramAddr >= 0x3f000000 && paramAddr <= 0x40000000)) {
                        WARN("extractParameterData: parameters pointer is corrupted address (0x%p)", parametersPtr);
                        return;
                    }
                    
                    if (!isValidPointer((void*)parametersPtr)) {
                        WARN("extractParameterData: parameters pointer is invalid (0x%p)", parametersPtr);
                        return;
                    }
                    
                    // Extract each parameter safely
                    for (uint32_t i = 0; i < reqs.numParameters; i++) {
                        try {
                            const _NT_parameter* param = &parametersPtr[i];
                            
                            // Validate parameter structure
                            if (!isValidPointer((void*)param->name)) {
                                WARN("Parameter %u has invalid name pointer", i);
                                continue;
                            }
                            
                            // Test name access
                            if (param->name[0] == '\0') {
                                WARN("Parameter %u has empty name", i);
                                continue;
                            }
                            
                            // Validate parameter ranges
                            if (param->min > param->max) {
                                WARN("Parameter %u has invalid range: %d > %d", i, param->min, param->max);
                                continue;
                            }
                            
                            // Copy parameter safely
                            _NT_parameter paramCopy;
                            paramCopy.name = param->name;
                            paramCopy.min = param->min;
                            paramCopy.max = param->max;
                            paramCopy.def = param->def;
                            paramCopy.unit = param->unit;
                            paramCopy.scaling = param->scaling;
                            paramCopy.enumStrings = param->enumStrings;
                            
                            parameters.push_back(paramCopy);
                            INFO("Extracted parameter %u: '%s' [%d-%d, def=%d]", 
                                 i, param->name, param->min, param->max, param->def);
                        } catch (...) {
                            WARN("Failed to extract parameter %u", i);
                            continue;
                        }
                    }
                }
            }
            
            INFO("Extracted %zu parameters", parameters.size());
            
            // Extract parameter pages if available
            if (parameterPagesPtr != nullptr && isValidPointer((void*)parameterPagesPtr)) {
                try {
                    uint32_t numPages = parameterPagesPtr->numPages;
                    const _NT_parameterPage* pagesArray = parameterPagesPtr->pages;
                    
                    if (numPages > 0 && numPages <= 32 && isValidPointer((void*)pagesArray)) { // Reasonable limit
                        for (uint32_t pageIdx = 0; pageIdx < numPages; pageIdx++) {
                            const _NT_parameterPage* page = &pagesArray[pageIdx];
                            
                            // Validate page structure
                            if (!isValidPointer((void*)page->name) || !isValidPointer((void*)page->params)) {
                                WARN("Page %u has invalid pointers", pageIdx);
                                continue;
                            }
                            
                            if (page->numParams == 0 || page->numParams > parameters.size()) {
                                WARN("Page %u has invalid param count: %u", pageIdx, page->numParams);
                                continue;
                            }
                            
                            // Copy page safely
                            _NT_parameterPage pageCopy;
                            pageCopy.name = page->name;
                            pageCopy.numParams = page->numParams;
                            pageCopy.params = page->params; // Keep pointer reference for now
                            
                            parameterPages.push_back(pageCopy);
                            INFO("Extracted page %u: '%s' (%u params)", 
                                 pageIdx, page->name, page->numParams);
                        }
                    }
                } catch (...) {
                    WARN("Failed to extract parameter pages");
                    parameterPages.clear();
                }
            }
            
            INFO("Extracted %zu parameter pages", parameterPages.size());
            
            // Initialize routing matrix with parameter defaults
            for (size_t i = 0; i < parameters.size() && i < routingMatrix.size(); i++) {
                if (parameters[i].min <= parameters[i].def && parameters[i].def <= parameters[i].max) {
                    routingMatrix[i] = parameters[i].def;
                } else {
                    routingMatrix[i] = parameters[i].min;
                }
            }
            
        } catch (const std::exception& e) {
            WARN("Parameter extraction failed with exception: %s", e.what());
            parameters.clear();
            parameterPages.clear();
        } catch (...) {
            WARN("Parameter extraction failed: Unknown error");
            parameters.clear();
            parameterPages.clear();
        }
        
        // Create default page if we have parameters but no pages
        if (!parameters.empty() && parameterPages.empty()) {
            INFO("Creating default parameter page");
            _NT_parameterPage defaultPage;
            defaultPage.name = "Parameters";
            defaultPage.numParams = std::min((size_t)parameters.size(), (size_t)255);
            
            // Create a parameter index array (this is a limitation - we need to store indices somewhere)
            // For now, we'll handle this in the parameter navigation functions
            defaultPage.params = nullptr; // Will handle this in navigation functions
            parameterPages.push_back(defaultPage);
        }
        
        // Initialize routing matrix with default parameter values
        if (!parameters.empty() && pluginAlgorithm) {
            for (size_t i = 0; i < parameters.size() && i < routingMatrix.size(); i++) {
                routingMatrix[i] = parameters[i].def;
            }
            
            // Call parameterChanged for each parameter to let plugin initialize
            if (pluginFactory && pluginFactory->parameterChanged) {
                for (size_t i = 0; i < parameters.size(); i++) {
                    safeExecutePlugin([&]() {
                        pluginFactory->parameterChanged(pluginAlgorithm, i);
                    }, "parameterChanged");
                }
            }
        }
        
        // Update parameter-based routing after extracting parameters
        if (!parameters.empty()) {
            updateParameterRouting();
            
            // Debug: Log routing information
            INFO("Parameter routing updated. Input mappings: %zu, Output mappings: %zu", 
                 paramInputRouting.size(), paramOutputRouting.size());
            for (const auto& pair : paramInputRouting) {
                INFO("Input routing: param[%d]='%s' value=%d -> bus %d", 
                     (int)pair.first, parameters[pair.first].name, routingMatrix[pair.first], pair.second.busIndex);
            }
            for (const auto& pair : paramOutputRouting) {
                INFO("Output routing: param[%d]='%s' value=%d -> bus %d", 
                     (int)pair.first, parameters[pair.first].name, routingMatrix[pair.first], pair.second.busIndex);
            }
        }
    }
    
    void toggleMenuMode() {
        if (menuMode == MENU_OFF) {
            // Only enter menu if plugin is valid and we have parameter data
            if (!pluginAlgorithm || parameters.empty() || parameterPages.empty()) {
                INFO("Cannot enter menu: pluginAlgorithm=%p, parameters.size()=%zu, parameterPages.size()=%zu", 
                     pluginAlgorithm, parameters.size(), parameterPages.size());
                return; // Can't enter menu without valid plugin and parameters
            }
            INFO("Entering parameter menu with %zu parameters across %zu pages", 
                 parameters.size(), parameterPages.size());
            menuMode = MENU_PAGE_SELECT;
            currentPageIndex = 0;
            currentParamIndex = 0;
        } else {
            INFO("Exiting parameter menu");
            menuMode = MENU_OFF;
            if (pluginAlgorithm) {
                applyRoutingChanges();
            }
        }
        displayDirty = true;
    }
    
    void processMenuNavigation() {
        // Safety checks - exit menu if plugin is invalid or no data
        if (!pluginAlgorithm || parameters.empty() || parameterPages.empty()) {
            menuMode = MENU_OFF;
            displayDirty = true;
            return;
        }
        
        // Track last values to detect changes
        static float lastLeftPotValue = -1.0f;
        static float lastCenterPotValue = -1.0f;
        static float lastRightPotValue = -1.0f;
        static int lastLeftEncoder = 0;
        static int lastRightEncoder = 0;
        
        // Read current values
        float leftPotValue = params[POT_L_PARAM].getValue();
        float centerPotValue = params[POT_C_PARAM].getValue();
        float rightPotValue = params[POT_R_PARAM].getValue();
        int currentLeftEncoder = (int)params[ENCODER_L_PARAM].getValue();
        int currentRightEncoder = (int)params[ENCODER_R_PARAM].getValue();
        
        // === Page Selection (Left Pot) ===
        if (std::fabs(leftPotValue - lastLeftPotValue) > 0.00001f && parameterPages.size() > 1) {
            int newPageIndex = (int)(leftPotValue * (parameterPages.size() - 1));
            if (newPageIndex != currentPageIndex && newPageIndex >= 0 && newPageIndex < (int)parameterPages.size()) {
                setCurrentPage(newPageIndex);
            }
            lastLeftPotValue = leftPotValue;
        }
        
        // === Parameter Selection (Center Pot or Left Encoder) ===
        const _NT_parameterPage& currentPage = parameterPages[currentPageIndex];
        
        // Center pot
        if (std::fabs(centerPotValue - lastCenterPotValue) > 0.00001f && currentPage.numParams > 1) {
            int newParamIndex = (int)(centerPotValue * (currentPage.numParams - 1));
            if (newParamIndex != currentParamIndex && newParamIndex >= 0 && newParamIndex < currentPage.numParams) {
                setCurrentParam(newParamIndex);
            }
            lastCenterPotValue = centerPotValue;
        }
        
        // Left encoder (relative)
        int leftDelta = currentLeftEncoder - lastLeftEncoder;
        if (leftDelta != 0 && currentPage.numParams > 0) {
            int newParamIndex = clamp(currentParamIndex + leftDelta, 0, (int)currentPage.numParams - 1);
            if (newParamIndex != currentParamIndex) {
                setCurrentParam(newParamIndex);
            }
            lastLeftEncoder = currentLeftEncoder;
        }
        
        // === Parameter Value Editing (Right Pot or Right Encoder) ===
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx >= 0 && paramIdx < (int)parameters.size()) {
            const _NT_parameter& param = parameters[paramIdx];
            int currentValue = routingMatrix[paramIdx];
            
            // Right pot (absolute)
            if (std::fabs(rightPotValue - lastRightPotValue) > 0.00001f) {
                int newValue = (int)(rightPotValue * (param.max - param.min)) + param.min;
                if (newValue != currentValue) {
                    setParameterValue(paramIdx, newValue);
                }
                lastRightPotValue = rightPotValue;
            }
            
            // Right encoder (relative)
            int rightDelta = currentRightEncoder - lastRightEncoder;
            if (rightDelta != 0) {
                int newValue = clamp(currentValue + rightDelta, (int)param.min, (int)param.max);
                if (newValue != currentValue) {
                    setParameterValue(paramIdx, newValue);
                }
                lastRightEncoder = currentRightEncoder;
            }
        }
        
        // Right encoder press: Optional (values are now applied immediately via pots/encoders)
        if (params[ENCODER_R_PRESS_PARAM].getValue() > 0 && !rightEncoderPressed) {
            rightEncoderPressed = true;
            // Could be used for additional functions in the future
            displayDirty = true;
        } else if (params[ENCODER_R_PRESS_PARAM].getValue() == 0) {
            rightEncoderPressed = false;
        }
    }
    
    // Process discrete encoder steps using raw value directly
    int processDiscreteEncoder(int paramId) {
        int currentValue = (int)params[paramId].getValue();
        
        // The raw value IS the number of steps to move
        int steps = currentValue;
        
        // Reset the parameter after consuming the steps
        if (steps != 0) {
            params[paramId].setValue(0.f);
            
        }
        
        return steps;
    }
    
    // Get encoder steps for menu navigation
    int getEncoderSteps(int paramId) {
        return processDiscreteEncoder(paramId);
    }
    
    // State update methods - single point of control
    void setCurrentPage(int pageIndex) {
        if (pageIndex != currentPageIndex && pageIndex >= 0 && pageIndex < (int)parameterPages.size()) {
            currentPageIndex = pageIndex;
            currentParamIndex = 0; // Reset to first parameter when changing pages
            displayDirty = true;
        }
    }
    
    void setCurrentParam(int paramIndex) {
        if (paramIndex != currentParamIndex && currentPageIndex < (int)parameterPages.size()) {
            const _NT_parameterPage& page = parameterPages[currentPageIndex];
            if (paramIndex >= 0 && paramIndex < page.numParams) {
                currentParamIndex = paramIndex;
                displayDirty = true;
            }
        }
    }
    
    bool isRoutingParameter(const _NT_parameter& param) {
        return param.unit == kNT_unitAudioInput || 
               param.unit == kNT_unitCvInput ||
               param.unit == kNT_unitAudioOutput || 
               param.unit == kNT_unitCvOutput ||
               param.unit == kNT_unitOutputMode;
    }
    
    void setParameterValue(int paramIdx, int value) {
        if (paramIdx >= 0 && paramIdx < (int)parameters.size()) {
            const _NT_parameter& param = parameters[paramIdx];
            value = clamp(value, (int)param.min, (int)param.max);
            if (routingMatrix[paramIdx] != value) {
                routingMatrix[paramIdx] = value;
                
                // Notify plugin of parameter change
                if (pluginFactory && pluginFactory->parameterChanged) {
                    pluginFactory->parameterChanged(pluginAlgorithm, paramIdx);
                }
                
                // Update routing immediately for routing parameters
                if (isRoutingParameter(param)) {
                    updateParameterRouting();
                }
                
                displayDirty = true;
            }
        }
    }
    
    int getCurrentParameterIndex() {
        if (currentPageIndex >= (int)parameterPages.size() || parameterPages.empty()) return -1;
        
        const _NT_parameterPage& page = parameterPages[currentPageIndex];
        if (currentParamIndex >= page.numParams) return -1;
        
        // Handle default page case (no parameter index array)
        if (page.params == nullptr) {
            // For default pages, assume sequential parameter indexing
            if (currentParamIndex >= (int)parameters.size()) return -1;
            return currentParamIndex;
        }
        
        // Handle normal pages with parameter index arrays
        int paramIdx = page.params[currentParamIndex];
        if (paramIdx < 0 || paramIdx >= (int)parameters.size()) return -1;
        
        return paramIdx;
    }
    
    int getCurrentParameterValue() {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0 || paramIdx >= 256) return 0;
        
        return routingMatrix[paramIdx];
    }
    
    void confirmParameterValue() {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0 || paramIdx >= 256) return;
        
        routingMatrix[paramIdx] = parameterEditValue;
        
        // Notify plugin of parameter change
        if (pluginAlgorithm) {
            safeExecutePlugin([&]() {
                if (pluginFactory->parameterChanged) {
                    pluginFactory->parameterChanged(pluginAlgorithm, paramIdx);
                }
            }, "parameterChanged");
        }
        
        routingDirty = true;
    }
    
    void applyRoutingChanges() {
        // Update bus routing based on parameter values
        updateBusRouting();
        
        // Save routing to module state
        routingDirty = true;
    }
    
    void updateParameterRouting() {
        // Clear existing routing
        paramInputRouting.clear();
        paramOutputRouting.clear();
        
        // Scan parameters for input/output types
        for (size_t i = 0; i < parameters.size(); i++) {
            const _NT_parameter& param = parameters[i];
            
            if (param.unit == kNT_unitAudioInput || param.unit == kNT_unitCvInput) {
                // This parameter controls an input routing
                paramInputRouting[i] = {
                    .paramIndex = (int)i,
                    .busIndex = routingMatrix[i] > 0 ? routingMatrix[i] - 1 : -1,  // Convert to 0-based
                    .isCV = (param.unit == kNT_unitCvInput)
                };
            }
            else if (param.unit == kNT_unitAudioOutput || param.unit == kNT_unitCvOutput) {
                // This parameter controls an output routing
                paramOutputRouting[i] = {
                    .paramIndex = (int)i,
                    .busIndex = routingMatrix[i] > 0 ? routingMatrix[i] - 1 : -1,  // Convert to 0-based
                    .isCV = (param.unit == kNT_unitCvOutput),
                    .hasOutputMode = false,
                    .outputModeParamIndex = -1
                };
                
                // Check if next parameter is output mode
                if (i + 1 < parameters.size() && parameters[i + 1].unit == kNT_unitOutputMode) {
                    paramOutputRouting[i].hasOutputMode = true;
                    paramOutputRouting[i].outputModeParamIndex = i + 1;
                }
            }
        }
        
        // Update the busInputMap and busOutputMap for compatibility
        updateBusRouting();
    }
    
    void updateBusRouting() {
        // Validate plugin state before updating routing
        if (!pluginAlgorithm || parameters.empty()) {
            return;
        }
        
        // This method is now called from updateParameterRouting()
        // and uses the paramInputRouting and paramOutputRouting maps
    }
    
    int getVCVInputForParameter(int paramIndex) {
        // Map parameter index to VCV input based on naming convention
        // This would need to be customized based on how parameters are named
        // For now, simple sequential mapping
        return paramIndex < 12 ? paramIndex : -1;
    }
    
    int getVCVOutputForParameter(int paramIndex) {
        // Map parameter index to VCV output
        // Assuming outputs come after inputs in parameter list
        return (paramIndex >= 12 && paramIndex < 18) ? paramIndex - 12 : -1;
    }
    
    void applyCustomRouting() {
        // Simple fixed input routing: VCV inputs 1-12 -> Disting NT buses 1-12
        // This matches the hardware architecture and works with any plugin
        int currentSample = busSystem.getCurrentSampleIndex();
        
        // Clear all buses first for current sample
        for (int bus = 0; bus < 28; bus++) {
            busSystem.setBus(bus, currentSample, 0.0f);
        }
        
        // Fixed routing: VCV Input 1 -> Bus 1, VCV Input 2 -> Bus 2, etc.
        for (int i = 0; i < NUM_INPUTS; i++) {
            if (inputs[AUDIO_INPUT_1 + i].isConnected()) {
                float voltage = inputs[AUDIO_INPUT_1 + i].getVoltage();
                int targetBus = i; // Bus 1-12 (0-based indices 0-11)
                busSystem.setBus(targetBus, currentSample, voltage);
                
                // Enhanced debug output - show when we have actual audio signal
                static int debugCount = 0;
                if (debugCount++ < 50 || (std::abs(voltage) > 0.001f && debugCount < 200)) {
                    INFO("Input routing: VCV input %d -> Bus %d (voltage=%.3f, connected=%d)", 
                         i + 1, targetBus + 1, voltage, inputs[AUDIO_INPUT_1 + i].isConnected());
                }
            }
        }
    }
    
    void applyCustomOutputRouting() {
        // Apply output routing based on parameter settings
        static int debugCount = 0;
        if (debugCount++ < 20) {
            INFO("CUSTOM ROUTING: applyCustomOutputRouting called, parameters.size()=%zu", parameters.size());
        }
        
        int currentSample = busSystem.getCurrentSampleIndex();
        
        // Clear outputs first
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            outputs[i].setVoltage(0.0f);
        }
        
        // Simple fixed routing: Disting NT output buses 13-20 -> VCV outputs 1-8
        // Bus 13 (index 12) -> VCV output 1 (AUDIO_OUTPUT_1)
        // Bus 14 (index 13) -> VCV output 2 (AUDIO_OUTPUT_2)
        // etc.
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            int sourceBus = 12 + i;  // Bus 13-20 (0-based indices 12-19)
            float busValue = busSystem.getBus(sourceBus, currentSample);
            outputs[AUDIO_OUTPUT_1 + i].setVoltage(busValue);
            
            // Enhanced debug output - show when we have actual audio signal
            static int debugCount = 0;
            if (debugCount++ < 50 || (std::abs(busValue) > 0.001f && debugCount < 200)) {
                INFO("Output routing: Bus %d -> VCV output %d (busValue=%.3f)", 
                     sourceBus + 1, i + 1, busValue);
            }
        }
        
        // Advance to next sample
        busSystem.nextSample();
    }
    
    // Plugin sandboxing utility
    template<typename Func>
    bool safeExecutePlugin(Func func, const char* operation) {
        if (!isPluginLoaded()) return false;
        
        try {
            func();
            return true;
        } catch (const std::exception& e) {
            WARN("Plugin %s failed: %s", operation, e.what());
            loadingMessage = string::f("Plugin Error: %s", operation);
            loadingMessageTimer = 3.0f;
            return false;
        } catch (...) {
            WARN("Plugin %s failed: Unknown exception", operation);
            loadingMessage = string::f("Plugin Error: %s", operation);
            loadingMessageTimer = 3.0f;
            return false;
        }
    }
    
    void process(const ProcessArgs& args) override {
        // Update loading message timer
        if (loadingMessageTimer > 0) {
            loadingMessageTimer -= args.sampleTime;
        }
        
        // Process MIDI input
        midi::Message msg;
        while (midiInput.tryPop(&msg, args.frame)) {
            processMidiMessage(msg);
            midiInputLight = 1.f;
        }
        
        // Update MIDI activity lights
        if (midiActivityDivider.process()) {
            midiInputLight *= 0.9f;  // Decay
            midiOutputLight *= 0.9f;
        }
        
        // Handle menu navigation
        if (params[ENCODER_L_PRESS_PARAM].getValue() > 0 && !leftEncoderPressed) {
            leftEncoderPressed = true;
            toggleMenuMode();
        } else if (params[ENCODER_L_PRESS_PARAM].getValue() == 0) {
            leftEncoderPressed = false;
        }
        
        if (menuMode != MENU_OFF) {
            processMenuNavigation();
        }
        
        // Handle control inputs
        processControls();
        
        // Sync routing matrix with plugin algorithm values if plugin is loaded
        // Skip sync during menu mode to avoid conflicts with parameter editing
        if (isPluginLoaded() && pluginAlgorithm && menuMode == MENU_OFF) {
            for (size_t i = 0; i < parameters.size() && i < routingMatrix.size(); i++) {
                routingMatrix[i] = pluginAlgorithm->v[i];
            }
        }
        
        // Apply custom routing if plugin is loaded, otherwise use default routing
        if (isPluginLoaded() && !parameters.empty()) {
            applyCustomRouting();
        } else {
            // Route inputs to buses (default behavior)
            busSystem.routeInputs(this);
        }
        
        // Accumulate samples for block processing
        sampleCounter++;
        if (sampleCounter >= BLOCK_SIZE) {
            // Process algorithm with 4-sample blocks
            if (isPluginLoaded()) {
                // Use plugin for audio processing
                safeExecutePlugin([&]() {
                    if (pluginFactory->step) {
                        pluginFactory->step(pluginAlgorithm, busSystem.getBuses(), 1);
                        
                        // Debug: Check what the plugin wrote to output buses
                        static int stepDebugCount = 0;
                        if (stepDebugCount++ < 100) {
                            int currentSample = busSystem.getCurrentSampleIndex();
                            for (int bus = 12; bus < 16; bus++) { // Check buses 13-16
                                float busValue = busSystem.getBus(bus, currentSample);
                                if (std::abs(busValue) > 0.001f || stepDebugCount < 20) {
                                    INFO("After plugin step: Bus %d = %.3f", bus + 1, busValue);
                                }
                            }
                        }
                    }
                }, "step");
            } else {
                // Use built-in emulator
                emulatorCore.processAudio(busSystem.getBuses(), 1); // 1 = numFramesBy4
            }
            sampleCounter = 0;
        }
        
        // Add basic debugging every time
        static int routingDebugCount = 0;
        if (routingDebugCount++ < 100) {
            INFO("ROUTING DEBUG: isPluginLoaded()=%d, parameters.size()=%zu", 
                 isPluginLoaded(), parameters.size());
        }
        
        // Apply custom output routing if plugin is loaded, otherwise use default routing
        if (isPluginLoaded() && !parameters.empty()) {
            static int customRoutingCount = 0;
            if (customRoutingCount++ < 10) {
                INFO("Using CUSTOM routing");
            }
            applyCustomOutputRouting();
        } else {
            // Debug why we're using default routing
            static int fallbackDebugCount = 0;
            if (fallbackDebugCount++ < 10) {
                INFO("Using DEFAULT routing: isPluginLoaded()=%d, parameters.size()=%zu", 
                     isPluginLoaded(), parameters.size());
            }
            // Route buses to outputs (default behavior)
            busSystem.routeOutputs(this);
        }
        
        // Update lights
        updateLights();
    }
    
    void processControls() {
        // Process button triggers
        for (int i = 0; i < 4; i++) {
            if (buttonTriggers[i].process(params[BUTTON_1_PARAM + i].getValue())) {
                onButtonPress(i, true);
                displayDirty = true;
            }
        }
        
        // Process encoder press triggers
        if (encoderLPressTrigger.process(params[ENCODER_L_PRESS_PARAM].getValue())) {
            onEncoderPress(0);
            displayDirty = true;
        }
        if (encoderRPressTrigger.process(params[ENCODER_R_PRESS_PARAM].getValue())) {
            onEncoderPress(1);
            displayDirty = true;
        }
        
        // Update hardware state for emulator core
        VCVHardwareState hwState;
        
        // Update pot values
        for (int i = 0; i < 3; i++) {
            hwState.pots[i] = params[POT_L_PARAM + i].getValue();
        }
        
        // Update button states
        for (int i = 0; i < 4; i++) {
            hwState.buttons[i] = params[BUTTON_1_PARAM + i].getValue() > 0.5f;
        }
        
        // Update encoder states
        hwState.pot_pressed[0] = params[ENCODER_L_PRESS_PARAM].getValue() > 0.5f;
        hwState.pot_pressed[2] = params[ENCODER_R_PRESS_PARAM].getValue() > 0.5f;
        
        // Set encoder values from params
        hwState.encoder_deltas[0] = 0;
        hwState.encoder_deltas[1] = 0;
        
        emulatorCore.updateHardwareState(hwState);
    }
    
    void updateLights() {
        // Update button lights based on state
        for (int i = 0; i < 4; i++) {
            lights[BUTTON_1_LIGHT + i].setBrightness(params[BUTTON_1_PARAM + i].getValue());
        }
        
        // Update port lights based on routing
        updatePortLights();
    }
    
    void updatePortLights() {
        // Light up connected inputs
        for (int i = 0; i < 12; i++) {
            bool isRouted = false;
            for (int bus = 0; bus < 28; bus++) {
                if (busInputMap[bus] == i) {
                    isRouted = true;
                    break;
                }
            }
            lights[INPUT_LIGHT_1 + i].setBrightness(isRouted ? 1.0f : 0.0f);
        }
        
        // Light up connected outputs
        for (int i = 0; i < 6; i++) {
            bool isRouted = false;
            for (int bus = 0; bus < 28; bus++) {
                if (busOutputMap[bus] == i) {
                    isRouted = true;
                    break;
                }
            }
            lights[OUTPUT_LIGHT_1 + i].setBrightness(isRouted ? 1.0f : 0.0f);
        }
        
        // Update MIDI activity lights
        lights[MIDI_INPUT_LIGHT].setBrightness(midiInputLight);
        lights[MIDI_OUTPUT_LIGHT].setBrightness(midiOutputLight);
    }
    
    void onParameterChange(int parameter, float value) {
        if (isPluginLoaded()) {
            // Forward parameter changes to plugin
            safeExecutePlugin([&]() {
                if (pluginFactory->parameterChanged) {
                    pluginFactory->parameterChanged(pluginAlgorithm, parameter);
                }
            }, "parameterChanged");
        } else {
            emulatorCore.setParameter(parameter, value);
        }
    }
    
    void onButtonPress(int button, bool pressed) {
        if (pressed) {
            // Button 1 is the menu button
            if (button == 0) {
                toggleMenuMode();
            } else {
                emulatorCore.pressButton(button);
            }
        } else {
            if (button != 0) { // Don't send button 1 release to emulator
                emulatorCore.releaseButton(button);
            }
        }
    }
    
    void onEncoderChange(int encoder, int delta) {
        emulatorCore.turnEncoder(encoder, delta);
        displayDirty = true;
    }
    
    void onEncoderPress(int encoder) {
        emulatorCore.pressEncoder(encoder);
        displayDirty = true;
    }
    
    void selectAlgorithm(int index) {
        if (emulatorCore.selectAlgorithm(index)) {
            displayDirty = true;
        }
    }
    
    // VCV Rack lifecycle methods
    void onSampleRateChange() override {
        emulatorCore.initialize(APP->engine->getSampleRate());
    }
    
    void onReset() override {
        emulatorCore.selectAlgorithm(0);
        displayDirty = true;
    }
    
    void onRandomize() override {
        // Randomize pot positions
        params[POT_L_PARAM].setValue(random::uniform());
        params[POT_C_PARAM].setValue(random::uniform());
        params[POT_R_PARAM].setValue(random::uniform());
        displayDirty = true;
    }
    
    json_t* dataToJson() override {
        json_t* rootJ = emulatorCore.saveState();
        
        // Save plugin info if loaded
        if (!pluginPath.empty()) {
            json_object_set_new(rootJ, "pluginPath", json_string(pluginPath.c_str()));
            json_object_set_new(rootJ, "lastFolder", json_string(lastPluginFolder.c_str()));
        }
        
        // Save parameter values
        json_t* paramsJ = json_array();
        for (int i = 0; i < 3; i++) {
            json_array_append_new(paramsJ, json_real(params[POT_L_PARAM + i].getValue()));
        }
        json_object_set_new(rootJ, "params", paramsJ);
        
        // Save routing matrix
        json_t* routingJ = json_array();
        for (int i = 0; i < 256; i++) {
            if (routingMatrix[i] != 0) {
                json_t* routeJ = json_object();
                json_object_set_new(routeJ, "param", json_integer(i));
                json_object_set_new(routeJ, "bus", json_integer(routingMatrix[i]));
                json_array_append_new(routingJ, routeJ);
            }
        }
        json_object_set_new(rootJ, "routing", routingJ);
        
        // Save MIDI settings
        json_object_set_new(rootJ, "midiInput", midiInput.toJson());
        json_object_set_new(rootJ, "midiOutput", midiOutput.toJson());
        
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        emulatorCore.loadState(rootJ);
        
        // Restore plugin if saved
        json_t* pluginPathJ = json_object_get(rootJ, "pluginPath");
        if (pluginPathJ) {
            std::string path = json_string_value(pluginPathJ);
            
            json_t* folderJ = json_object_get(rootJ, "lastFolder");
            if (folderJ) {
                lastPluginFolder = json_string_value(folderJ);
            }
            
            // Try to load plugin
            if (!path.empty() && rack::system::exists(path)) {
                loadPlugin(path);
            }
        }
        
        // Restore parameters
        json_t* paramsJ = json_object_get(rootJ, "params");
        if (paramsJ) {
            for (int i = 0; i < 3; i++) {
                json_t* paramJ = json_array_get(paramsJ, i);
                if (paramJ) {
                    params[POT_L_PARAM + i].setValue(json_real_value(paramJ));
                }
            }
        }
        
        // Load routing matrix
        json_t* routingJ = json_object_get(rootJ, "routing");
        if (routingJ) {
            // Reset to defaults first
            for (size_t i = 0; i < parameters.size(); i++) {
                if (i < routingMatrix.size()) {
                    routingMatrix[i] = parameters[i].def;
                }
            }
            
            // Apply saved routing
            size_t routingSize = json_array_size(routingJ);
            for (size_t i = 0; i < routingSize; i++) {
                json_t* routeJ = json_array_get(routingJ, i);
                if (routeJ) {
                    int param = json_integer_value(json_object_get(routeJ, "param"));
                    int bus = json_integer_value(json_object_get(routeJ, "bus"));
                    if (param >= 0 && param < 256) {
                        routingMatrix[param] = bus;
                    }
                }
            }
            
            // Apply routing
            updateBusRouting();
        }
        
        // Restore MIDI settings
        json_t* midiInputJ = json_object_get(rootJ, "midiInput");
        if (midiInputJ) {
            midiInput.fromJson(midiInputJ);
        }
        
        json_t* midiOutputJ = json_object_get(rootJ, "midiOutput");
        if (midiOutputJ) {
            midiOutput.fromJson(midiOutputJ);
        }
        
        displayDirty = true;
    }
};

// Custom OLED Widget with access to DistingNT module
struct DistingNTOLEDWidget : FramebufferWidget {
    DistingNT* module = nullptr;
    
    // Display dimensions matching Disting NT OLED
    static constexpr int DISPLAY_WIDTH = 256;
    static constexpr int DISPLAY_HEIGHT = 64;
    
    DistingNTOLEDWidget() {}
    
    void step() override {
        // Check if we need to redraw
        if (module && (module->displayDirty || module->emulatorCore.getDisplayBuffer().dirty)) {
            // Mark framebuffer as dirty to trigger redraw
            FramebufferWidget::dirty = true;
            module->displayDirty = false;
        }
        FramebufferWidget::step();
    }
    
    void draw(const DrawArgs& args) override {
        if (!module) {
            drawPlaceholder(args);
            return;
        }
        
        DistingNT* distingModule = dynamic_cast<DistingNT*>(module);
        
        // Set up coordinate system for 256x64 display
        nvgSave(args.vg);
        nvgScale(args.vg, box.size.x / DISPLAY_WIDTH, box.size.y / DISPLAY_HEIGHT);
        
        // Clear display background (black)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        // Show menu interface if active
        if (distingModule && distingModule->menuMode != DistingNT::MENU_OFF) {
            drawMenuInterface(args.vg, distingModule);
        } else if (distingModule && distingModule->loadingMessageTimer > 0) {
            nvgFillColor(args.vg, nvgRGB(0, 255, 255)); // Cyan
            nvgFontSize(args.vg, 16);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            
            // Fade out effect
            float alpha = std::min(1.0f, distingModule->loadingMessageTimer);
            nvgFillColor(args.vg, nvgRGBA(0, 255, 255, (int)(255 * alpha)));
            
            nvgText(args.vg, 128, 32, distingModule->loadingMessage.c_str(), NULL);
        } else {
            // Check if plugin has custom drawing
            if (distingModule && distingModule->isPluginLoaded()) {
                // Clear display first
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                nvgFillColor(args.vg, nvgRGB(0, 0, 0));
                nvgFill(args.vg);
                
                // Try plugin drawing
                bool pluginDrew = false;
                if (distingModule->pluginFactory && distingModule->pluginFactory->draw) {
                    static int drawCallCount = 0;
                    if (drawCallCount++ < 5) {
                        INFO("Calling plugin draw() function, attempt %d", drawCallCount);
                    }
                } else {
                    static int failureCount = 0;
                    if (failureCount++ < 5) {
                        INFO("Cannot call plugin draw(): pluginFactory=%p, draw=%p", 
                             distingModule->pluginFactory, 
                             distingModule->pluginFactory ? distingModule->pluginFactory->draw : nullptr);
                    }
                }
                
                if (distingModule->pluginFactory && distingModule->pluginFactory->draw) {
                    
                    // Clear NT_screen buffer before plugin draws
                    memset(NT_screen, 0, sizeof(NT_screen));
                    
                    static int pluginDrawCallCount = 0;
                    if (pluginDrawCallCount++ < 5) {
                        INFO("About to call plugin draw() for %s, attempt %d", 
                             distingModule->pluginFactory->name ? distingModule->pluginFactory->name : "unknown", 
                             pluginDrawCallCount);
                    }
                    
                    distingModule->safeExecutePlugin([&]() {
                        pluginDrew = distingModule->pluginFactory->draw(distingModule->pluginAlgorithm);
                        
                        static int drawResultCount = 0;
                        if (drawResultCount++ < 5) {
                            INFO("Plugin draw() returned %s, attempt %d", pluginDrew ? "true" : "false", drawResultCount);
                            
                            // Don't clear the buffer if plugin returned false - keep whatever it drew
                            // The plugin may have drawn text but returned false for other reasons
                            
                            // Check buffer state after plugin draw but before any potential clearing
                            INFO("NT_screen at row 40 (text area): %02x %02x %02x %02x %02x %02x %02x %02x",
                                 NT_screen[40 * 128], NT_screen[40 * 128 + 1], NT_screen[40 * 128 + 2], NT_screen[40 * 128 + 3],
                                 NT_screen[40 * 128 + 4], NT_screen[40 * 128 + 5], NT_screen[40 * 128 + 6], NT_screen[40 * 128 + 7]);
                                 
                            INFO("NT_screen first 16 bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                                 NT_screen[0], NT_screen[1], NT_screen[2], NT_screen[3],
                                 NT_screen[4], NT_screen[5], NT_screen[6], NT_screen[7],
                                 NT_screen[8], NT_screen[9], NT_screen[10], NT_screen[11],
                                 NT_screen[12], NT_screen[13], NT_screen[14], NT_screen[15]);
                        }
                    }, "draw");
                    
                    // Always sync NT_screen buffer to VCV display after plugin draw call
                    // This handles both direct NT_screen writes and NT_drawText/NT_drawShape calls
                    // Even if plugin returned false, it may have drawn text/shapes we want to display
                    VCVDisplayBuffer pluginBuffer;
                    syncNTScreenToVCVBuffer(pluginBuffer);
                    drawDisplayBuffer(args.vg, pluginBuffer);
                    pluginDrew = true; // Plugin attempted to draw, show the results even if it returned false
                    
                    static int syncCount = 0;
                    if (syncCount++ < 5) {
                        INFO("Synced NT_screen to VCV display buffer, attempt %d", syncCount);
                    }
                }
                
                if (!pluginDrew) {
                    // Show plugin name if no custom drawing
                    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                    nvgFontSize(args.vg, 14);
                    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    
                    if (distingModule->pluginFactory && distingModule->pluginFactory->name) {
                        nvgText(args.vg, 128, 32, distingModule->pluginFactory->name, NULL);
                    } else {
                        nvgText(args.vg, 128, 32, "Plugin Loaded", NULL);
                    }
                }
                
            } else {
                // Update emulator display
                module->emulatorCore.updateDisplay();
                
                // Draw the display buffer
                drawDisplayBuffer(args.vg, module->emulatorCore.getDisplayBuffer());
                
            }
        }
        
        nvgRestore(args.vg);
    }
    
    void drawPlaceholder(const DrawArgs& args) {
        // Draw placeholder when module is not available (browser preview)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        // Draw border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
        
        // Draw "OLED" text
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFontSize(args.vg, 14);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, "OLED DISPLAY", nullptr);
    }
    
    void syncNTScreenToVCVBuffer(VCVDisplayBuffer& buffer) {
        // Convert NT_screen buffer (4-bit grayscale, 2 pixels per byte) to VCV display buffer with full grayscale support
        // NT_screen format: 128*64 bytes, each byte contains 2 pixels (high nibble = even x, low nibble = odd x)
        // VCV format: 256*64 pixels, grayscale values 0-15
        
        buffer.clear();
        
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 256; x += 2) {
                int byte_idx = y * 128 + x / 2;
                uint8_t byte_val = NT_screen[byte_idx];
                
                // Extract high nibble (even x coordinate)
                uint8_t pixel_even = (byte_val >> 4) & 0x0F;
                // Extract low nibble (odd x coordinate)  
                uint8_t pixel_odd = byte_val & 0x0F;
                
                // Store full 4-bit grayscale values (0-15)
                buffer.setPixelGray(x, y, pixel_even);
                if (x + 1 < 256) {
                    buffer.setPixelGray(x + 1, y, pixel_odd);
                }
            }
        }
    }

    void drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer) {
        // Draw the display buffer pixel by pixel with full 4-bit grayscale support
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                uint8_t gray_value = buffer.getPixelGray(x, y);
                if (gray_value > 0) {
                    // Map 4-bit grayscale (0-15) to 0.0-1.0 range
                    float intensity = (float)gray_value / 15.0f;
                    
                    // Set color based on intensity (white pixels with varying alpha/brightness)
                    nvgFillColor(vg, nvgRGBA(255, 255, 255, (int)(intensity * 255)));
                    
                    nvgBeginPath(vg);
                    nvgRect(vg, x, y, 1, 1);
                    nvgFill(vg);
                }
            }
        }
    }
    
    void drawMenuInterface(NVGcontext* vg, DistingNT* module) {
        // Hardware-style two-column layout: PAGES | PARAMETERS + VALUES
        
        // Define column positions and widths
        const float pageColumn = 5;
        const float pageWidth = 65;
        const float paramColumn = 75;
        const float paramWidth = 85; 
        const float valueColumn = 165;
        const float valueWidth = 85;
        
        // Use small font for menu
        nvgFontSize(vg, 9);
        
        // === LEFT COLUMN: PAGE LIST (always visible, scrollable) ===
        const int visiblePages = 5;
        int pageStartIdx = 0;
        
        // Center current page in view
        if (module->currentPageIndex > 2 && module->parameterPages.size() > visiblePages) {
            pageStartIdx = std::min(module->currentPageIndex - 2, 
                                   std::max(0, (int)module->parameterPages.size() - visiblePages));
        }
        
        // Draw pages
        for (int i = 0; i < visiblePages && (pageStartIdx + i) < (int)module->parameterPages.size(); i++) {
            int pageIdx = pageStartIdx + i;
            const _NT_parameterPage& page = module->parameterPages[pageIdx];
            float y = 8 + i * 10;
            
            bool isCurrentPage = (pageIdx == module->currentPageIndex);
            
            // Brighter color for current page (instead of bold)
            nvgFillColor(vg, isCurrentPage ? nvgRGB(255, 255, 255) : nvgRGB(120, 120, 120));
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            
            // Truncate page name if too long
            char pageName[12];
            strncpy(pageName, page.name, 11);
            pageName[11] = '\0';
            nvgText(vg, pageColumn, y, pageName, NULL);
        }
        
        // (No font weight reset needed)
        
        // Page scroll indicators
        if (pageStartIdx > 0) {
            nvgFillColor(vg, nvgRGB(160, 160, 160));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(vg, pageColumn + pageWidth/2, 2, "▲", NULL);
        }
        if (pageStartIdx + visiblePages < (int)module->parameterPages.size()) {
            nvgFillColor(vg, nvgRGB(160, 160, 160));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(vg, pageColumn + pageWidth/2, 56, "▼", NULL);
        }
        
        // === RIGHT COLUMN: PARAMETER LIST WITH VALUES ===
        if (module->currentPageIndex < (int)module->parameterPages.size()) {
            const _NT_parameterPage& currentPage = module->parameterPages[module->currentPageIndex];
            
            if (currentPage.numParams > 0) {
                const int visibleParams = 5;
                int paramStartIdx = 0;
                
                // Center current parameter in view
                if (module->currentParamIndex > 2 && currentPage.numParams > visibleParams) {
                    paramStartIdx = std::min(module->currentParamIndex - 2, 
                                           std::max(0, (int)currentPage.numParams - visibleParams));
                }
                
                // Draw parameters with values
                for (int i = 0; i < visibleParams && (paramStartIdx + i) < currentPage.numParams; i++) {
                    int paramListIdx = paramStartIdx + i;
                    int paramIdx;
                    
                    // Handle default pages (no params array)
                    if (currentPage.params == nullptr) {
                        paramIdx = paramListIdx; // Sequential indexing
                    } else {
                        paramIdx = currentPage.params[paramListIdx];
                    }
                    
                    if (paramIdx < 0 || paramIdx >= (int)module->parameters.size()) continue;
                    
                    const _NT_parameter& param = module->parameters[paramIdx];
                    float y = 8 + i * 10;
                    
                    bool isCurrentParam = (paramListIdx == module->currentParamIndex);
                    
                    // PARAMETER NAME - brighter color for current parameter
                    nvgFillColor(vg, isCurrentParam ? nvgRGB(255, 255, 255) : nvgRGB(140, 140, 140));
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                    nvgText(vg, paramColumn, y, param.name, NULL);
                    
                    // PARAMETER VALUE (beside the name)
                    int value = module->routingMatrix[paramIdx];
                    char valueStr[32];
                    formatParameterValue(valueStr, param, value);
                    
                    nvgFillColor(vg, isCurrentParam ? nvgRGB(255, 255, 255) : nvgRGB(160, 160, 160));
                    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
                    nvgText(vg, valueColumn + valueWidth, y, valueStr, NULL);
                }
                
                // (No font weight reset needed)
                
                // Parameter scroll indicators
                if (paramStartIdx > 0) {
                    nvgFillColor(vg, nvgRGB(160, 160, 160));
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                    nvgText(vg, paramColumn + paramWidth/2, 2, "▲", NULL);
                }
                if (paramStartIdx + visibleParams < currentPage.numParams) {
                    nvgFillColor(vg, nvgRGB(160, 160, 160));
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                    nvgText(vg, paramColumn + paramWidth/2, 56, "▼", NULL);
                }
            }
        }
        
    }
    
    void formatParameterValue(char* str, const _NT_parameter& param, int value) const {
        // Apply scaling first
        float scaledValue = value;
        switch (param.scaling) {
            case kNT_scaling10: scaledValue = value / 10.0f; break;
            case kNT_scaling100: scaledValue = value / 100.0f; break;
            case kNT_scaling1000: scaledValue = value / 1000.0f; break;
        }
        
        // Format based on unit type
        switch (param.unit) {
            case kNT_unitNone:
                // Check if this is a Boolean parameter (min=0, max=1, no enum strings)
                if (param.min == 0 && param.max == 1 && param.enumStrings == nullptr) {
                    // Boolean parameter without enum strings
                    if (value == 0) {
                        strcpy(str, "Off");
                    } else {
                        strcpy(str, "On");
                    }
                } else {
                    snprintf(str, 32, "%d", value);
                }
                break;
                
            case kNT_unitEnum:
                if (param.enumStrings && value >= param.min && value <= param.max) {
                    strncpy(str, param.enumStrings[value - param.min], 31);
                    str[31] = '\0';
                } else if (param.min == 0 && param.max == 1 && param.enumStrings == nullptr) {
                    // Boolean enum without strings
                    if (value == 0) {
                        strcpy(str, "Off");
                    } else {
                        strcpy(str, "On");
                    }
                } else {
                    snprintf(str, 32, "%d", value);
                }
                break;
                
            case kNT_unitDb:
                snprintf(str, 32, "%.1f dB", scaledValue);
                break;
                
            case kNT_unitDb_minInf:
                if (value == param.min) {
                    strcpy(str, "-inf dB");
                } else {
                    snprintf(str, 32, "%.1f dB", scaledValue);
                }
                break;
                
            case kNT_unitPercent:
                snprintf(str, 32, "%d%%", value);
                break;
                
            case kNT_unitHz:
                if (scaledValue >= 1000) {
                    snprintf(str, 32, "%.1f kHz", scaledValue / 1000.0f);
                } else {
                    snprintf(str, 32, "%.1f Hz", scaledValue);
                }
                break;
                
            case kNT_unitSemitones:
                snprintf(str, 32, "%+d st", value);
                break;
                
            case kNT_unitCents:
                snprintf(str, 32, "%+d ct", value);
                break;
                
            case kNT_unitMs:
                snprintf(str, 32, "%.1f ms", scaledValue);
                break;
                
            case kNT_unitSeconds:
                snprintf(str, 32, "%.2f s", scaledValue);
                break;
                
            case kNT_unitFrames:
                snprintf(str, 32, "%d fr", value);
                break;
                
            case kNT_unitMIDINote:
                {
                    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                    int octave = (value / 12) - 1;
                    int note = value % 12;
                    snprintf(str, 32, "%s%d", noteNames[note], octave);
                }
                break;
                
            case kNT_unitMillivolts:
                snprintf(str, 32, "%d mV", value);
                break;
                
            case kNT_unitVolts:
                snprintf(str, 32, "%.2f V", scaledValue);
                break;
                
            case kNT_unitBPM:
                snprintf(str, 32, "%d BPM", value);
                break;
                
            case kNT_unitAudioInput:
            case kNT_unitCvInput:
                if (value == 0) {
                    strcpy(str, "None");
                } else if (value >= 1 && value <= 12) {
                    snprintf(str, 32, "Input %d", value);
                } else if (value >= 13 && value <= 20) {
                    snprintf(str, 32, "Output %d", value - 12);
                } else if (value >= 21 && value <= 28) {
                    snprintf(str, 32, "Aux %d", value - 20);
                } else {
                    snprintf(str, 32, "%d", value);
                }
                break;
                
            case kNT_unitAudioOutput:
            case kNT_unitCvOutput:
                if (value == 0) {
                    strcpy(str, "None");
                } else if (value >= 1 && value <= 12) {
                    snprintf(str, 32, "Input %d", value);
                } else if (value >= 13 && value <= 20) {
                    snprintf(str, 32, "Output %d", value - 12);
                } else if (value >= 21 && value <= 28) {
                    snprintf(str, 32, "Aux %d", value - 20);
                } else {
                    snprintf(str, 32, "%d", value);
                }
                break;
                
            case kNT_unitOutputMode:
                // Typical output modes
                if (value == 0) strcpy(str, "Direct");
                else if (value == 1) strcpy(str, "Add");
                else snprintf(str, 32, "Mode %d", value);
                break;
                
            default:
                snprintf(str, 32, "%d", value);
                break;
        }
    }
};

// Implementation of ContextAwareEncoderQuantity
std::string ContextAwareEncoderQuantity::getDisplayValueString() {
    if (!distingModule || distingModule->menuMode == DistingNT::MENU_OFF) {
        return EncoderParamQuantity::getDisplayValueString();
    }
    
    // Show current parameter name and value
    int paramIdx = distingModule->currentParamIndex;
    if (paramIdx >= 0 && paramIdx < (int)distingModule->parameters.size()) {
        const _NT_parameter& param = distingModule->parameters[paramIdx];
        int value = distingModule->routingMatrix[paramIdx];
        
        // Simple value display for now
        return std::string(param.name) + ": " + std::to_string(value);
    }
    
    return EncoderParamQuantity::getDisplayValueString();
}

// Implementation of DistingNTOutputPort
void DistingNTOutputPort::onHover(const HoverEvent& e) {
    PJ301MPort::onHover(e);
    
    // Update tooltip based on current routing
    if (distingModule && distingModule->isPluginLoaded()) {
        // Find parameter that routes to this output
        for (const auto& pair : distingModule->paramOutputRouting) {
            size_t paramIdx = pair.first;
            // Check if this parameter's routing would send to this output
            const std::string& paramName = distingModule->parameters[paramIdx].name;
            int targetOutput = -1;
            
            // Extract output number from parameter name
            if (sscanf(paramName.c_str(), "Output %d", &targetOutput) == 1 ||
                sscanf(paramName.c_str(), "Out %d", &targetOutput) == 1) {
                targetOutput--; // Convert to 0-based
                
                if (targetOutput == outputIndex) {
                    // Tooltip will be displayed through VCV's mechanism
                    return;
                }
            }
        }
    }
}

struct DistingNTWidget : ModuleWidget {
    DistingNTWidget(DistingNT* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DistingNT.svg")));

        // Add mounting screws (14HP module = 71.12mm width)
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // OLED Display (larger, centered at top)
        DistingNTOLEDWidget* display = new DistingNTOLEDWidget();
        display->box.pos = mm2px(Vec(8.0, 8.0));
        display->box.size = mm2px(Vec(55.12, 15.0));
        if (module) {
            display->module = module;
        }
        addChild(display);

        // 3 Pots (large knobs)
        addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(17.78, 35.0)), module, DistingNT::POT_L_PARAM));
        addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(35.56, 35.0)), module, DistingNT::POT_C_PARAM));
        addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(53.34, 35.0)), module, DistingNT::POT_R_PARAM));

        // 2 Encoders (infinite encoders with high sensitivity)
        auto* encoderL = createParamCentered<BefacoTinyKnob>(mm2px(Vec(26.67, 52.0)), module, DistingNT::ENCODER_L_PARAM);
        encoderL->speed = 10.0f;  // Much higher sensitivity for encoder behavior
        encoderL->snap = false;   // Disable snapping
        addParam(encoderL);
        
        auto* encoderR = createParamCentered<BefacoTinyKnob>(mm2px(Vec(44.45, 52.0)), module, DistingNT::ENCODER_R_PARAM);
        encoderR->speed = 10.0f;  // Much higher sensitivity for encoder behavior  
        encoderR->snap = false;   // Disable snapping
        addParam(encoderR);

        // 4 Buttons (vertical pairs - no LEDs)
        addParam(createParamCentered<TL1105>(mm2px(Vec(14.5, 48.0)), module, DistingNT::BUTTON_1_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(14.5, 56.0)), module, DistingNT::BUTTON_2_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(56.5, 48.0)), module, DistingNT::BUTTON_3_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(56.5, 56.0)), module, DistingNT::BUTTON_4_PARAM));

        // 12 Inputs (3 rows of 4) - balanced margins
        float inputStartY = 70.0;
        float jackSpacing = 9.0;
        float rowSpacing = 10.0;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                int index = row * 4 + col;
                float x = 10.0 + col * jackSpacing;  // Balanced positioning
                float y = inputStartY + row * rowSpacing;
                addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, DistingNT::AUDIO_INPUT_1 + index));
            }
        }

        // 8 Outputs (4 rows of 2) - balanced margins
        float outputStartX = 55.0;  // Balanced positioning
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 2; col++) {
                int index = row * 2 + col;
                float x = outputStartX + col * jackSpacing;
                float y = inputStartY + row * rowSpacing;
                
                // Create custom output port with dynamic tooltips
                DistingNTOutputPort* outputPort = createOutputCentered<DistingNTOutputPort>(
                    mm2px(Vec(x, y)), module, DistingNT::AUDIO_OUTPUT_1 + index);
                if (module) {
                    outputPort->distingModule = module;
                    outputPort->outputIndex = index;
                }
                addOutput(outputPort);
            }
        }
        
        // MIDI activity LEDs (near the display)
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(5.0, 8.0)), module, DistingNT::MIDI_INPUT_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(66.0, 8.0)), module, DistingNT::MIDI_OUTPUT_LIGHT));
    }
    
    void appendContextMenu(Menu* menu) override {
        DistingNT* module = dynamic_cast<DistingNT*>(this->module);
        if (!module) return;
        
        menu->addChild(new MenuSeparator);
        
        // Plugin loading submenu
        menu->addChild(createSubmenuItem("Plugin", "", [=](Menu* menu) {
            // Load plugin option
            menu->addChild(createMenuItem("Load Plugin...", "", [=]() {
                loadPluginDialog(module);
            }));
            
            // Recent folder shortcut
            if (!module->lastPluginFolder.empty()) {
                menu->addChild(createMenuItem("Load from Recent Folder...", 
                    module->lastPluginFolder, [=]() {
                    loadPluginDialog(module, module->lastPluginFolder);
                }));
            }
            
            // Unload option
            if (module->isPluginLoaded()) {
                menu->addChild(new MenuSeparator);
                menu->addChild(createMenuItem("Reload Plugin", "", [=]() {
                    module->reloadPlugin();
                }));
                menu->addChild(createMenuItem("Unload Plugin", "", [=]() {
                    module->unloadPlugin();
                }));
                
                // Show current plugin
                std::string filename = rack::system::getFilename(module->pluginPath);
                menu->addChild(createMenuLabel(string::f("Current: %s", filename.c_str())));
            }
        }));
        
        menu->addChild(new MenuSeparator);
        
        // MIDI Input submenu
        menu->addChild(createSubmenuItem("MIDI Input", "", [=](Menu* menu) {
            appendMidiMenu(menu, &module->midiInput);
        }));
        
        // MIDI Output submenu
        menu->addChild(createSubmenuItem("MIDI Output", "", [=](Menu* menu) {
            appendMidiMenu(menu, &module->midiOutput);
        }));
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Algorithm"));
        
        // TODO: Add algorithm selection items
        // This will be implemented when we have actual algorithms loaded
    }
    
    void loadPluginDialog(DistingNT* module, std::string startPath = "") {
        if (startPath.empty()) {
            startPath = module->lastPluginFolder.empty() ? 
                asset::user("") : module->lastPluginFolder;
        }
        
        osdialog_filters* filters = osdialog_filters_parse(
            "Disting NT Plugin:dylib,so,dll"
        );
        
        char* pathC = osdialog_file(OSDIALOG_OPEN, startPath.c_str(), NULL, filters);
        if (pathC) {
            std::string path = pathC;
            free(pathC);
            
            // Remember folder
            module->lastPluginFolder = rack::system::getDirectory(path);
            
            // Phase 1: Discover plugin specifications
            auto specInfo = module->discoverPluginSpecifications(path);
            
            if (specInfo.hasSpecifications()) {
                // Show specification dialog
                showSpecificationDialog(module, specInfo);
            } else {
                // No specifications needed, load directly
                module->loadPlugin(path);
            }
        }
        
        osdialog_filters_free(filters);
    }
    
    // Simplified specification dialog - using default values for now
    
    // Custom text field for numeric input with validation
    struct NumericTextField : ui::TextField {
        int32_t* valuePtr = nullptr;
        int32_t minValue = 0;
        int32_t maxValue = 0;
        
        NumericTextField(int32_t* valuePtr, int32_t minVal, int32_t maxVal) 
            : valuePtr(valuePtr), minValue(minVal), maxValue(maxVal) {
            box.size.x = 60.0f;
            if (valuePtr) {
                text = std::to_string(*valuePtr);
            } else {
                text = "0";
            }
        }
        
        void onSelectKey(const SelectKeyEvent& e) override {
            TextField::onSelectKey(e);
            
            // Update value when Enter is pressed or focus is lost
            if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
                updateValue();
            }
        }
        
        void onDeselect(const DeselectEvent& e) override {
            TextField::onDeselect(e);
            updateValue();
        }
        
        void updateValue() {
            if (valuePtr) {
                try {
                    int32_t newValue = std::stoi(text);
                    // Clamp to valid range
                    newValue = std::max(minValue, std::min(maxValue, newValue));
                    *valuePtr = newValue;
                    // Update text field to show clamped value
                    text = std::to_string(newValue);
                } catch (...) {
                    // Invalid input, restore previous value
                    text = std::to_string(*valuePtr);
                }
            }
        }
    };

    // Specification dialog window  
    struct SpecificationDialog : ui::MenuOverlay {
        DistingNT* module = nullptr;
        DistingNT::PluginSpecificationInfo specInfo;
        std::vector<int32_t> specificationValues;
        std::vector<NumericTextField*> textFields;
        
        SpecificationDialog(DistingNT* module, const DistingNT::PluginSpecificationInfo& info) 
            : module(module), specInfo(info) {
            
            // Validate specifications vector before using it
            if (specInfo.specifications.empty() || specInfo.specifications.size() > 64) {
                // Defensive fallback - don't create dialog if specifications are invalid
                return;
            }
            
            specificationValues.resize(specInfo.specifications.size());
            
            // Initialize with default values with bounds checking
            for (size_t i = 0; i < specInfo.specifications.size(); i++) {
                try {
                    // Validate that we can access this element safely
                    const auto& spec = specInfo.specifications.at(i);
                    specificationValues[i] = spec.def;
                } catch (const std::exception&) {
                    // If we can't access the specification safely, use default
                    specificationValues[i] = 0;
                }
            }
            
            // Create menu content
            ui::Menu* menu = new ui::Menu;
            menu->box.size.x = 400.0f; // Make menu wider to accommodate input fields
            
            // Title
            std::string titleText = specInfo.name.empty() ? "Configure Plugin" : ("Configure " + specInfo.name);
            menu->addChild(createMenuLabel(titleText));
            menu->addChild(new ui::MenuSeparator);
            
            if (!specInfo.description.empty()) {
                menu->addChild(createMenuLabel(specInfo.description));
                menu->addChild(new ui::MenuSeparator);
            }
            
            // Create input fields for each specification
            for (size_t i = 0; i < specInfo.specifications.size(); i++) {
                const auto& spec = specInfo.specifications[i];
                
                // Create a container for label and input field
                ui::MenuEntry* entry = new ui::MenuEntry;
                entry->box.size.x = menu->box.size.x;
                entry->box.size.y = 30.0f;
                
                // Create label with safe string handling - avoid dereferencing bad pointers
                ui::Label* label = new ui::Label;
                std::string specName = "Spec " + std::to_string(i); // Safe default
                
                // Skip any unsafe pointer access entirely for now
                // The spec.name pointer appears to be corrupted, so use fallback naming
                label->text = specName + " (" + std::to_string(spec.min) + " to " + std::to_string(spec.max) + "):";
                label->box.pos = math::Vec(10.0f, 8.0f);
                label->color = nvgRGB(0xFF, 0xFF, 0xFF);
                entry->addChild(label);
                
                // Create text field for input
                NumericTextField* textField = new NumericTextField(&specificationValues[i], spec.min, spec.max);
                textField->box.pos = math::Vec(menu->box.size.x - 80.0f, 5.0f);
                entry->addChild(textField);
                textFields.push_back(textField);
                
                menu->addChild(entry);
            }
            
            menu->addChild(new ui::MenuSeparator);
            
            // Buttons
            menu->addChild(createMenuItem("Load Plugin", "", [=]() {
                // Update all values from text fields before loading
                for (auto* field : textFields) {
                    field->updateValue();
                }
                // Load plugin with configured specifications
                module->loadPlugin(specInfo.path, specificationValues);
                requestDelete();
            }));
            
            menu->addChild(createMenuItem("Cancel", "", [=]() {
                requestDelete();
            }));
            
            addChild(menu);
        }
    };
    
    void showSpecificationDialog(DistingNT* module, const DistingNT::PluginSpecificationInfo& specInfo) {
        SpecificationDialog* dialog = new SpecificationDialog(module, specInfo);
        APP->scene->addChild(dialog);
    }
};

Model* modelDistingNT = createModel<DistingNT, DistingNTWidget>("DistingNT");