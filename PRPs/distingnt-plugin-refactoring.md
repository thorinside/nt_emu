name: "DistingNT Plugin Refactoring - Modular Architecture"
description: |

## Purpose

Refactor the monolithic DistingNT plugin code (1700+ lines) into a modular, maintainable architecture using appropriate design patterns while preserving all existing functionality.

## Core Principles

1. **Preserve Functionality**: No behavior changes, only structural improvements
2. **Follow VCV Patterns**: Align with VCV Rack's architecture conventions
3. **Progressive Refactoring**: Start with highest-impact extractions
4. **Testable Components**: Each module should be independently testable

---

## Goal

Transform the 34,000+ token DistingNT.cpp file into a well-organized, modular codebase with clear separation of concerns, making it easier to maintain, debug, and extend.

## Why

- **Reduced Cognitive Load**: Developers can focus on specific subsystems without understanding the entire codebase
- **Improved Maintainability**: Changes to one subsystem won't affect others
- **Better Testability**: Isolated components can be unit tested
- **Easier Debugging**: Issues can be traced to specific modules
- **Team Scalability**: Multiple developers can work on different modules simultaneously

## What

Refactor the DistingNT module into focused, single-responsibility components while maintaining 100% backward compatibility and preserving all existing features.

### Success Criteria

- [ ] All existing functionality works identically after refactoring
- [ ] No performance degradation (audio processing remains real-time safe)
- [ ] Code compiles without warnings
- [ ] Plugin loads successfully in VCV Rack
- [ ] All MIDI, parameter, and routing features work as before
- [ ] File size reduced from 34,000+ tokens to manageable chunks (<5,000 tokens per file)

## All Needed Context

### Documentation & References

```yaml
# MUST READ - Include these in your context window
- url: https://vcvrack.com/manual/PluginGuide
  why: Official VCV Rack plugin architecture guide

- url: https://vcvrack.com/manual/PluginDevelopmentTutorial
  why: Best practices for VCV plugin development

- url: https://github.com/bogaudio/BogaudioModules
  why: Excellent example of well-structured VCV modules

- url: https://github.com/VCVRack/Rack/tree/v2/include
  why: Core VCV Rack headers for API reference

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/src/EmulatorCore.hpp
  why: Existing pattern for encapsulating complex logic

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/src/dsp/BusSystem.hpp
  why: Pattern for audio routing encapsulation

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/src/display/OLEDWidget.hpp
  why: Display separation pattern already in use

- url: https://community.vcvrack.com/t/module-expanders-tutorial/4868
  why: Future expansion possibilities
```

### Current Codebase Structure

```bash
vcv-plugin/
├── src/
│   ├── DistingNT.cpp (34,805 tokens - NEEDS REFACTORING)
│   ├── EmulatorCore.hpp
│   ├── EncoderParamQuantity.hpp
│   ├── InfiniteEncoder.hpp
│   ├── algorithms/
│   │   ├── Algorithm.hpp
│   │   └── SimpleGainAlgorithm.cpp
│   ├── display/
│   │   ├── OLEDWidget.cpp
│   │   └── OLEDWidget.hpp
│   ├── dsp/
│   │   └── BusSystem.hpp
│   ├── nt_api_interface.h
│   ├── nt_api_provider.cpp
│   ├── plugin.cpp
│   └── plugin.hpp
└── Makefile
```

### Desired Codebase Structure

```bash
vcv-plugin/
├── src/
│   ├── DistingNT.cpp (thin facade ~500 lines)
│   ├── DistingNT.hpp (public interface)
│   ├── plugin/
│   │   ├── PluginManager.hpp
│   │   ├── PluginManager.cpp (plugin loading/unloading)
│   │   ├── PluginExecutor.hpp
│   │   └── PluginExecutor.cpp (safe plugin execution)
│   ├── parameter/
│   │   ├── ParameterSystem.hpp
│   │   ├── ParameterSystem.cpp (parameter management)
│   │   ├── ParameterRouter.hpp
│   │   └── ParameterRouter.cpp (parameter routing)
│   ├── menu/
│   │   ├── MenuSystem.hpp
│   │   ├── MenuSystem.cpp (menu state machine)
│   │   ├── MenuRenderer.hpp
│   │   └── MenuRenderer.cpp (menu display)
│   ├── midi/
│   │   ├── MidiProcessor.hpp
│   │   └── MidiProcessor.cpp (MIDI I/O handling)
│   ├── control/
│   │   ├── ControlProcessor.hpp
│   │   ├── ControlProcessor.cpp (hardware controls)
│   │   ├── EncoderHandler.hpp
│   │   └── EncoderHandler.cpp (encoder logic)
│   ├── routing/
│   │   ├── AudioRouter.hpp
│   │   ├── AudioRouter.cpp (bus routing)
│   │   ├── RoutingMatrix.hpp
│   │   └── RoutingMatrix.cpp (routing state)
│   ├── state/
│   │   ├── StateManager.hpp
│   │   └── StateManager.cpp (save/load state)
│   ├── ui/
│   │   ├── LightManager.hpp
│   │   └── LightManager.cpp (LED states)
│   └── nt_api/
│       ├── NTApiWrapper.hpp
│       └── NTApiWrapper.cpp (consolidated API functions)
└── Makefile (updated for new structure)
```

### Known Gotchas & Library Quirks

```cpp
// CRITICAL: VCV Rack threading model
// - Module methods are mutually exclusive but called from different threads
// - process() runs on audio thread - must be real-time safe
// - UI runs on separate thread - can allocate memory

// CRITICAL: Plugin pointer validation
// - Always check plugin pointers before use
// - Plugins can crash - use try-catch blocks
// - Unload plugin on any exception

// CRITICAL: Parameter bounds
// - VCV params are floats, plugin params are int16_t
// - Must handle conversion and clamping

// CRITICAL: Display updates
// - Only mark dirty when actually changed
// - Use FramebufferWidget for efficiency

// CRITICAL: MIDI thread safety
// - MIDI callbacks can come from any thread
// - Use proper synchronization
```

## Implementation Blueprint

### Phase 1: Core Infrastructure Classes

```cpp
// Common interfaces for dependency injection
class IParameterObserver {
    virtual void onParameterChanged(int index, int16_t value) = 0;
};

class IPluginStateObserver {
    virtual void onPluginLoaded(const std::string& path) = 0;
    virtual void onPluginUnloaded() = 0;
};

class IMenuObserver {
    virtual void onMenuStateChanged() = 0;
};
```

### List of Tasks (Priority Order)

```yaml
Task 1: Extract Plugin Management System
MODIFY src/DistingNT.cpp:
  - EXTRACT all plugin loading/unloading logic
  - MOVE to src/plugin/PluginManager.cpp
  - PRESERVE exact error handling patterns
  - KEEP validation logic intact

CREATE src/plugin/PluginManager.hpp:
  - DEFINE PluginManager class interface
  - INCLUDE observer pattern for state changes
  - EXPOSE methods: loadPlugin, unloadPlugin, isLoaded, etc.

CREATE src/plugin/PluginExecutor.hpp:
  - CENTRALIZE all try-catch plugin execution
  - IMPLEMENT safe wrappers for all plugin calls
  - MIRROR existing exception handling

Task 2: Extract Parameter System
CREATE src/parameter/ParameterSystem.hpp:
  - MOVE parameter extraction logic
  - IMPLEMENT parameter page management
  - HANDLE parameter value conversions

CREATE src/parameter/ParameterRouter.hpp:
  - EXTRACT routing matrix logic (256 elements)
  - IMPLEMENT custom routing application
  - PRESERVE exact routing algorithms

Task 3: Extract Menu System  
CREATE src/menu/MenuSystem.hpp:
  - IMPLEMENT state machine pattern
  - STATES: MENU_OFF, MENU_PAGE_SELECT, etc.
  - HANDLE all menu navigation logic

CREATE src/menu/MenuRenderer.hpp:
  - SEPARATE display logic from state
  - RENDER menu based on current state
  - UPDATE display buffer directly

Task 4: Extract MIDI Processing
CREATE src/midi/MidiProcessor.hpp:
  - CONSOLIDATE MIDI input/output handling
  - IMPLEMENT activity light management
  - PRESERVE thread safety patterns

Task 5: Extract Control Processing
CREATE src/control/ControlProcessor.hpp:
  - HANDLE button press/release
  - PROCESS encoder turns and presses
  - MANAGE pot value changes

CREATE src/control/EncoderHandler.hpp:
  - IMPLEMENT discrete encoder logic
  - HANDLE context-aware encoder behavior
  - SUPPORT both left and right encoders

Task 6: Extract Audio Routing
CREATE src/routing/AudioRouter.hpp:
  - MANAGE bus routing configuration
  - APPLY custom input/output routing
  - HANDLE routing persistence

Task 7: Extract State Management
CREATE src/state/StateManager.hpp:
  - IMPLEMENT JSON serialization
  - HANDLE all module state save/load
  - PRESERVE parameter values and routing

Task 8: Refactor Main Module
MODIFY src/DistingNT.cpp:
  - REDUCE to thin coordination layer
  - INJECT all extracted components
  - DELEGATE to appropriate subsystems
  - PRESERVE public Module interface
```

### Per-Task Implementation Details

```cpp
// Task 1: PluginManager implementation pattern
class PluginManager {
private:
    void* pluginHandle = nullptr;
    _NT_factory* factory = nullptr;
    _NT_algorithm* algorithm = nullptr;
    std::vector<IPluginStateObserver*> observers;
    
public:
    bool loadPlugin(const std::string& path) {
        // PATTERN: Always unload existing first
        if (isLoaded()) unloadPlugin();
        
        // CRITICAL: Use exact platform-specific loading
        #ifdef ARCH_WIN
            pluginHandle = LoadLibrary(path.c_str());
        #else
            pluginHandle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        #endif
        
        // PATTERN: Validate and extract factory
        // ... (follow existing validation pattern)
        
        // NOTIFY observers
        for (auto* obs : observers) {
            obs->onPluginLoaded(path);
        }
    }
    
    // PATTERN: Safe execution wrapper
    template<typename Func>
    void safeExecute(Func&& func, const char* context) {
        if (!isLoaded()) return;
        try {
            func();
        } catch (const std::exception& e) {
            INFO("Plugin crashed in %s: %s", context, e.what());
            unloadPlugin();
        } catch (...) {
            INFO("Plugin crashed in %s: unknown error", context);
            unloadPlugin();
        }
    }
};

// Task 3: MenuSystem state machine pattern
class MenuSystem {
    enum class State {
        OFF,
        PAGE_SELECT,
        PARAM_SELECT,
        VALUE_EDIT
    };
    
    State currentState = State::OFF;
    int pageIndex = 0;
    int paramIndex = 0;
    
    void transition(State newState) {
        // PATTERN: State exit logic
        onStateExit(currentState);
        currentState = newState;
        // PATTERN: State enter logic
        onStateEnter(currentState);
        notifyObservers();
    }
    
    void processEncoderInState(int encoder, int delta) {
        switch (currentState) {
            case State::PAGE_SELECT:
                // EXISTING LOGIC: Navigate pages
                break;
            case State::PARAM_SELECT:
                // EXISTING LOGIC: Navigate params
                break;
            // etc.
        }
    }
};
```

### Integration Points

```yaml
MAKEFILE:
  - pattern: "SOURCES += $(wildcard src/*/*.cpp)"
  - ensure: All new subdirectories included

MODULE_INIT:
  - location: DistingNT constructor
  - pattern: Initialize all subsystems with dependency injection

PROCESS_METHOD:
  - location: DistingNT::process()
  - pattern: Delegate to appropriate processors in order

JSON_METHODS:
  - location: dataToJson/dataFromJson
  - pattern: Delegate to StateManager
```

## Validation Loop

### Level 1: Syntax & Compilation

```bash
# Clean build to catch any issues
make clean
make dist

# Expected: Successful compilation, no warnings
# If errors: Check includes, circular dependencies
```

### Level 2: Unit Tests

```bash
# Create basic test harness for each component
# test_plugin_manager.cpp
g++ -std=c++17 -I./src -I./Rack-SDK/include \
    test_plugin_manager.cpp src/plugin/PluginManager.cpp \
    -o test_plugin_manager
./test_plugin_manager

# Expected: All component tests pass
# Test loading, unloading, error cases
```

### Level 3: Integration Test

```bash
# Load plugin in VCV Rack
cp dist/DistingNT/plugin.dylib ~/.Rack2/plugins/DistingNT/

# Start Rack with development flag
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

# Test checklist:
# - Module appears in browser
# - Can add to patch
# - All controls responsive
# - MIDI I/O works
# - Parameter menu works
# - Plugin loading works
# - Save/load patch preserves state
```

### Level 4: Performance Validation

```bash
# CPU usage comparison
# Before: Note CPU % with complex patch
# After: Should be same or better

# Real-time safety check
# Run with Rack's CPU meter
# Should show no audio dropouts
```

## Final Validation Checklist

- [ ] Code compiles without warnings
- [ ] All existing features work identically
- [ ] No memory leaks (test with valgrind/instruments)
- [ ] CPU usage unchanged or improved
- [ ] Plugin loads in VCV Rack 2.x
- [ ] MIDI input/output functional
- [ ] Parameter menu system works
- [ ] External plugin loading works
- [ ] State persistence (save/load) works
- [ ] All port lights update correctly
- [ ] No audio glitches or dropouts

## Anti-Patterns to Avoid

- ❌ Don't break VCV's threading model
- ❌ Don't allocate memory in process()
- ❌ Don't create circular dependencies
- ❌ Don't change external behavior
- ❌ Don't ignore plugin crash safety
- ❌ Don't mix concerns in extracted classes

---

## Confidence Score: 9/10

High confidence due to:
- Existing modular patterns in codebase (EmulatorCore, BusSystem)
- Clear separation of concerns identified
- Well-defined VCV Rack architecture constraints
- Comprehensive validation approach
- Progressive refactoring minimizes risk

Minor uncertainty around:
- Exact plugin API interactions during refactoring
- Potential hidden dependencies between subsystems