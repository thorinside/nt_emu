name: "Update VCV Rack Module Panel to Match Disting NT Hardware"
description: |
  Update the VCV Rack module panel layout to exactly match the physical Disting NT hardware with 12 inputs, 6 outputs, OLED display, 3 pots, 2 encoders, and 4 buttons positioned precisely as shown in the reference image.

---

# Task: Update VCV Rack Module Panel Layout

> Comprehensive task list to update the DistingNT VCV Rack module panel to match the exact hardware layout

## Critical Context

```yaml
context:
  docs:
    - url: https://vcvrack.com/manual/Panel
      focus: SVG panel creation and component positioning
    
    - url: https://vcvrack.com/manual/Components
      focus: Standard component sizes and spacing

  patterns:
    - file: vcv-plugin/src/DistingNT.cpp
      copy: Component positioning pattern using mm2px()
    
    - file: vcv-plugin/res/panels/DistingNT.svg
      copy: SVG structure for VCV panels

  gotchas:
    - issue: "VCV uses mm measurements, 1HP = 5.08mm"
      fix: "Use mm2px() for all positioning"
    
    - issue: "Component positions are center-based"
      fix: "Calculate center points, not top-left"
    
    - issue: "Jacks need labels for clarity"
      fix: "Add text elements in SVG or use createLabel()"
```

## Hardware Analysis from Image

Based on the reference image:
- **Module width**: Approximately 14HP (71.12mm)
- **OLED Display**: 256x64 pixels, centered at top
- **3 Pots**: Large knobs in upper middle section
- **2 Encoders**: Medium knobs below pots
- **4 Buttons**: 2 left, 2 right of encoders
- **12 Inputs**: 3 rows of 4 jacks, numbered 1-12
- **6 Outputs**: 3 rows of 2 jacks, numbered 1-6
- **Branding**: "expert sleepers" and "disting NT" text
- **Logo**: Lightning bolt symbol at top

## Visual Panel Layout (ASCII Representation)

```
┌─────────────────────────────────────────────────────────────┐
│  ●                        ⚡                             ●   │  <- Mounting screws
│                                                              │
│          ┌─────────────────────────────────────┐            │
│          │   expert sleepers                    │            │  <- OLED Display
│          │      disting NT                      │            │     (cyan text)
│          │ v1.0.0                               │            │
│          └─────────────────────────────────────┘            │
│                                                              │
│  ┌─┐            ◯              ◯              ◯             │  <- 3 Pots
│  │ │         (POT_L)        (POT_C)        (POT_R)          │     (large)
│  │ │                                                         │
│  │ │                                                         │  <- SD slot
│  │ │                                                         │
│  └─┘         ◉              ◉                               │  <- 2 Encoders
│           (ENC_L)        (ENC_R)                             │     (medium)
│                                                              │
│      ○ ○                                        ○ ○          │  <- 4 Buttons
│      1 2                                        3 4          │     with LEDs
│                                                              │
│    ① ② ③ ④                           ❶ ❷                    │  <- I/O Jacks
│                                                              │     12 inputs
│    ⑤ ⑥ ⑦ ⑧                           ❸ ❹                    │     6 outputs
│                                                              │
│    ⑨ ⑩ ⑪ ⑫                           ❺ ❻                    │
│                                                              │
│               expert sleepers                                │  <- Bottom text
│                 disting NT                                   │
│  ●                                                      ●    │  <- Mounting screws
└─────────────────────────────────────────────────────────────┘
```

## Precise Coordinate Mapping (in mm from top-left)

```yaml
panel_coordinates:
  width: 71.12  # 14HP
  height: 128.5  # Standard Eurorack 3U
  
  mounting_screws:
    top_left: [7.62, 3.0]
    top_right: [63.5, 3.0]
    bottom_left: [7.62, 125.5]
    bottom_right: [63.5, 125.5]
  
  oled_display:
    center: [35.56, 15.0]
    width: 50.0
    height: 12.5
  
  pots:  # Large knobs
    L: [17.78, 35.0]  # Left pot center
    C: [35.56, 35.0]  # Center pot center  
    R: [53.34, 35.0]  # Right pot center
    diameter: 12.0
  
  encoders:  # Medium knobs
    L: [26.67, 52.0]  # Left encoder center
    R: [44.45, 52.0]  # Right encoder center
    diameter: 8.0
  
  buttons:  # Small momentary
    1: [12.0, 52.0]   # Left button 1
    2: [17.0, 52.0]   # Left button 2
    3: [54.0, 52.0]   # Right button 3
    4: [59.0, 52.0]   # Right button 4
    diameter: 5.0
  
  inputs:  # 3.5mm jacks (3 rows x 4 columns)
    row1: [[14.0, 70.0], [23.0, 70.0], [32.0, 70.0], [41.0, 70.0]]  # 1-4
    row2: [[14.0, 80.0], [23.0, 80.0], [32.0, 80.0], [41.0, 80.0]]  # 5-8
    row3: [[14.0, 90.0], [23.0, 90.0], [32.0, 90.0], [41.0, 90.0]]  # 9-12
    
  outputs:  # 3.5mm jacks (3 rows x 2 columns)
    row1: [[50.0, 70.0], [59.0, 70.0]]  # 1-2
    row2: [[50.0, 80.0], [59.0, 80.0]]  # 3-4
    row3: [[50.0, 90.0], [59.0, 90.0]]  # 5-6
  
  sd_card_slot:
    top_left: [3.0, 30.0]
    width: 4.0
    height: 25.0
  
  labels:
    brand_top: [35.56, 8.0]      # "expert sleepers" 
    model_top: [35.56, 10.0]     # "disting NT"
    version: [10.0, 18.0]        # "v1.0.0"
    brand_bottom: [35.56, 110.0] # "expert sleepers"
    model_bottom: [35.56, 115.0] # "disting NT"
```

## Visual Validation Test Points

```python
# Validation script to verify layout (pseudo-code)
def validate_panel_layout():
    checks = {
        "oled_centered": abs(oled.x - panel.width/2) < 0.1,
        "pots_aligned": pot_L.y == pot_C.y == pot_R.y,
        "pots_spacing": abs((pot_R.x - pot_L.x) / 2 - pot_C.x + pot_L.x) < 0.1,
        "encoders_centered": abs((enc_L.x + enc_R.x) / 2 - panel.width/2) < 1.0,
        "buttons_symmetric": abs(btn_1.x + btn_4.x - panel.width) < 1.0,
        "inputs_grid": all(row[i].x == row[0].x + i*9.0 for row in inputs for i in range(4)),
        "outputs_aligned": all(outputs[i].x == outputs[0].x for i in [0,2,4]),
        "vertical_spacing": all(abs(row[i+1].y - row[i].y - 10.0) < 0.1 for row in [inputs, outputs])
    }
    return all(checks.values())
```

## Task Breakdown

### 1. Setup Tasks

```
READ vcv-plugin/src/DistingNT.cpp:
  - UNDERSTAND: Current panel dimensions and layout
  - FIND: Component positioning code
  - NOTE: Current enum definitions for params/inputs/outputs

READ vcv-plugin/res/panels/DistingNT.svg:
  - UNDERSTAND: SVG structure if exists
  - NOTE: Panel dimensions and background
  - CHECK: Coordinate system used
```

### 2. Update Module Definition

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: enum InputId {
  - MODIFY: Ensure 12 inputs defined (AUDIO_INPUT_1 through AUDIO_INPUT_12)
  - VALIDATE: grep -c "AUDIO_INPUT" src/DistingNT.cpp | grep 12
  - IF_FAIL: Count inputs and add missing ones

UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: enum OutputId {
  - MODIFY: Change to 6 outputs (AUDIO_OUTPUT_1 through AUDIO_OUTPUT_6)
  - VALIDATE: grep -c "AUDIO_OUTPUT" src/DistingNT.cpp | grep 6
  - IF_FAIL: Adjust output count

UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: config(PARAMS_LEN
  - UPDATE: Input/output configuration:
    ```cpp
    // Configure 12 inputs
    for (int i = 0; i < 12; i++) {
        configInput(AUDIO_INPUT_1 + i, string::f("Input %d", i + 1));
    }
    // Configure 6 outputs  
    for (int i = 0; i < 6; i++) {
        configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
    }
    ```
  - VALIDATE: make && echo "Build successful"
```

### 3. Create Panel SVG

```
CREATE vcv-plugin/res/panels/DistingNT.svg:
  - DIMENSIONS: 71.12mm x 128.5mm (14HP module)
  - BACKGROUND: Dark grey (#3A3A3F or similar)
  - ADD: Lightning bolt logo at top center
  - ADD: "expert sleepers" text in cyan (#00FFFF)
  - ADD: "disting NT" text below in cyan
  - ADD: Version "v1.0.0" in bottom left
  - VALIDATE: inkscape --query-width res/panels/DistingNT.svg | grep "71.12"
  - IF_FAIL: Check SVG dimensions in mm units
```

### 4. Update ModuleWidget Layout

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: struct DistingNTWidget : ModuleWidget {
  - REPLACE: Complete widget implementation with new layout:
    ```cpp
    struct DistingNTWidget : ModuleWidget {
        DistingNTWidget(DistingNT* module) {
            setModule(module);
            setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DistingNT.svg")));

            // Add mounting screws
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

            // OLED Display (centered at top)
            OLEDWidget* display = createWidget<OLEDWidget>(mm2px(Vec(35.56, 15.0)));
            display->box.size = mm2px(Vec(50.0, 12.5));
            display->module = module;
            addChild(display);

            // 3 Pots (row below display)
            float potY = 35.0;
            float potSpacing = 18.0;
            addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(17.78, potY)), module, DistingNT::POT_L_PARAM));
            addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(35.56, potY)), module, DistingNT::POT_C_PARAM));
            addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(53.34, potY)), module, DistingNT::POT_R_PARAM));

            // 2 Encoders (row below pots)
            float encoderY = 52.0;
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.67, encoderY)), module, DistingNT::ENCODER_L_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(44.45, encoderY)), module, DistingNT::ENCODER_R_PARAM));

            // 4 Buttons (2 left, 2 right of encoders)
            float buttonY = encoderY;
            addParam(createParamCentered<LEDButton>(mm2px(Vec(12.0, buttonY)), module, DistingNT::BUTTON_1_PARAM));
            addParam(createParamCentered<LEDButton>(mm2px(Vec(17.0, buttonY)), module, DistingNT::BUTTON_2_PARAM));
            addParam(createParamCentered<LEDButton>(mm2px(Vec(54.0, buttonY)), module, DistingNT::BUTTON_3_PARAM));
            addParam(createParamCentered<LEDButton>(mm2px(Vec(59.0, buttonY)), module, DistingNT::BUTTON_4_PARAM));

            // Button LEDs
            addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.0, buttonY)), module, DistingNT::BUTTON_1_LIGHT));
            addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.0, buttonY)), module, DistingNT::BUTTON_2_LIGHT));
            addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(54.0, buttonY)), module, DistingNT::BUTTON_3_LIGHT));
            addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(59.0, buttonY)), module, DistingNT::BUTTON_4_LIGHT));

            // 12 Inputs (3 rows of 4)
            float inputStartY = 70.0;
            float jackSpacing = 9.0;
            float rowSpacing = 10.0;
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 4; col++) {
                    int index = row * 4 + col;
                    float x = 14.0 + col * jackSpacing;
                    float y = inputStartY + row * rowSpacing;
                    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, DistingNT::AUDIO_INPUT_1 + index));
                    
                    // Add number labels
                    std::string label = std::to_string(index + 1);
                    addChild(createWidget<Label>(mm2px(Vec(x - 4.0, y - 7.0))));
                }
            }

            // 6 Outputs (3 rows of 2) 
            float outputStartX = 50.0;
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 2; col++) {
                    int index = row * 2 + col;
                    float x = outputStartX + col * jackSpacing;
                    float y = inputStartY + row * rowSpacing;
                    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, DistingNT::AUDIO_OUTPUT_1 + index));
                    
                    // Add number labels
                    std::string label = std::to_string(index + 1);
                    addChild(createWidget<Label>(mm2px(Vec(x - 4.0, y - 7.0))));
                }
            }
        }
    };
    ```
  - VALIDATE: make && echo "Widget updated successfully"
  - IF_FAIL: Check component names match VCV SDK
```

### 5. Implement SD Card Slot Visual

```
UPDATE vcv-plugin/res/panels/DistingNT.svg:
  - ADD: SD card slot graphic on left side (as shown in image)
  - POSITION: Left edge, vertically centered with pots
  - STYLE: Dark rectangle with highlight
  - VALIDATE: Visual inspection in Inkscape
```

### 6. Add Visual Labels

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: // Add number labels
  - IMPLEMENT: Proper label creation:
    ```cpp
    struct NumberLabel : Widget {
        std::string text;
        void draw(const DrawArgs& args) override {
            nvgFontSize(args.vg, 9);
            nvgFillColor(args.vg, nvgRGB(200, 200, 200));
            nvgText(args.vg, 0, 0, text.c_str(), NULL);
        }
    };
    
    // In widget constructor, replace Label creation with:
    NumberLabel* label = createWidget<NumberLabel>(mm2px(Vec(x - 3.0, y - 6.0)));
    label->text = std::to_string(index + 1);
    addChild(label);
    ```
  - VALIDATE: make && echo "Labels implemented"
```

### 7. Fine-tune Positioning

```
CHECKPOINT visual:
  - BUILD: make install
  - RUN: Rack -d
  - LOAD: DistingNT module
  - COMPARE: With reference image
  - ADJUST: Component positions in code
  - ITERATE: Until exact match

UPDATE vcv-plugin/src/DistingNT.cpp:
  - TUNE: Exact positions based on visual comparison
  - ADJUST: Spacing between components
  - ENSURE: Symmetric layout
  - VALIDATE: Visual inspection matches reference
```

### 8. Update Bus System

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: void process(const ProcessArgs& args)
  - UPDATE: Bus routing for 12 inputs / 6 outputs:
    ```cpp
    // Map 12 inputs to buses
    for (int i = 0; i < 12; i++) {
        if (inputs[AUDIO_INPUT_1 + i].isConnected()) {
            float v = inputs[AUDIO_INPUT_1 + i].getVoltage();
            buses[i][sampleIndex] = v / 5.0f; // ±5V to ±1.0
        }
    }
    
    // Map buses to 6 outputs
    for (int i = 0; i < 6; i++) {
        outputs[AUDIO_OUTPUT_1 + i].setVoltage(buses[20 + i][sampleIndex] * 5.0f);
    }
    ```
  - VALIDATE: make && ./test_audio_routing
  - IF_FAIL: Check bus array size and indices
```

### 9. Visual Validation Tool

```
CREATE vcv-plugin/tools/validate_panel.py:
  - IMPLEMENT: Visual panel validator
    ```python
    #!/usr/bin/env python3
    import json
    import sys
    from math import sqrt, fabs

    # Expected coordinates from PRP
    EXPECTED = {
        "panel_width": 71.12,
        "panel_height": 128.5,
        "oled": {"x": 35.56, "y": 15.0, "w": 50.0, "h": 12.5},
        "pots": {
            "L": {"x": 17.78, "y": 35.0},
            "C": {"x": 35.56, "y": 35.0},
            "R": {"x": 53.34, "y": 35.0}
        },
        "encoders": {
            "L": {"x": 26.67, "y": 52.0},
            "R": {"x": 44.45, "y": 52.0}
        },
        "buttons": {
            "1": {"x": 12.0, "y": 52.0},
            "2": {"x": 17.0, "y": 52.0},
            "3": {"x": 54.0, "y": 52.0},
            "4": {"x": 59.0, "y": 52.0}
        },
        "inputs": [
            # Row 1 (1-4)
            {"x": 14.0, "y": 70.0}, {"x": 23.0, "y": 70.0}, 
            {"x": 32.0, "y": 70.0}, {"x": 41.0, "y": 70.0},
            # Row 2 (5-8)
            {"x": 14.0, "y": 80.0}, {"x": 23.0, "y": 80.0},
            {"x": 32.0, "y": 80.0}, {"x": 41.0, "y": 80.0},
            # Row 3 (9-12)
            {"x": 14.0, "y": 90.0}, {"x": 23.0, "y": 90.0},
            {"x": 32.0, "y": 90.0}, {"x": 41.0, "y": 90.0}
        ],
        "outputs": [
            # Row 1 (1-2)
            {"x": 50.0, "y": 70.0}, {"x": 59.0, "y": 70.0},
            # Row 2 (3-4)
            {"x": 50.0, "y": 80.0}, {"x": 59.0, "y": 80.0},
            # Row 3 (5-6)
            {"x": 50.0, "y": 90.0}, {"x": 59.0, "y": 90.0}
        ]
    }

    def validate_coordinates(actual_coords):
        """Validate actual coordinates against expected"""
        errors = []
        warnings = []
        
        # Check panel dimensions
        if fabs(actual_coords["width"] - EXPECTED["panel_width"]) > 0.1:
            errors.append(f"Panel width {actual_coords['width']} != {EXPECTED['panel_width']}")
        
        # Check OLED centered
        oled_center = EXPECTED["panel_width"] / 2
        if fabs(actual_coords["oled"]["x"] - oled_center) > 0.5:
            errors.append(f"OLED not centered: {actual_coords['oled']['x']} != {oled_center}")
        
        # Check pot alignment
        pot_y = EXPECTED["pots"]["L"]["y"]
        for pot in ["L", "C", "R"]:
            if fabs(actual_coords["pots"][pot]["y"] - pot_y) > 0.1:
                errors.append(f"Pot {pot} misaligned vertically")
        
        # Check pot spacing
        pot_spacing = (EXPECTED["pots"]["R"]["x"] - EXPECTED["pots"]["L"]["x"]) / 2
        actual_spacing = (actual_coords["pots"]["R"]["x"] - actual_coords["pots"]["L"]["x"]) / 2
        if fabs(actual_spacing - pot_spacing) > 0.5:
            warnings.append(f"Pot spacing off: {actual_spacing} != {pot_spacing}")
        
        # Check input grid alignment
        for i in range(3):  # Check each row
            row_start = i * 4
            row_y = EXPECTED["inputs"][row_start]["y"]
            for j in range(4):  # Check each column
                idx = row_start + j
                if fabs(actual_coords["inputs"][idx]["y"] - row_y) > 0.1:
                    errors.append(f"Input {idx+1} not aligned in row")
        
        # Check output alignment
        for i in range(3):  # Check each row
            row_start = i * 2
            if fabs(actual_coords["outputs"][row_start]["x"] - 
                   actual_coords["outputs"][row_start+1]["x"] - 9.0) > 0.1:
                warnings.append(f"Output row {i+1} spacing incorrect")
        
        # Print results
        print("=== PANEL VALIDATION RESULTS ===")
        print(f"✓ Total checks: {len(errors) + len(warnings) + 10}")
        
        if errors:
            print(f"\n❌ ERRORS ({len(errors)}):")
            for e in errors:
                print(f"  - {e}")
        
        if warnings:
            print(f"\n⚠️  WARNINGS ({len(warnings)}):")
            for w in warnings:
                print(f"  - {w}")
        
        if not errors and not warnings:
            print("\n✅ ALL CHECKS PASSED! Panel layout matches specification.")
        
        # Generate visual diff
        print("\n=== VISUAL DIFF ===")
        print("Expected -> Actual (differences > 0.5mm shown)")
        for component in ["pots", "encoders", "buttons"]:
            for name, expected in EXPECTED[component].items():
                actual = actual_coords[component][name]
                dx = actual["x"] - expected["x"]
                dy = actual["y"] - expected["y"]
                if fabs(dx) > 0.5 or fabs(dy) > 0.5:
                    print(f"{component}.{name}: ({expected['x']}, {expected['y']}) -> ({actual['x']}, {actual['y']}) Δ({dx:+.1f}, {dy:+.1f})")
        
        return len(errors) == 0

    if __name__ == "__main__":
        # This would be populated by parsing the actual widget code
        # For now, use expected values as placeholder
        actual = {
            "width": 71.12,
            "oled": {"x": 35.56, "y": 15.0},
            "pots": EXPECTED["pots"],
            "encoders": EXPECTED["encoders"],
            "buttons": EXPECTED["buttons"],
            "inputs": EXPECTED["inputs"],
            "outputs": EXPECTED["outputs"]
        }
        
        sys.exit(0 if validate_coordinates(actual) else 1)
    ```
  - VALIDATE: python3 tools/validate_panel.py
  - IF_FAIL: Adjust coordinates based on diff output
```

### 10. Test Complete Module

```
CHECKPOINT integration:
  - BUILD: make clean && make
  - INSTALL: make install
  - TEST: Load in VCV Rack
  - VERIFY: All 12 inputs accept cables
  - VERIFY: All 6 outputs produce signal
  - VERIFY: Display renders correctly
  - VERIFY: All controls respond
  - CHECK: No crashes or errors in log
  - RUN: python3 tools/validate_panel.py
  - SCREENSHOT: Take screenshot for visual comparison
```

### 11. Create Visual Reference Documentation

```
CREATE vcv-plugin/docs/panel_reference.md:
  - CONTENT: Visual reference guide
    ```markdown
    # Disting NT VCV Rack Panel Reference
    
    ## Visual Layout Comparison
    
    ### Original Hardware (Reference Image)
    - 14HP Eurorack module
    - Dark grey panel with cyan OLED
    - 12 inputs (3x4 grid) on left
    - 6 outputs (3x2 grid) on right
    
    ### VCV Rack Implementation
    
    ```
    Component Layout (measurements in mm):
    
         0    10    20    30    40    50    60    70
         |-----|-----|-----|-----|-----|-----|-----|
     0 - ●                  ⚡                      ●    <- Screws
         |                                          |
    10 - |      [======= OLED DISPLAY =======]     |    <- 256x64
         |         expert sleepers                  |
    20 - |           disting NT                     |
         | v1.0.0                                   |
    30 - |SD|        ◯        ◯        ◯           |    <- Pots
         |┌┐|      17.78    35.56    53.34         |
    40 - |││|                                       |
         |││|                                       |
    50 - |└┘|      ◉               ◉               |    <- Encoders
         |    ○ ○  26.67         44.45      ○ ○    |    <- Buttons
    60 - |   12 17                          54 59  |
         |                                          |
    70 - |   ①②③④                          ❶❷      |    <- I/O
         |   14                             50      |
    80 - |   ⑤⑥⑦⑧                          ❸❹      |
         |                                          |
    90 - |   ⑨⑩⑪⑫                          ❺❻      |
         |                                          |
   100 - |                                          |
         |         expert sleepers                  |
   110 - |           disting NT                     |
         |                                          |
   120 - ●                                        ●    <- Screws
         |-----|-----|-----|-----|-----|-----|-----|
    ```
    
    ## Component Specifications
    
    | Component | Type | Size | Position (x,y) |
    |-----------|------|------|----------------|
    | OLED | Display | 50x12.5mm | 35.56, 15.0 |
    | Pot L | Large Knob | ø12mm | 17.78, 35.0 |
    | Pot C | Large Knob | ø12mm | 35.56, 35.0 |
    | Pot R | Large Knob | ø12mm | 53.34, 35.0 |
    | Encoder L | Medium Knob | ø8mm | 26.67, 52.0 |
    | Encoder R | Medium Knob | ø8mm | 44.45, 52.0 |
    | Button 1-4 | LED Button | ø5mm | See coordinates |
    | Inputs 1-12 | 3.5mm Jack | Standard | 9mm grid |
    | Outputs 1-6 | 3.5mm Jack | Standard | 9mm grid |
    
    ## Validation Checklist
    
    - [ ] Panel width exactly 14HP (71.12mm)
    - [ ] OLED perfectly centered horizontally
    - [ ] All pots aligned on same Y coordinate
    - [ ] Encoders symmetrically placed
    - [ ] Buttons: 2 left, 2 right of encoders
    - [ ] Input jacks: 3 rows × 4 columns
    - [ ] Output jacks: 3 rows × 2 columns
    - [ ] SD card slot graphic on left edge
    - [ ] Cyan text for branding
    - [ ] Lightning bolt logo at top
    ```
  - VALIDATE: File exists and is readable
```

### 12. Create Visual Screenshot Comparison Tool

```
CREATE vcv-plugin/tools/compare_panels.html:
  - IMPLEMENT: Side-by-side comparison tool
    ```html
    <!DOCTYPE html>
    <html>
    <head>
        <title>Disting NT Panel Comparison</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background: #222;
                color: #fff;
                margin: 20px;
            }
            .container {
                display: flex;
                gap: 20px;
                align-items: flex-start;
            }
            .panel {
                border: 2px solid #555;
                padding: 10px;
                background: #333;
            }
            .panel h3 {
                margin-top: 0;
                color: #0ff;
            }
            .overlay {
                position: relative;
                display: inline-block;
            }
            .grid-overlay {
                position: absolute;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                pointer-events: none;
                opacity: 0.3;
            }
            .measurements {
                margin-top: 10px;
                font-size: 12px;
                font-family: monospace;
            }
            .control {
                margin: 10px 0;
            }
            input[type="range"] {
                width: 200px;
            }
            .diff-mode {
                mix-blend-mode: difference;
            }
        </style>
    </head>
    <body>
        <h1>Disting NT Panel Visual Comparison</h1>
        
        <div class="control">
            <label>Grid Overlay: <input type="checkbox" id="gridToggle" checked></label>
            <label>Opacity: <input type="range" id="opacitySlider" min="0" max="100" value="30"></label>
            <label>Difference Mode: <input type="checkbox" id="diffMode"></label>
        </div>
        
        <div class="container">
            <div class="panel">
                <h3>Reference Hardware</h3>
                <div class="overlay">
                    <img src="reference.png" width="356" alt="Reference">
                    <svg class="grid-overlay" viewBox="0 0 71.12 128.5" xmlns="http://www.w3.org/2000/svg">
                        <!-- Grid lines every 10mm -->
                        <defs>
                            <pattern id="grid" width="10" height="10" patternUnits="userSpaceOnUse">
                                <path d="M 10 0 L 0 0 0 10" fill="none" stroke="cyan" stroke-width="0.1"/>
                            </pattern>
                        </defs>
                        <rect width="100%" height="100%" fill="url(#grid)" />
                        
                        <!-- Component markers -->
                        <circle cx="35.56" cy="15" r="2" fill="none" stroke="yellow" stroke-width="0.2"/>
                        <text x="37" y="15" fill="yellow" font-size="2">OLED</text>
                        
                        <!-- Pot markers -->
                        <circle cx="17.78" cy="35" r="1" fill="red" opacity="0.5"/>
                        <circle cx="35.56" cy="35" r="1" fill="red" opacity="0.5"/>
                        <circle cx="53.34" cy="35" r="1" fill="red" opacity="0.5"/>
                        
                        <!-- Input/Output markers -->
                        <g stroke="lime" stroke-width="0.1" fill="none">
                            <rect x="11" y="67" width="33" height="26"/>
                            <text x="12" y="65" fill="lime" font-size="2">INPUTS</text>
                        </g>
                        <g stroke="orange" stroke-width="0.1" fill="none">
                            <rect x="47" y="67" width="15" height="26"/>
                            <text x="48" y="65" fill="orange" font-size="2">OUTS</text>
                        </g>
                    </svg>
                </div>
                <div class="measurements">
                    <strong>Expected Measurements:</strong><br>
                    Width: 71.12mm (14HP)<br>
                    Height: 128.5mm (3U)<br>
                    OLED: 50×12.5mm @ (35.56, 15)<br>
                    Pots Y: 35.0mm<br>
                    Encoders Y: 52.0mm<br>
                    I/O Start Y: 70.0mm
                </div>
            </div>
            
            <div class="panel">
                <h3>VCV Rack Implementation</h3>
                <div class="overlay">
                    <img src="vcv_screenshot.png" width="356" alt="VCV Implementation" id="vcvImage">
                    <svg class="grid-overlay" viewBox="0 0 71.12 128.5" xmlns="http://www.w3.org/2000/svg">
                        <!-- Same grid overlay -->
                        <rect width="100%" height="100%" fill="url(#grid)" />
                    </svg>
                </div>
                <div class="measurements">
                    <strong>Actual Measurements:</strong><br>
                    <span id="actualMeasurements">
                        (Take screenshot and measure)
                    </span>
                </div>
            </div>
        </div>
        
        <script>
            const gridToggle = document.getElementById('gridToggle');
            const opacitySlider = document.getElementById('opacitySlider');
            const diffMode = document.getElementById('diffMode');
            const vcvImage = document.getElementById('vcvImage');
            const overlays = document.querySelectorAll('.grid-overlay');
            
            gridToggle.addEventListener('change', (e) => {
                overlays.forEach(o => o.style.display = e.target.checked ? 'block' : 'none');
            });
            
            opacitySlider.addEventListener('input', (e) => {
                overlays.forEach(o => o.style.opacity = e.target.value / 100);
            });
            
            diffMode.addEventListener('change', (e) => {
                vcvImage.classList.toggle('diff-mode', e.target.checked);
            });
            
            // Allow drag and drop of screenshots
            vcvImage.addEventListener('dragover', (e) => e.preventDefault());
            vcvImage.addEventListener('drop', (e) => {
                e.preventDefault();
                const file = e.dataTransfer.files[0];
                if (file && file.type.startsWith('image/')) {
                    const reader = new FileReader();
                    reader.onload = (e) => vcvImage.src = e.target.result;
                    reader.readAsDataURL(file);
                }
            });
        </script>
    </body>
    </html>
    ```
  - USAGE: Open in browser, drag VCV screenshot to compare
  - VALIDATE: HTML file renders correctly
```

```
UPDATE vcv-plugin/README.md:
  - ADD: Screenshot of new panel
  - DOCUMENT: Input/output mapping
  - EXPLAIN: Control layout
  - NOTE: Differences from hardware if any
  - VALIDATE: markdown-lint README.md
```

## Validation Checklist

- [ ] Panel dimensions match 14HP width
- [ ] OLED display positioned at top center
- [ ] 3 pots in correct positions with proper sizing
- [ ] 2 encoders positioned below pots
- [ ] 4 buttons (2 left, 2 right) with LED indicators
- [ ] 12 inputs in 3x4 grid with labels 1-12
- [ ] 6 outputs in 3x2 grid with labels 1-6
- [ ] SD card slot graphic on left side
- [ ] "expert sleepers" branding in cyan
- [ ] "disting NT" text properly positioned
- [ ] Lightning bolt logo at top
- [ ] Version number in bottom left
- [ ] All components functional
- [ ] No build warnings or errors
- [ ] Module loads successfully in VCV Rack

## Rollback Strategy

```bash
# If changes break functionality:
git stash  # Save current changes
git checkout HEAD -- vcv-plugin/src/DistingNT.cpp
git checkout HEAD -- vcv-plugin/res/panels/DistingNT.svg
make clean && make install
# Then apply changes incrementally
```

## Performance Notes

- Panel complexity may impact browser preview performance
- Consider using FramebufferWidget for OLED if needed
- Test with multiple instances for CPU impact

Remember: Exact visual matching is critical - use reference image for all positioning decisions.