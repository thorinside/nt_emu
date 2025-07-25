# Adding Outputs to VCV Module Pattern

This guide shows the specific pattern for adding outputs to a VCV Rack module.

## Current Implementation (6 Outputs)

### Module Definition
```cpp
enum OutputIds {
    AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3,
    AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
    NUM_OUTPUTS
};
```

### Configuration
```cpp
// In constructor
config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

// Configure each output
for (int i = 0; i < 6; i++) {
    configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
}
```

### Widget Layout
```cpp
// 6 Outputs (3 rows of 2)
for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 2; col++) {
        int index = row * 2 + col;
        float x = outputStartX + col * jackSpacing;
        float y = inputStartY + row * rowSpacing;
        addOutput(createOutputCentered<PJ301MPort>(
            mm2px(Vec(x, y)), module, AUDIO_OUTPUT_1 + index));
    }
}
```

## Expanding to 8 Outputs

### Required Changes

1. **Update Enum**:
```cpp
enum OutputIds {
    AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3,
    AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
    AUDIO_OUTPUT_7, AUDIO_OUTPUT_8,  // Add these
    NUM_OUTPUTS  // Automatically becomes 8
};
```

2. **Update Configuration Loop**:
```cpp
for (int i = 0; i < 8; i++) {  // Change from 6 to 8
    configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
}
```

3. **Update Widget Layout**:
```cpp
// 8 Outputs (4 rows of 2)
for (int row = 0; row < 4; row++) {  // Change from 3 to 4
    for (int col = 0; col < 2; col++) {
        int index = row * 2 + col;
        // Rest remains the same
    }
}
```

4. **Update SVG Panel**:
   - Add output jack markers for positions 7 and 8
   - Add labels "7" and "8"
   - Adjust bottom spacing if needed

### Bus System Considerations

The hardware has 28 buses total. Current mapping:
- Buses 0-11: Inputs
- Buses 12-19: Internal
- Buses 20-25: Outputs 1-6
- Buses 26-27: Currently unused (perfect for outputs 7-8)

Update routing:
```cpp
// In routeOutputs method
for (int i = 0; i < 8; i++) {  // Change from 6 to 8
    float busValue = getBus(20 + i, currentSample);
    module->outputs[ModuleType::AUDIO_OUTPUT_1 + i].setVoltage(busValue);
}
```

### Testing Outputs 7-8

Create a simple test patch:
1. Load plugin with test algorithm
2. Route test signal to outputs 7-8
3. Verify signal appears at new outputs
4. Check parameter routing if applicable