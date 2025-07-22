name: "Fix Encoder Functionality in Parameter Menu System"
description: |
  Fix the broken encoder behavior in the VCV Rack DistingNT module's parameter menu. Encoders should work as infinite rotary controls with proper delta detection, matching the hardware behavior where Encoder L duplicates Pot C functionality (parameter selection) and Encoder R duplicates Pot R functionality (value editing).

---

# Task: Fix VCV Rack Encoder Behavior for Parameter Menu

> Restore proper infinite encoder functionality with delta-based navigation and value editing

## Critical Context

```yaml
context:
  docs:
    - url: https://vcvrack.com/docs-v2/rack/app/ParamQuantity
      focus: Understanding how VCV handles infinite encoders vs absolute knobs
    
    - url: https://github.com/VCVRack/Rack/blob/v2/include/engine/Param.hpp
      focus: Param value accumulation and bounds handling
    
    - url: https://github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp
      focus: Example of proper encoder handling in VCV

  patterns:
    - file: https://github.com/bogaudio/BogaudioModules/blob/master/src/AddrSeq.cpp
      copy: Encoder delta calculation pattern
    
    - file: https://github.com/Befaco/BefacoModules/blob/v2/src/EvenVCO.cpp
      copy: Infinite encoder with snap behavior

  gotchas:
    - issue: "VCV accumulates encoder values instead of providing deltas"
      fix: "Track previous value and calculate delta manually"
    
    - issue: "Encoders need different sensitivity than pots"
      fix: "Scale encoder deltas appropriately for menu navigation"
    
    - issue: "Encoder params need infinite range"
      fix: "Configure with large min/max or use configButton for discrete steps"
    
    - issue: "Tooltip shows accumulated value which confuses users"
      fix: "Use custom ParamQuantity to show meaningful display"
```

## Problem Analysis

The current implementation incorrectly treats encoders as absolute position controls instead of relative delta controls. VCV Rack accumulates encoder turns into a value, but we need to:

1. Track the previous value
2. Calculate the delta (change) 
3. Apply the delta to our menu navigation/editing
4. Handle encoder acceleration/sensitivity

## Implementation Tasks

### 1. Debug Current Encoder Behavior

```
READ vcv-plugin/src/DistingNT.cpp:
  - FIND: getEncoderDelta function
  - UNDERSTAND: Current delta calculation
  - NOTE: How encoder params are configured

CREATE vcv-plugin/test/encoder_debug.cpp:
  - IMPLEMENT: Test harness to log encoder behavior
    ```cpp
    #include <iostream>
    #include <rack.hpp>
    
    void debugEncoderBehavior() {
        // Create test module
        DistingNT module;
        
        // Simulate encoder turns
        std::cout << "=== Encoder Debug Test ===" << std::endl;
        
        // Test left encoder
        float leftStart = module.params[DistingNT::ENCODER_L_PARAM].getValue();
        std::cout << "Left encoder start: " << leftStart << std::endl;
        
        // Simulate clockwise turn
        module.params[DistingNT::ENCODER_L_PARAM].setValue(leftStart + 1.0f);
        int delta = module.getEncoderDelta(DistingNT::ENCODER_L_PARAM, module.lastLeftEncoderValue);
        std::cout << "After +1 turn, delta: " << delta << std::endl;
        
        // Simulate counter-clockwise turn  
        module.params[DistingNT::ENCODER_L_PARAM].setValue(leftStart);
        delta = module.getEncoderDelta(DistingNT::ENCODER_L_PARAM, module.lastLeftEncoderValue);
        std::cout << "After -1 turn, delta: " << delta << std::endl;
    }
    ```
  - VALIDATE: Understand exact encoder behavior
```

### 2. Fix Encoder Parameter Configuration

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
  - REPLACE: Encoder configuration
    ```cpp
    // Remove old encoder params that were misconfigured
    // ENCODER_L_PARAM, ENCODER_R_PARAM,
    
    // Add new encoder tracking (not as params but as internal state)
    struct DistingNT : Module {
        // ... existing members ...
        
        // Encoder state tracking
        float encoderLAccumulator = 0.f;
        float encoderRAccumulator = 0.f;
        float lastEncoderLValue = 0.f;
        float lastEncoderRValue = 0.f;
        
        // For discrete stepping
        float encoderLStepAccumulator = 0.f;
        float encoderRStepAccumulator = 0.f;
        const float ENCODER_STEP_THRESHOLD = 0.1f; // Sensitivity
    };
    
    // In constructor
    DistingNT() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        // ... existing param configs ...
        
        // Configure encoders as infinite controls
        configParam(ENCODER_L_PARAM, -INFINITY, INFINITY, 0.f, "Left Encoder");
        configParam(ENCODER_R_PARAM, -INFINITY, INFINITY, 0.f, "Right Encoder");
        
        // Make them snap to center (0) when not turning
        paramQuantities[ENCODER_L_PARAM]->snapEnabled = true;
        paramQuantities[ENCODER_R_PARAM]->snapEnabled = true;
    }
    ```
  - VALIDATE: make && echo "Encoder params reconfigured"
```

### 3. Implement Proper Delta Detection

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - REPLACE: getEncoderDelta with proper implementation
    ```cpp
    // Calculate encoder delta with acceleration support
    int getEncoderDelta(int paramId, float& lastValue, float& stepAccumulator) {
        float currentValue = params[paramId].getValue();
        float delta = currentValue - lastValue;
        lastValue = currentValue;
        
        // Accumulate fractional turns
        stepAccumulator += delta;
        
        // Convert to discrete steps when threshold reached
        int steps = 0;
        if (std::abs(stepAccumulator) >= ENCODER_STEP_THRESHOLD) {
            steps = (int)(stepAccumulator / ENCODER_STEP_THRESHOLD);
            stepAccumulator -= steps * ENCODER_STEP_THRESHOLD;
        }
        
        // Reset encoder to center after reading
        if (delta != 0.f) {
            params[paramId].setValue(0.f);
        }
        
        return steps;
    }
    
    // Simplified delta for testing
    int getEncoderDeltaSimple(int paramId) {
        float value = params[paramId].getValue();
        int delta = 0;
        
        if (value > 0.5f) {
            delta = 1;
        } else if (value < -0.5f) {
            delta = -1;
        }
        
        // Reset to center
        if (delta != 0) {
            params[paramId].setValue(0.f);
        }
        
        return delta;
    }
    ```
  - VALIDATE: Encoder deltas detected correctly
```

### 4. Update Menu Navigation Logic

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: processMenuNavigation
  - REPLACE: Encoder handling section
    ```cpp
    void processMenuNavigation() {
        // Get current page info
        if (currentPageIndex >= parameterPages.size()) return;
        const _NT_parameterPage& page = parameterPages[currentPageIndex];
        
        // Left pot OR Left encoder: Parameter selection within page
        if (menuMode >= MENU_PARAM_SELECT) {
            // Pot C - absolute positioning
            float potCValue = params[POT_C_PARAM].getValue();
            int potPosition = (int)(potCValue * (page.numParams - 1));
            
            // Encoder L - relative movement
            int encoderLDelta = getEncoderDelta(ENCODER_L_PARAM, lastEncoderLValue, encoderLStepAccumulator);
            
            // Combine both inputs
            if (encoderLDelta != 0) {
                // Encoder takes precedence when turned
                currentParamIndex = clamp(currentParamIndex + encoderLDelta, 0, (int)page.numParams - 1);
            } else if (std::abs(potPosition - currentParamIndex) > 0) {
                // Pot movement detected
                currentParamIndex = potPosition;
            }
        }
        
        // Right pot OR Right encoder: Value editing
        if (menuMode == MENU_VALUE_EDIT) {
            int paramIdx = getCurrentParameterIndex();
            if (paramIdx < 0) return;
            
            const _NT_parameter& param = parameters[paramIdx];
            
            // Pot R - absolute positioning
            float potRValue = params[POT_R_PARAM].getValue();
            int potValue = (int)(potRValue * (param.max - param.min) + param.min);
            
            // Encoder R - relative movement
            int encoderRDelta = getEncoderDelta(ENCODER_R_PARAM, lastEncoderRValue, encoderRStepAccumulator);
            
            // Apply changes
            if (encoderRDelta != 0) {
                // Encoder takes precedence
                parameterEditValue = clamp(parameterEditValue + encoderRDelta, (int)param.min, (int)param.max);
            } else if (std::abs(potValue - parameterEditValue) > 1) {
                // Pot movement detected (with deadzone)
                parameterEditValue = potValue;
            }
        }
        
        // Mode transitions remain the same
        // Left encoder press: Enter/exit menu
        if (params[ENCODER_L_PRESS_PARAM].getValue() > 0 && !leftEncoderPressed) {
            leftEncoderPressed = true;
            toggleMenuMode();
        } else if (params[ENCODER_L_PRESS_PARAM].getValue() == 0) {
            leftEncoderPressed = false;
        }
        
        // Right encoder press: Confirm value
        if (params[ENCODER_R_PRESS_PARAM].getValue() > 0 && !rightEncoderPressed) {
            rightEncoderPressed = true;
            if (menuMode == MENU_VALUE_EDIT) {
                confirmParameterValue();
                menuMode = MENU_PARAM_SELECT;
            } else if (menuMode == MENU_PARAM_SELECT) {
                // Enter edit mode
                menuMode = MENU_VALUE_EDIT;
                parameterEditValue = getCurrentParameterValue();
            }
        } else if (params[ENCODER_R_PRESS_PARAM].getValue() == 0) {
            rightEncoderPressed = false;
        }
    }
    ```
  - VALIDATE: Both pots and encoders control navigation
```

### 5. Add Custom ParamQuantity for Better Display

```
CREATE vcv-plugin/src/EncoderParamQuantity.hpp:
  - IMPLEMENT: Custom quantity for encoder display
    ```cpp
    struct EncoderParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            return "Turn";  // Don't show accumulated value
        }
        
        std::string getString() override {
            return getParamInfo()->label;
        }
    };
    ```

UPDATE vcv-plugin/src/DistingNT.cpp:
  - AFTER: configParam for encoders
  - ADD: Custom quantity assignment
    ```cpp
    // Replace default quantities with custom ones
    delete paramQuantities[ENCODER_L_PARAM];
    delete paramQuantities[ENCODER_R_PARAM];
    
    paramQuantities[ENCODER_L_PARAM] = new EncoderParamQuantity();
    paramQuantities[ENCODER_L_PARAM]->module = this;
    paramQuantities[ENCODER_L_PARAM]->paramId = ENCODER_L_PARAM;
    
    paramQuantities[ENCODER_R_PARAM] = new EncoderParamQuantity();
    paramQuantities[ENCODER_R_PARAM]->module = this;
    paramQuantities[ENCODER_R_PARAM]->paramId = ENCODER_R_PARAM;
    ```
  - VALIDATE: Tooltips show "Turn" instead of value
```

### 6. Remove Help Text from Display

```
UPDATE vcv-plugin/src/display/OLEDWidget.cpp:
  - FIND: Draw navigation hints at bottom
  - REMOVE: Help text section
    ```cpp
    // Remove this entire section:
    /*
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
    */
    ```
  - VALIDATE: Display shows only parameter info
```

### 7. Comprehensive User Testing

```
CHECKPOINT user_test_1:
  - BUILD: make clean && make install
  - RUN: Rack -d
  - LOAD: Plugin with multiple parameter pages
  - TEST: Navigation with Pot C
    - [ ] Turn pot C to select different parameters
    - [ ] Selection follows pot position smoothly
    - [ ] All parameters reachable
  - TEST: Navigation with Encoder L
    - [ ] Turn encoder L clockwise - selection moves down
    - [ ] Turn encoder L counter-clockwise - selection moves up  
    - [ ] Fast turning moves multiple steps
    - [ ] Stops at first/last parameter
  - VERIFY: Both controls work independently

CHECKPOINT user_test_2:
  - TEST: Value editing with Pot R
    - [ ] Enter edit mode (right encoder press)
    - [ ] Turn pot R to change value
    - [ ] Value follows pot position
    - [ ] Full range accessible
  - TEST: Value editing with Encoder R
    - [ ] Turn encoder R to increment/decrement
    - [ ] Small turns = single steps
    - [ ] Fast turns = larger steps
    - [ ] Respects min/max bounds
  - VERIFY: Both controls edit values correctly

CHECKPOINT user_test_3:
  - TEST: Mode transitions
    - [ ] Left encoder press enters menu
    - [ ] Navigate to parameter with encoder L
    - [ ] Right encoder press enters edit mode
    - [ ] Edit value with encoder R
    - [ ] Right encoder press confirms value
    - [ ] Left encoder press exits menu
  - VERIFY: Complete workflow functions

CHECKPOINT user_test_4:
  - TEST: Edge cases
    - [ ] Rapid encoder turning
    - [ ] Switching between pot and encoder
    - [ ] Menu exit during editing
    - [ ] Parameter bounds checking
    - [ ] Empty parameter pages
  - VERIFY: No crashes or stuck states

CHECKPOINT user_test_5:
  - TEST: User experience
    - [ ] Navigation feels responsive
    - [ ] No lag or stuttering
    - [ ] Value changes are smooth
    - [ ] Display updates immediately
    - [ ] Encoder sensitivity appropriate
  - DOCUMENT: Any UX issues found
```

### 8. Performance Optimization

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - ADD: Optimizations if needed
    ```cpp
    // Only process menu navigation when menu is active
    void process(const ProcessArgs& args) override {
        // Skip menu processing if not in menu
        if (menuMode == MENU_OFF) {
            // Just check for menu entry
            if (params[ENCODER_L_PRESS_PARAM].getValue() > 0 && !leftEncoderPressed) {
                leftEncoderPressed = true;
                toggleMenuMode();
            } else if (params[ENCODER_L_PRESS_PARAM].getValue() == 0) {
                leftEncoderPressed = false;
            }
        } else {
            // Full menu processing
            processMenuNavigation();
        }
        
        // ... rest of audio processing ...
    }
    ```
  - VALIDATE: CPU usage acceptable
```

## Validation Checklist

- [ ] Encoder L navigates parameters (same as Pot C)
- [ ] Encoder R edits values (same as Pot R)  
- [ ] Encoders work as infinite controls (no stops)
- [ ] Delta detection is accurate and responsive
- [ ] Fast turning produces appropriate acceleration
- [ ] Encoder press buttons still function
- [ ] No accumulated value shown in tooltips
- [ ] Both encoder and pot can be used interchangeably
- [ ] No help text cluttering the display
- [ ] Menu operation matches hardware exactly
- [ ] Performance is not impacted
- [ ] All parameter types work correctly

## Rollback Strategy

```bash
# If encoder changes break functionality:
git stash
git checkout HEAD -- vcv-plugin/src/DistingNT.cpp
git checkout HEAD -- vcv-plugin/src/display/OLEDWidget.cpp
make clean && make install

# Test with previous working version
# Then apply fixes incrementally
```

## User Testing Script

```markdown
## Disting NT Menu System Test Protocol

### Setup
1. Load DistingNT module in VCV Rack
2. Load a plugin with multiple parameter pages
3. Connect audio inputs/outputs for testing

### Test 1: Basic Navigation
1. Press left encoder to enter menu
2. Turn POT C slowly - verify parameter selection follows
3. Turn ENCODER L slowly - verify same behavior as pot
4. Try both controls in same session

### Test 2: Value Editing  
1. Select a parameter
2. Press right encoder to edit
3. Turn POT R to change value
4. Exit and re-enter edit mode
5. Turn ENCODER R to change value
6. Verify both show same value changes

### Test 3: Rapid Navigation
1. Turn encoder L quickly
2. Verify multiple parameters are skipped
3. Turn encoder R quickly while editing
4. Verify value changes appropriately

### Test 4: Edge Cases
1. Try to navigate past first/last parameter
2. Try to edit value past min/max
3. Exit menu while editing
4. Switch between pot and encoder rapidly

### Results
- [ ] All navigation works as expected
- [ ] No visual glitches
- [ ] No crashes or hangs
- [ ] Feels responsive and natural
```

This comprehensive fix addresses the encoder issues while maintaining compatibility with the pot controls, giving users the full Disting NT experience in VCV Rack.