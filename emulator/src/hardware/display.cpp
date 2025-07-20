#include "display.h"
#include "../core/api_shim.h"
#include <imgui.h>

Display::Display() 
    : pixel_color_on_(1.0f, 1.0f, 1.0f, 1.0f)    // White
    , pixel_color_off_(0.1f, 0.1f, 0.1f, 1.0f)   // Dark gray
    , background_color_(0.0f, 0.0f, 0.0f, 1.0f)   // Black
{
    buffer_.clear();
}

Display::~Display() {
}

void Display::render() {
    renderDisplayWindow();
}

void Display::clear() {
    buffer_.clear();
}

void Display::updateFromApiState() {
    // Copy from the API state to our local buffer
    buffer_ = ApiShim::getState().display;
}

void Display::renderDisplayWindow() {
    ImGui::Begin("Disting NT Display");
    
    // Display info
    ImGui::Text("Resolution: %dx%d", WIDTH, HEIGHT);
    ImGui::Text("Scale: %.1fx", SCALE);
    
    ImGui::Separator();
    
    // Get drawing context
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size(WIDTH * SCALE, HEIGHT * SCALE);
    
    // Reserve space for the display
    ImGui::InvisibleButton("display_canvas", canvas_size);
    
    // Draw background
    draw_list->AddRectFilled(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        ImGui::ColorConvertFloat4ToU32(background_color_)
    );
    
    // Draw pixels
    drawPixelGrid(draw_list, canvas_pos, canvas_size);
    
    // Draw border
    draw_list->AddRect(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(128, 128, 128, 255),
        0.0f,
        0,
        2.0f
    );
    
    ImGui::End();
}

void Display::drawPixelGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {
    (void)canvas_size;  // Unused for now
    
    // Determine pixel size
    float pixel_width = SCALE;
    float pixel_height = SCALE;
    
    // Draw each pixel
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            bool pixel_on = buffer_.getPixel(x, y);
            
            if (pixel_on) {
                ImVec2 pixel_pos(
                    canvas_pos.x + x * pixel_width,
                    canvas_pos.y + y * pixel_height
                );
                
                ImVec2 pixel_end(
                    pixel_pos.x + pixel_width,
                    pixel_pos.y + pixel_height
                );
                
                draw_list->AddRectFilled(
                    pixel_pos,
                    pixel_end,
                    ImGui::ColorConvertFloat4ToU32(pixel_color_on_)
                );
            }
        }
    }
}