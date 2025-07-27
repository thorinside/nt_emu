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
    // Test all three font sizes
    NT_drawText(5, 5, "ABC", 15, kNT_textLeft, kNT_textTiny);
    NT_drawText(5, 20, "ABC", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(5, 40, "ABC", 15, kNT_textLeft, kNT_textLarge);
    
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

extern "C" const _NT_factory* NT_getFactoryPtr() {
    return &factory;
}