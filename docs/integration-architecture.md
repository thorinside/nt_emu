# Integration Architecture - NT_EMU

## Overview

The NT_EMU project consists of two main components that work together to provide a complete Disting NT plugin development environment. This document describes how these components integrate and communicate.

## Component Integration Map

```
┌──────────────────────────────────────────────────────┐
│                  VCV Rack Host                        │
│                                                       │
│  ┌──────────────────────────────────────────────┐   │
│  │         VCV Rack NT Emulator Module          │   │
│  │              (vcv-plugin/)                   │   │
│  │                                              │   │
│  │  • User Interface (Knobs, Jacks, Display)   │   │
│  │  • Audio/CV Processing                      │   │
│  │  • VCV Rack Integration                     │   │
│  └──────────────────────────────────────────────┘   │
│                         │                             │
│                         │ Uses                        │
│                         ▼                             │
│  ┌──────────────────────────────────────────────┐   │
│  │        Disting NT API Emulator Core          │   │
│  │               (emulator/)                    │   │
│  │                                              │   │
│  │  • NT Hardware API Implementation           │   │
│  │  • Plugin Loading & Execution               │   │
│  │  • Display & Parameter Management           │   │
│  └──────────────────────────────────────────────┘   │
│                         │                             │
│                         │ Loads                       │
│                         ▼                             │
│  ┌──────────────────────────────────────────────┐   │
│  │          Disting NT Plugins                  │   │
│  │         (.dylib/.so/.dll files)             │   │
│  └──────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────┘
```

## Integration Points

### 1. VCV Module ↔ Emulator Core

#### API Headers Integration
- **Location**: `emulator/include/nt_api/`
- **Method**: Direct C++ header inclusion
- **Compile-time**: Headers shared via `-I../emulator/include` flag

```cpp
// In vcv-plugin/src/NtEmu.cpp
#include "nt_api/nt_api.h"
#include "nt_api/nt_display.h"
#include "nt_api/nt_parameters.h"
```

#### Build System Integration
```makefile
# In vcv-plugin/Makefile
FLAGS += -I../emulator/include
```

### 2. Parameter System Integration

#### Data Flow
```
VCV UI Control → VCV Module → Emulator API → NT Plugin
                     ↓              ↓             ↓
                CV Input     Parameter Update  Process Audio
```

#### Parameter Mapping
```cpp
// VCV parameter index → NT parameter index
void NtEmu::paramChange(int vcvParam) {
    float value = params[vcvParam].getValue();
    emulator->setParameterValue(vcvParam, value);
}
```

### 3. Display System Integration

#### Display Pipeline
```
NT Plugin → Display API → Frame Buffer → VCV Widget → Screen
              ↓              ↓              ↓           ↓
          Draw Calls    Pixel Buffer   SVG Overlay  User View
```

#### Display Synchronization
```cpp
// Emulator maintains frame buffer
class DisplayBuffer {
    uint8_t pixels[128][64];
    bool dirty;
};

// VCV polls for updates
if (emulator->isDisplayDirty()) {
    widget->updateDisplay(emulator->getDisplayBuffer());
}
```

### 4. Audio Processing Integration

#### Audio Routing
```
VCV Audio Inputs → Buffer Conversion → NT Process → VCV Audio Outputs
        ↓                ↓                ↓              ↓
   Jack Inputs      Float Arrays      DSP Block     Jack Outputs
```

#### Sample Processing
```cpp
void NtEmu::process(const ProcessArgs& args) {
    // Collect VCV inputs
    float* ntInputs[4];
    for (int i = 0; i < 4; i++) {
        ntInputs[i] = inputs[AUDIO_INPUT + i].getVoltages();
    }

    // Process through emulator
    emulator->processBlock(ntInputs, ntOutputs, args.frames);

    // Send to VCV outputs
    for (int i = 0; i < 4; i++) {
        outputs[AUDIO_OUTPUT + i].setVoltages(ntOutputs[i]);
    }
}
```

### 5. MIDI Integration

#### MIDI Message Flow
```
External MIDI Device → RtMidi → Emulator → NT Plugin
                          ↓         ↓          ↓
                    MIDI Queue  Message Queue  Process
```

#### MIDI Routing
```cpp
// MIDI input processing
void MidiProcessor::processMidi() {
    MidiMessage msg;
    while (midiIn->getMessage(msg)) {
        emulator->sendMidiToPlugin(msg);
    }
}
```

### 6. Plugin Loading Integration

#### Plugin Discovery
```
Plugin Directory → File System Scan → Validation → Loading
        ↓                ↓               ↓           ↓
   .dylib files    Pattern Match    API Check    dlopen
```

#### Plugin Management
```cpp
class PluginManager {
    std::vector<PluginInfo> availablePlugins;
    PluginInstance* currentPlugin;

    void loadPlugin(const std::string& path) {
        // Validate plugin
        if (validatePlugin(path)) {
            // Load into emulator
            currentPlugin = emulator->loadPlugin(path);
        }
    }
};
```

## Communication Protocols

### Internal Communication

#### Method Calls
- **Direction**: VCV → Emulator
- **Type**: Direct C++ method invocation
- **Threading**: Single audio thread

#### Callbacks
- **Direction**: Emulator → VCV
- **Type**: Observer pattern notifications
- **Use Cases**: Display updates, parameter changes

### Data Structures

#### Shared Structures
```cpp
// Shared between components
struct ProcessContext {
    float** audioInputs;
    float** audioOutputs;
    int blockSize;
    float sampleRate;
};

struct ParameterInfo {
    float value;
    float min;
    float max;
    std::string name;
};
```

## State Synchronization

### State Management
```
VCV State (JSON) ↔ Module State ↔ Emulator State ↔ Plugin State
      ↓                ↓               ↓              ↓
  Rack Save      Internal State   Core State    Plugin Memory
```

### Persistence
```cpp
// VCV serialization
json_t* NtEmu::dataToJson() {
    json_t* root = json_object();
    json_object_set_new(root, "plugin",
        json_string(currentPluginPath.c_str()));
    json_object_set_new(root, "parameters",
        parametersToJson());
    return root;
}

void NtEmu::dataFromJson(json_t* root) {
    // Restore plugin and parameters
}
```

## Error Handling Across Boundaries

### Error Propagation
```
Plugin Error → Emulator Catches → VCV Notified → User Display
      ↓             ↓                  ↓             ↓
  Exception    Try/Catch          Error Code    Warning LED
```

### Recovery Strategies
1. **Plugin Crash**: Emulator enters bypass mode
2. **API Error**: Default values returned
3. **Display Error**: Fallback to text display
4. **Audio Error**: Silent output (no noise)

## Performance Considerations

### Threading Model
- **Audio Thread**: Real-time priority, no blocking
- **UI Thread**: Display updates, parameter changes
- **Background Thread**: Plugin loading, preset management

### Buffer Management
```cpp
// Double buffering for display
class DoubleBuffer {
    DisplayBuffer buffers[2];
    int activeBuffer = 0;

    void swap() {
        activeBuffer = 1 - activeBuffer;
    }
};
```

### Optimization Points
1. **Parameter Smoothing**: Prevent zipper noise
2. **Display Throttling**: Max 60 FPS updates
3. **Audio Block Processing**: Process in chunks
4. **Memory Pooling**: Reuse audio buffers

## Testing Integration

### Integration Test Points
1. **Parameter Range**: VCV values → NT values
2. **Audio Levels**: VCV ±5V → NT ±1.0
3. **Display Mapping**: Pixel coordinates
4. **MIDI Timing**: Sample-accurate events
5. **State Persistence**: Save/load cycles

### Test Infrastructure
```bash
# Test plugin loading
test_customui_plugin.dylib

# Test display system
drawtest_plugin.dylib

# Test parameter system
parameter_test_plugin.dylib
```

## Future Integration Enhancements

### Planned Improvements
1. **IPC Communication**: Separate process plugins
2. **Network Integration**: Remote plugin debugging
3. **Multi-instance**: Multiple emulator instances
4. **Cross-plugin Communication**: Message passing
5. **External Control**: OSC/MIDI control surfaces

### API Evolution
- **Version 2.1**: Extended display capabilities
- **Version 3.0**: Network plugin repository
- **Version 4.0**: Cloud-based development

## Summary

The integration between the VCV Rack module and the Disting NT emulator core is designed to be:
- **Transparent**: Plugins run unmodified
- **Efficient**: Minimal overhead
- **Robust**: Error isolation
- **Extensible**: Easy to add features

This architecture enables seamless desktop development of Disting NT plugins while maintaining full compatibility with the hardware platform.