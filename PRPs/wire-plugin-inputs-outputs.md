# PRP: Wire Plugin Inputs/Outputs and Update Tooltips

## Goal
Enable full audio/CV processing through loaded plugins by wiring inputs/outputs to the plugin step function, updating parameter tooltips dynamically based on plugin properties, and ensuring proper data flow between VCV Rack and the Disting NT emulation layer.

## Why
Currently, plugins are loaded but their inputs/outputs are not connected to the actual audio/CV processing. Without this wiring, plugins cannot process audio or CV signals, making the emulator non-functional for its primary purpose. Dynamic tooltips will provide essential user feedback about parameter names and routing.

## What

### User-Visible Behavior
1. **Audio/CV Processing**: When a plugin is loaded, audio and CV inputs are routed to the plugin, processed, and outputs are sent to the correct output jacks
2. **Dynamic Tooltips**: Hovering over output jacks shows the parameter name (e.g., "Gain Output" instead of generic "Output 1")
3. **Parameter Display**: Encoder tooltips show the currently selected parameter name and value with proper units
4. **Real-time Updates**: All processing happens in real-time at audio rate with proper 4-sample block processing

### Technical Requirements
1. Map plugin input/output parameters to bus indices for routing
2. Update tooltip strings dynamically when parameters change
3. Process audio/CV through plugin step function every 4 samples
4. Convert between VCV Rack (±5V) and Disting NT (±1.0) voltage ranges
5. Handle both replace and add output modes for plugin outputs

## All Needed Context

### Documentation URLs
- **VCV Rack Plugin Development**: https://vcvrack.com/manual/PluginDevelopmentTutorial
- **VCV Voltage Standards**: https://vcvrack.com/manual/VoltageStandards
- **VCV DSP Guide**: https://vcvrack.com/manual/DSP
- **Disting NT Manual**: https://www.expert-sleepers.co.uk/distingNTfirmwareupdates.html

### Key Code Examples

#### Bus System Architecture (vcv-plugin/src/dsp/BusSystem.hpp)
```cpp
class BusSystem {
private:
    alignas(16) float buses[28][4];  // 28 buses, 4 samples each
    int sampleIndex = 0;
    
    // Buses 0-11: Inputs
    // Buses 12-19: Internal
    // Buses 20-25: Outputs
    // Buses 26-27: Additional
```

#### Plugin Step Function Pattern (test_plugins/examples/gain.cpp)
```cpp
void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    int numFrames = numFramesBy4 * 4;
    const float* in = busFrames + (self->v[kParamInput] - 1) * numFrames;
    float* out = busFrames + (self->v[kParamOutput] - 1) * numFrames;
    bool replace = self->v[kParamOutputMode];
    
    for (int i = 0; i < numFrames; ++i) {
        if (replace)
            out[i] = in[i] * gain;
        else
            out[i] += in[i] * gain;
    }
}
```

#### Parameter Types (include/distingnt/api.h)
```cpp
// Input/Output parameter units
#define kNT_unitAudioInput  15
#define kNT_unitCvInput     16
#define kNT_unitAudioOutput 17
#define kNT_unitCvOutput    18
#define kNT_unitOutputMode  19

// Macros for common parameter types
#define NT_PARAMETER_AUDIO_INPUT(NAME, DEF, MIN) \
    {.name = NAME, .min = MIN, .max = 12, .def = DEF, .unit = kNT_unitAudioInput}
```

### Gotchas and Patterns

1. **Bus Indices are 1-based**: Parameters store bus indices 1-28, but array access needs 0-27
2. **4-Sample Block Processing**: The step function processes 4 samples at once for efficiency
3. **Voltage Scaling**: VCV uses ±5V, Disting NT uses ±1.0 normalized range
4. **Output Modes**: Plugins can replace or add to existing bus content
5. **Parameter Access**: Use `algorithm->v[paramIndex]` for current parameter values
6. **Memory Layout**: Bus data is contiguous: [Bus0_S0-S3][Bus1_S0-S3]...[Bus27_S0-S3]

### Existing Patterns to Follow

1. **EncoderParamQuantity** (vcv-plugin/src/EncoderParamQuantity.hpp): Custom ParamQuantity for encoders
2. **BusSystem** routing methods already handle input/output mapping
3. **Parameter extraction** in extractParameterData() already caches parameter info
4. **Menu system** tracks currentParamIndex for context-aware updates

## Implementation Blueprint

### Phase 1: Parameter-Based Routing Setup

```cpp
// In DistingNT.cpp, add method to build routing table
void updateParameterRouting() {
    // Clear existing routing
    paramInputRouting.clear();
    paramOutputRouting.clear();
    
    // Scan parameters for input/output types
    for (size_t i = 0; i < parameters.size(); i++) {
        const _NT_parameter& param = parameters[i];
        
        if (param.unit == kNT_unitAudioInput || param.unit == kNT_unitCvInput) {
            // This parameter controls an input routing
            paramInputRouting[i] = {
                .paramIndex = i,
                .busIndex = pluginAlgorithm->v[i] - 1,  // Convert to 0-based
                .isCV = (param.unit == kNT_unitCvInput)
            };
        }
        else if (param.unit == kNT_unitAudioOutput || param.unit == kNT_unitCvOutput) {
            // This parameter controls an output routing
            paramOutputRouting[i] = {
                .paramIndex = i,
                .busIndex = pluginAlgorithm->v[i] - 1,  // Convert to 0-based
                .isCV = (param.unit == kNT_unitCvOutput),
                .hasOutputMode = false
            };
            
            // Check if next parameter is output mode
            if (i + 1 < parameters.size() && parameters[i + 1].unit == kNT_unitOutputMode) {
                paramOutputRouting[i].hasOutputMode = true;
                paramOutputRouting[i].outputModeParamIndex = i + 1;
            }
        }
    }
}
```

### Phase 2: Dynamic Tooltip System

```cpp
// Create custom ParamQuantity for outputs
struct OutputParamQuantity : ParamQuantity {
    DistingNT* distingModule = nullptr;
    int outputIndex = 0;
    
    std::string getLabel() override {
        if (!distingModule || !distingModule->isPluginLoaded()) {
            return ParamQuantity::getLabel();
        }
        
        // Find parameter that routes to this output
        for (const auto& [paramIdx, routing] : distingModule->paramOutputRouting) {
            int vcvOutputIdx = routing.busIndex - 20;  // Buses 20-25 map to outputs 0-5
            if (vcvOutputIdx == outputIndex) {
                return distingModule->parameters[paramIdx].name;
            }
        }
        
        return ParamQuantity::getLabel();
    }
};

// In constructor, configure outputs with custom quantities
for (int i = 0; i < 6; i++) {
    configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
    
    // Replace with custom quantity after configuration
    auto* pq = new OutputParamQuantity();
    pq->distingModule = this;
    pq->outputIndex = i;
    // Note: This is pseudocode - actual implementation needs proper VCV API usage
}
```

### Phase 3: Wire Audio/CV Processing

```cpp
// Update process() method to route through parameters
void process(const ProcessArgs& args) override {
    // ... existing code ...
    
    if (isPluginLoaded() && pluginAlgorithm) {
        // Route inputs based on parameter mapping
        for (const auto& [paramIdx, routing] : paramInputRouting) {
            int inputJack = routing.busIndex;  // Which VCV input to read
            int destBus = pluginAlgorithm->v[paramIdx] - 1;  // Which bus to write to
            
            if (inputJack >= 0 && inputJack < 12 && inputs[AUDIO_INPUT_1 + inputJack].isConnected()) {
                float voltage = inputs[AUDIO_INPUT_1 + inputJack].getVoltage();
                busSystem.setBus(destBus, sampleIndex, voltage / 5.0f);  // Scale to ±1.0
            }
        }
        
        // Process 4-sample blocks
        if (++sampleCounter >= 4) {
            if (pluginFactory && pluginFactory->step) {
                pluginFactory->step(pluginAlgorithm, busSystem.getBuses(), 1);
            }
            sampleCounter = 0;
        }
        
        // Route outputs based on parameter mapping  
        for (const auto& [paramIdx, routing] : paramOutputRouting) {
            int srcBus = pluginAlgorithm->v[paramIdx] - 1;  // Which bus to read from
            int outputJack = routing.busIndex - 20;  // Buses 20-25 map to outputs 0-5
            
            if (outputJack >= 0 && outputJack < 6) {
                float busValue = busSystem.getBus(srcBus, sampleIndex);
                outputs[AUDIO_OUTPUT_1 + outputJack].setVoltage(busValue * 5.0f);  // Scale to ±5V
            }
        }
        
        // Advance sample index
        busSystem.advanceSample();
    }
}
```

### Phase 4: Update Encoder Tooltips

```cpp
// Extend EncoderParamQuantity to show current parameter
struct ContextAwareEncoderQuantity : EncoderParamQuantity {
    DistingNT* distingModule = nullptr;
    bool isLeftEncoder = true;
    
    std::string getDisplayValueString() override {
        if (!distingModule || distingModule->menuMode == DistingNT::MENU_OFF) {
            return EncoderParamQuantity::getDisplayValueString();
        }
        
        // Show current parameter name and value
        int paramIdx = distingModule->currentParamIndex;
        if (paramIdx >= 0 && paramIdx < distingModule->parameters.size()) {
            const _NT_parameter& param = distingModule->parameters[paramIdx];
            int value = distingModule->pluginAlgorithm->v[paramIdx];
            
            char valueStr[64];
            distingModule->formatParameterValue(valueStr, param, value);
            return std::string(param.name) + ": " + valueStr;
        }
        
        return EncoderParamQuantity::getDisplayValueString();
    }
};
```

## Validation Loop

### Level 1: Syntax and Type Checking
```bash
# Check C++ syntax
cd vcv-plugin
make clean && make

# Run type checking (if using clang-tidy)
clang-tidy src/*.cpp -- -I../Rack-SDK/include -I../Rack-SDK/dep/include
```

### Level 2: Unit Tests
```bash
# Test parameter extraction and routing
cd vcv-plugin
make -f Makefile.test test_integration
./test_integration

# Test with actual plugins
./test_distingnt ./gain.dylib
./test_distingnt ./scaling_demo.dylib
```

### Level 3: Integration Testing
```bash
# Load VCV Rack with the plugin
cd vcv-plugin
make install
# In VCV Rack:
# 1. Add DistingNT module
# 2. Load a test plugin (gain.dylib)
# 3. Connect audio input to Input 1
# 4. Check Output shows "Gain Output" tooltip
# 5. Verify audio passes through with gain applied
```

### Level 4: Audio Validation
```python
# Create validation script to test audio routing
import numpy as np
import soundfile as sf

# Generate test tone
sample_rate = 48000
duration = 1.0
frequency = 440.0
t = np.linspace(0, duration, int(sample_rate * duration))
test_signal = np.sin(2 * np.pi * frequency * t)

# Save as test input
sf.write('test_input.wav', test_signal, sample_rate)

# After processing through VCV:
# 1. Load test_input.wav into VCV sampler
# 2. Route through DistingNT with gain plugin
# 3. Record output
# 4. Verify gain is applied correctly
```

## Checklist

- [ ] Create parameter routing tables for inputs/outputs
- [ ] Implement custom ParamQuantity for dynamic output tooltips  
- [ ] Wire input routing from VCV jacks to plugin buses
- [ ] Implement 4-sample block processing with plugin step function
- [ ] Wire output routing from plugin buses to VCV jacks
- [ ] Add voltage scaling between VCV (±5V) and Disting NT (±1.0)
- [ ] Update encoder tooltips to show current parameter context
- [ ] Handle output modes (replace vs add)
- [ ] Test with multiple plugin types (audio, CV, multi-channel)
- [ ] Verify real-time performance with no audio dropouts
- [ ] Add error handling for invalid bus indices
- [ ] Update display to show routing information

## Success Metrics

1. Audio passes through plugins correctly with processing applied
2. CV signals control plugin parameters as expected  
3. Tooltips update dynamically to show parameter names
4. No audio dropouts or performance issues
5. All test plugins work correctly (gain, scaling_demo, etc.)

**Confidence Score: 9/10**

The research is comprehensive, implementation patterns are clear from existing code, and the validation approach covers all critical aspects. The only uncertainty is around the exact VCV Rack API for replacing ParamQuantity objects after module construction, but this can be worked around if needed.