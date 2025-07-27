#!/usr/bin/env python3
"""Convert TTF font to C++ bitmap array for VCV Rack"""

import argparse
from PIL import Image, ImageDraw, ImageFont
import numpy as np

def ttf_to_bitmap(ttf_path, size, output_name):
    """Convert TTF font to bitmap array"""
    try:
        font = ImageFont.truetype(ttf_path, size)
    except IOError:
        print(f"Error: Could not load font file {ttf_path}")
        return False
    
    print(f"Converting {ttf_path} at size {size} to {output_name}")
    
    # Determine maximum character dimensions
    max_width = 0
    max_height = 0
    char_data = []
    
    for char_code in range(32, 127):
        char = chr(char_code)
        
        # Get text size using getbbox
        bbox = font.getbbox(char)
        char_width = bbox[2] - bbox[0]
        char_height = bbox[3] - bbox[1]
        
        max_width = max(max_width, char_width)
        max_height = max(max_height, char_height)
        
        char_data.append({
            'char': char,
            'width': char_width,
            'height': char_height,
            'bbox': bbox
        })
    
    # Add padding
    canvas_width = max_width + 2
    canvas_height = max_height + 2
    
    print(f"Character dimensions: {max_width}x{max_height}, canvas: {canvas_width}x{canvas_height}")
    
    # Generate header file with bitmap data
    with open(f"fonts/{output_name}.h", "w") as f:
        f.write(f"// Generated from {ttf_path}\n")
        f.write(f"// Font size: {size}pt\n")
        f.write(f"// Character dimensions: {canvas_width}x{canvas_height}\n")
        f.write(f"#pragma once\n\n")
        f.write(f"namespace fonts {{\n\n")
        
        # Generate bitmap data for each character
        f.write(f"// Bitmap data for ASCII 32-126 ({len(char_data)} characters)\n")
        f.write(f"const unsigned char {output_name}Font[{len(char_data)}][{canvas_height}] = {{\n")
        
        for i, char_info in enumerate(char_data):
            char = char_info['char']
            char_code = ord(char)
            
            # Create image for this character
            img = Image.new('L', (canvas_width, canvas_height), 0)  # Black background
            draw = ImageDraw.Draw(img)
            
            # Calculate baseline position - use font metrics for proper alignment
            # Get the full font metrics for baseline calculation
            ascent, descent = font.getmetrics()
            baseline_y = ascent  # Position baseline at ascent height from top
            
            # Draw character aligned to baseline (not centered)
            x_offset = (canvas_width - char_info['width']) // 2  # Center horizontally
            y_offset = baseline_y - char_info['bbox'][3]  # Align to baseline using bbox
            draw.text((x_offset, y_offset), char, font=font, fill=255)  # White text
            
            # Convert to bitmap array
            img_array = np.array(img)
            
            # Convert to byte array (each row is one byte with bits for each column)
            char_comment = char if char.isprintable() and char != '\\' and char != '"' else f"char_{char_code}"
            f.write(f"    {{  // '{char_comment}' ({char_code})\n")
            
            for row in range(canvas_height):
                byte_val = 0
                for col in range(min(8, canvas_width)):  # Pack up to 8 pixels per byte
                    if col < canvas_width and img_array[row, col] > 128:  # Threshold for white
                        byte_val |= (1 << (7 - col))
                
                f.write(f"        0x{byte_val:02X}")
                if row < canvas_height - 1:
                    f.write(",")
                f.write("\n")
            
            f.write("    }")
            if i < len(char_data) - 1:
                f.write(",")
            f.write("\n")
        
        f.write("};\n\n")
        
        # Generate character width array for proportional fonts
        f.write(f"// Character widths for proportional spacing\n")
        f.write(f"const unsigned char {output_name}Widths[{len(char_data)}] = {{\n")
        for i, char_info in enumerate(char_data):
            # Cap width to 8 pixels to match bitmap data limitation
            actual_width = min(8, char_info['width'])
            f.write(f"    {actual_width}")
            if i < len(char_data) - 1:
                f.write(",")
            if (i + 1) % 16 == 0:
                f.write("\n")
        f.write("\n};\n\n")
        
        # Generate constants
        f.write(f"const int {output_name.upper()}_WIDTH = {canvas_width};\n")
        f.write(f"const int {output_name.upper()}_HEIGHT = {canvas_height};\n")
        f.write(f"const int {output_name.upper()}_FIRST_CHAR = 32;\n")
        f.write(f"const int {output_name.upper()}_LAST_CHAR = 126;\n")
        f.write(f"const int {output_name.upper()}_SPACING = 1;\n\n")
        
        f.write(f"}} // namespace fonts\n")
    
    print(f"Successfully generated fonts/{output_name}.h")
    return True

def main():
    parser = argparse.ArgumentParser(description="Convert TTF font to C++ bitmap array")
    parser.add_argument("--ttf", required=True, help="Input TTF file path")
    parser.add_argument("--size", type=int, default=12, help="Font size in points")
    parser.add_argument("--output", required=True, help="Output name (without extension)")
    
    args = parser.parse_args()
    
    success = ttf_to_bitmap(args.ttf, args.size, args.output)
    if not success:
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())