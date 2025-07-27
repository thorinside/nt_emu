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
    NT_drawText(10, 0, "Line", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(70, 0, "Box", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(130, 0, "Rect", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(200, 0, "Circle", 15, kNT_textLeft, kNT_textNormal);
    NT_drawText(10, 35, "Pixels", 15, kNT_textLeft, kNT_textNormal);
    
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
extern "C" _NT_algorithmFactory* pluginEntry() {
    static _NT_algorithmFactory factory = {
        .apiVersion = 1,
        .construct = []() -> _NT_algorithm* { return new DrawTest(); },
        .name = "Draw Test",
        .uuid = {0xD4, 0x4A, 0x77, 0x01, 0x22, 0x33, 0x44, 0x55,
                 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD}
    };
    return &factory;
}