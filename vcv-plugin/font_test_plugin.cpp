#include <cstdint>
#include "../../emulator/src/core/fonts.h"

// VCV plugin specific implementation that uses NT_screen directly
extern "C" uint8_t NT_screen[128 * 64];

// VCV-specific color-enabled functions
namespace NT {
void drawChar(int x, int y, char c, FontType font, int color);
void drawText(int x, int y, const char* text, FontType font, int color);
}

// Simple test function to render all three fonts
extern "C" void test_all_fonts() {
    // Clear screen
    memset(NT_screen, 0, 128 * 64);
    
    // Test text
    const char* test_text = "Quick brown fox";
    
    // Draw in all three font sizes
    NT::drawText(5, 5, "TINY:", NT::FontType::TINY, 15);
    NT::drawText(40, 5, test_text, NT::FontType::TINY, 15);
    
    NT::drawText(5, 20, "NORMAL:", NT::FontType::NORMAL, 15);
    NT::drawText(60, 20, test_text, NT::FontType::NORMAL, 15);
    
    NT::drawText(5, 40, "LARGE:", NT::FontType::LARGE, 15);
    NT::drawText(60, 40, test_text, NT::FontType::LARGE, 15);
}