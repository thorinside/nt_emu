#include <iostream>
#include <iomanip>
#include "fonts/tom_thumb.h"
#include "fonts/pixelmix.h"
#include "fonts/selawik_bitmap.h"

void printTomThumbG() {
    std::cout << "=== TOM THUMB 'G' (ASCII 71) ===" << std::endl;
    
    // G is at index 71 - 32 = 39
    int char_index = 71 - 32;
    const fonts::TomThumbGlyph& glyph = fonts::tomThumbGlyphs[char_index];
    
    std::cout << "Width: " << (int)glyph.width << ", Height: " << (int)glyph.height << std::endl;
    std::cout << "Offset: " << glyph.bitmap_offset << std::endl;
    
    for (int row = 0; row < glyph.height; row++) {
        uint8_t bitmap_row = fonts::tomThumbBitmaps[glyph.bitmap_offset + row];
        std::cout << "Row " << row << ": 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)bitmap_row << " = ";
        
        // MSB-first rendering like our code
        for (int col = 0; col < 8; col++) {
            if (col < glyph.width && (bitmap_row & (0x80 >> col))) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printPixelMixG() {
    int char_index = 71 - 32; // G
    const uint8_t* char_data = fonts::pixelMixFont[char_index];
    
    std::cout << "=== PIXELMIX 'G' - MSB8 (bits 7-0) ===" << std::endl;
    for (int row = 0; row < 7; row++) {
        uint8_t bitmap_row = char_data[row];
        for (int col = 0; col < 8; col++) {
            if (bitmap_row & (0x80 >> col)) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== PIXELMIX 'G' - MSB5 (bits 4-0) ===" << std::endl;
    for (int row = 0; row < 7; row++) {
        uint8_t bitmap_row = char_data[row];
        for (int col = 0; col < 5; col++) {
            if (bitmap_row & (0x10 >> col)) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== PIXELMIX 'G' - LSB5 (bits 0-4) ===" << std::endl;
    for (int row = 0; row < 7; row++) {
        uint8_t bitmap_row = char_data[row];
        for (int col = 0; col < 5; col++) {
            if (bitmap_row & (0x01 << col)) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printSelawikG() {
    int char_index = 71 - 32; // G
    const uint8_t* char_data = fonts::selawik_bitmapFont[char_index];
    
    std::cout << "=== SELAWIK 'G' - MSB8 (bits 7-0) ===" << std::endl;
    for (int row = 0; row < 15; row++) {
        uint8_t bitmap_row = char_data[row];
        for (int col = 0; col < 8; col++) {
            if (bitmap_row & (0x80 >> col)) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== SELAWIK 'G' - LSB8 (bits 0-7) ===" << std::endl;
    for (int row = 0; row < 15; row++) {
        uint8_t bitmap_row = char_data[row];
        for (int col = 0; col < 8; col++) {
            if (bitmap_row & (0x01 << col)) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    printTomThumbG();
    printPixelMixG();
    printSelawikG();
    return 0;
}