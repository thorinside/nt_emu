# Architecture Documentation - Disting NT API Emulator

## Executive Summary

The Disting NT API Emulator provides a complete software implementation of the Disting NT hardware API, enabling desktop-based development and testing of NT algorithm plugins. This emulator core serves as the foundation that allows NT plugins to run unchanged in various host environments, including the VCV Rack module.

## Purpose and Goals

### Primary Objective
Provide a faithful emulation of the Disting NT C++ API to enable plugin development without hardware dependencies.

### Design Principles
- **API Fidelity**: 100% compatible with hardware NT API
- **Safety First**: Crash-resistant plugin execution environment
- **Platform Agnostic**: Cross-platform C++ implementation
- **Extensible**: Easy to integrate into different host environments

## Technology Stack

| Component | Technology | Purpose |
|-----------|------------|---------|
| **Core Language** | C++ | Hardware API compatibility |
| **UI Library** | Dear ImGui | Debug interface and visualization |
| **JSON** | nlohmann/json | Configuration and state serialization |
| **MIDI** | RtMidi | MIDI device communication |
| **Logging** | spdlog | Debug and error logging |
| **Build** | CMake/Make | Cross-platform building |

## Architecture Overview

### System Layers

```
┌─────────────────────────────────────────────┐
│         Host Application Layer              │
│    (VCV Rack, Standalone, Test Harness)     │
└─────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│         NT API Emulation Layer              │
│          include/nt_api/*.h                 │
└─────────────────────────────────────────────┘
                     │
      ┌──────────────┼──────────────┐
      ▼              ▼              ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│   Core   │  │   MIDI   │  │ Display  │
│  Engine  │  │  Handler │  │   API    │
└──────────┘  └──────────┘  └──────────┘
      │              │              │
      ▼              ▼              ▼
┌─────────────────────────────────────────────┐
│          Plugin Execution Context           │
│         Safe Plugin Loading & Running        │
└─────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│        NT Algorithm Plugins (.dylib)        │
│         Compiled Against NT API             │
└─────────────────────────────────────────────┘
```

## Core Components

### NT API Implementation (`include/nt_api/`)

The emulator implements the complete Disting NT hardware API:

#### Parameter API
```cpp
// Parameter management
void setParameterValue(uint8_t param, float value);
float getParameterValue(uint8_t param);
void setParameterMin(uint8_t param, float min);
void setParameterMax(uint8_t param, float max);
```

#### Display API
```cpp
// Display operations (128x64 OLED emulation)
void clearDisplay();
void drawPixel(int16_t x, int16_t y, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void drawText(int16_t x, int16_t y, const char* text);
void display(); // Flush to display
```

#### Audio Processing API
```cpp
// DSP context
struct ProcessContext {
    float* audioInputs[4];    // 4 input channels
    float* audioOutputs[4];   // 4 output channels
    int32_t blockSize;        // Sample block size
    float sampleRate;         // Current sample rate
};

void processBlock(const ProcessContext& context);
```

#### MIDI API
```cpp
// MIDI handling
void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
bool getMidiMessage(MidiMessage& msg);
void setMidiChannel(uint8_t channel);
```

### Core Engine (`src/core/`)

#### Plugin Loader
- **Dynamic Loading**: Uses dlopen/LoadLibrary for plugin loading
- **Symbol Resolution**: Validates required plugin entry points
- **Version Checking**: Ensures API compatibility

#### Execution Context
- **Sandboxing**: Isolated execution environment per plugin
- **Resource Management**: Memory and CPU usage tracking
- **Exception Handling**: Catches and recovers from plugin crashes

#### State Management
```cpp
class EmulatorState {
    PluginInstance* currentPlugin;
    ParameterSet parameters;
    DisplayBuffer display;
    MidiQueue midiEvents;
    AudioBuffers audioBuffers;
};
```

### MIDI Handler (`src/core/midi_handler.cpp`)

- **Device Discovery**: Enumerate available MIDI devices
- **Message Routing**: Route MIDI to/from plugins
- **Clock Sync**: MIDI clock generation and following
- **CC Mapping**: Parameter control via MIDI CC

### Display Emulation

#### Frame Buffer Management
```cpp
class DisplayBuffer {
    uint8_t pixels[128 * 64 / 8];  // 1-bit display
    bool dirty;
    Region dirtyRegion;
};
```

#### Rendering Pipeline
1. Plugin calls draw functions
2. Commands accumulated in display buffer
3. Dirty regions tracked for efficiency
4. Host notified of display changes
5. Host renders display buffer

## Plugin System Architecture

### Plugin Lifecycle

```
Load Plugin → Validate API → Initialize → Process Loop → Cleanup
     ↓             ↓            ↓              ↓            ↓
Check Version  Get Symbols  Call Init    Call Process  Call Cleanup
     ↓             ↓            ↓              ↓            ↓
Handle Error   Store Ptrs   Set State    Handle Audio   Free Resources
```

### Plugin Interface

Every NT plugin must export these C functions:

```cpp
extern "C" {
    // Required entry points
    const char* nt_get_plugin_name();
    uint32_t nt_get_plugin_version();
    void nt_init(const NTContext* context);
    void nt_process(const ProcessContext* context);
    void nt_cleanup();

    // Optional entry points
    void nt_parameter_changed(uint8_t param, float value);
    void nt_midi_received(const MidiMessage* msg);
    void nt_save_state(StateBuffer* buffer);
    void nt_load_state(const StateBuffer* buffer);
}
```

### Error Handling Strategy

```cpp
class SafePluginExecutor {
    void executePlugin() {
        try {
            plugin->process(context);
        } catch (const std::exception& e) {
            logError(e.what());
            enterBypassMode();
        } catch (...) {
            logError("Unknown exception");
            enterSafeMode();
        }
    }
};
```

## Test Plugin Infrastructure

### Test Plugins (`test_plugins/examples/`)

#### Parameter Test Plugin
- **File**: `README_PARAMETER_TESTS.md`
- **Purpose**: Validate parameter API implementation
- **Tests**: Range checking, smoothing, modulation

#### Custom UI Test Plugin
- **File**: `test_customui_plugin.cpp`
- **Purpose**: Test display API and custom graphics
- **Tests**: Drawing primitives, text rendering, animations

#### Draw Test Plugin
- **File**: `drawtest_plugin.cpp`
- **Purpose**: Stress test display system
- **Tests**: Complex graphics, performance benchmarks

#### Font Test Plugin
- **File**: `fonttest_plugin.cpp`
- **Purpose**: Validate font rendering system
- **Tests**: Different fonts, sizes, Unicode support

## Integration Patterns

### Host Integration

The emulator can be integrated into different hosts:

```cpp
// Minimal host integration
class NTEmulatorHost {
public:
    void loadPlugin(const std::string& path) {
        emulator.loadPlugin(path);
    }

    void processAudio(float** inputs, float** outputs, int frames) {
        emulator.processBlock(inputs, outputs, frames);
    }

    DisplayBuffer* getDisplay() {
        return emulator.getDisplayBuffer();
    }
};
```

### VCV Rack Integration

The VCV module integrates the emulator:

```cpp
// In NtEmu.cpp
class NtEmu : public Module {
    NTEmulator emulator;

    void process(const ProcessArgs& args) override {
        // Bridge VCV audio to emulator
        float* inputs[4] = { /* map from VCV inputs */ };
        float* outputs[4] = { /* map to VCV outputs */ };

        emulator.processBlock(inputs, outputs, args.frames);

        // Update display widget
        if (emulator.displayDirty()) {
            displayWidget->updateFromBuffer(emulator.getDisplay());
        }
    }
};
```

## Performance Characteristics

### Real-time Constraints

- **Audio Latency**: < 5ms processing overhead
- **Display Updates**: 30-60 FPS depending on complexity
- **Parameter Smoothing**: 1ms update rate
- **MIDI Timing**: Sample-accurate event scheduling

### Resource Usage

- **Memory**: ~10MB base + plugin memory
- **CPU**: < 5% overhead on modern processors
- **Plugin Loading**: < 100ms typical

## Safety and Reliability

### Crash Protection

1. **Exception Isolation**: Each plugin runs in try/catch wrapper
2. **Stack Protection**: Guard pages prevent stack overflow
3. **Timeout Detection**: Watchdog for infinite loops
4. **Memory Bounds**: Checked array access in API

### Recovery Mechanisms

- **Bypass Mode**: Audio passes through on plugin failure
- **Safe Mode**: Minimal functionality after critical error
- **Hot Reload**: Replace crashed plugin without restart
- **State Preservation**: Save state before risky operations

## Development and Debugging

### Debug Features

```cpp
// Enable debug mode
emulator.setDebugLevel(DEBUG_VERBOSE);

// Performance profiling
emulator.enableProfiling(true);
auto stats = emulator.getProfilingStats();

// API call tracing
emulator.setApiTracing(true);
```

### Logging System

- **Levels**: ERROR, WARNING, INFO, DEBUG, TRACE
- **Output**: Console, file, or custom handler
- **Filtering**: Per-component log levels
- **Performance**: Minimal overhead when disabled

## Future Enhancements

### Planned Features

1. **GPU Acceleration**: OpenGL/Metal display rendering
2. **Plugin Marketplace**: Online plugin repository
3. **Remote Debugging**: Network-based debug interface
4. **Plugin Signing**: Cryptographic plugin verification
5. **State Snapshots**: Save/restore complete emulator state

### API Extensions

- **Expanded Display**: Color display support
- **Network API**: Plugin network capabilities
- **File I/O**: Controlled file system access
- **Inter-plugin Communication**: Message passing between plugins

## Conclusion

The Disting NT API Emulator provides a robust, safe, and performant environment for NT plugin development. Its faithful API implementation ensures plugins work identically on hardware and in emulation, while its safety features and debugging capabilities make it an ideal development platform.