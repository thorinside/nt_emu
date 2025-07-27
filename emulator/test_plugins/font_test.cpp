#include <cstring>
#include "../include/nt_api_interface.h"

// Font test plugin for Disting NT
extern "C" {

static bool initialized = false;

void initialize() {
    initialized = true;
}

void process() {
    if (!initialized) {
        initialize();
    }
    
    // Clear the screen
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 256; x++) {
            setPixel(x, y, 0);
        }
    }
    
    // Test text
    const char* test_text = "Quick brown fox";
    
    // Draw labels and test text in all three font sizes
    drawText(5, 5, "TINY:", 15, kNT_textLeft, kNT_textTiny);
    drawText(40, 5, test_text, 15, kNT_textLeft, kNT_textTiny);
    
    drawText(5, 20, "NORMAL:", 15, kNT_textLeft, kNT_textNormal);
    drawText(60, 20, test_text, 15, kNT_textLeft, kNT_textNormal);
    
    drawText(5, 40, "LARGE:", 15, kNT_textLeft, kNT_textLarge);
    drawText(60, 40, test_text, 15, kNT_textLeft, kNT_textLarge);
}

void onButton(int button, bool pressed) {
    // No button handling needed for font test
}

void onEncoder(int encoder, int delta) {
    // No encoder handling needed for font test
}

void onMidi(const uint8_t* data, int length) {
    // No MIDI handling needed for font test
}

// Plugin specification
const char* getName() {
    return "Font Test";
}

const char* getAuthor() {
    return "NT Emulator";
}

const char* getVersion() {
    return "1.0.0";
}

const char* getDescription() {
    return "Tests all three font sizes";
}

int getNumInputs() {
    return 0;
}

int getNumOutputs() {
    return 0;
}

const char* getInputName(int index) {
    return nullptr;
}

const char* getOutputName(int index) {
    return nullptr;
}

} // extern "C"