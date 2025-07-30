#include <distingnt/api.h>
#include <cstring>
#include <new>

struct FontTest : _NT_algorithm {
    FontTest() {}
    ~FontTest() {}
};

void calculateRequirements(_NT_algorithmRequirements &req, const int32_t *specifications) {
    req.numParameters = 0;
    req.sram = sizeof(FontTest);
    req.dtc = 0;
    req.dram = 0;
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs &ptrs, 
                        const _NT_algorithmRequirements &req, 
                        const int32_t *specifications) {
    return new (ptrs.sram) FontTest();
}

bool draw(_NT_algorithm *self) {
    // Test text for all three font sizes
    const char* test_text = "Quick brown fox";
    
    // Draw labels and test text in all three font sizes
    NT_drawText(5, 5, "TINY:", 15, kNT_textLeft, kNT_textTiny);
    NT_drawText(40, 5, test_text, 15, kNT_textLeft, kNT_textTiny);
    
    NT_drawText(5, 20, "NORMAL:", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(60, 20, test_text, 15, kNT_textLeft, kNT_textNormal);
    
    NT_drawText(5, 40, "LARGE:", 15, kNT_textLeft, kNT_textLarge);
    NT_drawText(60, 40, test_text, 15, kNT_textLeft, kNT_textLarge);
    
    return true; // Hide parameter display
}

// Factory
_NT_factory factory = {
    .guid = NT_MULTICHAR('F', 'O', 'N', 'T'),
    .name = "Font Test",
    .description = "Test all three font sizes",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = NULL,
    .step = NULL,
    .draw = draw,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = (uint32_t)(kNT_tagUtility),
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
};

// Plugin entry point for NT emulator
extern "C" _NT_factory* pluginFactory() {
    return &factory;
}