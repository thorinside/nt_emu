# PRP: Real-Time Parameter Synchronization

## Goal
Ensure parameter changes in the menu system immediately update the audio routing in the VCV module, so when a user changes a routing parameter (e.g., output routing), the audio signal is immediately routed to the new destination without requiring menu exit.

## Why
Currently, when users change routing parameters in the menu, the changes are not applied to the actual audio routing until they exit the menu. This creates confusion as users expect immediate feedback when adjusting parameters, especially for routing which should have an audible effect. The delay violates the principle of immediate response in modular synthesis environments.

## What

### User-Visible Behavior
1. **Immediate Routing Updates**: When changing an output routing parameter from "Output 1" to "Output 2", audio immediately appears at the new output
2. **Real-time Feedback**: All parameter changes take effect instantly during editing
3. **Consistent State**: The displayed parameter value always matches the active routing
4. **No Menu Exit Required**: Changes work without needing to exit the parameter menu

### Technical Changes
1. Update routing maps when routing parameters change
2. Or preferably: Remove intermediate routing maps and read parameters directly
3. Ensure thread-safe parameter access in audio callback
4. Maintain existing plugin notification behavior

## All Needed Context

### Current Implementation Issues

#### Parameter Update Flow (src/DistingNT.cpp)
```cpp
// Current: setParameterValue only updates matrix, not routing
void setParameterValue(int paramIdx, int value) {
    if (paramIdx >= 0 && paramIdx < parameters.size()) {
        const _NT_parameter& param = parameters[paramIdx];
        value = clamp(value, (int)param.min, (int)param.max);
        if (routingMatrix[paramIdx] != value) {
            routingMatrix[paramIdx] = value;
            // Notifies plugin but doesn't update routing maps
            if (pluginFactory && pluginFactory->parameterChanged) {
                pluginFactory->parameterChanged(pluginAlgorithm, paramIdx);
            }
            displayDirty = true;
        }
    }
}

// Routing maps only updated on menu exit
void toggleMenuMode() {
    // ...
    if (menuMode != MENU_OFF) {
        menuMode = MENU_OFF;
        if (pluginAlgorithm) {
            applyRoutingChanges();  // Only here!
        }
    }
}

// Maps store stale bus indices
struct OutputRouting {
    int paramIndex;
    int busIndex;  // This becomes stale when parameter changes!
    bool isCV;
    bool hasOutputMode;
    int outputModeParamIndex;
};
```

#### Audio Routing Implementation
```cpp
void applyCustomOutputRouting() {
    // Uses stale routing map but fresh parameter value
    for (const auto& [paramIdx, routing] : paramOutputRouting) {
        // Gets current value from matrix
        int sourceBus = routingMatrix[paramIdx] - 1;
        // But uses stale routing data to determine output
        // This mismatch causes the sync issue
    }
}
```

### VCV Rack Best Practices

From VCV documentation and community modules:

1. **Thread Safety**: Module methods are mutually exclusive - no locking needed
2. **Immediate Updates**: Routing changes should apply immediately in process()
3. **Direct Parameter Reading**: Best practice is to read parameters directly rather than caching

Example from VCV community:
```cpp
// Good: Direct parameter reading
void process(const ProcessArgs& args) override {
    int route = params[ROUTE_PARAM].getValue();
    // Apply routing immediately based on current value
}

// Avoid: Cached routing state that can go stale
void process(const ProcessArgs& args) override {
    // Using cached_route that might not match parameter
}
```

### Gotchas and Patterns

1. **Parameter Value Storage**: Parameters are stored in `routingMatrix[]` not VCV params
2. **Bus Index Conversion**: Parameters use 1-based indices, internal uses 0-based
3. **Output Detection**: Must parse parameter names to determine VCV output mapping
4. **Performance**: Routing updates happen at audio rate, must be efficient

## Implementation Blueprint

### Option 1: Update Routing Maps on Parameter Change (Quick Fix)

```cpp
void setParameterValue(int paramIdx, int value) {
    if (paramIdx >= 0 && paramIdx < parameters.size()) {
        const _NT_parameter& param = parameters[paramIdx];
        value = clamp(value, (int)param.min, (int)param.max);
        if (routingMatrix[paramIdx] != value) {
            routingMatrix[paramIdx] = value;
            
            // Notify plugin
            if (pluginFactory && pluginFactory->parameterChanged) {
                pluginFactory->parameterChanged(pluginAlgorithm, paramIdx);
            }
            
            // NEW: Update routing if this is a routing parameter
            if (param.unit == kNT_unitAudioInput || param.unit == kNT_unitCvInput ||
                param.unit == kNT_unitAudioOutput || param.unit == kNT_unitCvOutput) {
                updateSpecificRouting(paramIdx);
            }
            
            displayDirty = true;
        }
    }
}

// New method to update just one routing entry
void updateSpecificRouting(int paramIdx) {
    const _NT_parameter& param = parameters[paramIdx];
    
    if (param.unit == kNT_unitAudioInput || param.unit == kNT_unitCvInput) {
        paramInputRouting[paramIdx] = {
            .paramIndex = paramIdx,
            .busIndex = routingMatrix[paramIdx] > 0 ? routingMatrix[paramIdx] - 1 : -1,
            .isCV = (param.unit == kNT_unitCvInput)
        };
    }
    else if (param.unit == kNT_unitAudioOutput || param.unit == kNT_unitCvOutput) {
        paramOutputRouting[paramIdx] = {
            .paramIndex = paramIdx,
            .busIndex = routingMatrix[paramIdx] > 0 ? routingMatrix[paramIdx] - 1 : -1,
            .isCV = (param.unit == kNT_unitCvOutput),
            .hasOutputMode = false,
            .outputModeParamIndex = -1
        };
        
        // Check for output mode parameter
        if (paramIdx + 1 < parameters.size() && 
            parameters[paramIdx + 1].unit == kNT_unitOutputMode) {
            paramOutputRouting[paramIdx].hasOutputMode = true;
            paramOutputRouting[paramIdx].outputModeParamIndex = paramIdx + 1;
        }
    }
}
```

### Option 2: Remove Routing Maps, Read Directly (Better Solution)

```cpp
// Simplify by removing intermediate maps
void applyCustomOutputRouting() {
    int currentSample = busSystem.getCurrentSampleIndex();
    
    // Clear outputs first
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].setVoltage(0.0f);
    }
    
    // Scan parameters directly for output routing
    for (size_t paramIdx = 0; paramIdx < parameters.size(); paramIdx++) {
        const _NT_parameter& param = parameters[paramIdx];
        
        // Skip non-output parameters
        if (param.unit != kNT_unitAudioOutput && param.unit != kNT_unitCvOutput) {
            continue;
        }
        
        // Get current routing value
        int sourceBus = routingMatrix[paramIdx] - 1; // Convert to 0-based
        if (sourceBus < 0 || sourceBus >= 28) continue;
        
        // Determine VCV output from parameter name
        const std::string& paramName = param.name;
        int vcvOutput = -1;
        
        if (sscanf(paramName.c_str(), "Output %d", &vcvOutput) == 1 ||
            sscanf(paramName.c_str(), "Out %d", &vcvOutput) == 1) {
            vcvOutput--; // Convert to 0-based
        } else if (strcmp(paramName.c_str(), "Output") == 0) {
            vcvOutput = 0;
        }
        
        if (vcvOutput >= 0 && vcvOutput < NUM_OUTPUTS) {
            float busValue = busSystem.getBus(sourceBus, currentSample);
            
            // Check for output mode
            bool replace = true;
            if (paramIdx + 1 < parameters.size() && 
                parameters[paramIdx + 1].unit == kNT_unitOutputMode) {
                replace = routingMatrix[paramIdx + 1] > 0;
            }
            
            if (replace) {
                outputs[AUDIO_OUTPUT_1 + vcvOutput].setVoltage(busValue);
            } else {
                float existing = outputs[AUDIO_OUTPUT_1 + vcvOutput].getVoltage();
                outputs[AUDIO_OUTPUT_1 + vcvOutput].setVoltage(existing + busValue);
            }
        }
    }
    
    busSystem.nextSample();
}
```

### Option 3: Hybrid Approach (Recommended)

Keep routing maps for performance but update them immediately:

```cpp
// Add to DistingNT class
bool isRoutingParameter(const _NT_parameter& param) {
    return param.unit == kNT_unitAudioInput || 
           param.unit == kNT_unitCvInput ||
           param.unit == kNT_unitAudioOutput || 
           param.unit == kNT_unitCvOutput ||
           param.unit == kNT_unitOutputMode;
}

// Modified setParameterValue
void setParameterValue(int paramIdx, int value) {
    if (paramIdx >= 0 && paramIdx < parameters.size()) {
        const _NT_parameter& param = parameters[paramIdx];
        value = clamp(value, (int)param.min, (int)param.max);
        if (routingMatrix[paramIdx] != value) {
            routingMatrix[paramIdx] = value;
            
            // Notify plugin
            if (pluginFactory && pluginFactory->parameterChanged) {
                pluginFactory->parameterChanged(pluginAlgorithm, paramIdx);
            }
            
            // Update routing immediately for routing parameters
            if (isRoutingParameter(param)) {
                updateParameterRouting();  // Full update for simplicity
            }
            
            displayDirty = true;
        }
    }
}
```

## Validation Loop

### Level 1: Compilation
```bash
cd vcv-plugin
make clean && make
# Verify no syntax errors
```

### Level 2: Unit Test Script
```bash
# Create test script
cat > test_routing_sync.sh << 'EOF'
#!/bin/bash
echo "Testing real-time parameter sync..."
echo "1. Load nt_emu module"
echo "2. Load gain plugin"
echo "3. Connect input to Input 1"
echo "4. Enter menu (Left encoder press)"
echo "5. Navigate to Output parameter"
echo "6. Change from Output 1 to Output 2"
echo "7. Verify audio immediately appears at Output 2"
echo "8. Change to Output 3"
echo "9. Verify audio immediately moves to Output 3"
echo "10. Exit menu and verify routing persists"
EOF
chmod +x test_routing_sync.sh
```

### Level 3: Manual Testing
```
1. Load VCV Rack with nt_emu module
2. Load a test plugin (gain.dylib)
3. Connect oscillator to Input 1
4. Connect scope to all outputs
5. Enter parameter menu
6. Navigate to output routing parameter
7. Change output - verify immediate routing change
8. Test multiple routing changes without exiting menu
9. Exit menu - verify routing persists
```

### Level 4: Automated Test
```cpp
// Add to test suite
void testRealtimeRouting() {
    // Create module
    DistingNT module;
    
    // Load test plugin
    module.loadPlugin("./gain.dylib");
    
    // Set input routing
    module.setParameterValue(0, 1); // Route to bus 1
    
    // Change output routing
    int outputParam = findOutputParameter(module);
    module.setParameterValue(outputParam, 20); // Bus 20 = Output 1
    
    // Verify routing map updated
    assert(module.paramOutputRouting[outputParam].busIndex == 19);
    
    // Change again
    module.setParameterValue(outputParam, 21); // Bus 21 = Output 2
    assert(module.paramOutputRouting[outputParam].busIndex == 20);
}
```

## Checklist

- [ ] Identify routing parameters by unit type
- [ ] Update `setParameterValue` to detect routing parameters
- [ ] Add immediate routing update for routing parameters
- [ ] Test input routing parameter changes
- [ ] Test output routing parameter changes
- [ ] Test output mode parameter changes
- [ ] Verify no performance impact
- [ ] Ensure thread safety maintained
- [ ] Test with multiple plugins
- [ ] Verify persistence after menu exit
- [ ] Update any related documentation

## Success Metrics

1. Routing changes apply immediately without menu exit
2. No audio glitches during parameter changes
3. Parameter display matches active routing
4. No performance degradation
5. Existing save/load functionality preserved

**Confidence Score: 9/10**

The issue is well-understood and the fix is straightforward. The hybrid approach (Option 3) provides the best balance of performance and simplicity. The main risk is ensuring all routing parameter types are handled correctly.