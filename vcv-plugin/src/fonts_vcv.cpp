#include <cstdint>
#include <algorithm>
#include "../../emulator/src/core/fonts.h"
#include "../../fonts/tom_thumb_4x6.h"
#include "../../fonts/pixelmix_baseline.h"
#include "../../fonts/selawik_baseline.h"

// VCV plugin specific implementation that uses NT_screen directly
extern "C" uint8_t NT_screen[128 * 64];

// Forward declarations for VCV-specific color-enabled functions
namespace NT {
void drawChar(int x, int y, char c, FontType font, int color);
void drawText(int x, int y, const char* text, FontType font, int color);
}

namespace NT {

FontMetrics getFontMetrics(FontType type) {
    switch (type) {
    case FontType::TINY:
        return {
            reinterpret_cast<const unsigned char*>(fonts::tomThumb4x6Font),
            nullptr, // fixed width
            4, // Tom Thumb fixed width (actual character width)
            6, // Tom Thumb height
            0, // no additional spacing
            32, // first char
            126 // last char
        };
    case FontType::NORMAL:
        return {
            reinterpret_cast<const unsigned char*>(fonts::pixelmix_baselineFont),
            fonts::pixelmix_baselineWidths, // use actual character widths
            0, // width will come from widths array
            fonts::PIXELMIX_BASELINE_HEIGHT,
            0, // no additional spacing
            fonts::PIXELMIX_BASELINE_FIRST_CHAR,
            fonts::PIXELMIX_BASELINE_LAST_CHAR
        };
    case FontType::LARGE:
        return {
            reinterpret_cast<const unsigned char*>(fonts::selawik_baselineFont),
            fonts::selawik_baselineWidths, // use actual character widths
            0, // width will come from widths array
            fonts::SELAWIK_BASELINE_HEIGHT,
            0, // no additional spacing
            fonts::SELAWIK_BASELINE_FIRST_CHAR,
            fonts::SELAWIK_BASELINE_LAST_CHAR
        };
    }
    // Default fallback
    return {nullptr, nullptr, 0, 0, 0, 0, 0};
}

void setPixel(int x, int y, int color) {
    if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
    
    int byte_idx = y * 128 + x / 2;
    if (byte_idx < 0 || byte_idx >= (128 * 64)) return;  // Extra bounds check
    
    color = color & 0x0F;  // Ensure 4-bit value
    
    if (x % 2 == 0) {
        // Even x: high nibble (bits 4-7)
        NT_screen[byte_idx] = (NT_screen[byte_idx] & 0x0F) | (color << 4);
    } else {
        // Odd x: low nibble (bits 0-3)
        NT_screen[byte_idx] = (NT_screen[byte_idx] & 0xF0) | color;
    }
}

void drawChar(int x, int y, char c, FontType font, int color = 15) {
    // Safety checks
    if (x < -20 || x > 300 || y < -20 || y > 100) return;  // Reasonable bounds
    if (color < 0 || color > 15) color = 15;  // Clamp color
    
    FontMetrics metrics = getFontMetrics(font);
    if (!metrics.data) return;
    if (metrics.height <= 0 || metrics.height > 50) return;  // Sanity check
    
    int char_index = c - metrics.first_char;
    if (char_index < 0 || char_index > (metrics.last_char - metrics.first_char)) return;
    
    // Get character width
    int char_width = metrics.width;
    if (metrics.widths) {
        char_width = metrics.widths[char_index];
    }
    
    if (font == FontType::TINY) {
        // Tom Thumb 4x6: uses MSB ordering, render full bitmap but advance by character width
        const uint8_t (*font_data)[6] = reinterpret_cast<const uint8_t (*)[6]>(metrics.data);
        for (int row = 0; row < metrics.height; row++) {
            uint8_t bitmap_row = font_data[char_index][row];
            for (int col = 0; col < 4; col++) {  // Full 4-pixel bitmap
                if (bitmap_row & (0x80 >> col)) {  // MSB: Start from bit 7, move right
                    setPixel(x + col, y + row, color);
                }
            }
        }
    } else if (font == FontType::NORMAL) {
        // PixelMix baseline: uses MSB ordering, render full bitmap but advance by character width
        const uint8_t (*font_data)[10] = reinterpret_cast<const uint8_t (*)[10]>(metrics.data);
        for (int row = 0; row < metrics.height; row++) {
            uint8_t bitmap_row = font_data[char_index][row];
            for (int col = 0; col < 8; col++) {  // Full 8-bit bitmap data
                if (bitmap_row & (0x80 >> col)) {  // MSB: Start from bit 7, move right
                    setPixel(x + col, y + row, color);
                }
            }
        }
    } else {
        // Selawik baseline: uses MSB ordering with multi-byte rows, render full bitmap but advance by character width
        const uint8_t (*font_data)[30] = reinterpret_cast<const uint8_t (*)[30]>(metrics.data); // 15 rows * 2 bytes per row
        for (int row = 0; row < metrics.height; row++) {
            for (int col = 0; col < 14; col++) {  // Full 14-pixel bitmap
                int byte_idx = row * 2 + (col / 8);  // 2 bytes per row
                int bit_idx = 7 - (col % 8);         // MSB first within each byte
                if (font_data[char_index][byte_idx] & (1 << bit_idx)) {
                    setPixel(x + col, y + row, color);
                }
            }
        }
    }
}

void drawText(int x, int y, const char* text, FontType font, int color = 15) {
    if (!text) return;
    
    FontMetrics metrics = getFontMetrics(font);
    int current_x = x;
    
    while (*text) {
        drawChar(current_x, y, *text, font, color);
        
        int char_width = metrics.width;
        if (metrics.widths) {
            int char_index = *text - metrics.first_char;
            if (char_index >= 0 && char_index <= (metrics.last_char - metrics.first_char)) {
                char_width = metrics.widths[char_index];
            }
        }
        
        current_x += char_width + metrics.spacing;
        text++;
    }
}

int getTextWidth(const char* text, FontType font) {
    if (!text) return 0;
    
    FontMetrics metrics = getFontMetrics(font);
    int width = 0;
    
    while (*text) {
        int char_width = metrics.width;
        if (metrics.widths) {
            int char_index = *text - metrics.first_char;
            if (char_index >= 0 && char_index <= (metrics.last_char - metrics.first_char)) {
                char_width = metrics.widths[char_index];
            }
        }
        
        width += char_width + metrics.spacing;
        text++;
    }
    
    // Remove trailing spacing
    if (width > 0) width -= metrics.spacing;
    
    return width;
}

}  // namespace NT

// C-style wrapper function for VCV plugin
extern "C" void vcv_drawText(int x, int y, const char* text, NT::FontType font, int color) {
    NT::drawText(x, y, text, font, color);
}