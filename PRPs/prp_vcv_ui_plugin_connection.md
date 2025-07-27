# PRP: Connect VCV UI Controls to Hosted Plugin with Soft Takeover

## Goal
Enable VCV Rack UI controls (knobs, buttons, encoders) to properly communicate with hosted NT plugins, implementing soft takeover to prevent parameter jumps when controls don't match current values.

## Why
Currently, the VCV module tracks hardware state but doesn't connect it to the plugin's custom UI system. This prevents plugins from receiving control updates and causes jarring parameter jumps when switching between plugins or entering/exiting menu mode.

## What
### User-Visible Behavior
- Knobs, buttons, and encoders control plugin parameters smoothly
- Soft takeover prevents parameter jumps when physical controls don't match values
- Encoders provide incremental control with proper delta values
- Plugin custom UI behaviors are respected (e.g., specific knob assignments)

### Technical Requirements
- Implement `setupUi()` calls for parameter synchronization
- Route control changes through plugin's `customUi()` when available
- Calculate and pass encoder deltas correctly
- Maintain parameter value consistency across UI modes

## All Needed Context

### NT API Documentation
```yaml
context:
  api_reference:
    - file: emulator/include/distingnt/api.h
      functions:
        - setupUi: "Called when algorithm UI appears, writes current param values (0-1) to pots array"
        - customUi: "Handles real-time control input when hasCustomUi() returns non-zero"
        - hasCustomUi: "Returns bitmask of controls plugin wants to handle"
        - NT_setParameterFromUi: "Thread-safe parameter update from UI"
        
  parameter_system:
    - Values stored as int16_t in algorithm->v[]
    - Scaling factors: x1, x10, x100, x1000
    - Units: percent, Hz, dB, volts, ms, etc.
    - Normalization: Pots provide 0.0-1.0, params have custom ranges
```

### Example Plugin Patterns
```cpp
// From gainCustomUI.cpp - Soft takeover implementation
void setupUi(_NT_algorithm* self, _NT_float3& pots) {
    pots[1] = self->v[kParamGain] * 0.01f;  // Convert 0-100 to 0.0-1.0
}

uint32_t hasCustomUi(_NT_algorithm* self) {
    return kNT_potC;  // Use center pot
}

void customUi(_NT_algorithm* self, const _NT_uiData& data) {
    if (data.controls & kNT_potC) {
        int value = round(100.0f * data.pots[1]);
        NT_setParameterFromUi(NT_algorithmIndex(self), 
                              kParamGain + NT_parameterOffset(), value);
    }
}
```

### Current VCV Implementation
```yaml
issues:
  - Encoder deltas always set to 0
  - No setupUi/customUi calls
  - Direct parameter setting without soft takeover
  - Hardware state not connected to plugin UI system
  
existing_code:
  - file: vcv-plugin/src/DistingNT.cpp
    key_areas:
      - process(): Main update loop
      - processControls(): Hardware state update
      - setParameterValue(): Direct parameter updates
      - routingMatrix: Parameter value storage
```

### Gotchas & Pitfalls
- **Thread Safety**: Use NT_setParameterFromUi() not NT_setParameterFromAudio()
- **Parameter Offset**: Always add NT_parameterOffset() to parameter indices
- **Encoder Deltas**: Must calculate from position changes, not absolute values
- **UI Data Structure**: Must populate controls bitmask to indicate changes
- **Soft Takeover**: Must call setupUi() when switching to plugin UI

## Implementation Blueprint

### Phase 1: Add Plugin UI Support Detection
```cpp
// In DistingNT class, add:
bool hasPluginCustomUI() {
    return pluginFactory && pluginFactory->hasCustomUi && 
           pluginFactory->hasCustomUi(pluginAlgorithm) != 0;
}

uint32_t getPluginUIControls() {
    if (hasPluginCustomUI()) {
        return pluginFactory->hasCustomUi(pluginAlgorithm);
    }
    return 0;
}
```

### Phase 2: Implement Soft Takeover
```cpp
// Add to DistingNT class:
struct SoftTakeoverState {
    bool active[3] = {false, false, false};  // For 3 pots
    float targetValue[3] = {0.5f, 0.5f, 0.5f};
    float threshold = 0.05f;  // 5% catch threshold
    
    bool checkCatch(int pot, float currentValue) {
        if (!active[pot]) return true;
        if (fabs(currentValue - targetValue[pot]) < threshold) {
            active[pot] = false;  // Caught up
            return true;
        }
        return false;
    }
    
    void setTarget(int pot, float value) {
        targetValue[pot] = value;
        active[pot] = true;
    }
};

SoftTakeoverState softTakeover;

// Call when entering plugin UI mode:
void initializePluginUI() {
    if (pluginFactory && pluginFactory->setupUi) {
        _NT_float3 potValues = {0.5f, 0.5f, 0.5f};
        pluginFactory->setupUi(pluginAlgorithm, potValues);
        
        // Set soft takeover targets
        for (int i = 0; i < 3; i++) {
            softTakeover.setTarget(i, potValues[i]);
        }
    }
}
```

### Phase 3: Route Controls Through Plugin UI
```cpp
// In process() method:
void processPluginUI() {
    if (!hasPluginCustomUI()) return;
    
    // Build UI data structure
    _NT_uiData uiData;
    uiData.controls = 0;
    
    // Check pot changes with soft takeover
    for (int i = 0; i < 3; i++) {
        float potValue = params[POT_L_PARAM + i].getValue();
        if (softTakeover.checkCatch(i, potValue)) {
            uiData.pots[i] = potValue;
            if (potValue != lastPotValues[i]) {
                uiData.controls |= (kNT_potL << i);
                lastPotValues[i] = potValue;
            }
        } else {
            uiData.pots[i] = softTakeover.targetValue[i];
        }
    }
    
    // Handle buttons
    for (int i = 0; i < 4; i++) {
        bool pressed = buttonTriggers[i].process(params[BUTTON_1_PARAM + i].getValue());
        if (pressed) {
            uiData.controls |= (kNT_button1 << i);
        }
    }
    uiData.lastButtons = lastButtonState;
    lastButtonState = currentButtonState;
    
    // Calculate encoder deltas
    for (int i = 0; i < 2; i++) {
        float currentPos = params[ENCODER_L_PARAM + i].getValue();
        float delta = currentPos - lastEncoderPos[i];
        
        // Handle infinite encoder wrap
        if (fabs(delta) > 0.5f) {
            delta = 0;  // Ignore large jumps
        }
        
        uiData.encoders[i] = (int)round(delta * encoderSensitivity);
        if (uiData.encoders[i] != 0) {
            uiData.controls |= (kNT_encoderL << i);
        }
        lastEncoderPos[i] = currentPos;
    }
    
    // Call plugin's custom UI
    if (uiData.controls && pluginFactory->customUi) {
        pluginFactory->customUi(pluginAlgorithm, uiData);
    }
}
```

### Phase 4: Integrate with Existing System
```cpp
// Modify process() method:
void process(const ProcessArgs& args) override {
    // ... existing code ...
    
    // Determine UI mode
    bool usePluginUI = !inMenuMode && hasPluginCustomUI();
    
    if (usePluginUI) {
        // Check if just entered plugin UI mode
        if (!wasUsingPluginUI) {
            initializePluginUI();
        }
        processPluginUI();
    } else {
        // Existing menu/parameter handling
        processMenuNavigation(args.sampleTime);
    }
    
    wasUsingPluginUI = usePluginUI;
    
    // ... rest of existing code ...
}
```

### Task List
1. **Add UI detection methods** to check if plugin has custom UI
2. **Implement SoftTakeoverState** class for smooth parameter transitions
3. **Add initializePluginUI()** to call setupUi when entering plugin mode
4. **Create processPluginUI()** to handle control routing
5. **Calculate encoder deltas** from position changes
6. **Build _NT_uiData** structure with proper control flags
7. **Integrate with process()** to switch between menu and plugin UI
8. **Test with example plugins** (gainCustomUI, etc.)
9. **Add debug logging** for UI state transitions
10. **Update hardware state** to pass encoder deltas to emulator

## Validation Loop

### Level 1: Syntax and Build
```bash
cd vcv-plugin
make clean
make
# Should compile without errors
```

### Level 2: Unit Testing
```bash
# Test with gain plugin that has custom UI
cd emulator
./build/emulator test_plugins/examples/libgainCustomUI.so

# In another terminal, monitor parameter changes
tail -f vcv-plugin/log.txt | grep -E "(setupUi|customUi|parameter)"
```

### Level 3: Integration Testing
```bash
# 1. Build and install VCV plugin
cd vcv-plugin
make install

# 2. Launch VCV Rack
# 3. Add DistingNT module
# 4. Load gainCustomUI plugin
# 5. Test each control:
#    - Center pot should control gain (0-100%)
#    - Soft takeover should prevent jumps
#    - Encoders should provide incremental control
#    - Buttons should trigger parameter changes

# Expected behavior:
# - No parameter jumps when entering plugin UI
# - Smooth control response after soft takeover
# - Plugin receives all control changes
```

### Level 4: Advanced Validation
```cpp
// Add debug output to verify UI flow
void processPluginUI() {
    DEBUG("Plugin UI active, controls: %x", uiData.controls);
    // ... implementation ...
}

// Create test plugin that logs all UI calls
// Verify setupUi called once per UI activation
// Verify customUi called for each control change
// Verify parameter values match expected ranges
```

## Success Criteria
- ✓ Plugin custom UI receives control updates
- ✓ Soft takeover prevents parameter jumps
- ✓ Encoders provide proper delta values
- ✓ All controls respect plugin's hasCustomUi mask
- ✓ Smooth transition between menu and plugin UI modes
- ✓ Parameter values remain consistent
- ✓ Thread-safe parameter updates
- ✓ No memory leaks or crashes

## Implementation Notes
- Consider adding visual feedback for soft takeover state
- Future enhancement: parameter smoothing for continuous changes
- May want to add preference for soft takeover threshold
- Consider caching hasCustomUi result for performance

## Confidence Score: 8/10
High confidence due to:
- Clear API documentation and examples
- Existing infrastructure in place
- Well-defined patterns to follow
- Straightforward integration path

Minor uncertainties:
- Exact encoder delta calculation may need tuning
- Soft takeover threshold might need adjustment