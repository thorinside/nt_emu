name: "disting NT Emulator PRP"
description: |

## Purpose

A simple, functional disting NT emulator that enables plugin development on macOS without hardware.

## Core Principles

1. **Simplicity First**: Start with minimal viable functionality
2. **Working Code**: Focus on getting basic features operational
3. **Iterative Development**: Add complexity only as needed
4. **Developer Experience**: Make plugin development faster and easier

---

## Goal

Create a simple macOS application that loads and runs disting NT plugins without requiring the hardware.

Key features:
- Load native macOS plugin binaries (.dylib) using the disting NT API
- Process audio at 96kHz through the plugin
- Display plugin graphics on a simulated 256x64 screen
- Provide basic controls (4 pots, 2 buttons, 1 encoder)
- Hot reload plugins when the binary changes

This enables developers to test their plugins quickly before cross-compiling for hardware.

## Why

- The disting NT API is daunting at first, and the round-tripping to the hardware only to find that the hardware
  can't load the plugin is quite annoying.
- This is the initial version of the emulator. We are working towards a minimum viable product here.
- The emulator will give developers a tool that allows more rapid development of Disting NT plugins, quicker turn
  around time, and assurances that the final code will run properly on the device.

## What

A minimal C++ macOS application that:
- Loads .dylib plugins compiled with the disting NT API
- Implements the core NT_* API functions the plugins need
- Routes audio through PortAudio at 96kHz
- Renders the display using Dear ImGui
- Simulates hardware controls with mouse/keyboard
- Watches plugin files and reloads on changes

Focus on getting plugins running, not perfect hardware simulation.

### Success Criteria

- [ ] Disting NT Plugins can be recompiled and run in the emulator
- [ ] The emulator can be connected to MIDI and Audio inputs and outputs
- [ ] The emulator will display the output of the plugin's Draw function
- [ ] The emulator will validate the plugin will run on the real hardware
- [ ] The emulator will hot reload a plugin that has changed on disk
- [ ] The emulator simulates the I/O and Physical knob and buttons interface of the hardware

## All Needed Context

### Documentation & References (list all context needed to implement the feature)

```yaml
# MUST READ - Include these in your context window
- url: https://raw.githubusercontent.com/expertsleepersltd/distingNT_API/refs/heads/main/include/distingnt/api.h
  why: Main C++ API for the Disting NT
- url: https://raw.githubusercontent.com/expertsleepersltd/distingNT_API/refs/heads/main/include/distingnt/serialisation.h
  why: Serialisation API, for saving and restoring JSON from inside the plugin. Can be considered optional.
- url: https://raw.githubusercontent.com/expertsleepersltd/distingNT_API/refs/heads/main/include/distingnt/slot.h
  why: Slot API for accessing data about other running plugins. Can be considered optional.

- url: https://github.com/expertsleepersltd/distingNT_API/tree/main/examples
  why: Patterns to follow, examples, and a makefile example as well.
```

### Current Codebase tree (run `tree` in the root of the project) to get an overview of the codebase

```bash
nt_emu/
├── CLAUDE.md                    # Claude Code project instructions
├── README.md                    # Project overview
├── pyproject.toml              # Python packaging (for PRP framework)
├── .claude/                    # Claude Code configuration
│   ├── settings.local.json     # Tool permissions
│   └── commands/               # 28+ Claude commands
│       ├── development/        # Core dev utilities
│       ├── code-quality/       # Review commands
│       ├── rapid-development/  # Parallel creation tools
│       └── typescript/         # TS-specific commands
├── PRPs/                       # Product Requirement Prompts
│   ├── README.md
│   ├── ai_docs/               # Curated Claude docs
│   ├── scripts/               # PRP execution scripts
│   ├── templates/             # PRP templates
│   ├── nt-emulator-prp.md     # This document
│   └── example-*.md           # Example PRPs
└── claude_md_files/           # Framework-specific examples
    ├── CLAUDE-ASTRO.md
    ├── CLAUDE-JAVA-*.md
    ├── CLAUDE-NODE.md
    ├── CLAUDE-PYTHON-BASIC.md
    └── CLAUDE-REACT.md
```

### Desired Codebase tree with files to be added and responsibility of file

```bash
nt_emu/
├── emulator/                   # Core emulator implementation (NEW)
│   ├── CMakeLists.txt         # Build configuration
│   ├── Makefile               # Alternative build system
│   ├── src/                   # Source code
│   │   ├── main.cpp           # Entry point and UI main loop
│   │   ├── core/              # Core emulator systems
│   │   │   ├── emulator.h/cpp # Main emulator class
│   │   │   ├── plugin_loader.h/cpp # Dynamic library loading (.dylib)
│   │   │   ├── api_shim.h/cpp # disting NT API implementation layer
│   │   │   ├── audio_engine.h/cpp  # 28-bus audio processing
│   │   │   ├── midi_handler.h/cpp  # MIDI I/O management
│   │   │   ├── memory_manager.h/cpp # Memory constraint simulation
│   │   │   ├── timing_validator.h/cpp # Real-time constraint checking
│   │   │   └── performance_monitor.h/cpp # CPU usage and timing analysis
│   │   ├── hardware/          # Hardware simulation
│   │   │   ├── hardware_interface.h/cpp # Physical controls simulation
│   │   │   ├── display.h/cpp  # Display rendering and simulation
│   │   │   └── io_manager.h/cpp # Audio/MIDI routing
│   │   ├── ui/                # User interface
│   │   │   ├── main_window.h/cpp # Main emulator window
│   │   │   ├── plugin_panel.h/cpp # Plugin control panel
│   │   │   ├── audio_panel.h/cpp  # Audio I/O configuration
│   │   │   └── midi_panel.h/cpp   # MIDI I/O configuration
│   │   └── utils/             # Utilities
│   │       ├── file_watcher.h/cpp # Hot reload functionality
│   │       ├── logger.h/cpp   # Logging system
│   │       └── config.h/cpp   # Configuration management
│   ├── include/               # Header files
│   │   └── distingnt/         # Copied disting NT API headers
│   │       ├── api.h          # Main API
│   │       ├── serialisation.h # JSON serialization
│   │       └── slot.h         # Slot management
│   ├── test_plugins/          # Example plugins for testing
│   │   ├── simple_gain/       # Basic gain plugin
│   │   ├── sine_osc/          # Simple oscillator
│   │   └── midi_echo/         # MIDI processing example
│   ├── tests/                 # Unit and integration tests
│   │   ├── test_plugin_loader.cpp
│   │   ├── test_audio_engine.cpp
│   │   ├── test_midi_handler.cpp
│   │   └── integration/       # End-to-end tests
│   ├── docs/                  # Documentation
│   │   ├── API.md             # Emulator API documentation
│   │   ├── BUILDING.md        # Build instructions
│   │   └── PLUGIN_DEV.md      # Plugin development guide
│   └── third_party/           # Dependencies
│       ├── portaudio/         # Cross-platform audio
│       ├── rtmidi/            # MIDI I/O
│       ├── imgui/             # GUI framework
│       ├── json/              # JSON parsing for config/state
│       ├── spdlog/            # High-performance logging
│       └── catch2/            # Unit testing framework
├── [existing PRP framework structure...]
```

### Known Gotchas of our codebase & Library Quirks

```cpp
// CRITICAL: Disting NT API Requirements and Quirks

// PLUGIN LOADING:
// - Plugins MUST export _NT_factory symbol (C linkage, not C++)
// - Factory function signature: _NT_factory* NT_getFactoryPtr()
// - Plugin instances allocated via calculateRequirements() + construct()
// - Memory layout is critical - shared memory vs instance memory

// MEMORY MODEL:
// - Two-stage allocation: static/shared memory + instance memory
// - Memory types: DRAM (general), SRAM (fast), DTC (ultra-fast data), ITC (code)
// - No dynamic allocation after initialization
// - 16-byte alignment required for SIMD operations
// - Placement new used for object construction
// - Host manages all memory allocation, plugin just declares needs

// AUDIO PROCESSING:
// - Audio processing happens in step() function with 4-sample blocks
// - 28 audio busses total (configurable per plugin via requirements)
// - Sample rate is fixed at 96kHz for disting NT hardware
// - All audio is 32-bit float, range typically -1.0 to +1.0

// CROSS-COMPILATION WORKFLOW:
// - Development: Compile plugins as native macOS .dylib with clang/gcc
// - Testing: Run native plugins in emulator with full debugging
// - Deployment: Cross-compile with arm-none-eabi-gcc for Cortex-M7
// - Memory constraints simulated: limited heap/stack matching hardware
// - Performance monitoring to predict hardware behavior

// UI/DISPLAY:
// - Display is 256x64 monochrome OLED (CRITICAL: not 128x64!)
// - Custom drawing functions: NT_drawText(), NT_drawShapeI/F()
// - UI updates happen in draw() callback, separate from audio thread
// - Hardware has 4 pots, 2 buttons, 1 encoder - simulate these precisely
// - Display refresh rate ~30Hz, independent of audio processing
// - Parameter soft takeover required to prevent jumps

// TIMING CONSTRAINTS:
// - step() function MUST complete within audio buffer time
// - No malloc/free in audio thread - pre-allocate everything
// - MIDI events processed immediately, audio buffered

// PORTAUDIO QUIRKS:
// - Requires specific callback patterns for low-latency audio
// - Buffer sizes should match disting NT (likely 64 or 128 samples)
// - Handle audio device changes gracefully

// HOT RELOAD:
// - Watch .so/.dylib files, not source files
// - Validate plugin symbols before loading
// - Gracefully handle plugin crashes during development
```

## Implementation Blueprint

### Data models and structure

Create the core data models to ensure type safety and consistency in the emulator.

```cpp
// Minimal data structures for MVP

struct PluginInstance {
    void* handle;            // dylib handle
    _NT_factory* factory;    // Plugin factory
    _NT_algorithm* algorithm; // Plugin instance
    void* shared_memory;     // Shared memory block
    void* instance_memory;   // Instance memory block
    std::string path;        // For hot reload
    time_t last_modified;    // For hot reload
};

struct HardwareState {
    float pots[4];           // Potentiometer values [0.0, 1.0]
    bool buttons[2];         // Button states
    int encoder_value;       // Encoder position
};

struct DisplayBuffer {
    uint8_t pixels[256 * 64 / 8]; // Monochrome bitmap (1bpp)
    bool dirty;
};

class SimpleEmulator {
private:
    // Core components
    PluginInstance plugin_;
    HardwareState hardware_;
    DisplayBuffer display_;
    float audio_buses_[28][4];  // 28 buses, 4 samples each
    
    // Audio system
    PaStream* audio_stream_;
    
public:
    // Essential methods only
    bool loadPlugin(const std::string& path);
    void processAudio(float* input, float* output, int frames);
    void updateDisplay();
    void setControl(int control, float value);
    
    // API implementation (called by plugins)
    static void NT_drawText(int x, int y, const char* text, int size);
    static void NT_drawShapeI(int shape, int x, int y, int w, int h);
};
```

### Core Tasks (MVP)

```yaml
Task 1: "Basic Setup"
  - Create CMakeLists.txt with PortAudio, ImGui
  - Download disting NT API headers

Task 2: "Plugin Loader"
  - Load .dylib with dlopen
  - Get factory function and validate

Task 3: "API Implementation"  
  - Implement NT_drawText, NT_drawShapeI
  - Basic parameter functions

Task 4: "Audio Engine"
  - PortAudio callback at 96kHz
  - Call plugin step() with 4-sample blocks

Task 5: "Display Window"
  - ImGui window with 256x64 canvas
  - Render display buffer

Task 6: "Controls"
  - 4 sliders for pots
  - 2 buttons, 1 encoder simulation

Task 7: "Hot Reload"
  - Watch plugin file
  - Reload on change

Task 8: "Simple Test Plugin"
  - Create basic gain plugin
  - Test full pipeline
```

### Basic Implementation

```cpp
// Simple plugin loader
bool loadPlugin(const std::string& path) {
    handle = dlopen(path.c_str(), RTLD_LAZY);
    auto getFactory = (FactoryFunc)dlsym(handle, "_NT_getFactoryPtr");
    factory = getFactory();
    
    // Initialize plugin
    auto staticReqs = factory->calculateStaticRequirements();
    shared_memory = malloc(staticReqs.memorySize);
    factory->initialise(shared_memory);
    
    auto reqs = factory->calculateRequirements();
    instance_memory = malloc(reqs.memorySize);
    algorithm = factory->construct(instance_memory);
    return true;
}

// Audio callback
int audioCallback(const void* input, void* output, unsigned long frames,
                 const PaStreamCallbackTimeInfo* timeInfo,
                 PaStreamCallbackFlags statusFlags, void* userData) {
    auto emu = (SimpleEmulator*)userData;
    
    // Process in 4-sample blocks
    for (int i = 0; i < frames; i += 4) {
        emu->plugin_.algorithm->step(emu->audio_buses_, 4);
        // Copy bus 0 to output
        memcpy((float*)output + i, emu->audio_buses_[0], 4 * sizeof(float));
    }
    return paContinue;
}

// Display rendering
void renderDisplay() {
    // Clear and let plugin draw
    memset(display_.pixels, 0, sizeof(display_.pixels));
    plugin_.algorithm->draw();
    
    // Render to ImGui
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 256; x++) {
            int byte = (y * 256 + x) / 8;
            int bit = 7 - (x % 8);
            bool pixel = display_.pixels[byte] & (1 << bit);
            // Draw pixel in ImGui canvas
        }
    }
}
```

## Validation

```bash
# Build emulator
mkdir build && cd build
cmake .. && make

# Build test plugin
cd test_plugin
make

# Run emulator with plugin
./DistingNTEmulator test_plugin/gain.dylib

# Test:
- Plugin loads without errors
- Audio passes through
- Display shows plugin UI
- Controls affect parameters
- File changes trigger reload
```

## Success Criteria

- [ ] Basic plugin loads and runs
- [ ] Audio processes at 96kHz  
- [ ] Display renders correctly
- [ ] Controls work
- [ ] Hot reload functions
