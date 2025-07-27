#include "fonts.h"
#include "api_shim.h"

namespace NT {

FontMetrics getFontMetrics(FontType type) {
    switch (type) {
    case FontType::TINY:
        return {
            reinterpret_cast<const unsigned char*>(fonts::tomThumbBitmaps),
            nullptr, // fixed width font
            fonts::TOMTHUMB_WIDTH,
            fonts::TOMTHUMB_HEIGHT,
            1, // spacing
            fonts::TOMTHUMB_FIRST_CHAR,
            fonts::TOMTHUMB_LAST_CHAR
        };
    case FontType::NORMAL:
        return {
            reinterpret_cast<const unsigned char*>(fonts::pixelMixFont),
            fonts::pixelMixWidths,
            0, // variable width
            fonts::PIXELMIX_HEIGHT,
            fonts::PIXELMIX_SPACING,
            fonts::PIXELMIX_FIRST_CHAR,
            fonts::PIXELMIX_LAST_CHAR
        };
    case FontType::LARGE:
        return {
            reinterpret_cast<const unsigned char*>(fonts::selawik_bitmapFont),
            fonts::selawik_bitmapWidths,
            0, // variable width
            fonts::SELAWIK_BITMAP_HEIGHT,
            fonts::SELAWIK_BITMAP_SPACING,
            fonts::SELAWIK_BITMAP_FIRST_CHAR,
            fonts::SELAWIK_BITMAP_LAST_CHAR
        };
    }
    
    // Default fallback to NORMAL
    return getFontMetrics(FontType::NORMAL);
}

int getCharWidth(char c, FontType font) {
    if (c < 32 || c > 126) return 0;
    
    FontMetrics fm = getFontMetrics(font);
    int charIndex = c - fm.first_char;
    
    if (charIndex < 0 || charIndex >= (fm.last_char - fm.first_char + 1)) {
        return 0;
    }
    
    if (fm.widths) {
        // Variable width font
        return fm.widths[charIndex];
    } else {
        // Fixed width font
        return fm.width;
    }
}

int getTextWidth(const char* text, FontType font) {
    if (!text) return 0;
    
    FontMetrics fm = getFontMetrics(font);
    int totalWidth = 0;
    
    for (const char* p = text; *p; p++) {
        totalWidth += getCharWidth(*p, font);
        if (*(p + 1)) { // Add spacing between characters (not after last char)
            totalWidth += fm.spacing;
        }
    }
    
    return totalWidth;
}

// Helper function to set pixel through display buffer
static void setPixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
    
    color = color & 0x0F; // Clamp to 4-bit
    uint8_t* buffer = ApiShim::getScreenBuffer();
    int byte_idx = y * 128 + x / 2;
    
    if (x & 1) {
        // Odd x: low nibble
        buffer[byte_idx] = (buffer[byte_idx] & 0xF0) | color;
    } else {
        // Even x: high nibble
        buffer[byte_idx] = (buffer[byte_idx] & 0x0F) | (color << 4);
    }
}

void drawChar(int x, int y, char c, FontType font) {
    if (c < 32 || c > 126) return;
    
    FontMetrics fm = getFontMetrics(font);
    int charIndex = c - fm.first_char;
    
    if (charIndex < 0 || charIndex >= (fm.last_char - fm.first_char + 1)) {
        return;
    }
    
    int charWidth = getCharWidth(c, font);
    if (charWidth == 0) return;
    
    if (font == FontType::TINY) {
        // Tom Thumb uses special glyph structure
        const fonts::TomThumbGlyph* glyph = &fonts::tomThumbGlyphs[charIndex];
        
        for (int row = 0; row < glyph->height; row++) {
            if (glyph->bitmap_offset + row >= sizeof(fonts::tomThumbBitmaps)) break;
            
            unsigned char rowData = fonts::tomThumbBitmaps[glyph->bitmap_offset + row];
            
            for (int col = 0; col < glyph->width; col++) {
                if (rowData & (1 << (7 - col))) {
                    setPixel(x + glyph->x_offset + col, y + glyph->y_offset + row, 15); // white
                }
            }
        }
    } else {
        // PixelMix and Selawik use array format [char][row] with LSB-first bit ordering
        const unsigned char* charData = fm.data + (charIndex * fm.height);
        
        for (int row = 0; row < fm.height; row++) {
            unsigned char rowData = charData[row];
            
            for (int col = 0; col < charWidth && col < 8; col++) {
                if (rowData & (1 << col)) { // LSB-first: bit 0 is leftmost pixel
                    setPixel(x + col, y + row, 15); // white
                }
            }
        }
    }
}

void drawText(int x, int y, const char* text, FontType font) {
    if (!text) return;
    
    FontMetrics fm = getFontMetrics(font);
    int currentX = x;
    
    for (const char* p = text; *p; p++) {
        drawChar(currentX, y, *p, font);
        currentX += getCharWidth(*p, font);
        if (*(p + 1)) { // Add spacing between characters
            currentX += fm.spacing;
        }
    }
}

} // namespace NT