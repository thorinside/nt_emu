# TASK PRP: Drawing Primitives Test Plugin

## Goal
Create a test plugin that validates all drawing primitive APIs work correctly in the emulator by rendering one of each shape type with proper byte packing.

## Context

### Screen Specs
- 256x64 pixels, 4-bit grayscale (0=black, 15=white)  
- Buffer: `NT_screen[128 * 64]` - 2 pixels per byte
- Even X = high nibble, Odd X = low nibble

### Available APIs
```cpp
void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour = 15);
// Shapes: kNT_point, kNT_line, kNT_box, kNT_rectangle, kNT_circle
```

### Reference Pattern
- emulator/plugins/pixeltest_plugin.h - pixel drawing example
- emulator/plugins/helloworld_plugin.h - basic plugin structure

## Tasks

### 1. Create Plugin Structure
**ACTION** emulator/plugins/drawtest_plugin.h:
```cpp
// Based on helloworld_plugin.h pattern
struct DrawTest : _NT_algorithm {
    // No parameters needed - just visual test
    void draw(int deltaT) override {
        // Clear screen
        NT_clearScreen();
        
        // Draw each primitive...
        return true; // Hide parameter display
    }
};
```
**VALIDATE**: File exists with proper structure

### 2. Implement Shape Drawing
**ACTION** Update draw() function:
```cpp
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
```
**VALIDATE**: Compile with `make`

### 3. Add Plugin Entry
**ACTION** Add at end of file:
```cpp
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
```
**VALIDATE**: Plugin appears in emulator

### 4. Iterate Visual Validation
**ACTION** Run emulator and verify:
- Line appears diagonal
- Box is unfilled rectangle outline
- Rectangle is filled solid
- Circle is round, not elliptical
- Pixel pattern shows checkerboard
- All shapes use correct grayscale values

**IF_FAIL**: Check byte packing in API shim, verify coordinates

## Validation Loop
1. `make` - Compile emulator
2. `./nt_emu` - Run and select "Draw Test" plugin  
3. Visual inspection with user
4. Adjust coordinates/colors as needed
5. Repeat until all shapes render correctly

## Success Criteria
- All 5 primitive types visible
- Correct shapes (not distorted)
- Proper grayscale values
- No pixel corruption/artifacts