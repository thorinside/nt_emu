name: "Implement Parameter Menu System for Jack Socket Mapping"
description: |
  Implement a multi-tier parameter menu system in the VCV Rack DistingNT module that allows mapping of jack sockets to plugin inputs/outputs, following the Disting NT hardware paradigm with parameter pages, navigation via encoders/pots, and OLED display feedback.

---

# Task: Parameter Menu System for Jack Socket Mapping

> Implement the Disting NT parameter page navigation system for configuring input/output routing in VCV Rack

## Critical Context

```yaml
context:
  docs:
    - url: https://vcvrack.com/docs-v2/rack/widget/Widget#onButton
      focus: Mouse and keyboard event handling for UI navigation
    
    - url: https://vcvrack.com/docs-v2/rack/app/ParamQuantity
      focus: Parameter value handling and display
    
    - file: /emulator/include/distingnt/api.h
      focus: _NT_parameter, _NT_parameterPage structures for menu data

  patterns:
    - file: https://github.com/bogaudio/BogaudioModules/blob/master/src/Analyzer.cpp
      copy: Multi-page parameter display with menu navigation
    
    - file: https://github.com/VCVRack/Fundamental/blob/v2/src/ADSR.cpp 
      copy: Display modes and parameter visualization

  gotchas:
    - issue: "VCV params are continuous, Disting NT uses discrete pages"
      fix: "Store page state separately, map encoder deltas to page navigation"
    
    - issue: "Hardware has dedicated menu button (left encoder press)"
      fix: "Track menu mode state, toggle on encoder button press"
    
    - issue: "Bus system has 28 buses vs 12 inputs + 6 outputs"
      fix: "Show only relevant buses based on plugin specifications"
    
    - issue: "Parameter changes need immediate audio routing updates"
      fix: "Update routing matrix in parameterChanged callback"
```

## Menu System Architecture

Based on the Disting NT API, the parameter menu system consists of:

1. **Parameter Pages** (_NT_parameterPage)
   - Named groups of related parameters
   - Each page contains parameter indices
   - Navigate between pages with left pot

2. **Parameters** (_NT_parameter)
   - Input/output routing parameters (buses 0-28)
   - Min/max/default values
   - Unit types (audio/CV input/output)

3. **Navigation Flow**
   - Left encoder press: Enter/exit menu mode
   - Left pot: Navigate between parameter pages
   - Left encoder: Select parameter within page
   - Right encoder: Change parameter value
   - Right encoder press: Confirm value

## Implementation Tasks

### 1. Add Menu State Management

```
READ vcv-plugin/src/DistingNT.cpp:
  - UNDERSTAND: Current module structure
  - FIND: Display widget interaction
  - NOTE: How encoder/pot values are tracked

UPDATE vcv-plugin/src/DistingNT.cpp:
  - ADD: Menu state variables
    ```cpp
    struct DistingNT : Module {
        // ... existing members ...
        
        // Menu system state
        enum MenuMode {
            MENU_OFF,
            MENU_PAGE_SELECT,
            MENU_PARAM_SELECT,
            MENU_VALUE_EDIT
        };
        
        MenuMode menuMode = MENU_OFF;
        int currentPageIndex = 0;
        int currentParamIndex = 0;
        int parameterEditValue = 0;
        
        // Cached parameter data from plugin
        std::vector<_NT_parameterPage> parameterPages;
        std::vector<_NT_parameter> parameters;
        
        // Routing matrix (parameter index -> bus mapping)
        std::array<int, 256> routingMatrix;
        
        // UI interaction tracking
        bool leftEncoderPressed = false;
        bool rightEncoderPressed = false;
        float lastLeftPotValue = 0.5f;
        int lastLeftEncoderValue = 0;
        int lastRightEncoderValue = 0;
    };
    ```
  - VALIDATE: make && echo "State variables added"
```

### 2. Extract Parameter Data from Plugin

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: loadPlugin method
  - ADD: Parameter extraction after plugin load
    ```cpp
    void loadPlugin(const std::string& path) {
        if (pluginLoader->load(path)) {
            // ... existing loading code ...
            
            // Extract parameter information
            if (currentAlgorithm) {
                extractParameterData();
            }
        }
    }
    
    void extractParameterData() {
        _NT_algorithm* algo = pluginLoader->getAlgorithm();
        if (!algo || !algo->parameters || !algo->parameterPages) {
            return;
        }
        
        // Clear existing data
        parameterPages.clear();
        parameters.clear();
        
        // Copy parameter definitions
        int paramIndex = 0;
        while (algo->parameters[paramIndex].name != nullptr) {
            parameters.push_back(algo->parameters[paramIndex]);
            paramIndex++;
        }
        
        // Copy parameter pages
        if (algo->parameterPages) {
            for (uint32_t i = 0; i < algo->parameterPages->numPages; i++) {
                parameterPages.push_back(algo->parameterPages->pages[i]);
            }
        }
        
        // Initialize routing matrix with defaults
        for (size_t i = 0; i < parameters.size(); i++) {
            routingMatrix[i] = parameters[i].def;
        }
    }
    ```
  - VALIDATE: Plugin loads and extracts parameter data
```

### 3. Implement Menu Navigation Logic

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - ADD: Navigation handling in process()
    ```cpp
    void process(const ProcessArgs& args) override {
        // Handle menu navigation
        if (params[ENCODER_L_PRESS_PARAM].getValue() > 0 && !leftEncoderPressed) {
            leftEncoderPressed = true;
            toggleMenuMode();
        } else if (params[ENCODER_L_PRESS_PARAM].getValue() == 0) {
            leftEncoderPressed = false;
        }
        
        if (menuMode != MENU_OFF) {
            processMenuNavigation();
        }
        
        // ... existing audio processing ...
    }
    
    void toggleMenuMode() {
        if (menuMode == MENU_OFF) {
            menuMode = MENU_PAGE_SELECT;
            currentPageIndex = 0;
            currentParamIndex = 0;
        } else {
            menuMode = MENU_OFF;
            applyRoutingChanges();
        }
    }
    
    void processMenuNavigation() {
        // Left pot: Page selection
        float leftPotValue = params[POT_L_PARAM].getValue();
        if (menuMode == MENU_PAGE_SELECT && parameterPages.size() > 0) {
            int newPageIndex = (int)(leftPotValue * (parameterPages.size() - 1));
            if (newPageIndex != currentPageIndex) {
                currentPageIndex = newPageIndex;
                currentParamIndex = 0;
            }
        }
        
        // Left encoder: Parameter selection
        int leftEncoderDelta = getEncoderDelta(ENCODER_L_PARAM, lastLeftEncoderValue);
        if (leftEncoderDelta != 0 && menuMode >= MENU_PAGE_SELECT) {
            if (menuMode == MENU_PAGE_SELECT) {
                menuMode = MENU_PARAM_SELECT;
            }
            
            if (menuMode == MENU_PARAM_SELECT) {
                navigateParameter(leftEncoderDelta);
            }
        }
        
        // Right encoder: Value editing
        int rightEncoderDelta = getEncoderDelta(ENCODER_R_PARAM, lastRightEncoderValue);
        if (rightEncoderDelta != 0 && menuMode >= MENU_PARAM_SELECT) {
            if (menuMode == MENU_PARAM_SELECT) {
                menuMode = MENU_VALUE_EDIT;
                parameterEditValue = getCurrentParameterValue();
            }
            
            if (menuMode == MENU_VALUE_EDIT) {
                adjustParameterValue(rightEncoderDelta);
            }
        }
        
        // Right encoder press: Confirm value
        if (params[ENCODER_R_PRESS_PARAM].getValue() > 0 && !rightEncoderPressed) {
            rightEncoderPressed = true;
            if (menuMode == MENU_VALUE_EDIT) {
                confirmParameterValue();
                menuMode = MENU_PARAM_SELECT;
            }
        } else if (params[ENCODER_R_PRESS_PARAM].getValue() == 0) {
            rightEncoderPressed = false;
        }
    }
    
    int getEncoderDelta(int paramId, int& lastValue) {
        int currentValue = (int)params[paramId].getValue();
        int delta = currentValue - lastValue;
        lastValue = currentValue;
        return delta;
    }
    
    void navigateParameter(int delta) {
        if (currentPageIndex >= parameterPages.size()) return;
        
        const _NT_parameterPage& page = parameterPages[currentPageIndex];
        currentParamIndex = clamp(currentParamIndex + delta, 0, (int)page.numParams - 1);
    }
    
    void adjustParameterValue(int delta) {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0) return;
        
        const _NT_parameter& param = parameters[paramIdx];
        parameterEditValue = clamp(parameterEditValue + delta, (int)param.min, (int)param.max);
    }
    
    int getCurrentParameterIndex() {
        if (currentPageIndex >= parameterPages.size()) return -1;
        
        const _NT_parameterPage& page = parameterPages[currentPageIndex];
        if (currentParamIndex >= page.numParams) return -1;
        
        return page.params[currentParamIndex];
    }
    
    int getCurrentParameterValue() {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0 || paramIdx >= 256) return 0;
        
        return routingMatrix[paramIdx];
    }
    
    void confirmParameterValue() {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0 || paramIdx >= 256) return;
        
        routingMatrix[paramIdx] = parameterEditValue;
        
        // Notify plugin of parameter change
        if (currentAlgorithm) {
            currentAlgorithm->parameterChanged(paramIdx, parameterEditValue);
        }
    }
    ```
  - VALIDATE: Navigation responds to controls correctly
```

### 4. Update OLED Display for Menu

```
UPDATE vcv-plugin/src/display/OLEDWidget.cpp:
  - FIND: draw method
  - ADD: Menu rendering matching Disting NT style
    ```cpp
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        DistingNT* distingModule = dynamic_cast<DistingNT*>(module);
        
        nvgSave(args.vg);
        nvgScale(args.vg, box.size.x / 256.f, box.size.y / 64.f);
        
        // Clear display
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, 256, 64);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        if (distingModule->menuMode != DistingNT::MENU_OFF) {
            drawMenuInterface(args.vg, distingModule);
        } else if (distingModule->loadingMessageTimer > 0) {
            // ... existing loading message code ...
        } else if (distingModule->currentAlgorithm) {
            // ... existing algorithm display ...
        }
        
        nvgRestore(args.vg);
    }
    
    void drawMenuInterface(NVGcontext* vg, DistingNT* module) {
        // Disting NT uses a three-column layout: PAGE | PARAMETER | VALUE
        
        // Define column positions
        const float pageColumn = 5;
        const float paramColumn = 85;
        const float valueColumn = 256 - 60; // Right-aligned values
        
        // Use small font for menu
        nvgFontSize(vg, 10);
        
        // Determine visible range (5 parameters shown at once)
        const int visibleParams = 5;
        int startIdx = 0;
        
        if (module->currentPageIndex < module->parameterPages.size()) {
            const _NT_parameterPage& page = module->parameterPages[module->currentPageIndex];
            
            // Center current parameter in view
            if (module->currentParamIndex > 2) {
                startIdx = std::min(module->currentParamIndex - 2, 
                                   std::max(0, (int)page.numParams - visibleParams));
            }
            
            // Draw parameters
            for (int i = 0; i < visibleParams && (startIdx + i) < page.numParams; i++) {
                int paramListIdx = startIdx + i;
                int paramIdx = page.params[paramListIdx];
                if (paramIdx >= module->parameters.size()) continue;
                
                const _NT_parameter& param = module->parameters[paramIdx];
                float y = 12 + i * 11; // Compact line spacing
                
                bool isSelected = (paramListIdx == module->currentParamIndex);
                bool isEditing = isSelected && (module->menuMode == DistingNT::MENU_VALUE_EDIT);
                
                // PAGE column - show page name only on first line
                if (i == 0 || isSelected) {
                    nvgFillColor(vg, isSelected ? nvgRGB(255, 255, 255) : nvgRGB(100, 100, 100));
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                    
                    // Show abbreviated page name
                    char pageAbbrev[16];
                    strncpy(pageAbbrev, page.name, 12);
                    pageAbbrev[12] = '\0';
                    nvgText(vg, pageColumn, y, pageAbbrev, NULL);
                }
                
                // PARAMETER column
                nvgFillColor(vg, isSelected ? nvgRGB(255, 255, 255) : nvgRGB(150, 150, 150));
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                nvgText(vg, paramColumn, y, param.name, NULL);
                
                // VALUE column
                char valueStr[32];
                int value = isEditing ? module->parameterEditValue : module->routingMatrix[paramIdx];
                formatParameterValue(valueStr, param, value);
                
                if (isEditing) {
                    // Invert colors for editing (black text on white background)
                    nvgBeginPath(vg);
                    nvgRect(vg, valueColumn - 5, y - 1, 65, 11);
                    nvgFillColor(vg, nvgRGB(255, 255, 255));
                    nvgFill(vg);
                    nvgFillColor(vg, nvgRGB(0, 0, 0));
                } else {
                    nvgFillColor(vg, isSelected ? nvgRGB(255, 255, 255) : nvgRGB(150, 150, 150));
                }
                
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
                nvgText(vg, valueColumn + 55, y, valueStr, NULL);
            }
            
            // Draw scroll indicators if needed
            if (startIdx > 0) {
                // Up arrow
                nvgFillColor(vg, nvgRGB(200, 200, 200));
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgText(vg, 128, 2, "^", NULL);
            }
            
            if (startIdx + visibleParams < page.numParams) {
                // Down arrow
                nvgFillColor(vg, nvgRGB(200, 200, 200));
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgText(vg, 128, 58, "v", NULL);
            }
        }
        
        // Draw navigation hints at bottom (smaller, dimmer)
        nvgFillColor(vg, nvgRGB(100, 100, 100));
        nvgFontSize(vg, 8);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        
        if (module->menuMode == DistingNT::MENU_PAGE_SELECT) {
            nvgText(vg, 5, 62, "POT=Page", NULL);
        } else if (module->menuMode == DistingNT::MENU_PARAM_SELECT) {
            nvgText(vg, 5, 62, "L.ENC=Param", NULL);
        } else if (module->menuMode == DistingNT::MENU_VALUE_EDIT) {
            nvgText(vg, 5, 62, "R.ENC=Value", NULL);
        }
        
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        nvgText(vg, 251, 62, "L.ENC=Exit", NULL);
    }
    
    void formatParameterValue(char* str, const _NT_parameter& param, int value) {
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
                sprintf(str, "%d", value);
                break;
                
            case kNT_unitEnum:
                if (param.enumStrings && value >= param.min && value <= param.max) {
                    strncpy(str, param.enumStrings[value - param.min], 31);
                    str[31] = '\0';
                } else {
                    sprintf(str, "%d", value);
                }
                break;
                
            case kNT_unitDb:
                sprintf(str, "%.1f dB", scaledValue);
                break;
                
            case kNT_unitDb_minInf:
                if (value == param.min) {
                    strcpy(str, "-inf dB");
                } else {
                    sprintf(str, "%.1f dB", scaledValue);
                }
                break;
                
            case kNT_unitPercent:
                sprintf(str, "%d%%", value);
                break;
                
            case kNT_unitHz:
                if (scaledValue >= 1000) {
                    sprintf(str, "%.1f kHz", scaledValue / 1000.0f);
                } else {
                    sprintf(str, "%.1f Hz", scaledValue);
                }
                break;
                
            case kNT_unitSemitones:
                sprintf(str, "%+d st", value);
                break;
                
            case kNT_unitCents:
                sprintf(str, "%+d ct", value);
                break;
                
            case kNT_unitMs:
                sprintf(str, "%.1f ms", scaledValue);
                break;
                
            case kNT_unitSeconds:
                sprintf(str, "%.2f s", scaledValue);
                break;
                
            case kNT_unitFrames:
                sprintf(str, "%d fr", value);
                break;
                
            case kNT_unitMIDINote:
                {
                    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                    int octave = (value / 12) - 1;
                    int note = value % 12;
                    sprintf(str, "%s%d", noteNames[note], octave);
                }
                break;
                
            case kNT_unitMillivolts:
                sprintf(str, "%d mV", value);
                break;
                
            case kNT_unitVolts:
                sprintf(str, "%.2f V", scaledValue);
                break;
                
            case kNT_unitBPM:
                sprintf(str, "%d BPM", value);
                break;
                
            case kNT_unitAudioInput:
                if (value == 0) {
                    strcpy(str, "Off");
                } else if (value <= 4) {
                    sprintf(str, "In %d", value);
                } else if (value <= 28) {
                    sprintf(str, "Bus %d", value);
                }
                break;
                
            case kNT_unitCvInput:
                if (value == 0) {
                    strcpy(str, "Off");
                } else if (value >= 13 && value <= 20) {
                    sprintf(str, "CV %d", value - 12);
                } else if (value <= 28) {
                    sprintf(str, "Bus %d", value);
                }
                break;
                
            case kNT_unitAudioOutput:
                if (value >= 21 && value <= 24) {
                    sprintf(str, "Out %d", value - 20);
                } else if (value <= 28) {
                    sprintf(str, "Bus %d", value);
                } else {
                    strcpy(str, "Off");
                }
                break;
                
            case kNT_unitCvOutput:
                if (value >= 25 && value <= 28) {
                    sprintf(str, "CV %d", value - 24);
                } else if (value <= 28) {
                    sprintf(str, "Bus %d", value);
                } else {
                    strcpy(str, "Off");
                }
                break;
                
            case kNT_unitOutputMode:
                // Typical output modes
                if (value == 0) strcpy(str, "Direct");
                else if (value == 1) strcpy(str, "Add");
                else sprintf(str, "Mode %d", value);
                break;
                
            default:
                sprintf(str, "%d", value);
                break;
        }
    }
    ```
  - VALIDATE: Menu displays correctly matching Disting NT style
```

### 5. Implement Routing Matrix Application

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - ADD: Routing application logic
    ```cpp
    void applyRoutingChanges() {
        // Update bus routing based on parameter values
        updateBusRouting();
        
        // Save routing to module state
        routingDirty = true;
    }
    
    void updateBusRouting() {
        // Clear existing routing
        for (int i = 0; i < 28; i++) {
            busInputMap[i] = -1;
            busOutputMap[i] = -1;
        }
        
        // Apply routing from parameters
        for (size_t i = 0; i < parameters.size(); i++) {
            const _NT_parameter& param = parameters[i];
            int busIndex = routingMatrix[i];
            
            if (busIndex == 0) continue; // 0 = disconnected
            
            // Map based on parameter unit type
            switch (param.unit) {
                case kNT_unitAudioInput:
                case kNT_unitCvInput:
                    // Map VCV input to bus
                    if (busIndex >= 1 && busIndex <= 28) {
                        int vcvInput = getVCVInputForParameter(i);
                        if (vcvInput >= 0) {
                            busInputMap[busIndex - 1] = vcvInput;
                        }
                    }
                    break;
                    
                case kNT_unitAudioOutput:
                case kNT_unitCvOutput:
                    // Map bus to VCV output
                    if (busIndex >= 1 && busIndex <= 28) {
                        int vcvOutput = getVCVOutputForParameter(i);
                        if (vcvOutput >= 0) {
                            busOutputMap[busIndex - 1] = vcvOutput;
                        }
                    }
                    break;
            }
        }
    }
    
    int getVCVInputForParameter(int paramIndex) {
        // Map parameter index to VCV input based on naming convention
        // This would need to be customized based on how parameters are named
        // For now, simple sequential mapping
        return paramIndex < 12 ? paramIndex : -1;
    }
    
    int getVCVOutputForParameter(int paramIndex) {
        // Map parameter index to VCV output
        // Assuming outputs come after inputs in parameter list
        return (paramIndex >= 12 && paramIndex < 18) ? paramIndex - 12 : -1;
    }
    
    // Update process() to use routing
    void process(const ProcessArgs& args) override {
        // ... menu handling ...
        
        // Apply routing to buses
        if (sampleCounter == 0) {
            // Input routing
            for (int bus = 0; bus < 28; bus++) {
                int vcvInput = busInputMap[bus];
                if (vcvInput >= 0 && vcvInput < INPUTS_LEN) {
                    if (inputs[vcvInput].isConnected()) {
                        buses[bus][0] = inputs[vcvInput].getVoltage() / 5.0f;
                    } else {
                        buses[bus][0] = 0.0f;
                    }
                } else {
                    buses[bus][0] = 0.0f;
                }
            }
        }
        
        // ... algorithm processing ...
        
        // Output routing
        if (sampleCounter == 3) {
            for (int bus = 0; bus < 28; bus++) {
                int vcvOutput = busOutputMap[bus];
                if (vcvOutput >= 0 && vcvOutput < OUTPUTS_LEN) {
                    outputs[vcvOutput].setVoltage(buses[bus][3] * 5.0f);
                }
            }
        }
        
        // ... rest of process ...
    }
    ```
  - VALIDATE: Audio routes correctly based on menu settings
```

### 6. Add Preset Persistence for Routing

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: dataToJson/dataFromJson
  - ADD: Routing matrix serialization
    ```cpp
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        
        // ... existing serialization ...
        
        // Save routing matrix
        json_t* routingJ = json_array();
        for (int i = 0; i < 256; i++) {
            if (routingMatrix[i] != 0) {
                json_t* routeJ = json_object();
                json_object_set_new(routeJ, "param", json_integer(i));
                json_object_set_new(routeJ, "bus", json_integer(routingMatrix[i]));
                json_array_append_new(routingJ, routeJ);
            }
        }
        json_object_set_new(rootJ, "routing", routingJ);
        
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        // ... existing deserialization ...
        
        // Load routing matrix
        json_t* routingJ = json_object_get(rootJ, "routing");
        if (routingJ) {
            // Reset to defaults first
            for (size_t i = 0; i < parameters.size(); i++) {
                routingMatrix[i] = parameters[i].def;
            }
            
            // Apply saved routing
            size_t routingSize = json_array_size(routingJ);
            for (size_t i = 0; i < routingSize; i++) {
                json_t* routeJ = json_array_get(routingJ, i);
                if (routeJ) {
                    int param = json_integer_value(json_object_get(routeJ, "param"));
                    int bus = json_integer_value(json_object_get(routeJ, "bus"));
                    if (param >= 0 && param < 256) {
                        routingMatrix[param] = bus;
                    }
                }
            }
            
            // Apply routing
            updateBusRouting();
        }
    }
    ```
  - VALIDATE: Save/load preset preserves routing
```

### 7. Add Visual Feedback for Active Routing

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - ADD: Port lighting based on routing
    ```cpp
    // In process(), after routing updates
    void updatePortLights() {
        // Light up connected inputs
        for (int i = 0; i < 12; i++) {
            bool isRouted = false;
            for (int bus = 0; bus < 28; bus++) {
                if (busInputMap[bus] == i) {
                    isRouted = true;
                    break;
                }
            }
            lights[INPUT_LIGHT_1 + i].setBrightness(isRouted ? 1.0f : 0.0f);
        }
        
        // Light up connected outputs
        for (int i = 0; i < 6; i++) {
            bool isRouted = false;
            for (int bus = 0; bus < 28; bus++) {
                if (busOutputMap[bus] == i) {
                    isRouted = true;
                    break;
                }
            }
            lights[OUTPUT_LIGHT_1 + i].setBrightness(isRouted ? 1.0f : 0.0f);
        }
    }
    ```
  - VALIDATE: Port lights indicate active routing
```

### 8. Test Complete System

```
CHECKPOINT integration:
  - BUILD: make clean && make install
  - RUN: Rack -d
  - LOAD: Plugin with parameter pages
  - TEST: Press left encoder to enter menu
  - VERIFY: OLED shows parameter pages
  - TEST: Turn left pot to select pages
  - VERIFY: Page selection updates display
  - TEST: Turn left encoder to select parameters
  - VERIFY: Parameter highlight moves
  - TEST: Turn right encoder to edit value
  - VERIFY: Value changes in yellow
  - TEST: Press right encoder to confirm
  - VERIFY: Value saved, returns to selection
  - TEST: Press left encoder to exit menu
  - VERIFY: Routing applied to audio
  - TEST: Save and reload preset
  - VERIFY: Routing preserved
```

## Validation Checklist

- [ ] Left encoder press toggles menu mode
- [ ] OLED displays parameter pages and values
- [ ] Left pot navigates between pages
- [ ] Left encoder selects parameters
- [ ] Right encoder edits values
- [ ] Right encoder press confirms changes
- [ ] Parameter values show correct bus mappings
- [ ] Audio routing follows parameter settings
- [ ] Visual feedback shows active routings
- [ ] Preset save/load preserves routing
- [ ] Menu exits cleanly and applies changes
- [ ] No crashes with missing parameter data

## Rollback Strategy

```bash
# If menu system causes issues:
git stash
git checkout HEAD -- vcv-plugin/src/DistingNT.cpp
git checkout HEAD -- vcv-plugin/src/display/OLEDWidget.cpp
make clean && make install
```

## Performance Notes

- Menu rendering only active when menu open
- Routing matrix updates only on parameter change
- Use dirty flags to avoid unnecessary updates
- Cache parameter data to avoid repeated lookups

## Future Enhancements

- Scroll long parameter lists
- Quick routing presets
- Visual routing diagram mode
- MIDI learn for menu navigation
- Touch screen support for menu