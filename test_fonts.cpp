#include <iostream>
#include <cstdint>
#include "fonts/tom_thumb_4x6.h"
#include "fonts/pixelmix_baseline.h"
#include "fonts/selawik_baseline.h"

// Mock NT_screen buffer
uint8_t NT_screen[128 * 64];

// Include the VCV font implementation
#include "vcv-plugin/src/fonts_vcv.cpp"

void printChar(char c, int font_idx) {
    // Clear screen
    memset(NT_screen, 0, sizeof(NT_screen));
    
    // Draw the character
    NT::FontType font = static_cast<NT::FontType>(font_idx);
    NT::drawChar(0, 0, c, font, 15);
    
    // Get font metrics to know dimensions
    NT::FontMetrics metrics = NT::getFontMetrics(font);
    
    std::cout << "\nFont " << font_idx << " (width=" << metrics.width << ", height=" << metrics.height << ") - Character '" << c << "':\n";
    
    // Print the rendered character
    for (int y = 0; y < metrics.height; y++) {
        for (int x = 0; x < metrics.width; x++) {
            int byte_idx = y * 128 + x / 2;
            uint8_t pixel_value;
            if (x % 2 == 0) {
                pixel_value = (NT_screen[byte_idx] >> 4) & 0x0F;
            } else {
                pixel_value = NT_screen[byte_idx] & 0x0F;
            }
            std::cout << (pixel_value > 0 ? '*' : ' ');
        }
        std::cout << "\n";
    }
}

void testTextRendering() {
    // Clear screen
    memset(NT_screen, 0, sizeof(NT_screen));
    
    // Test string with different character heights for baseline alignment
    const char* test_str = "Tg";
    
    std::cout << "\n\nTesting text rendering with baseline alignment:\n";
    std::cout << "String: \"" << test_str << "\"\n";
    
    for (int font_idx = 0; font_idx < 3; font_idx++) {
        memset(NT_screen, 0, sizeof(NT_screen));
        
        NT::FontType font = static_cast<NT::FontType>(font_idx);
        NT::FontMetrics metrics = NT::getFontMetrics(font);
        
        // Draw the text
        NT::drawText(0, 0, test_str, font, 15);
        
        std::cout << "\nFont " << font_idx << " (canvas " << metrics.width << "x" << metrics.height << ", spacing=" << metrics.spacing << "):\n";
        
        // Calculate expected width
        int expected_width = strlen(test_str) * metrics.width + (strlen(test_str) - 1) * metrics.spacing;
        
        // Print the rendered text
        for (int y = 0; y < metrics.height; y++) {
            for (int x = 0; x < expected_width; x++) {
                int byte_idx = y * 128 + x / 2;
                uint8_t pixel_value;
                if (x % 2 == 0) {
                    pixel_value = (NT_screen[byte_idx] >> 4) & 0x0F;
                } else {
                    pixel_value = NT_screen[byte_idx] & 0x0F;
                }
                std::cout << (pixel_value > 0 ? '*' : ' ');
            }
            std::cout << "\n";
        }
        
        std::cout << "Expected width: " << expected_width << " pixels\n";
    }
}

int main() {
    std::cout << "Font Implementation Test\n";
    std::cout << "========================\n";
    
    // Test individual characters for baseline alignment
    printChar('T', 0); // TINY
    printChar('t', 0);
    printChar('g', 0);
    
    printChar('T', 1); // NORMAL  
    printChar('t', 1);
    printChar('g', 1);
    
    printChar('T', 2); // LARGE
    printChar('t', 2);
    printChar('g', 2);
    
    // Test text rendering
    testTextRendering();
    
    return 0;
}