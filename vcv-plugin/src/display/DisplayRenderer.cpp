#include "DisplayRenderer.hpp"
#include "../EmulatorCore.hpp"
#include "../plugin/PluginManager.hpp"
#include "../parameter/ParameterSystem.hpp"

namespace DisplayRenderer {

    ModuleOLEDWidget::ModuleOLEDWidget() {}

    void ModuleOLEDWidget::step() {
        // Check if we need to redraw
        if (dataProvider && (dataProvider->isDisplayDirty() || dataProvider->getDisplayBuffer().dirty)) {
            // Mark framebuffer as dirty to trigger redraw
            FramebufferWidget::dirty = true;
            dataProvider->setDisplayDirty(false);
        }
        FramebufferWidget::step();
    }

    void ModuleOLEDWidget::draw(const DrawArgs& args) {
        if (!dataProvider) {
            drawPlaceholder(args);
            return;
        }
        
        // Set up coordinate system for 256x64 display
        nvgSave(args.vg);
        nvgScale(args.vg, box.size.x / DISPLAY_WIDTH, box.size.y / DISPLAY_HEIGHT);
        
        // Clear display background (black)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        // Show menu interface if active
        if (dataProvider->getMenuMode() != 0) { // Assuming MENU_OFF = 0
            drawMenuInterface(args.vg, dataProvider);
        } else if (dataProvider->getPluginManagerPtr() && dataProvider->getPluginManagerPtr()->getLoadingMessageTimer() > 0) {
            nvgFillColor(args.vg, nvgRGB(0, 255, 255)); // Cyan
            nvgFontSize(args.vg, 16);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            
            // Fade out effect
            float alpha = std::min(1.0f, dataProvider->getPluginManagerPtr()->getLoadingMessageTimer());
            nvgFillColor(args.vg, nvgRGBA(0, 255, 255, (int)(255 * alpha)));
            
            nvgText(args.vg, 128, 32, dataProvider->getPluginManagerPtr()->getLoadingMessage().c_str(), NULL);
        } else {
            // Check if plugin has custom drawing
            if (dataProvider->hasLoadedPlugin()) {
                // Clear display first
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                nvgFillColor(args.vg, nvgRGB(0, 0, 0));
                nvgFill(args.vg);
                
                // Try plugin drawing
                bool pluginDrew = false;
                auto pluginManager = dataProvider->getPluginManagerPtr();
                if (pluginManager && pluginManager->getFactory() && pluginManager->getFactory()->draw) {
                    static int drawCallCount = 0;
                    if (drawCallCount++ < 5) {
                        INFO("Calling plugin draw() function, attempt %d", drawCallCount);
                    }
                } else {
                    static int failureCount = 0;
                    if (failureCount++ < 5) {
                        INFO("Cannot call plugin draw(): pluginManager=%p, pluginFactory=%p, draw=%p", 
                             pluginManager,
                             pluginManager ? pluginManager->getFactory() : nullptr, 
                             (pluginManager && pluginManager->getFactory()) ? pluginManager->getFactory()->draw : nullptr);
                    }
                }
                
                if (pluginManager && pluginManager->getFactory() && pluginManager->getFactory()->draw) {
                    
                    // Clear NT_screen buffer before plugin draws
                    memset(NT_screen, 0, sizeof(NT_screen));
                    
                    static int pluginDrawCallCount = 0;
                    if (pluginDrawCallCount++ < 5) {
                        INFO("About to call plugin draw() for %s, attempt %d", 
                             pluginManager->getFactory()->name ? pluginManager->getFactory()->name : "unknown", 
                             pluginDrawCallCount);
                    }
                    
                    dataProvider->safeExecutePlugin([&]() {
                        pluginDrew = pluginManager->getFactory()->draw(pluginManager->getAlgorithm());
                        
                        static int drawResultCount = 0;
                        if (drawResultCount++ < 5) {
                            INFO("Plugin draw() returned %s, attempt %d", pluginDrew ? "true" : "false", drawResultCount);
                            
                            // Don't clear the buffer if plugin returned false - keep whatever it drew
                            // The plugin may have drawn text but returned false for other reasons
                            
                            // Check buffer state after plugin draw but before any potential clearing
                            INFO("NT_screen at row 40 (text area): %02x %02x %02x %02x %02x %02x %02x %02x",
                                 NT_screen[40 * 128], NT_screen[40 * 128 + 1], NT_screen[40 * 128 + 2], NT_screen[40 * 128 + 3],
                                 NT_screen[40 * 128 + 4], NT_screen[40 * 128 + 5], NT_screen[40 * 128 + 6], NT_screen[40 * 128 + 7]);
                                 
                            INFO("NT_screen first 16 bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                                 NT_screen[0], NT_screen[1], NT_screen[2], NT_screen[3],
                                 NT_screen[4], NT_screen[5], NT_screen[6], NT_screen[7],
                                 NT_screen[8], NT_screen[9], NT_screen[10], NT_screen[11],
                                 NT_screen[12], NT_screen[13], NT_screen[14], NT_screen[15]);
                        }
                    }, "draw");
                    
                    // Always sync NT_screen buffer to VCV display after plugin draw call
                    // This handles both direct NT_screen writes and NT_drawText/NT_drawShape calls
                    // Even if plugin returned false, it may have drawn text/shapes we want to display
                    VCVDisplayBuffer pluginBuffer;
                    syncNTScreenToVCVBuffer(pluginBuffer);
                    drawDisplayBuffer(args.vg, pluginBuffer);
                    pluginDrew = true; // Plugin attempted to draw, show the results even if it returned false
                    
                    static int syncCount = 0;
                    if (syncCount++ < 5) {
                        INFO("Synced NT_screen to VCV display buffer, attempt %d", syncCount);
                    }
                }
                
                if (!pluginDrew) {
                    // Show plugin name if no custom drawing
                    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                    nvgFontSize(args.vg, 14);
                    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    
                    auto pluginManager = dataProvider->getPluginManagerPtr();
                    if (pluginManager && pluginManager->getFactory() && pluginManager->getFactory()->name) {
                        nvgText(args.vg, 128, 32, pluginManager->getFactory()->name, NULL);
                    } else {
                        nvgText(args.vg, 128, 32, "Plugin Loaded", NULL);
                    }
                }
                
            } else {
                // Update emulator display
                dataProvider->updateDisplay();
                
                // Draw the display buffer
                drawDisplayBuffer(args.vg, dataProvider->getDisplayBuffer());
                
            }
        }
        
        nvgRestore(args.vg);
    }

    void ModuleOLEDWidget::drawPlaceholder(const DrawArgs& args) {
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

    void ModuleOLEDWidget::syncNTScreenToVCVBuffer(VCVDisplayBuffer& buffer) {
        // Convert NT_screen buffer (4-bit grayscale, 2 pixels per byte) to VCV display buffer with full grayscale support
        // NT_screen format: 128*64 bytes, each byte contains 2 pixels (high nibble = even x, low nibble = odd x)
        // VCV format: 256*64 pixels, grayscale values 0-15
        
        buffer.clear();
        
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 256; x += 2) {
                int byte_idx = y * 128 + x / 2;
                uint8_t byte_val = NT_screen[byte_idx];
                
                // Extract high nibble (even x coordinate)
                uint8_t pixel_even = (byte_val >> 4) & 0x0F;
                // Extract low nibble (odd x coordinate)  
                uint8_t pixel_odd = byte_val & 0x0F;
                
                // Store full 4-bit grayscale values (0-15)
                buffer.setPixelGray(x, y, pixel_even);
                if (x + 1 < 256) {
                    buffer.setPixelGray(x + 1, y, pixel_odd);
                }
            }
        }
    }

    void ModuleOLEDWidget::drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer) {
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

    void ModuleOLEDWidget::drawMenuInterface(NVGcontext* vg, IDisplayDataProvider* dataProvider) {
        // Hardware-style two-column layout: PAGES | PARAMETERS + VALUES
        
        // Define column positions and widths
        const float pageColumn = 5;
        const float pageWidth = 65;
        const float paramColumn = 75;
        const float paramWidth = 85; 
        const float valueColumn = 165;
        const float valueWidth = 85;
        
        // Use small font for menu
        nvgFontSize(vg, 9);
        
        auto parameterSystem = dataProvider->getParameterSystemPtr();
        if (!parameterSystem) return;
        
        // === LEFT COLUMN: PAGE LIST (always visible, scrollable) ===
        const int visiblePages = 5;
        int pageStartIdx = 0;
        
        // Center current page in view
        if (parameterSystem->getCurrentPageIndex() > 2 && parameterSystem->getPageCount() > visiblePages) {
            pageStartIdx = std::min(parameterSystem->getCurrentPageIndex() - 2, 
                                   std::max(0, (int)parameterSystem->getPageCount() - visiblePages));
        }
        
        // Draw pages
        for (int i = 0; i < visiblePages && (pageStartIdx + i) < (int)parameterSystem->getPageCount(); i++) {
            int pageIdx = pageStartIdx + i;
            const _NT_parameterPage& page = parameterSystem->getParameterPages()[pageIdx];
            float y = 8 + i * 10;
            
            bool isCurrentPage = (pageIdx == parameterSystem->getCurrentPageIndex());
            
            // Brighter color for current page (instead of bold)
            nvgFillColor(vg, isCurrentPage ? nvgRGB(255, 255, 255) : nvgRGB(120, 120, 120));
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            
            // Truncate page name if too long
            char pageName[12];
            strncpy(pageName, page.name, 11);
            pageName[11] = '\0';
            nvgText(vg, pageColumn, y, pageName, NULL);
        }
        
        // Page scroll indicators
        if (pageStartIdx > 0) {
            nvgFillColor(vg, nvgRGB(160, 160, 160));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(vg, pageColumn + pageWidth/2, 2, "▲", NULL);
        }
        if (pageStartIdx + visiblePages < (int)parameterSystem->getPageCount()) {
            nvgFillColor(vg, nvgRGB(160, 160, 160));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgText(vg, pageColumn + pageWidth/2, 56, "▼", NULL);
        }
        
        // === RIGHT COLUMN: PARAMETER LIST WITH VALUES ===
        if (parameterSystem->getCurrentPageIndex() < (int)parameterSystem->getPageCount()) {
            const _NT_parameterPage& currentPage = parameterSystem->getParameterPages()[parameterSystem->getCurrentPageIndex()];
            
            if (currentPage.numParams > 0) {
                const int visibleParams = 5;
                int paramStartIdx = 0;
                
                // Center current parameter in view
                if (parameterSystem->getCurrentParamIndex() > 2 && currentPage.numParams > visibleParams) {
                    paramStartIdx = std::min(parameterSystem->getCurrentParamIndex() - 2, 
                                           std::max(0, (int)currentPage.numParams - visibleParams));
                }
                
                // Draw parameters with values
                for (int i = 0; i < visibleParams && (paramStartIdx + i) < currentPage.numParams; i++) {
                    int paramListIdx = paramStartIdx + i;
                    int paramIdx;
                    
                    // Handle default pages (no params array)
                    if (currentPage.params == nullptr) {
                        paramIdx = paramListIdx; // Sequential indexing
                    } else {
                        paramIdx = currentPage.params[paramListIdx];
                    }
                    
                    if (paramIdx < 0 || paramIdx >= (int)parameterSystem->getParameterCount()) continue;
                    
                    const _NT_parameter& param = parameterSystem->getParameters()[paramIdx];
                    float y = 8 + i * 10;

                    bool isCurrentParam = (paramListIdx == parameterSystem->getCurrentParamIndex());
                    bool isGrayed = parameterSystem->isParameterGrayedOut(paramIdx);
                    int alpha = isGrayed ? 128 : 255;

                    // PARAMETER NAME - brighter color for current parameter, dimmed if grayed
                    if (isCurrentParam) {
                        nvgFillColor(vg, nvgRGBA(255, 255, 255, alpha));
                    } else {
                        nvgFillColor(vg, nvgRGBA(140, 140, 140, alpha));
                    }
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                    nvgText(vg, paramColumn, y, param.name, NULL);

                    // PARAMETER VALUE (beside the name)
                    int value = parameterSystem->getRoutingMatrix()[paramIdx];
                    char valueStr[32];
                    formatParameterValue(valueStr, param, value);

                    if (isCurrentParam) {
                        nvgFillColor(vg, nvgRGBA(255, 255, 255, alpha));
                    } else {
                        nvgFillColor(vg, nvgRGBA(160, 160, 160, alpha));
                    }
                    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
                    nvgText(vg, valueColumn + valueWidth, y, valueStr, NULL);
                }
                
                // Parameter scroll indicators
                if (paramStartIdx > 0) {
                    nvgFillColor(vg, nvgRGB(160, 160, 160));
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                    nvgText(vg, paramColumn + paramWidth/2, 2, "▲", NULL);
                }
                if (paramStartIdx + visibleParams < currentPage.numParams) {
                    nvgFillColor(vg, nvgRGB(160, 160, 160));
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                    nvgText(vg, paramColumn + paramWidth/2, 56, "▼", NULL);
                }
            }
        }
    }

    void ModuleOLEDWidget::formatParameterValue(char* str, const _NT_parameter& param, int value) const {
        // Apply scaling first
        float scaledValue = value;
        switch (param.scaling) {
            case kNT_scaling10: scaledValue = value / 10.0f; break;
            case kNT_scaling100: scaledValue = value / 100.0f; break;
            case kNT_scaling1000: scaledValue = value / 1000.0f; break;
        }
        
        // Format based on unit type
        switch (param.unit) {
            case kNT_unitNone:
                // Check if this is a Boolean parameter (min=0, max=1, no enum strings)
                if (param.min == 0 && param.max == 1 && param.enumStrings == nullptr) {
                    // Boolean parameter without enum strings
                    if (value == 0) {
                        strcpy(str, "Off");
                    } else {
                        strcpy(str, "On");
                    }
                } else {
                    snprintf(str, 32, "%d", value);
                }
                break;
                
            case kNT_unitEnum:
                if (param.enumStrings && value >= param.min && value <= param.max) {
                    strncpy(str, param.enumStrings[value - param.min], 31);
                    str[31] = '\0';
                } else if (param.min == 0 && param.max == 1 && param.enumStrings == nullptr) {
                    // Boolean enum without strings
                    if (value == 0) {
                        strcpy(str, "Off");
                    } else {
                        strcpy(str, "On");
                    }
                } else {
                    snprintf(str, 32, "%d", value);
                }
                break;
                
            case kNT_unitDb:
                snprintf(str, 32, "%.1f dB", scaledValue);
                break;
                
            case kNT_unitDb_minInf:
                if (value == param.min) {
                    strcpy(str, "-inf dB");
                } else {
                    snprintf(str, 32, "%.1f dB", scaledValue);
                }
                break;
                
            case kNT_unitPercent:
                snprintf(str, 32, "%d%%", value);
                break;
                
            case kNT_unitHz:
                if (scaledValue >= 1000) {
                    snprintf(str, 32, "%.1f kHz", scaledValue / 1000.0f);
                } else {
                    snprintf(str, 32, "%.1f Hz", scaledValue);
                }
                break;
                
            case kNT_unitSemitones:
                snprintf(str, 32, "%+d st", value);
                break;
                
            case kNT_unitCents:
                snprintf(str, 32, "%+d ct", value);
                break;
                
            case kNT_unitMs:
                snprintf(str, 32, "%.1f ms", scaledValue);
                break;
                
            case kNT_unitSeconds:
                snprintf(str, 32, "%.2f s", scaledValue);
                break;
                
            case kNT_unitFrames:
                snprintf(str, 32, "%d fr", value);
                break;
                
            case kNT_unitMIDINote:
                {
                    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                    int octave = (value / 12) - 1;
                    int note = value % 12;
                    snprintf(str, 32, "%s%d", noteNames[note], octave);
                }
                break;
                
            case kNT_unitMillivolts:
                snprintf(str, 32, "%d mV", value);
                break;
                
            case kNT_unitVolts:
                snprintf(str, 32, "%.2f V", scaledValue);
                break;
                
            case kNT_unitBPM:
                snprintf(str, 32, "%d BPM", value);
                break;
                
            case kNT_unitAudioInput:
            case kNT_unitCvInput:
                if (value == 0) {
                    strcpy(str, "None");
                } else if (value >= 1 && value <= 12) {
                    snprintf(str, 32, "Input %d", value);
                } else if (value >= 13 && value <= 20) {
                    snprintf(str, 32, "Output %d", value - 12);
                } else if (value >= 21 && value <= 28) {
                    snprintf(str, 32, "Aux %d", value - 20);
                } else {
                    snprintf(str, 32, "Bus %d", value);
                }
                break;
                
            case kNT_unitAudioOutput:
            case kNT_unitCvOutput:
                if (value == 0) {
                    strcpy(str, "None");
                } else if (value >= 1 && value <= 12) {
                    snprintf(str, 32, "Input %d", value);
                } else if (value >= 13 && value <= 20) {
                    snprintf(str, 32, "Output %d", value - 12);
                } else if (value >= 21 && value <= 28) {
                    snprintf(str, 32, "Aux %d", value - 20);
                } else {
                    snprintf(str, 32, "Bus %d", value);
                }
                break;
                
            case kNT_unitOutputMode:
                if (value == 0) strcpy(str, "Replace");
                else if (value == 1) strcpy(str, "Add");
                else snprintf(str, 32, "Mode %d", value);
                break;
                
            default:
                snprintf(str, 32, "%d", value);
                break;
        }
    }

} // namespace DisplayRenderer