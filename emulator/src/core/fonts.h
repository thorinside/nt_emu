#pragma once

#include "../../../fonts/tom_thumb.h"
#include "../../../fonts/pixelmix.h"
#include "../../../fonts/selawik_bitmap.h"

namespace NT {

enum class FontType {
    TINY,    // Tom Thumb 3x5
    NORMAL,  // PixelMix 6pt (or alternative)
    LARGE    // Selawik bitmap
};

struct FontMetrics {
    const unsigned char* data;
    const unsigned char* widths; // for proportional fonts (null for fixed-width)
    int width;   // character width (0 for variable width fonts)
    int height;  // character height
    int spacing; // spacing between characters
    int first_char; // first character code
    int last_char;  // last character code
};

// Get font metrics for a specific font type
FontMetrics getFontMetrics(FontType type);

// Draw a single character at specified position
void drawChar(int x, int y, char c, FontType font = FontType::NORMAL);

// Draw text string at specified position
void drawText(int x, int y, const char* text, FontType font = FontType::NORMAL);

// Get text width in pixels
int getTextWidth(const char* text, FontType font = FontType::NORMAL);

// Get character width in pixels
int getCharWidth(char c, FontType font = FontType::NORMAL);

} // namespace NT