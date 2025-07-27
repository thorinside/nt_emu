# NT Emulator Pixel Format Documentation

## Overview

The Disting NT uses a packed pixel format for its 256×64 OLED display. This document describes the exact bit layout, conversion methods, and common implementation patterns.

## Buffer Format

### NT_screen Buffer Specification

- **Size**: 128 × 64 bytes = 8,192 bytes total
- **Display Resolution**: 256 × 64 pixels (2 pixels per byte)
- **Color Depth**: 4-bit grayscale per pixel (16 levels: 0=black, 15=white)
- **Memory Layout**: Row-major order

### Bit Layout

Each byte in `NT_screen` contains exactly 2 pixels:

```
Byte at NT_screen[i]:
┌─────────────┬─────────────┐
│ Bit 7-4     │ Bit 3-0     │
│ Even X      │ Odd X       │
│ Pixel       │ Pixel       │
└─────────────┴─────────────┘
```

**Memory Address Calculation:**
```c
int byte_index = y * 128 + x / 2;
```

**Pixel Extraction:**
```c
uint8_t byte_val = NT_screen[byte_index];
uint8_t even_pixel = (byte_val >> 4) & 0x0F;  // Bits 7-4
uint8_t odd_pixel = byte_val & 0x0F;           // Bits 3-0
```

**Pixel Assignment:**
```c
if (x & 1) {
    // Odd x coordinate: low nibble (bits 3-0)
    NT_screen[byte_index] = (NT_screen[byte_index] & 0xF0) | (pixel_value & 0x0F);
} else {
    // Even x coordinate: high nibble (bits 7-4)
    NT_screen[byte_index] = (NT_screen[byte_index] & 0x0F) | ((pixel_value & 0x0F) << 4);
}
```

## Examples

### Example 1: Single Pixel Manipulation

To set pixel at coordinate (5, 10) to gray level 12:

```c
void setPixel(int x, int y, uint8_t gray_value) {
    if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
    
    int byte_idx = y * 128 + x / 2;
    gray_value = gray_value & 0x0F;  // Ensure 4-bit value
    
    if (x & 1) {
        // Odd x: store in low nibble
        NT_screen[byte_idx] = (NT_screen[byte_idx] & 0xF0) | gray_value;
    } else {
        // Even x: store in high nibble
        NT_screen[byte_idx] = (NT_screen[byte_idx] & 0x0F) | (gray_value << 4);
    }
}

// Usage:
setPixel(5, 10, 12);  // x=5 is odd, so value goes in low nibble
```

### Example 2: Pattern Analysis

**Alternating Column Pattern (0xF0):**
```c
// Writing 0xF0 to a byte creates:
// High nibble (even x) = 0xF = 15 (white)
// Low nibble (odd x)   = 0x0 = 0  (black)
// Visual result: |████|    |████|    | (white-black columns)

for (int y = 0; y < 64; y++) {
    for (int byte_x = 0; byte_x < 128; byte_x++) {
        NT_screen[y * 128 + byte_x] = 0xF0;
    }
}
```

**Gradient Pattern:**
```c
// Create horizontal gradient from black (0) to white (15)
for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 256; x++) {
        uint8_t gray_level = (x * 15) / 255;
        setPixel(x, y, gray_level);
    }
}
```

## API Integration

### Drawing API Functions

The NT API provides higher-level drawing functions that automatically handle pixel packing:

```c
// Point/pixel drawing
NT_drawShapeI(kNT_point, x, y, x, y, color);

// Line drawing  
NT_drawShapeI(kNT_line, x0, y0, x1, y1, color);

// Rectangle (filled)
NT_drawShapeI(kNT_rectangle, x0, y0, x1, y1, color);

// Box (outline only)
NT_drawShapeI(kNT_box, x0, y0, x1, y1, color);

// Text rendering
NT_drawText(x, y, "Hello", color, kNT_textLeft, kNT_textNormal);
```

### VCV Rack Integration

The VCV Rack plugin converts NT_screen to displayable pixels:

```cpp
void syncNTScreenToVCVBuffer(VCVDisplayBuffer& buffer) {
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 256; x += 2) {
            int byte_idx = y * 128 + x / 2;
            uint8_t byte_val = NT_screen[byte_idx];
            
            // Extract pixels
            uint8_t pixel_even = (byte_val >> 4) & 0x0F;
            uint8_t pixel_odd = byte_val & 0x0F;
            
            // Store with full grayscale support
            buffer.setPixelGray(x, y, pixel_even);
            if (x + 1 < 256) {
                buffer.setPixelGray(x + 1, y, pixel_odd);
            }
        }
    }
}
```

## Grayscale Levels

The 4-bit grayscale provides 16 distinct levels:

| Value | Description | Visual |
|-------|-------------|--------|
| 0     | Black       | ⬛ |
| 1-3   | Dark Gray   | ◾ |
| 4-7   | Medium Gray | ◽ |
| 8-11  | Light Gray  | ⬜ |
| 12-15 | White       | 🤍 |

**Intensity Mapping:**
```c
// Convert 4-bit to float intensity (0.0 - 1.0)
float intensity = (float)pixel_value / 15.0f;

// Convert to 8-bit RGB
uint8_t rgb_value = (pixel_value * 255) / 15;
```

## Common Pitfalls and Solutions

### 1. Nibble Order Confusion

**Problem**: Assuming wrong nibble corresponds to even/odd pixels.

**Solution**: Always remember:
- **Even X coordinates** → **High nibble** (bits 7-4)
- **Odd X coordinates** → **Low nibble** (bits 3-0)

### 2. Incomplete Buffer Filling

**Problem**: Forgetting that each byte represents TWO pixels.

```c
// WRONG: Only fills half the screen
for (int x = 0; x < 256; x += 2) {
    NT_screen[y * 128 + x/2] = pattern;
}

// CORRECT: Fill entire row
for (int byte_x = 0; byte_x < 128; byte_x++) {
    NT_screen[y * 128 + byte_x] = pattern;
}
```

### 3. Coordinate Calculation Errors

**Problem**: Off-by-one errors in byte index calculation.

```c
// WRONG
int byte_idx = y * 128 + x;  // Assumes 1 pixel per byte

// CORRECT  
int byte_idx = y * 128 + x / 2;  // 2 pixels per byte
```

### 4. Value Range Issues

**Problem**: Using values outside 0-15 range.

```c
// WRONG: May corrupt adjacent pixel
NT_screen[idx] |= value;  

// CORRECT: Mask to 4 bits
NT_screen[idx] |= (value & 0x0F);
```

## Performance Considerations

### Optimized Byte Operations

For better performance, operate on entire bytes when possible:

```c
// Fast: Set both pixels at once
NT_screen[byte_idx] = (left_pixel << 4) | right_pixel;

// Slower: Set pixels individually 
setPixel(x, y, left_pixel);
setPixel(x+1, y, right_pixel);
```

### Lookup Tables

For intensive operations, pre-compute nibble operations:

```c
// Pre-computed masks for setting high/low nibbles
static const uint8_t HIGH_NIBBLE_MASK[16] = {
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
    0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0
};

// Fast pixel setting using lookup
if (x & 1) {
    NT_screen[byte_idx] = (NT_screen[byte_idx] & 0xF0) | pixel_value;
} else {
    NT_screen[byte_idx] = (NT_screen[byte_idx] & 0x0F) | HIGH_NIBBLE_MASK[pixel_value];
}
```

## Testing and Validation

### Pattern Tests

Use these patterns to verify correct implementation:

1. **Alternating Columns**: `0xF0` → Should show white-black vertical stripes
2. **Alternating Rows**: Alternate `0xFF` and `0x00` → Horizontal stripes  
3. **Gradient**: Values `0x01, 0x23, 0x45, 0x67...` → Smooth brightness transition
4. **Single Pixels**: `0xF0` at byte 0 → One white pixel at (0,0)

### Debug Functions

```c
void dumpPixelRegion(int start_x, int start_y, int width, int height) {
    printf("Pixel dump (%d,%d) %dx%d:\n", start_x, start_y, width, height);
    
    for (int y = start_y; y < start_y + height && y < 64; y++) {
        for (int x = start_x; x < start_x + width && x < 256; x += 2) {
            int byte_idx = y * 128 + x / 2;
            uint8_t byte_val = NT_screen[byte_idx];
            
            printf("Byte[%04d]=0x%02X -> Pixel[%3d,%2d]=%2d, Pixel[%3d,%2d]=%2d\n",
                   byte_idx, byte_val,
                   x, y, (byte_val >> 4) & 0x0F,
                   x+1, y, byte_val & 0x0F);
        }
    }
}
```

## References

- **SSD1322 OLED Controller**: Similar 4-bit pixel packing used in hardware displays
- **NT API Documentation**: `/emulator/include/distingnt/api.h`
- **VCV Plugin Implementation**: `/vcv-plugin/src/DistingNT.cpp`
- **Test Plugin**: `/emulator/test_plugins/pixel_test.cpp`

---

*This documentation ensures consistent pixel format handling across the NT emulator ecosystem and prevents the nibble order confusion that previously caused display issues.*