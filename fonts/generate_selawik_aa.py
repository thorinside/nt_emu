#!/usr/bin/env python3
"""
Generate 4-bit grayscale anti-aliased Selawik font for NT emulator.

The NT display uses 4-bit grayscale (0-15 levels), which allows for
proper anti-aliasing of the Selawik font to match hardware rendering.
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Configuration
FONT_PATH = os.path.expanduser("~/Downloads/Selawik_Release/selawk.ttf")
FONT_SIZE = 18  # Adjust to get ~20px rendered height (capital letter height ~2x normal font)
FIRST_CHAR = 32
LAST_CHAR = 126
MAX_CHAR_WIDTH = 20  # Maximum expected character width
CHAR_HEIGHT = 22  # Total height including descenders

def get_char_metrics(font, char):
    """Get the bounding box and advance width for a character."""
    # Create a temporary image to measure
    temp = Image.new('L', (MAX_CHAR_WIDTH * 2, CHAR_HEIGHT * 2), 0)
    draw = ImageDraw.Draw(temp)

    # Get the bounding box
    bbox = draw.textbbox((0, 0), char, font=font)
    left, top, right, bottom = bbox

    # Get the advance width (how much to move cursor)
    # Use textlength for proper character spacing
    advance = int(font.getlength(char))

    return {
        'left': left,
        'top': top,
        'right': right,
        'bottom': bottom,
        'width': right - left,
        'height': bottom - top,
        'advance': advance
    }

def render_char(font, char, char_height, max_width):
    """Render a character and return grayscale pixel data (0-15)."""
    # Create image with some padding
    img = Image.new('L', (max_width, char_height), 0)
    draw = ImageDraw.Draw(img)

    # Get font metrics for positioning
    ascent, descent = font.getmetrics()

    # Draw at position that puts baseline at consistent location
    # The baseline should be at (char_height - descent) from top
    baseline_y = char_height - 4  # Leave 4 pixels for descenders

    # Draw the character
    draw.text((0, baseline_y), char, font=font, fill=255, anchor="ls")  # ls = left baseline

    # Convert to 4-bit grayscale (0-15)
    pixels = []
    for y in range(char_height):
        row = []
        for x in range(max_width):
            # Get pixel value (0-255) and convert to 0-15
            val = img.getpixel((x, y))
            gray4 = (val * 15 + 127) // 255  # Round to nearest 4-bit value
            row.append(gray4)
        pixels.append(row)

    return pixels

def find_actual_width(pixels, max_width):
    """Find the actual used width of a character (rightmost non-zero column + 1)."""
    actual_width = 0
    for y in range(len(pixels)):
        for x in range(max_width - 1, -1, -1):
            if pixels[y][x] > 0:
                actual_width = max(actual_width, x + 1)
                break
    return actual_width if actual_width > 0 else 1  # At least 1 for space

def generate_header():
    """Generate the C header file with anti-aliased font data."""
    if not os.path.exists(FONT_PATH):
        print(f"Error: Font not found at {FONT_PATH}")
        return

    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    ascent, descent = font.getmetrics()
    print(f"Font loaded: size={FONT_SIZE}, ascent={ascent}, descent={descent}")

    # Measure all characters to find max dimensions
    all_chars = [chr(c) for c in range(FIRST_CHAR, LAST_CHAR + 1)]
    max_rendered_width = 0
    char_data = []

    for char in all_chars:
        metrics = get_char_metrics(font, char)
        max_rendered_width = max(max_rendered_width, metrics['advance'])

    # Add padding
    bitmap_width = min(max_rendered_width + 2, MAX_CHAR_WIDTH)
    print(f"Bitmap dimensions: {bitmap_width}x{CHAR_HEIGHT}")

    # Render all characters
    for char in all_chars:
        pixels = render_char(font, char, CHAR_HEIGHT, bitmap_width)
        actual_width = find_actual_width(pixels, bitmap_width)

        # Get advance width for proper spacing
        advance = int(font.getlength(char))
        if advance == 0:
            advance = 4  # Space character

        char_data.append({
            'char': char,
            'pixels': pixels,
            'width': advance,  # Use advance width for spacing
            'bitmap_width': bitmap_width
        })

    # Generate header file
    num_chars = LAST_CHAR - FIRST_CHAR + 1

    header = f"""// Generated from {FONT_PATH}
// Font size: {FONT_SIZE}pt
// Character dimensions: {bitmap_width}x{CHAR_HEIGHT}
// Format: 1 byte per pixel, values 0-15 (4-bit grayscale)
// Baseline is at y={CHAR_HEIGHT - 4} from top
#pragma once

namespace fonts {{

constexpr int SELAWIK_AA_WIDTH = {bitmap_width};
constexpr int SELAWIK_AA_HEIGHT = {CHAR_HEIGHT};
constexpr int SELAWIK_AA_ASCENT = {CHAR_HEIGHT - 4};  // Baseline from top
constexpr int SELAWIK_AA_FIRST_CHAR = {FIRST_CHAR};
constexpr int SELAWIK_AA_LAST_CHAR = {LAST_CHAR};

// Character widths (advance width for proper spacing)
const unsigned char selawik_aaWidths[{num_chars}] = {{
"""

    # Output widths
    for i, data in enumerate(char_data):
        if i % 16 == 0:
            header += "    "
        header += f"{data['width']:2d},"
        if i % 16 == 15 or i == len(char_data) - 1:
            header += "\n"
        else:
            header += " "

    header += f"""
}};

// Bitmap data: {num_chars} characters, {bitmap_width}x{CHAR_HEIGHT} pixels each
// Each byte is a grayscale value 0-15
const unsigned char selawik_aaFont[{num_chars}][{bitmap_width * CHAR_HEIGHT}] = {{
"""

    # Output bitmap data
    for i, data in enumerate(char_data):
        char = data['char']
        char_display = char if char.isprintable() and char != '\\' and char != "'" else f"char_{ord(char)}"
        header += f"    {{  // '{char_display}' ({ord(char)})\n"

        for y, row in enumerate(data['pixels']):
            header += "        "
            for x, val in enumerate(row):
                header += f"{val:2d},"
            header += f"  // row {y}\n"

        header += "    },\n"

    header += """
};

}  // namespace fonts
"""

    # Write the header file
    output_path = os.path.join(os.path.dirname(__file__), "selawik_aa.h")
    with open(output_path, 'w') as f:
        f.write(header)

    print(f"Generated {output_path}")
    print(f"  Characters: {num_chars} (ASCII {FIRST_CHAR}-{LAST_CHAR})")
    print(f"  Dimensions: {bitmap_width}x{CHAR_HEIGHT} pixels")
    print(f"  Data size: {num_chars * bitmap_width * CHAR_HEIGHT} bytes")

if __name__ == "__main__":
    generate_header()
