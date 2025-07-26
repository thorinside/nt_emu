#include "OLEDWidget.hpp"
#include "../EmulatorCore.hpp"

// Forward declaration implementation will be resolved at link time
struct DistingNT;

void OLEDWidget::step() {
    // We'll implement this in the DistingNT.cpp file to avoid circular dependencies
    FramebufferWidget::step();
}

void OLEDWidget::draw(const DrawArgs& args) {
    if (!module) {
        drawPlaceholder(args);
        return;
    }
    
    // Clear display background (black) - use widget box size, let VCV handle scaling
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFillColor(args.vg, nvgRGB(0, 0, 0));
    nvgFill(args.vg);
    
    // The actual display logic will be implemented inline in DistingNT.cpp
    // Drawing will be done at 1:1 pixel coordinates (0-255, 0-63) and VCV will scale to fit box
}

void OLEDWidget::drawPlaceholder(const DrawArgs& args) {
    // Draw placeholder when module is not available (browser preview)
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFillColor(args.vg, nvgRGB(20, 20, 20));
    nvgFill(args.vg);
    
    // Draw border
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
    nvgStrokeWidth(args.vg, 1.0f);
    nvgStroke(args.vg);
    
    // Draw "OLED" text
    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
    nvgFontSize(args.vg, 14);
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(args.vg, box.size.x / 2, box.size.y / 2, "OLED DISPLAY", nullptr);
}

void OLEDWidget::drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer) {
    // Draw the display buffer pixel by pixel with full 4-bit grayscale support
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            uint8_t gray_value = buffer.getPixelGray(x, y);
            if (gray_value > 0) {
                // Map 4-bit grayscale (0-15) to 0.0-1.0 range
                float intensity = (float)gray_value / 15.0f;
                
                // Set color based on intensity (white pixels with varying alpha/brightness)
                nvgFillColor(vg, nvgRGBA(255, 255, 255, (int)(intensity * 255)));
                
                nvgBeginPath(vg);
                nvgRect(vg, x, y, 1, 1);
                nvgFill(vg);
            }
        }
    }
}

void OLEDWidget::drawNoAlgorithm(NVGcontext* vg) {
    // Draw message when no algorithm is loaded
    nvgFillColor(vg, nvgRGB(255, 255, 255));
    nvgFontSize(vg, 12);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(vg, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "NO ALGORITHM", nullptr);
    
    // Draw some decorative elements
    nvgBeginPath(vg);
    nvgRect(vg, 10, 10, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 20);
    nvgStrokeColor(vg, nvgRGB(128, 128, 128));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);
}