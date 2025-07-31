#pragma once

#include <rack.hpp>
#include "../nt_api_interface.h"
#include "IDisplayDataProvider.hpp"

using namespace rack;

// Forward declarations  
struct VCVDisplayBuffer;

namespace DisplayRenderer {

    class ModuleOLEDWidget : public FramebufferWidget {
    public:
        IDisplayDataProvider* dataProvider = nullptr;
        
        // Display dimensions matching Disting NT OLED
        static constexpr int DISPLAY_WIDTH = 256;
        static constexpr int DISPLAY_HEIGHT = 64;
        
        ModuleOLEDWidget();
        
        void step() override;
        void draw(const DrawArgs& args) override;
        
    private:
        void drawPlaceholder(const DrawArgs& args);
        void syncNTScreenToVCVBuffer(VCVDisplayBuffer& buffer);
        void drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer);
        void drawMenuInterface(NVGcontext* vg, IDisplayDataProvider* dataProvider);
        void formatParameterValue(char* str, const _NT_parameter& param, int value) const;
    };

} // namespace DisplayRenderer