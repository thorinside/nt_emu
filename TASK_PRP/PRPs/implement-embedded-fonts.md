# TASK PRP: Implement Embedded Bitmap Fonts

## Goal
Replace placeholder font implementations with properly licensed embedded bitmap fonts: PixelMix (normal), Tom Thumb (tiny), and Microsoft Selawik (large) converted to bitmap format.

## Why
- Current implementation uses basic placeholder fonts that don't match the design specifications
- Proper fonts are required for accurate UI rendering and professional appearance
- Font licenses must be properly handled and documented

## What
### User-Visible Behavior
- Text rendering will use the correct pixel fonts as specified in the manual
- Three font sizes available: tiny (Tom Thumb 3x5), normal (PixelMix 6pt), large (Selawik converted to bitmap)
- Consistent appearance between emulator and VCV plugin

### Technical Requirements
- Embed font data as C++ bitmap arrays (VCV standard approach)
- Support ASCII character set (32-126) minimum
- Maintain 4-bit grayscale rendering capability
- Consolidate duplicate implementations between emulator and VCV plugin

## Context
### Documentation
```yaml
context:
  docs:
    - url: https://github.com/robey/tom-thumb
      focus: MIT licensed 3x5 pixel font bitmap data
    
    - url: https://github.com/microsoft/Selawik
      focus: SIL Open Font License, TTF format
    
    - url: https://manual.vcvrack.com/PluginDevelopmentTutorial#custom-widgets
      focus: VCV Rack font embedding patterns

  patterns:
    - file: emulator/src/core/api_shim.cpp:407-504
      copy: Current bitmap font implementation pattern
    
    - file: vcv-plugin/src/DistingNT.cpp:36-137
      copy: VCV-specific font array format

  gotchas:
    - issue: "PixelMix has commercial license"
      fix: "Must obtain font file from Expert Sleepers or use open alternative"
    
    - issue: "Selawik is TTF format"
      fix: "Convert to bitmap using FontForge or similar tool"
    
    - issue: "Code duplication between emulator and VCV"
      fix: "Create shared header file for font data"
```

## Task Breakdown

### SETUP path/fonts:
- OPERATION: Create fonts directory and documentation
- VALIDATE: `ls -la fonts/ && cat fonts/README.md`
- IF_FAIL: `mkdir -p fonts`
- ROLLBACK: `rm -rf fonts`

```bash
mkdir -p fonts
cat > fonts/README.md << 'EOF'
# Font Licenses

## PixelMix (normal font)
- Author: Andrew Tyler
- License: Commercial license held by Expert Sleepers Ltd
- Usage: 6pt pixel font for normal UI text

## Tom Thumb (tiny font)
- Author: Robey Pointer
- License: MIT
- Usage: 3x5 pixel font for tiny UI elements
- Source: https://github.com/robey/tom-thumb

## Microsoft Selawik (large font)
- Author: Microsoft Corporation
- License: SIL Open Font License 1.1
- Usage: Converted to bitmap for large UI text
- Source: https://github.com/microsoft/Selawik
EOF
```

### TASK1 fonts/tom_thumb.h:
- OPERATION: Create Tom Thumb bitmap font header
- VALIDATE: `grep -E "tomThumbFont\[95\]\[15\]" fonts/tom_thumb.h`
- IF_FAIL: Check array dimensions (95 chars × 15 bytes per char)
- ROLLBACK: `git checkout fonts/tom_thumb.h`

```cpp
// Tom Thumb 3x5 pixel font (MIT License)
// Author: Robey Pointer
// Adapted for NT emulator

#pragma once

namespace fonts {

// 3x5 font, 5 bytes per character (1 byte per row)
// Characters 32-126 (95 total)
const unsigned char tomThumbFont[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space (32)
    {0x00, 0x17, 0x00, 0x00, 0x00}, // !
    {0x03, 0x00, 0x03, 0x00, 0x00}, // "
    // ... complete character set from Tom Thumb source
};

const int TOMTHUMB_WIDTH = 3;
const int TOMTHUMB_HEIGHT = 5;
const int TOMTHUMB_SPACING = 4; // 3 + 1 pixel spacing

} // namespace fonts
```

### TASK2 fonts/pixelmix.h:
- OPERATION: Create PixelMix bitmap font header (or alternative)
- VALIDATE: `grep -E "pixelMixFont\[95\]\[" fonts/pixelmix.h`
- IF_FAIL: Verify license or use open alternative like "Pixel Operator"
- ROLLBACK: `git checkout fonts/pixelmix.h`

```cpp
// PixelMix 6pt pixel font
// Commercial license held by Expert Sleepers Ltd
// OR: Use open alternative if PixelMix unavailable

#pragma once

namespace fonts {

// 5x7 font with variable width
// Characters 32-126 (95 total)
const unsigned char pixelMixFont[95][7] = {
    // Font data here
};

const unsigned char pixelMixWidths[95] = {
    // Character widths for proportional spacing
};

const int PIXELMIX_HEIGHT = 7;
const int PIXELMIX_SPACING = 1;

} // namespace fonts
```

### TASK3 tools/ttf_to_bitmap.py:
- OPERATION: Create TTF to bitmap conversion tool
- VALIDATE: `python3 tools/ttf_to_bitmap.py --help`
- IF_FAIL: `pip install pillow fonttools`
- ROLLBACK: `rm tools/ttf_to_bitmap.py`

```python
#!/usr/bin/env python3
"""Convert TTF font to C++ bitmap array for VCV Rack"""

import argparse
from PIL import Image, ImageDraw, ImageFont
import numpy as np

def ttf_to_bitmap(ttf_path, size, output_name):
    """Convert TTF font to bitmap array"""
    font = ImageFont.truetype(ttf_path, size)
    
    # Generate header file with bitmap data
    with open(f"fonts/{output_name}.h", "w") as f:
        f.write(f"// Generated from {ttf_path}\n")
        f.write(f"#pragma once\n\n")
        f.write(f"namespace fonts {{\n\n")
        
        # Process ASCII 32-126
        f.write(f"const unsigned char {output_name}Font[95][{height}] = {{\n")
        
        for char_code in range(32, 127):
            char = chr(char_code)
            # Render character to bitmap
            # ... conversion logic ...
            
        f.write("};\n\n")
        f.write(f"}} // namespace fonts\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ttf", required=True, help="Input TTF file")
    parser.add_argument("--size", type=int, default=12, help="Font size")
    parser.add_argument("--output", required=True, help="Output name")
    args = parser.parse_args()
    
    ttf_to_bitmap(args.ttf, args.size, args.output)
```

### TASK4 fonts/selawik_bitmap.h:
- OPERATION: Convert Selawik TTF to bitmap font
- VALIDATE: `test -f fonts/selawik_bitmap.h && grep "selawikFont" fonts/selawik_bitmap.h`
- IF_FAIL: `python3 tools/ttf_to_bitmap.py --ttf Selawik.ttf --size 12 --output selawik_bitmap`
- ROLLBACK: `rm fonts/selawik_bitmap.h`

### TASK5 emulator/src/core/fonts.h:
- OPERATION: Create unified font interface header
- VALIDATE: `grep -E "drawText.*FontType" emulator/src/core/fonts.h`
- IF_FAIL: Check include paths and namespace
- ROLLBACK: `git checkout emulator/src/core/fonts.h`

```cpp
#pragma once

#include "../../../fonts/tom_thumb.h"
#include "../../../fonts/pixelmix.h"
#include "../../../fonts/selawik_bitmap.h"

namespace NT {

enum class FontType {
    TINY,    // Tom Thumb 3x5
    NORMAL,  // PixelMix 6pt
    LARGE    // Selawik bitmap
};

struct FontMetrics {
    const unsigned char* data;
    int width;
    int height;
    int spacing;
    const unsigned char* widths; // for proportional fonts
};

FontMetrics getFontMetrics(FontType type);

void drawChar(int x, int y, char c, FontType font = FontType::NORMAL);
void drawText(int x, int y, const char* text, FontType font = FontType::NORMAL);

} // namespace NT
```

### TASK6 emulator/src/core/fonts.cpp:
- OPERATION: Implement unified font rendering
- VALIDATE: `g++ -c emulator/src/core/fonts.cpp -o /tmp/fonts.o`
- IF_FAIL: Check includes and font array bounds
- ROLLBACK: `git checkout emulator/src/core/fonts.cpp`

```cpp
#include "fonts.h"
#include "api_shim.h"

namespace NT {

FontMetrics getFontMetrics(FontType type) {
    switch (type) {
    case FontType::TINY:
        return {
            reinterpret_cast<const unsigned char*>(fonts::tomThumbFont),
            fonts::TOMTHUMB_WIDTH,
            fonts::TOMTHUMB_HEIGHT,
            fonts::TOMTHUMB_SPACING,
            nullptr
        };
    case FontType::NORMAL:
        return {
            reinterpret_cast<const unsigned char*>(fonts::pixelMixFont),
            0, // variable width
            fonts::PIXELMIX_HEIGHT,
            fonts::PIXELMIX_SPACING,
            fonts::pixelMixWidths
        };
    case FontType::LARGE:
        return {
            reinterpret_cast<const unsigned char*>(fonts::selawikFont),
            fonts::SELAWIK_WIDTH,
            fonts::SELAWIK_HEIGHT,
            fonts::SELAWIK_SPACING,
            nullptr
        };
    }
}

void drawChar(int x, int y, char c, FontType font) {
    if (c < 32 || c > 126) return;
    
    FontMetrics fm = getFontMetrics(font);
    int charIndex = c - 32;
    
    // Render character bitmap
    for (int row = 0; row < fm.height; row++) {
        unsigned char rowData = fm.data[charIndex * fm.height + row];
        int width = fm.widths ? fm.widths[charIndex] : fm.width;
        
        for (int col = 0; col < width; col++) {
            if (rowData & (1 << (7 - col))) {
                setPixel(x + col, y + row, 15); // white
            }
        }
    }
}

// ... drawText implementation ...

} // namespace NT
```

### TASK7 emulator/src/core/api_shim.cpp:
- OPERATION: Update to use new font system
- VALIDATE: `grep -n "fonts.h" emulator/src/core/api_shim.cpp`
- IF_FAIL: Add include at top of file
- ROLLBACK: `git checkout emulator/src/core/api_shim.cpp`

```diff
+ #include "fonts.h"

- // Old font implementation
- static const uint8_t font5x7[96][7] = { ... };

void NT_drawText(int x, int y, const char* text, NT_TextSize size, NT_Align align) {
-    // Old implementation
+    FontType fontType;
+    switch (size) {
+    case NT_TEXT_TINY: fontType = FontType::TINY; break;
+    case NT_TEXT_NORMAL: fontType = FontType::NORMAL; break;
+    case NT_TEXT_LARGE: fontType = FontType::LARGE; break;
+    }
+    
+    NT::drawText(x, y, text, fontType);
}
```

### TASK8 vcv-plugin/src/DistingNT.cpp:
- OPERATION: Update VCV plugin to use shared fonts
- VALIDATE: `grep -n "fonts.h" vcv-plugin/src/DistingNT.cpp`
- IF_FAIL: Check relative include path
- ROLLBACK: `git checkout vcv-plugin/src/DistingNT.cpp`

```diff
+ #include "../../emulator/src/core/fonts.h"

- // Remove duplicate font implementation
- static const uint8_t font5x7[96][7] = { ... };

void NT_drawText(int x, int y, const char* text, NT_TextSize size, NT_Align align) {
-    // Old duplicate implementation
+    // Use shared implementation
+    FontType fontType;
+    switch (size) {
+    case NT_TEXT_TINY: fontType = FontType::TINY; break;
+    case NT_TEXT_NORMAL: fontType = FontType::NORMAL; break;
+    case NT_TEXT_LARGE: fontType = FontType::LARGE; break;
+    }
+    
+    NT::drawText(x, y, text, fontType);
}
```

## Validation Loop

### Level 1: Syntax & Compilation
```bash
# Compile font system
g++ -c emulator/src/core/fonts.cpp -I. -std=c++11 -o /tmp/fonts.o

# Compile api_shim with fonts
g++ -c emulator/src/core/api_shim.cpp -I. -std=c++11 -o /tmp/api_shim.o

# Test VCV plugin compilation
cd vcv-plugin && make clean && make
```

### Level 2: Font Rendering Test
```bash
# Create test program
cat > test_fonts.cpp << 'EOF'
#include "emulator/src/core/fonts.h"
#include <iostream>

int main() {
    // Test each font type
    NT::drawText(10, 10, "Tiny Font Test", NT::FontType::TINY);
    NT::drawText(10, 20, "Normal Font Test", NT::FontType::NORMAL);
    NT::drawText(10, 35, "Large Font Test", NT::FontType::LARGE);
    
    std::cout << "Font rendering test complete\n";
    return 0;
}
EOF

g++ test_fonts.cpp -o test_fonts && ./test_fonts
```

### Level 3: Visual Validation
```bash
# Run emulator with font test
./build_emulator.sh && ./emulator --test-fonts

# Check VCV plugin
cd vcv-plugin && make install
# Launch VCV Rack and visually verify fonts
```

### Level 4: License Compliance
```bash
# Verify all font licenses documented
test -f fonts/README.md && grep -E "(PixelMix|Tom Thumb|Selawik)" fonts/README.md

# Check for license headers in generated files
grep -l "License" fonts/*.h
```

## Rollback Strategy
```bash
# Complete rollback
git checkout emulator/src/core/api_shim.cpp
git checkout vcv-plugin/src/DistingNT.cpp
rm -rf fonts/
rm -f emulator/src/core/fonts.{h,cpp}
rm -f tools/ttf_to_bitmap.py
```

## Success Criteria
- [ ] All three fonts properly embedded as bitmap arrays
- [ ] Text rendering works in both emulator and VCV plugin
- [ ] No code duplication between implementations
- [ ] All font licenses properly documented
- [ ] Visual appearance matches specification
- [ ] Compilation succeeds without warnings
- [ ] Character spacing and alignment correct

## Debug Strategies
- If fonts appear garbled: Check byte order and bit shifting in rendering
- If spacing is wrong: Verify width tables for proportional fonts
- If characters missing: Ensure full ASCII range 32-126 covered
- If build fails: Check include paths and namespace usage

## Notes
- Consider caching rendered text for performance
- May need anti-aliasing for larger fonts
- Test with different background colors
- Verify alignment options work correctly