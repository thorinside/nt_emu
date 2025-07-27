# TASK PRP: Fix Display Pixel Format Conversion

## Problem Summary
NT plugins use a packed pixel format (2 pixels per byte, 4-bit grayscale) that must be correctly interpreted and converted for display in VCV Rack. Current implementation has nibble ordering issues and inconsistent pixel handling.

## Understanding the Pixel Format

### NT_screen Buffer Format
- **Size**: 128 × 64 bytes = 8,192 bytes
- **Display**: 256 × 64 pixels (2 pixels per byte)
- **Each byte**: Contains 2 pixels, 4 bits each
- **Grayscale**: 16 levels (0=black, 15=white)

### Pixel Packing in Memory
```
Byte at NT_screen[i]:
[Bit 7-4: Pixel A] [Bit 3-0: Pixel B]

Which pixel is A and which is B? This is what we need to determine.
```

## Context

### Current Issues
1. **Nibble order confusion**: Code comments conflict with implementation
2. **Direct buffer access**: Plugins write directly to NT_screen bypassing conversion
3. **Multiple buffer copies**: Synchronization issues between buffers
4. **Grayscale to monochrome**: VCV only shows 1-bit, losing grayscale information

### Documentation
- SSD1322 OLED controllers use similar 4-bit packing
- Common convention: Low nibble = first pixel, high nibble = second pixel
- But implementation may differ - need to verify with pattern tests

### Test Plugins Available
- `/Users/nealsanche/nosuch/spectre`
- `/Users/nealsanche/nosuch/nt_grids`
- `/Users/nealsanche/nosuch/dnb_seq`
- `/Users/nealsanche/github/distingNT_API/examples/modJulia.cpp`

## Task List with Human Validation

### Phase 1: Create Diagnostic Pattern Generator

**TASK 1: Create pixel pattern test plugin**
```
CREATE emulator/test_plugins/pixel_test.cpp:
  - IMPLEMENT: NT plugin that draws known patterns
  - PATTERN 1: Alternating columns (0xF0 = white-black pattern)
  - PATTERN 2: Alternating rows using direct NT_screen writes
  - PATTERN 3: Single pixels at known coordinates
  - PATTERN 4: Gradient from 0 to 15 across screen
  - COMPILE: Using existing plugin build system
  - HUMAN VALIDATION: "Please compile this test plugin and confirm it builds successfully"
  - WAIT FOR: Human confirmation before proceeding
  - IF_FAIL: Check for compilation errors, fix syntax
  - ROLLBACK: Remove test file
```

**TASK 2: Create visual comparison tool**
```
CREATE vcv-plugin/src/debug/pixel_debug.h:
  - ADD: Debug output function to print pixel values
  - FUNCTION: dumpPixelRegion(x, y, width, height)
  - OUTPUT: Show hex values and interpreted pixels
  - FORMAT: "Byte[idx]=0xAB -> Pixel[x,y]=A, Pixel[x+1,y]=B"
  - HUMAN VALIDATION: "Please review the debug output format - is it clear?"
  - WAIT FOR: Human feedback on readability
  - IF_FAIL: Adjust format based on feedback
  - ROLLBACK: Remove debug file
```

### Phase 2: Determine Correct Nibble Order

**TASK 3: Run pattern test with current implementation**
```
TEST current nibble ordering:
  - HUMAN ACTION: "Please run the pixel_test plugin in VCV Rack"
  - OBSERVE: Does alternating column pattern (0xF0) show as:
    - Option A: White pixel, then black pixel (high nibble first)
    - Option B: Black pixel, then white pixel (low nibble first)
  - DOCUMENT: Take screenshot of the display
  - HUMAN VALIDATION: "What pattern do you see? Please describe"
  - WAIT FOR: Human description of observed pattern
  - RECORD: Current behavior for comparison
```

**TASK 4: Test direct buffer writes**
```
TEST direct NT_screen writes:
  - HUMAN ACTION: "Run test plugin that writes 0xF0 to NT_screen[0]"
  - HUMAN ACTION: "Also run test that uses NT_drawShapeI to draw at (0,0) and (1,0)"
  - COMPARE: Do both methods produce the same visual result?
  - HUMAN VALIDATION: "Do the patterns match? Yes/No and describe any differences"
  - WAIT FOR: Human comparison results
  - IF_MISMATCH: We have found the nibble order bug
```

### Phase 3: Fix Nibble Order (If Needed)

**TASK 5: Update pixel unpacking logic**
```
EDIT vcv-plugin/src/DistingNT.cpp syncNTScreenToVCVBuffer():
  - CURRENT: High nibble = even x, low nibble = odd x
  - IF TESTS SHOW REVERSED:
    - CHANGE TO: Low nibble = even x, high nibble = odd x
    - UPDATE: Pixel extraction logic
  - HUMAN VALIDATION: "Please recompile and test with pattern plugin"
  - WAIT FOR: "Does the alternating pattern now display correctly?"
  - IF_FAIL: Try opposite nibble order
  - ROLLBACK: Restore original code
```

**TASK 6: Update API shim to match**
```
EDIT emulator/src/core/api_shim.cpp setPixel():
  - ENSURE: Nibble order matches VCV implementation
  - UPDATE: Comments to clearly document order
  - ADD: ASCII diagram showing bit layout
  - HUMAN VALIDATION: "Please test with standalone emulator"
  - WAIT FOR: "Does the display match VCV Rack output?"
  - IF_FAIL: Check for x coordinate calculation errors
  - ROLLBACK: Restore original implementation
```

### Phase 4: Add Grayscale Support

**TASK 7: Implement grayscale rendering**
```
EDIT vcv-plugin/src/display/OLEDWidget.cpp:
  - CURRENT: 1-bit threshold at value 8
  - CHANGE: Render all 16 grayscale levels
  - MAP: 4-bit value to NanoVG color (0.0 to 1.0)
  - USE: nvgFillColor with gray value
  - HUMAN VALIDATION: "Please test gradient pattern - do you see 16 distinct gray levels?"
  - WAIT FOR: Human confirmation of grayscale rendering
  - IF_FAIL: Check color calculation (value / 15.0)
  - ROLLBACK: Return to 1-bit rendering
```

### Phase 5: Comprehensive Testing

**TASK 8: Test with real plugins**
```
TEST with graphics-intensive plugins:
  - HUMAN ACTION: "Please test these plugins in order:"
    1. modJulia.cpp - "Do you see the Julia set fractal?"
    2. spectre - "Do the spectrum bars display correctly?"
    3. nt_grids - "Are the grid patterns visible?"
    4. dnb_seq - "Does the sequencer UI render properly?"
  - DOCUMENT: Screenshot each plugin's display
  - HUMAN VALIDATION: "Do all plugins display correctly? List any issues"
  - WAIT FOR: Complete test results
  - IF_ISSUES: Create specific fix for each problem
```

**TASK 9: Create pixel format documentation**
```
CREATE docs/pixel_format.md:
  - DOCUMENT: Exact bit layout with examples
  - INCLUDE: Conversion formulas
  - ADD: Common pitfalls section
  - EXAMPLE: Show how to write specific patterns
  - HUMAN VALIDATION: "Please review - is this clear for future developers?"
  - WAIT FOR: Documentation feedback
  - UPDATE: Based on feedback
```

### Phase 6: Performance Optimization

**TASK 10: Optimize pixel conversion**
```
OPTIMIZE vcv-plugin/src/DistingNT.cpp:
  - CURRENT: Pixel-by-pixel conversion
  - ADD: Lookup table for nibble extraction
  - CONSIDER: SIMD operations if available
  - BENCHMARK: Time the conversion
  - HUMAN VALIDATION: "Please run CPU meter in VCV - what's the usage?"
  - WAIT FOR: Performance metrics
  - IF_SLOW: Try byte-wise operations
  - ROLLBACK: If optimization causes bugs
```

## Pattern Test Reference

### Test Patterns for Validation

1. **Column Test** (0xF0 pattern)
   ```
   If high nibble = first pixel: |████|    |████|    |
   If low nibble = first pixel:  |    |████|    |████|
   ```

2. **Row Test** (alternating 0xFF and 0x00)
   ```
   Should show alternating white and black horizontal lines
   ```

3. **Single Pixel Test**
   ```
   Write 0xF0 at position 0
   Should light up pixel at (0,0) or (1,0) depending on nibble order
   ```

4. **Gradient Test**
   ```
   Values 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
   Should show smooth gradient from black to white
   ```

## Validation Commands

### Build Test Plugin
```bash
cd emulator/test_plugins
g++ -shared -fPIC -o pixel_test.dylib pixel_test.cpp -I../include
```

### Test in VCV Rack
```bash
cd vcv-plugin && make install
# Launch VCV Rack
# Load DistingNT module
# Load pixel_test algorithm
# Observe display output
```

### Test in Standalone
```bash
cd emulator
./nt_emu test_plugins/pixel_test.dylib
# Compare output with VCV Rack
```

## Success Criteria

1. ✓ Pattern tests clearly show nibble order
2. ✓ Direct buffer writes match API drawing calls
3. ✓ All test plugins render correctly
4. ✓ Grayscale levels visible (not just black/white)
5. ✓ Performance acceptable (< 1% CPU)
6. ✓ Documentation prevents future confusion

## Troubleshooting Guide

### If patterns are inverted:
- Swap nibble extraction order
- Check x coordinate even/odd logic

### If pixels are shifted:
- Verify byte index calculation
- Check for off-by-one errors in x coordinate

### If grayscale looks wrong:
- Verify value range (0-15)
- Check threshold values
- Ensure proper scaling to 0.0-1.0

### If performance is poor:
- Profile the conversion loop
- Consider caching unchanging pixels
- Use lookup tables for nibble extraction

## Notes for Implementer

- **Be patient**: Pixel format issues can be tricky to debug visually
- **Document everything**: Future developers will thank you
- **Test incrementally**: Don't change too much at once
- **Use screenshots**: Visual comparison helps catch subtle issues
- **Ask for help**: If patterns don't make sense, describe what you see

This systematic approach with human validation at each step ensures we correctly identify and fix the pixel format issues without guessing.