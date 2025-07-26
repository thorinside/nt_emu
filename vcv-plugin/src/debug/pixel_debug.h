#pragma once

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>
#include <sstream>

namespace PixelDebug {
    
/**
 * Debug output function to print pixel values and their interpretation
 * Shows both the raw byte values and how they're interpreted as pixels
 */
inline void dumpPixelRegion(const uint8_t* buffer, int start_x, int start_y, int width, int height, const std::string& label = "") {
    if (!buffer) {
        std::cout << "ERROR: Null buffer passed to dumpPixelRegion" << std::endl;
        return;
    }
    
    std::cout << "\n=== PIXEL DEBUG DUMP" << (label.empty() ? "" : " - " + label) << " ===" << std::endl;
    std::cout << "Region: (" << start_x << "," << start_y << ") to (" 
              << (start_x + width - 1) << "," << (start_y + height - 1) << ")" << std::endl;
    std::cout << "Buffer format: 256x64 pixels, 2 pixels per byte, 4-bit grayscale (0=black, 15=white)" << std::endl;
    std::cout << "Nibble interpretation: [Bit 7-4: ?] [Bit 3-0: ?] <- This is what we're testing!" << std::endl;
    
    for (int y = start_y; y < start_y + height && y < 64; y++) {
        std::cout << "\nRow " << std::setw(2) << y << ": ";
        
        for (int x = start_x; x < start_x + width && x < 256; x += 2) {
            int byte_idx = y * 128 + x / 2;
            uint8_t byte_val = buffer[byte_idx];
            
            // Extract nibbles
            uint8_t high_nibble = (byte_val >> 4) & 0x0F;
            uint8_t low_nibble = byte_val & 0x0F;
            
            // Show byte and interpretation
            std::cout << "Byte[" << std::setw(4) << byte_idx << "]=0x" 
                      << std::hex << std::setw(2) << std::setfill('0') << (int)byte_val
                      << std::dec << std::setfill(' ');
            
            std::cout << " -> Pixel[" << std::setw(3) << x << "," << std::setw(2) << y << "]=" 
                      << std::setw(2) << (int)high_nibble
                      << ", Pixel[" << std::setw(3) << (x+1) << "," << std::setw(2) << y << "]=" 
                      << std::setw(2) << (int)low_nibble;
            
            if (x + 2 < start_x + width && x + 2 < 256) {
                std::cout << " | ";
            }
        }
    }
    std::cout << "\n" << std::endl;
}

/**
 * Dump a specific pattern to help identify nibble order
 * Creates a test pattern and shows how it should be interpreted
 */
inline void dumpTestPattern(uint8_t* buffer, const std::string& pattern_name) {
    std::cout << "\n=== TEST PATTERN: " << pattern_name << " ===" << std::endl;
    
    if (pattern_name == "ALTERNATING_COLUMNS") {
        // Write 0xF0 pattern to first few bytes
        for (int i = 0; i < 8; i++) {
            buffer[i] = 0xF0;
        }
        
        std::cout << "Pattern: 0xF0 bytes (should show alternating white-black columns)" << std::endl;
        std::cout << "If high nibble = first pixel: |████|    |████|    |" << std::endl;
        std::cout << "If low nibble = first pixel:  |    |████|    |████|" << std::endl;
        
        dumpPixelRegion(buffer, 0, 0, 16, 1, "0xF0 Pattern Test");
        
    } else if (pattern_name == "SINGLE_PIXELS") {
        // Clear buffer first
        memset(buffer, 0, 128 * 64);
        
        // Set specific bytes to test individual pixels
        buffer[0] = 0xF0;  // Should light up one pixel at (0,0) or (1,0)
        buffer[1] = 0x0F;  // Should light up one pixel at (2,0) or (3,0)
        buffer[128] = 0xAB; // Test row 1: 0xAB = should show values 10 and 11
        
        std::cout << "Pattern: Single pixel tests" << std::endl;
        std::cout << "buffer[0] = 0xF0, buffer[1] = 0x0F, buffer[128] = 0xAB" << std::endl;
        
        dumpPixelRegion(buffer, 0, 0, 8, 2, "Single Pixel Test");
        
    } else if (pattern_name == "GRADIENT") {
        // Create a simple gradient
        for (int i = 0; i < 16; i++) {
            uint8_t val = i;
            buffer[i] = (val << 4) | val;  // Same value in both nibbles
        }
        
        std::cout << "Pattern: Gradient (same value in both nibbles)" << std::endl;
        dumpPixelRegion(buffer, 0, 0, 32, 1, "Gradient Test");
    }
}

/**
 * Compare two interpretations of the same buffer
 * Useful for comparing API calls vs direct buffer writes
 */
inline void compareBuffers(const uint8_t* buffer1, const uint8_t* buffer2, 
                          int width, int height, 
                          const std::string& label1 = "Buffer 1", 
                          const std::string& label2 = "Buffer 2") {
    std::cout << "\n=== BUFFER COMPARISON ===" << std::endl;
    std::cout << "Comparing " << label1 << " vs " << label2 << std::endl;
    
    bool differences_found = false;
    
    for (int y = 0; y < height && y < 64; y++) {
        for (int x = 0; x < width && x < 256; x += 2) {
            int byte_idx = y * 128 + x / 2;
            
            if (buffer1[byte_idx] != buffer2[byte_idx]) {
                if (!differences_found) {
                    differences_found = true;
                    std::cout << "\nDifferences found:" << std::endl;
                }
                
                std::cout << "Position (" << x << "," << y << ") - Byte[" << byte_idx << "]: "
                          << label1 << "=0x" << std::hex << std::setw(2) << std::setfill('0') << (int)buffer1[byte_idx]
                          << ", " << label2 << "=0x" << std::setw(2) << std::setfill('0') << (int)buffer2[byte_idx]
                          << std::dec << std::setfill(' ') << std::endl;
            }
        }
    }
    
    if (!differences_found) {
        std::cout << "No differences found - buffers are identical!" << std::endl;
    }
    std::cout << std::endl;
}

/**
 * Analyze nibble order from a known pattern
 * This is the key function to determine the correct pixel format
 */
inline void analyzeNibbleOrder(const uint8_t* buffer, int test_byte_offset = 0) {
    std::cout << "\n=== NIBBLE ORDER ANALYSIS ===" << std::endl;
    
    uint8_t test_byte = buffer[test_byte_offset];
    uint8_t high_nibble = (test_byte >> 4) & 0x0F;
    uint8_t low_nibble = test_byte & 0x0F;
    
    std::cout << "Analyzing byte at offset " << test_byte_offset << std::endl;
    std::cout << "Byte value: 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)test_byte << std::dec << std::endl;
    std::cout << "High nibble (bits 7-4): " << (int)high_nibble << std::endl;
    std::cout << "Low nibble (bits 3-0): " << (int)low_nibble << std::endl;
    
    // Convert byte offset to pixel coordinates
    int row = test_byte_offset / 128;
    int byte_in_row = test_byte_offset % 128;
    int x_even = byte_in_row * 2;
    int x_odd = x_even + 1;
    
    std::cout << "\nThis byte represents pixels at:" << std::endl;
    std::cout << "  Even X pixel: (" << x_even << "," << row << ")" << std::endl;
    std::cout << "  Odd X pixel:  (" << x_odd << "," << row << ")" << std::endl;
    
    std::cout << "\nPossible interpretations:" << std::endl;
    std::cout << "  If high nibble = even X pixel: (" << x_even << "," << row << ") = " << (int)high_nibble 
              << ", (" << x_odd << "," << row << ") = " << (int)low_nibble << std::endl;
    std::cout << "  If low nibble = even X pixel:  (" << x_even << "," << row << ") = " << (int)low_nibble 
              << ", (" << x_odd << "," << row << ") = " << (int)high_nibble << std::endl;
    
    std::cout << "\nTo determine correct order, observe which interpretation matches the visual display!" << std::endl;
}

/**
 * Dump raw bytes in a readable hex format
 */
inline void dumpRawBytes(const uint8_t* buffer, int start_offset, int count, const std::string& label = "") {
    std::cout << "\n=== RAW BYTE DUMP" << (label.empty() ? "" : " - " + label) << " ===" << std::endl;
    std::cout << "Offset " << start_offset << " to " << (start_offset + count - 1) << ":" << std::endl;
    
    for (int i = 0; i < count; i++) {
        if (i % 16 == 0) {
            std::cout << "\n" << std::setw(4) << std::setfill('0') << (start_offset + i) << ": ";
        }
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[start_offset + i] << " ";
        if ((i + 1) % 8 == 0) std::cout << " ";
    }
    std::cout << std::dec << std::setfill(' ') << "\n" << std::endl;
}

} // namespace PixelDebug