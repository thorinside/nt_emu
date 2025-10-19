# Architecture Documentation - VCV Rack NT Emulator Module

## Executive Summary

The NT_EMU VCV Rack module is a sophisticated hardware emulation environment that implements the Disting NT C++ API within the VCV Rack modular synthesis platform. This allows developers to test and debug Disting NT plugins on a desktop computer without requiring the physical hardware device. The module provides a complete emulation of the NT's display, controls, parameter system, and DSP infrastructure.

## Purpose and Goals

### Primary Objective
Enable desktop-based development and testing of Disting NT algorithm plugins by providing a fully-featured emulation environment within VCV Rack.

### Key Benefits
- **Rapid Development**: Test plugins instantly without hardware upload cycles
- **Debug Capabilities**: Use desktop debugging tools and logging
- **Visual Feedback**: See real-time display output and parameter changes
- **Integration Testing**: Test plugins alongside other VCV modules
- **Cross-platform**: Develop on macOS, Linux, or Windows

## Technology Stack

| Component | Technology | Version | Purpose |
|-----------|------------|---------|---------|
| **Core Language** | C++ | C++11 | VCV Rack compatibility requirement |
| **Audio Platform** | VCV Rack SDK | 2.0 | Modular synthesis environment |
| **Plugin Format** | VCV Rack Module | 2.0.0 | Desktop audio plugin |
| **Build System** | GNU Make | Standard | VCV Rack standard build |
| **UI Framework** | VCV Rack Widgets | 2.0 | Native VCV UI components |
| **JSON Library** | nlohmann/json | 3.x | Parameter serialization |
| **MIDI Support** | RtMidi | Included | MIDI I/O processing |
| **Display Emulation** | Custom Renderer | - | OLED display simulation |

## Architecture Pattern

### Modular Component Architecture

The system follows a **clean modular architecture** with clear separation of concerns:

```
┌─────────────────────────────────────────────┐
│           VCV Rack Host Environment         │
└─────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│             NtEmu Main Module               │
│         (Coordination & Integration)         │
└─────────────────────────────────────────────┘
                     │
      ┌──────────────┼──────────────┐
      ▼              ▼              ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│  Plugin  │  │Parameter │  │   Menu   │
│ Manager  │  │  System  │  │  System  │
└──────────┘  └──────────┘  └──────────┘
      │              │              │
      ▼              ▼              ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│   MIDI   │  │ Display  │  │   DSP    │
│Processor │  │ Renderer │  │   Bus    │
└──────────┘  └──────────┘  └──────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│          NT API Emulation Layer             │
│    (Disting NT C++ API Implementation)      │
└─────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│        Disting NT Algorithm Plugins         │
│         (.dylib/.so/.dll plugins)           │
└─────────────────────────────────────────────┘
```

## Component Architecture

### Core Components

#### 1. NtEmu Main Module (`NtEmu.cpp` - 2089 lines)
- **Role**: Central coordinator and VCV Rack interface
- **Responsibilities**:
  - Module initialization and lifecycle management
  - Component orchestration via dependency injection
  - VCV Rack API integration
  - Audio/CV processing pipeline coordination

#### 2. Plugin Management System (`plugin/` - 465 lines)
- **PluginManager**: Dynamic loading of NT algorithm plugins
- **PluginExecutor**: Safe execution with exception handling
- **Features**:
  - Hot-reload capability
  - Crash protection
  - Plugin discovery and validation

#### 3. Parameter System (`parameter/` - 450 lines)
- **ParameterSystem**: Complete parameter routing matrix
- **Features**:
  - 8 knob parameters with ranges
  - CV modulation support
  - Parameter smoothing and scaling
  - Preset management

#### 4. Menu Navigation (`menu/` - 380 lines)
- **MenuSystem**: State machine for menu navigation
- **Emulates**: NT's encoder-based menu system
- **Features**:
  - Hierarchical menu structure
  - Context-sensitive options
  - Algorithm selection interface

#### 5. MIDI Processing (`midi/` - 220 lines)
- **MidiProcessor**: MIDI I/O handling
- **Features**:
  - MIDI input/output routing
  - Activity indicators
  - Clock synchronization
  - CC mapping

#### 6. Display Rendering (`display/` - 490 lines)
- **DisplayRenderer**: OLED display emulation
- **IDisplayDataProvider**: Clean interface pattern
- **Features**:
  - 128x64 pixel display simulation
  - Text and graphics rendering
  - Real-time updates

#### 7. DSP Bus System (`dsp/`)
- **BusSystem**: Audio routing infrastructure
- **Features**:
  - 4 input/output audio buses
  - Signal routing matrix
  - Mix level control

#### 8. NT API Wrapper (`api/`)
- **NTApiWrapper**: Bridge to NT hardware API
- **Implements**: Complete Disting NT C++ API
- **Allows**: Unmodified NT plugins to run

## Data Architecture

### Parameter Flow

```
VCV Knobs → Parameter System → NT API → Plugin → DSP Processing → VCV Outputs
     ↑                ↓                    ↓
CV Inputs     Display Updates      MIDI Events
```

### State Management

The module maintains several state systems:

1. **Plugin State**: Current algorithm, parameter values
2. **Menu State**: Navigation position, selection
3. **Display State**: Frame buffer, dirty regions
4. **MIDI State**: Active notes, clock position
5. **Preset State**: Stored parameter snapshots

## API Design

### NT Hardware API Emulation

The module implements the complete Disting NT API:

```cpp
// Core API functions emulated
void nt_set_parameter(int index, float value);
float nt_get_parameter(int index);
void nt_draw_text(int x, int y, const char* text);
void nt_draw_pixel(int x, int y, bool state);
void nt_process_audio(float* inputs, float* outputs, int frames);
```

### Plugin Interface

Plugins interact through standardized entry points:

```cpp
extern "C" {
    void plugin_init();
    void plugin_process(const ProcessContext& context);
    void plugin_parameter_changed(int index, float value);
    void plugin_cleanup();
}
```

## Component Interactions

### Initialization Sequence

1. VCV Rack loads module
2. NtEmu initializes subsystems
3. PluginManager scans for plugins
4. Default plugin loaded
5. Display initialized with welcome screen
6. Parameter values restored from JSON

### Audio Processing Loop

1. VCV Rack calls `process()`
2. Collect input samples
3. Update parameter values
4. Call plugin's process function
5. Route audio through bus system
6. Output to VCV jacks
7. Update display if needed

### Menu Interaction Flow

1. User turns encoder → MenuSystem updates position
2. User presses encoder → MenuSystem selects item
3. Algorithm change → PluginManager loads new plugin
4. Parameter change → ParameterSystem updates routing
5. Display refreshes → DisplayRenderer draws new state

## Source Organization

### Directory Structure

```
vcv-plugin/src/
├── NtEmu.cpp                # Main module
├── plugin/                  # Plugin management
├── parameter/               # Parameter system
├── menu/                    # Menu navigation
├── midi/                    # MIDI processing
├── display/                 # Display rendering
├── api/                     # NT API implementation
├── dsp/                     # Audio routing
├── widgets/                 # Custom UI components
└── algorithms/              # Built-in algorithms
```

### Build Configuration

- **Makefile**: Standard VCV Rack plugin makefile
- **Auto-discovery**: `$(wildcard src/*/*.cpp)` includes all subdirectories
- **Dependencies**: Automatically managed by build system

## Development Workflow

### Adding New Features

1. Create subsystem in appropriate directory
2. Implement interface following existing patterns
3. Wire into NtEmu main module
4. Build and test in VCV Rack

### Testing Strategy

#### Unit Testing
- JSON bridge tests (16 tests)
- Parameter range validation
- Plugin loading verification

#### Integration Testing
- Test plugins: `test_customui_plugin.dylib`
- Display rendering: `drawtest_plugin.dylib`
- Font system: `fonttest_plugin.dylib`

#### Manual Testing
- Load in VCV Rack
- Test with various algorithm plugins
- Verify parameter ranges and CV response
- Check MIDI functionality

## Deployment Architecture

### Distribution

- **Format**: VCV Rack plugin package
- **Platform Builds**:
  - macOS: `.dylib`
  - Linux: `.so`
  - Windows: `.dll`
- **Installation**: Copy to VCV Rack plugins folder

### Resource Management

- **Panel Graphics**: SVG resources in `res/`
- **Fonts**: Shared font system
- **Presets**: JSON-based storage

## Performance Considerations

### Real-time Constraints

- **Audio Thread**: Must complete within sample buffer deadline
- **Display Updates**: Throttled to avoid CPU overhead
- **Plugin Execution**: Exception-safe to prevent crashes

### Optimization Strategies

- **Dirty Flag System**: Only redraw changed display regions
- **Parameter Smoothing**: Prevent zipper noise
- **Efficient Routing**: Pre-calculated routing matrices

## Security & Safety

### Plugin Isolation

- **Exception Handling**: Catches plugin crashes
- **Memory Protection**: Bounded array access
- **Resource Limits**: Prevents infinite loops

### Error Recovery

- **Graceful Degradation**: Falls back to bypass on error
- **Logging**: Detailed error reporting
- **User Notification**: Display shows error states

## Future Enhancements

### Planned Features

1. **Multi-algorithm Chains**: Series/parallel plugin routing
2. **Preset Morphing**: Smooth interpolation between presets
3. **MIDI Learn**: Dynamic CC mapping
4. **Display Themes**: Customizable display colors
5. **Plugin Hot-Reload**: Development mode for rapid iteration

### Architecture Evolution

- Consider extracting emulator core as standalone library
- Implement plugin sandboxing for better isolation
- Add network support for remote plugin development
- Create plugin validation test suite

## Conclusion

The NT_EMU VCV Rack module successfully bridges the gap between hardware and software development, providing a complete emulation environment for Disting NT plugin development. Its modular architecture ensures maintainability and extensibility, while the comprehensive API implementation ensures full compatibility with hardware-targeted plugins.