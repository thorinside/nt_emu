#pragma once

#include "../core/api_shim.h"
#include <imgui.h>

class Display {
public:
    static constexpr int WIDTH = 256;
    static constexpr int HEIGHT = 64;
    static constexpr float SCALE = 3.0f;  // Scale factor for visibility
    
    Display();
    ~Display();
    
    void render();
    void clear();
    
    // Access to display buffer
    DisplayBuffer& getBuffer() { return buffer_; }
    const DisplayBuffer& getBuffer() const { return buffer_; }
    
    // Update from API state
    void updateFromApiState();
    
private:
    DisplayBuffer buffer_;
    ImVec4 pixel_color_on_;
    ImVec4 pixel_color_off_;
    ImVec4 background_color_;
    
    void renderDisplayWindow();
    void drawPixelGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
};