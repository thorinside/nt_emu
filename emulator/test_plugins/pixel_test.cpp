/*
 * Pixel Test Plugin for NT Emulator
 * Tests various pixel patterns to determine correct nibble order
 */

#include <distingnt/api.h>
#include <cstring>
#include <new>

enum TestPatterns {
    PATTERN_COLUMNS = 0,    // Alternating columns using 0xF0
    PATTERN_ROWS = 1,       // Alternating rows using 0xFF/0x00
    PATTERN_SINGLE = 2,     // Single pixels at known coords
    PATTERN_GRADIENT = 3,   // Smooth gradient 0-15
    PATTERN_DIRECT = 4,     // Direct NT_screen writes
    NUM_PATTERNS
};

struct PixelTestData {
    int current_pattern;
    int frame_counter;
    bool use_direct_buffer;
};

struct PixelTestAlgorithm : public _NT_algorithm {
    PixelTestData* data;
    
    PixelTestAlgorithm(PixelTestData* d) : data(d) {}
};

enum {
    kParamPattern,
    kParamMode
};

char const *const patternStrings[] = {
    "Columns (0xF0)",
    "Rows (0xFF/0x00)", 
    "Single Pixels",
    "Gradient",
    "Direct Buffer",
    NULL
};

char const *const modeStrings[] = {
    "API Calls",
    "Direct Buffer",
    NULL
};

_NT_parameter parameters[] = {
    {.name = "Pattern", .min = 0, .max = NUM_PATTERNS - 1, .def = 0, 
     .unit = kNT_unitEnum, .scaling = 0, .enumStrings = patternStrings},
    {.name = "Mode", .min = 0, .max = 1, .def = 0,
     .unit = kNT_unitEnum, .scaling = 0, .enumStrings = modeStrings}
};

uint8_t page1[] = {kParamPattern, kParamMode};

_NT_parameterPage pages[] = {
    {.name = "Test", .numParams = ARRAY_SIZE(page1), .params = page1}
};

_NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages
};

void calculateRequirements(_NT_algorithmRequirements &req, const int32_t *specifications) {
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof(PixelTestAlgorithm);
    req.dtc = sizeof(PixelTestData);
    req.dram = 0;
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs &ptrs, 
                        const _NT_algorithmRequirements &req, 
                        const int32_t *specifications) {
    PixelTestData* data = new (ptrs.dtc) PixelTestData();
    PixelTestAlgorithm* alg = new (ptrs.sram) PixelTestAlgorithm(data);
    
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    
    data->current_pattern = 0;
    data->frame_counter = 0;
    data->use_direct_buffer = false;
    
    return alg;
}

void setPixelDirect(int x, int y, uint8_t color) {
    if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
    
    int byte_idx = y * 128 + x / 2;
    color = color & 0x0F;
    
    if (x & 1) {
        // Odd x: test shows this should be low nibble
        NT_screen[byte_idx] = (NT_screen[byte_idx] & 0xF0) | color;
    } else {
        // Even x: test shows this should be high nibble
        NT_screen[byte_idx] = (NT_screen[byte_idx] & 0x0F) | (color << 4);
    }
}

void drawColumnPattern(bool direct) {
    // Pattern: 0xF0 bytes should show alternating white-black columns
    if (direct) {
        // Fill entire NT_screen buffer with 0xF0 pattern
        for (int y = 0; y < 64; y++) {
            for (int byte_x = 0; byte_x < 128; byte_x++) {
                int byte_idx = y * 128 + byte_x;
                NT_screen[byte_idx] = 0xF0;  // High nibble = 15 (white), Low nibble = 0 (black)
            }
        }
    } else {
        // Use API calls - same pattern but through API
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 256; x += 2) {
                NT_drawShapeI(kNT_point, x, y, x, y, 15);     // White pixel at even x
                NT_drawShapeI(kNT_point, x+1, y, x+1, y, 0); // Black pixel at odd x
            }
        }
    }
}

void drawRowPattern(bool direct) {
    // Alternating rows of 0xFF and 0x00
    if (direct) {
        for (int y = 0; y < 64; y++) {
            uint8_t pattern = (y & 1) ? 0xFF : 0x00;
            for (int byte_x = 0; byte_x < 128; byte_x++) {
                int byte_idx = y * 128 + byte_x;
                NT_screen[byte_idx] = pattern;
            }
        }
    } else {
        for (int y = 0; y < 64; y++) {
            uint8_t color = (y & 1) ? 15 : 0;
            for (int x = 0; x < 256; x++) {
                NT_drawShapeI(kNT_point, x, y, x, y, color);
            }
        }
    }
}

void drawSinglePixels(bool direct) {
    // Clear screen first
    if (direct) {
        memset(NT_screen, 0, sizeof(NT_screen));
    }
    
    // Draw single pixels at known coordinates
    // Test pixel at (0,0), (1,0), (2,0), (3,0)
    for (int x = 0; x < 4; x++) {
        if (direct) {
            setPixelDirect(x, 0, 15);
            setPixelDirect(x, 10, 8);   // Mid-gray
            setPixelDirect(x, 20, 4);   // Dark gray
        } else {
            NT_drawShapeI(kNT_point, x, 0, x, 0, 15);
            NT_drawShapeI(kNT_point, x, 10, x, 10, 8);
            NT_drawShapeI(kNT_point, x, 20, x, 20, 4);
        }
    }
    
    // Draw test patterns to show nibble ordering
    // Row 30: Test the nibble order with known values
    if (direct) {
        for (int x = 0; x < 8; x++) {
            int byte_idx = 30 * 128 + x;
            NT_screen[byte_idx] = 0x01 + x;  // Values 0x01, 0x02, 0x03, etc.
        }
    }
}

void drawGradient(bool direct) {
    // Horizontal gradient from 0 to 15
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 256; x++) {
            uint8_t color = (x * 15) / 255;
            if (direct) {
                setPixelDirect(x, y, color);
            } else {
                NT_drawShapeI(kNT_point, x, y, x, y, color);
            }
        }
    }
}

void drawDirectBufferTest(bool direct) {
    // Direct writes with specific patterns to test nibble order
    memset(NT_screen, 0, sizeof(NT_screen));
    
    // Write specific byte patterns
    // First row: alternating 0xF0 and 0x0F
    for (int x = 0; x < 128; x++) {
        NT_screen[x] = (x & 1) ? 0x0F : 0xF0;
    }
    
    // Second row: gradient
    for (int x = 0; x < 128; x++) {
        int row_offset = 128;
        uint8_t val = (x * 15) / 127;
        NT_screen[row_offset + x] = (val << 4) | val;
    }
    
    // Third row: specific test values
    for (int x = 0; x < 16; x++) {
        int row_offset = 256;
        NT_screen[row_offset + x] = 0x01 | (x << 4);
    }
}

void parameterChanged(_NT_algorithm *self_base, int p) {
    PixelTestAlgorithm* self = (PixelTestAlgorithm*)self_base;
    
    if (p == kParamPattern) {
        self->data->current_pattern = self->v[kParamPattern];
    } else if (p == kParamMode) {
        self->data->use_direct_buffer = (self->v[kParamMode] == 1);
    }
}

void step(_NT_algorithm *self_base, float *busFrames, int numFramesBy4) {
    PixelTestAlgorithm* self = (PixelTestAlgorithm*)self_base;
    self->data->frame_counter++;
}

bool draw(_NT_algorithm *self_base) {
    PixelTestAlgorithm* self = (PixelTestAlgorithm*)self_base;
    
    // Clear screen
    memset(NT_screen, 0, sizeof(NT_screen));
    
    // Draw current pattern
    switch (self->data->current_pattern) {
        case PATTERN_COLUMNS:
            drawColumnPattern(self->data->use_direct_buffer);
            break;
        case PATTERN_ROWS:
            drawRowPattern(self->data->use_direct_buffer);
            break;
        case PATTERN_SINGLE:
            drawSinglePixels(self->data->use_direct_buffer);
            break;
        case PATTERN_GRADIENT:
            drawGradient(self->data->use_direct_buffer);
            break;
        case PATTERN_DIRECT:
            drawDirectBufferTest(true);  // Always direct for this pattern
            break;
    }
    
    // Draw info text at bottom
    char info[64];
    const char* mode_str = self->data->use_direct_buffer ? "Direct" : "API";
    const char* pattern_str = patternStrings[self->data->current_pattern];
    
    // Simple info display
    NT_drawText(0, 50, pattern_str, 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(0, 58, mode_str, 15, kNT_textLeft, kNT_textNormal);
    
    return true;  // Hide parameter display
}

// Factory
_NT_factory factory = {
    .guid = NT_MULTICHAR('P', 'I', 'X', 'T'),
    .name = "Pixel Test",
    .description = "Test pixel format patterns",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = (uint32_t)(kNT_tagUtility),
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
};

// Plugin entry point
uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
        case kNT_selector_version:
            return kNT_apiVersionCurrent;
        case kNT_selector_numFactories:
            return 1;
        case kNT_selector_factoryInfo:
            return (uintptr_t)((data == 0) ? &factory : NULL);
    }
    return 0;
}