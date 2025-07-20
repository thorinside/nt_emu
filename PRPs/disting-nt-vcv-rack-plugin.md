name: "Disting NT VCV Rack Plugin - Comprehensive Multi-Algorithm Module"
description: |

## Purpose

Pivot the existing Disting NT hardware emulator to a VCV Rack plugin, leveraging VCV's mature audio/MIDI infrastructure while preserving the multi-algorithm capabilities and unique interface of the Disting NT eurorack module.

## Core Principles

1. **Context is King**: All VCV Rack SDK documentation, patterns from similar modules, and existing codebase patterns included
2. **Validation Loops**: Executable build commands, manual testing procedures, and integration tests
3. **Information Dense**: Keywords from VCV Rack ecosystem and Disting NT architecture
4. **Progressive Success**: Start with basic module structure, validate, then add algorithms

---

## Goal

Create a fully functional VCV Rack plugin that emulates the Expert Sleepers Disting NT, including its multi-algorithm architecture, OLED display simulation, hardware controls, and bus-based audio routing system, allowing users to access Disting NT functionality within VCV Rack.

## Why

- **Eliminates I/O complexity**: VCV Rack handles all audio device management, MIDI routing, and sample rate conversion
- **Large user base**: Access to VCV Rack's community of modular synthesis enthusiasts
- **Stable framework**: Mature platform with established patterns for complex modules
- **Cross-platform**: Single codebase works on Windows, macOS, and Linux
- **Integration benefits**: Users can combine Disting NT algorithms with other VCV modules

## What

A VCV Rack module that provides:
- All Disting NT algorithms in a single module
- 256x64 OLED display simulation
- 3 pots, 2 encoders, 4 buttons interface
- 4 audio inputs/outputs + CV inputs/outputs
- Algorithm switching via menu or CV
- Preset management
- Bus-based routing system (internal)

### Success Criteria

- [ ] Module loads in VCV Rack without errors
- [ ] Display renders correctly with algorithm visuals
- [ ] All controls (pots, buttons, encoders) function properly
- [ ] Audio passes through and processes correctly
- [ ] Algorithm switching works seamlessly
- [ ] Presets save and load properly
- [ ] Performance is optimized (low CPU usage)

## All Needed Context

### Documentation & References

```yaml
# MUST READ - Include these in your context window
- url: https://vcvrack.com/manual/PluginDevelopmentTutorial
  why: Official tutorial covering module creation, DSP, and UI

- url: https://vcvrack.com/docs-v2/
  why: Complete API reference for all VCV Rack classes and functions

- url: https://github.com/VCVRack/Rack/blob/v2/include/rack.hpp
  why: Main header file showing all available APIs

- url: https://github.com/bogaudio/BogaudioModules
  why: Example of complex multi-algorithm modules with displays

- url: https://github.com/clone45/voxglitch
  why: File loading and preset management patterns

- file: /emulator/src/core/emulator.h
  why: Existing Disting NT emulator architecture to port

- file: /emulator/src/hardware/display.h
  why: Display rendering logic to adapt for VCV

- file: /emulator/include/distingnt/plugin.h
  why: Disting NT plugin API to understand algorithm interface

- docfile: PRPs/ai_docs/vcv-rack-complex-modules.md
  why: Curated examples of VCV modules with similar complexity
```

### Current Codebase Structure

```bash
nt_emu/
├── emulator/                    # Existing standalone emulator
│   ├── src/
│   │   ├── core/               # Core emulator logic to reuse
│   │   ├── hardware/           # Hardware simulation to adapt
│   │   └── ui/                 # UI components (reference only)
│   ├── include/distingnt/      # Plugin API headers
│   └── test_plugins/           # Example algorithms
├── PRPs/                       # Product requirement prompts
└── CLAUDE.md                   # Project instructions
```

### Desired VCV Plugin Structure

```bash
nt_emu/
├── vcv-plugin/                 # New VCV Rack plugin directory
│   ├── Makefile               # VCV Rack plugin Makefile
│   ├── plugin.json            # Plugin metadata
│   ├── README.md              # Plugin documentation
│   ├── src/
│   │   ├── plugin.cpp         # Plugin initialization
│   │   ├── DistingNT.cpp      # Main module implementation
│   │   ├── algorithms/        # Ported algorithms
│   │   │   ├── Algorithm.hpp  # Base algorithm interface
│   │   │   └── *.cpp          # Individual algorithms
│   │   ├── display/           # OLED display widget
│   │   │   └── OLEDWidget.cpp # Custom display rendering
│   │   └── dsp/               # DSP utilities and bus system
│   │       └── BusSystem.cpp  # 28-bus architecture
│   └── res/                   # Resources
│       ├── panels/            # SVG panel graphics
│       └── fonts/             # Display fonts
```

### Known Gotchas & Library Quirks

```cpp
// CRITICAL: VCV Rack specifics
// 1. Module methods are mutually exclusive - no threading needed
// 2. NULL module check required in ModuleWidget for browser preview
// 3. Sample rate can change - use args.sampleRate not fixed value
// 4. Process audio in small blocks for UI responsiveness
// 5. Polyphonic cables have up to 16 channels - check getChannels()
// 6. Parameter changes are smoothed automatically
// 7. Use ParamQuantity for custom parameter behavior
// 8. Framebuffer widgets improve performance for complex displays
```

## Implementation Blueprint

### Core Architecture

```cpp
// Algorithm interface matching Disting NT
struct IDistingAlgorithm {
    virtual void prepare(float sampleRate) = 0;
    virtual void step(float* buses, int frames) = 0;
    virtual void parameterChanged(int param, float value) = 0;
    virtual void buttonPressed(int button) = 0;
    virtual void drawDisplay(NVGcontext* vg, int width, int height) = 0;
    virtual const char* getName() = 0;
    virtual json_t* dataToJson() { return nullptr; }
    virtual void dataFromJson(json_t* root) {}
};

// Main module structure
struct DistingNT : Module {
    enum ParamId {
        POT_L_PARAM, POT_C_PARAM, POT_R_PARAM,
        BUTTON_1_PARAM, BUTTON_2_PARAM, BUTTON_3_PARAM, BUTTON_4_PARAM,
        ENCODER_L_DEC_PARAM, ENCODER_L_INC_PARAM, ENCODER_L_PRESS_PARAM,
        ENCODER_R_DEC_PARAM, ENCODER_R_INC_PARAM, ENCODER_R_PRESS_PARAM,
        ALGORITHM_PARAM,
        PARAMS_LEN
    };
    
    enum InputId {
        AUDIO_INPUT_1, AUDIO_INPUT_2, AUDIO_INPUT_3, AUDIO_INPUT_4,
        CV_INPUT_1, CV_INPUT_2, CV_INPUT_3, CV_INPUT_4,
        CV_INPUT_5, CV_INPUT_6, CV_INPUT_7, CV_INPUT_8,
        INPUTS_LEN
    };
    
    enum OutputId {
        AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3, AUDIO_OUTPUT_4,
        CV_OUTPUT_1, CV_OUTPUT_2, CV_OUTPUT_3, CV_OUTPUT_4,
        OUTPUTS_LEN
    };
    
    enum LightId {
        BUTTON_1_LIGHT, BUTTON_2_LIGHT, BUTTON_3_LIGHT, BUTTON_4_LIGHT,
        LIGHTS_LEN
    };
    
    // Bus system (28 buses as in original)
    alignas(16) float buses[28][4]; // 4-sample blocks
    int sampleCounter = 0;
    
    // Algorithm management
    std::vector<std::unique_ptr<IDistingAlgorithm>> algorithms;
    int currentAlgorithmIndex = 0;
    IDistingAlgorithm* currentAlgorithm = nullptr;
    
    // Display state
    bool displayDirty = true;
    
    DistingNT() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        // Configure all parameters, inputs, outputs
        loadAlgorithms();
    }
    
    void process(const ProcessArgs& args) override {
        // Accumulate samples for 4-sample processing
        // Route inputs to buses
        // Process algorithm
        // Route buses to outputs
    }
};
```

### List of tasks to be completed

```yaml
Task 1:
CREATE vcv-plugin/Makefile:
  - COPY standard VCV plugin Makefile structure
  - SET SLUG = DistingNT
  - ADD all source files to SOURCES

Task 2:
CREATE vcv-plugin/plugin.json:
  - DEFINE plugin metadata
  - SET brand: "Expert Sleepers"
  - ADD module: DistingNT
  - INCLUDE tags: ["Effect", "Oscillator", "Utility", "Multiple"]

Task 3:
CREATE vcv-plugin/src/plugin.cpp:
  - IMPLEMENT Plugin instance
  - REGISTER DistingNT module
  - SET plugin pointer

Task 4:
CREATE vcv-plugin/src/DistingNT.cpp:
  - PORT core emulator logic from /emulator/src/core/emulator.cpp
  - ADAPT to VCV Module base class
  - IMPLEMENT parameter configuration
  - CREATE bus routing system

Task 5:
CREATE vcv-plugin/src/display/OLEDWidget.cpp:
  - PORT display logic from /emulator/src/hardware/display.cpp
  - ADAPT to VCV's NVGcontext rendering
  - IMPLEMENT FramebufferWidget for performance
  - HANDLE 256x64 pixel rendering

Task 6:
CREATE vcv-plugin/src/algorithms/Algorithm.hpp:
  - DEFINE IDistingAlgorithm interface
  - MATCH Disting NT plugin API structure

Task 7:
PORT algorithms from test_plugins:
  - START with simple_gain as test case
  - ADAPT plugin_t interface to IDistingAlgorithm
  - PRESERVE DSP logic exactly

Task 8:
CREATE vcv-plugin/res/panels/DistingNT.svg:
  - DESIGN panel matching Disting NT layout
  - INCLUDE cutouts for display
  - POSITION all controls accurately

Task 9:
IMPLEMENT algorithm switching:
  - CREATE context menu for algorithm selection
  - UPDATE display when algorithm changes
  - HANDLE parameter persistence

Task 10:
OPTIMIZE performance:
  - PROFILE CPU usage
  - IMPLEMENT SIMD optimizations
  - USE dirty flags for display updates
```

### Per-task Implementation Details

```cpp
// Task 4: Bus System Implementation
class BusSystem {
    alignas(16) float buses[28][4];
    
    void routeInputsToBuses(Module* module) {
        // Audio inputs -> buses 0-3
        for (int i = 0; i < 4; i++) {
            if (module->inputs[AUDIO_INPUT_1 + i].isConnected()) {
                float v = module->inputs[AUDIO_INPUT_1 + i].getVoltage();
                buses[i][sampleIndex] = v / 5.0f; // Convert from ±5V to ±1.0
            }
        }
        
        // CV inputs -> buses 12-19
        for (int i = 0; i < 8; i++) {
            if (module->inputs[CV_INPUT_1 + i].isConnected()) {
                float v = module->inputs[CV_INPUT_1 + i].getVoltage();
                buses[12 + i][sampleIndex] = v / 10.0f; // Convert from ±10V to ±1.0
            }
        }
    }
};

// Task 5: Display Widget
struct OLEDWidget : FramebufferWidget {
    DistingNT* module;
    
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        // Set up 256x64 coordinate system
        nvgSave(args.vg);
        nvgScale(args.vg, box.size.x / 256.f, box.size.y / 64.f);
        
        // Clear display
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, 256, 64);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        // Let algorithm draw
        if (module->currentAlgorithm) {
            module->currentAlgorithm->drawDisplay(args.vg, 256, 64);
        }
        
        nvgRestore(args.vg);
    }
};
```

### Integration Points

```yaml
BUILD:
  - Requires: VCV Rack SDK 2.x
  - Toolchain: Same as VCV Rack (MSYS2/MinGW on Windows)
  - Dependencies: None (all included in SDK)

PANEL:
  - Size: 24HP (121.9mm) to fit all controls
  - Layout: OLED top, pots middle, I/O bottom
  - Graphics: Inkscape for SVG creation

PRESETS:
  - Location: vcv-plugin/presets/DistingNT/
  - Format: .vcvm JSON files
  - Include: Factory presets for each algorithm
```

## Validation Loop

### Level 1: Build & Syntax

```bash
# Set up build environment
cd vcv-plugin
export RACK_DIR=~/Rack-SDK

# Initial build - fix any compiler errors
make clean
make

# Expected: Successful build creating plugin.dylib/dll/so
# If errors: Check include paths, fix syntax errors
```

### Level 2: Load & Initialize

```bash
# Install to Rack
make install

# Launch Rack in dev mode
~/Rack/Rack -d

# Expected: Plugin loads, DistingNT appears in module browser
# Check: No errors in terminal output
# If crash: Debug with gdb/lldb
```

### Level 3: Functional Testing

```bash
# Manual testing checklist:
# 1. Add DistingNT module to patch
# 2. Display shows algorithm name/graphics
# 3. All knobs respond to mouse
# 4. Buttons light up when pressed
# 5. Audio passes through when patched
# 6. Algorithm switching via right-click menu
# 7. Preset saving/loading works
# 8. No audio glitches or CPU spikes

# Test each algorithm:
for algo in $(cat algorithm_list.txt); do
  echo "Testing $algo"
  # Select algorithm
  # Patch audio
  # Verify processing
done
```

### Level 4: Performance Validation

```bash
# CPU usage testing
# 1. Load 16 instances of DistingNT
# 2. Monitor CPU in Rack's menu bar
# 3. Should stay under 50% on modern CPU

# Memory leak testing  
# 1. Add/remove module repeatedly
# 2. Switch algorithms frequently
# 3. Monitor memory usage stays stable

# Audio quality testing
# 1. Use spectrum analyzer
# 2. Check for aliasing/artifacts
# 3. Verify 96kHz internal processing
```

### Level 5: Integration Testing

```yaml
# Test with common VCV workflows:
- Clock sync: Patch clock to trigger inputs
- CV modulation: Modulate all parameters
- Polyphonic cables: Test with poly inputs
- Preset morphing: Automate algorithm switching
- Recording: Verify clean output to Recorder module
```

## Final Validation Checklist

- [ ] Plugin builds without warnings: `make clean && make`
- [ ] Loads in VCV Rack without errors
- [ ] All controls visually respond
- [ ] Audio processing works correctly
- [ ] Display renders algorithm graphics
- [ ] Algorithm switching is smooth
- [ ] Presets save/load properly
- [ ] CPU usage is reasonable (<5% per instance)
- [ ] No memory leaks after extended use
- [ ] Works with VCV Recorder module
- [ ] Panel graphics look professional

---

## Anti-Patterns to Avoid

- ❌ Don't use system threads - VCV handles concurrency
- ❌ Don't allocate in process() - causes audio glitches
- ❌ Don't assume 44.1kHz - use args.sampleRate
- ❌ Don't skip NULL module checks in widgets
- ❌ Don't use blocking I/O in audio thread
- ❌ Don't hardcode paths - use asset::plugin()
- ❌ Don't ignore VCV's UI conventions
- ❌ Don't forget to handle polyphonic cables

## Confidence Score: 9/10

This PRP provides comprehensive context for implementing a VCV Rack plugin version of the Disting NT. The high confidence score reflects:
- Complete documentation references
- Clear mapping from existing code to VCV concepts  
- Executable validation steps
- Examples from similar successful VCV modules
- Detailed implementation blueprint

The only uncertainty (preventing 10/10) is the exact effort required to port each algorithm, but the framework and process are clear.