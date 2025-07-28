#pragma once
#include <rack.hpp>

using namespace rack;

// Forward declarations
struct EmulatorModule;
struct VCVDisplayBuffer;

struct OLEDWidget : FramebufferWidget {
    EmulatorModule* module = nullptr;
    
    // Display dimensions matching Disting NT OLED
    static constexpr int DISPLAY_WIDTH = 256;
    static constexpr int DISPLAY_HEIGHT = 64;
    
    // UI state
    bool displayDirty = true;
    
    OLEDWidget() {
        // FramebufferWidget will handle efficient rendering
    }
    
    void step() override;
    void draw(const DrawArgs& args) override;
    void drawPlaceholder(const DrawArgs& args);
    void drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer);
    void drawNoAlgorithm(NVGcontext* vg);
};