# PRP: Display Drawing API Implementation for Disting NT Emulator

## Goal
Implement complete drawing API support for the Disting NT emulator, enabling plugins to render custom UI elements including text (3 sizes), shapes (lines, circles, rectangles), and direct pixel manipulation on the 256x64 monochrome display with 4-bit grayscale support.

## Why
Plugins need drawing capabilities to create custom user interfaces and visual feedback. Currently, the drawing API functions are stubbed out with incomplete implementations, preventing plugins from rendering anything beyond basic pixel manipulation. This feature is essential for plugin compatibility with the hardware Disting NT.

## What

### User-Visible Behavior
- Plugins can draw text in three sizes (tiny, normal, large) with alignment options
- Shapes render correctly: points, lines, boxes (unfilled), rectangles (filled), circles
- Antialiased drawing works for float-coordinate functions
- Direct pixel access via NT_screen array with 4-bit grayscale per pixel
- Display updates efficiently in both standalone emulator and VCV Rack plugin

### Technical Requirements
- Implement all drawing functions defined in `emulator/include/distingnt/api.h`
- Support 256x64 resolution with 4-bit grayscale (16 levels)
- Integrate bitmap fonts for text rendering
- Use efficient integer-based algorithms for primitive drawing
- Ensure thread-safe display updates in VCV Rack
- Maintain compatibility with existing plugin code

## All Needed Context

### API Reference
The complete API is defined in `emulator/include/distingnt/api.h`:
```cpp
// Text drawing
void NT_drawText(int x, int y, const char* str, int colour = 15, 
                 _NT_textAlignment align = kNT_textLeft, 
                 _NT_textSize size = kNT_textNormal);

// Shape drawing (integer coordinates) 
void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1, int y1, int colour = 15);

// Shape drawing (float coordinates, antialiased)
void NT_drawShapeF(_NT_shape shape, float x0, float y0, float x1, float y1, float colour = 15);

// Direct screen access - 256x64 pixels, 2 pixels per byte (4-bit each)
extern uint8_t NT_screen[128 * 64];
```

Shape types: `kNT_point`, `kNT_line`, `kNT_box`, `kNT_rectangle`, `kNT_circle`

### Display Architecture
- **Resolution**: 256x64 pixels
- **Color depth**: 4-bit grayscale (0-15)
- **Memory layout**: Each byte contains 2 pixels (high/low nibbles)
- **Total buffer**: 8192 bytes (128 * 64)

### Current Implementation Issues
File `emulator/src/api_shim.cpp` has bugs:
- Uses wrong shape constants (e.g., `kNT_shape_filled_rectangle` instead of `kNT_rectangle`)
- Coordinate system mismatch (expects x1,y1,x2,y2 but uses x,y,w,h)
- Missing implementations for most shapes
- Placeholder text rendering

### Font Requirements
Three fonts needed (as per manual):
1. **PixelMix** (normal) - 6pt pixel font, commercial license with Expert Sleepers
2. **Microsoft Selawik** (large) - SIL Open Font License 1.1
3. **Tom Thumb** (tiny) - 3x5 pixels, MIT license

Recommended: Start with Tom Thumb as it's MIT licensed and available in C++ format from Adafruit GFX library.

### Drawing Algorithms

**Bresenham's Line Algorithm** (integer-only, fast):
```cpp
void drawLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        setPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}
```

**Midpoint Circle Algorithm**:
Uses 8-way symmetry for efficiency. See research findings for full implementation.

### VCV Rack Integration Points
- `vcv-plugin/src/EmulatorCore.hpp` - Contains `VCVDisplayBuffer` class
- `vcv-plugin/src/display/OLEDWidget.cpp` - Renders display using NanoVG
- Display updates via `displayDirty` flag checked in `step()` method
- Thread safety needed for audio thread access

### Gotchas and Patterns
1. **Pixel packing**: High nibble = even x coordinate, low nibble = odd x coordinate
2. **Coordinate validation**: Always check bounds (0-255, 0-63)
3. **Thread safety**: Use atomic operations or mutex for VCV Rack
4. **Performance**: Batch pixel operations when possible
5. **Font rendering**: Use lookup tables for bitmap fonts
6. **Example usage**: See `test_plugins/examples/gainCustomUI.cpp` and `modJulia.cpp`

## Implementation Blueprint

### Phase 1: Fix Core Infrastructure
```pseudocode
1. Fix api_shim.cpp shape constants:
   - Replace kNT_shape_filled_rectangle → kNT_rectangle
   - Fix coordinate system (x1,y1,x2,y2 not x,y,w,h)
   
2. Update pixel access functions for 4-bit grayscale:
   void setPixel(int x, int y, uint8_t color) {
       if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
       int idx = y * 128 + x / 2;
       if (x & 1) {
           NT_screen[idx] = (NT_screen[idx] & 0xF0) | (color & 0x0F);
       } else {
           NT_screen[idx] = (NT_screen[idx] & 0x0F) | ((color & 0x0F) << 4);
       }
   }
```

### Phase 2: Implement Drawing Primitives
```pseudocode
3. Implement each shape in NT_drawShapeI:
   - kNT_point: Direct setPixel call
   - kNT_line: Bresenham's algorithm
   - kNT_box: Four line calls
   - kNT_rectangle: Filled rectangle loop
   - kNT_circle: Midpoint circle algorithm

4. Implement NT_drawShapeF with antialiasing:
   - Use Wu's algorithm for lines
   - Apply intensity-based antialiasing
   - Map intensities to grayscale levels
```

### Phase 3: Add Font Support
```pseudocode  
5. Integrate Tom Thumb font:
   - Add font data arrays to api_shim.cpp
   - Create glyph lookup function
   - Implement bitmap rendering in NT_drawText
   
6. Support text alignment and sizes:
   - Calculate text width for alignment
   - Scale bitmap for different sizes
   - Handle newlines and wrapping
```

### Phase 4: VCV Rack Integration
```pseudocode
7. Create API bridge for VCV:
   - Add getCurrentDisplayBuffer() to EmulatorCore
   - Implement thread-safe pixel updates
   - Set displayDirty flag on any draw operation
   
8. Connect drawing functions:
   - Route API calls to VCV display buffer
   - Ensure proper synchronization
   - Test with example plugins
```

### Task Checklist
- [ ] Fix shape constants and coordinate system in api_shim.cpp
- [ ] Implement 4-bit grayscale pixel access functions  
- [ ] Add Bresenham's line algorithm
- [ ] Add midpoint circle algorithm
- [ ] Implement filled/unfilled rectangles
- [ ] Integrate Tom Thumb font data
- [ ] Create text rendering with alignment
- [ ] Add Wu's antialiasing for float coordinates
- [ ] Create VCV Rack API bridge
- [ ] Implement thread-safe display updates
- [ ] Test with gainCustomUI.cpp example
- [ ] Test with modJulia.cpp example
- [ ] Profile and optimize hot paths

## Validation Gates

### Level 1: Syntax and Build
```bash
# Build emulator
cd emulator && make clean && make

# Build VCV plugin  
cd vcv-plugin && make clean && make
```

### Level 2: Unit Tests
```bash
# Create test file: emulator/tests/test_drawing.cpp
# Test each drawing primitive
cd emulator && make test && ./test_drawing
```

### Level 3: Visual Validation
```bash
# Run emulator with test plugin
cd emulator && ./nt_emu test_plugins/examples/gainCustomUI.cpp

# Check that UI elements appear correctly:
# - Text displays at correct position
# - Gain meter draws as filled rectangle
# - All shapes render properly
```

### Level 4: VCV Rack Integration
```bash
# Install plugin and test in VCV Rack
cd vcv-plugin && make install
# Launch VCV Rack
# Add Disting NT module
# Load gainCustomUI algorithm
# Verify display shows custom UI
```

### Level 5: Performance Validation
```bash
# Profile drawing performance
# Target: 60 FPS with full screen updates
# Use VCV Rack's performance meter
```

## External Resources

### Drawing Algorithms
- https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
- https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
- https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm

### Font Resources
- Tom Thumb font: https://robey.lag.net/2010/01/23/tiny-monospace-font.html
- Adafruit GFX Library (has Tom Thumb in C format): https://github.com/adafruit/Adafruit-GFX-Library
- Microsoft Selawik: https://github.com/microsoft/Selawik

### VCV Rack Development
- https://vcvrack.com/manual/PluginDevelopmentTutorial
- https://github.com/VCVRack/Rack/blob/v2/include/widget/FramebufferWidget.hpp

## Success Criteria

The implementation is complete when:
1. All drawing API functions work as documented
2. Text renders in three sizes with proper alignment
3. All shapes draw correctly (aliased and antialiased)
4. Display updates efficiently in both emulator and VCV Rack
5. Example plugins (gainCustomUI, modJulia) render their UIs correctly
6. Performance maintains 60 FPS in VCV Rack

## Confidence Score: 8/10

High confidence due to:
- Complete API documentation available
- Existing partial implementation to build upon
- Well-understood algorithms for drawing primitives
- Clear integration points in VCV Rack

Minor uncertainty around:
- Exact font bitmap data acquisition
- Thread synchronization edge cases
- Performance optimization requirements