#include <distingnt/api.h>
#include <cstring>
#include <new>

struct DrawTest : _NT_algorithm {
    DrawTest() {}
    ~DrawTest() {}
};

void calculateRequirements(_NT_algorithmRequirements &req, const int32_t *specifications) {
    req.numParameters = 0;
    req.sram = sizeof(DrawTest);
    req.dtc = 0;
    req.dram = 0;
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs &ptrs, 
                        const _NT_algorithmRequirements &req, 
                        const int32_t *specifications) {
    return new (ptrs.sram) DrawTest();
}

bool draw(_NT_algorithm *self) {
    // Layout primitives across screen for visibility
    // Line: diagonal across top-left
    NT_drawShapeI(kNT_line, 10, 10, 50, 30, 15);

    // Box: unfilled rectangle  
    NT_drawShapeI(kNT_box, 70, 10, 110, 30, 12);

    // Rectangle: filled
    NT_drawShapeI(kNT_rectangle, 130, 10, 170, 30, 8);

    // Circle: center at (210, 20), radius ~10
    NT_drawShapeI(kNT_circle, 200, 10, 220, 30, 15);

    // Pixel pattern: checkerboard at bottom
    for(int y = 40; y < 50; y++) {
        for(int x = 10; x < 30; x++) {
            if((x + y) & 1) {
                NT_drawShapeI(kNT_point, x, y, x, y, 15);
            }
        }
    }

    // Add labels
    NT_drawText(10, 0, "Line", 15);
    NT_drawText(70, 0, "Box", 15);
    NT_drawText(130, 0, "Rect", 15);
    NT_drawText(200, 0, "Circle", 15);
    NT_drawText(10, 35, "Pixels", 15);
    
    return true; // Hide parameter display
}

// Factory
_NT_factory factory = {
    .guid = NT_MULTICHAR('D', 'R', 'A', 'W'),
    .name = "Draw Test",
    .description = "Test drawing primitive APIs",
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