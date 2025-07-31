# Safe C++ Plugin Refactoring: NtEmu.cpp Modularization with Comprehensive Testing

## Goal

Perform an extended, incremental refactoring of the large VCV Rack NtEmu.cpp file (2961 lines) by extracting the easiest and highest-impact code components into separate files and namespaces. Each refactoring step must be validated through comprehensive unit testing and git checkpoints to ensure 100% functional equivalence with the original working code. The end result should be a significantly smaller, more maintainable NtEmu.cpp file while preserving all existing functionality.

## Why

- **Maintainability**: The 2961-line NtEmu.cpp file has become unwieldy and difficult to maintain, debug, and extend
- **Technical Debt**: Large monolithic files violate Single Responsibility Principle and make code changes risky
- **Development Velocity**: Smaller, focused modules enable faster development and easier collaboration
- **Testing**: Modular code is more testable and allows for better isolation of bugs
- **Code Reuse**: Extracted components can be reused across different parts of the system
- **Proven Architecture**: The project already has established modular patterns that need completion

## What

Transform the monolithic NtEmu.cpp into a modular architecture by extracting ~1400 lines (47% of the file) into focused, reusable components while maintaining the existing established modular patterns. Each extraction must be validated through automated tests and manual verification before proceeding to the next step.

### Success Criteria

- [ ] Extract 700+ lines in Phase 1 (24% reduction) - low risk, high impact extractions
- [ ] Extract additional 400+ lines in Phase 2 (14% reduction) - medium risk extractions
- [ ] All existing tests continue to pass after each extraction step
- [ ] Build system (`make clean && make`) continues to work after each step
- [ ] VCV Rack plugin loads and functions identically after each step
- [ ] Create unit tests for each extracted component that validate functional equivalence
- [ ] Git commit created before and after each successful refactoring step
- [ ] Final NtEmu.cpp is ~1500 lines (50% reduction from original)

## All Needed Context

### Documentation & References

```yaml
- url: https://refactoring.guru/
  why: Comprehensive refactoring patterns and techniques - extract method, extract class patterns

- url: https://github.com/google/googletest
  why: C++ testing framework documentation for unit test implementation

- url: https://clang.llvm.org/extra/clang-tidy/
  why: Static analysis and automated refactoring assistance

- url: https://isocpp.github.io/CppCoreGuidelines/
  why: Modern C++ best practices for modular design

- url: https://approvaltestscpp.readthedocs.io/
  why: Characterization testing for legacy C++ code refactoring

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/src/plugin/PluginManager.hpp
  why: Example of successful modular component extraction pattern to follow

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/src/parameter/ParameterSystem.hpp
  why: Established parameter management patterns and Observer pattern implementation

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/test_plugins/test_integration.cpp
  why: Testing patterns for VCV plugin components and validation approaches

- file: /Users/nealsanche/nosuch/nt_emu/vcv-plugin/tests/test_json_bridge_standalone.cpp
  why: Unit testing patterns and comprehensive test validation (16/16 tests passing)

- docfile: PRPs/ai_docs/vcv-rack-plugin-audio-processing.md
  why: VCV Rack plugin architecture and real-time safety considerations
```

### Current Codebase Structure

```bash
vcv-plugin/src/
├── NtEmu.cpp                           # 2961 lines - TARGET FOR REFACTORING
├── plugin/
│   ├── PluginManager.hpp               # 465 lines - ESTABLISHED PATTERN
│   └── PluginExecutor.hpp              # 190 lines - Safe execution with error handling
├── parameter/
│   └── ParameterSystem.hpp             # 450 lines - Parameter management & routing
├── menu/
│   └── MenuSystem.hpp                  # 380 lines - State machine for menu navigation  
├── midi/
│   └── MidiProcessor.hpp               # 220 lines - MIDI I/O with activity tracking
├── dsp/
│   └── BusSystem.hpp                   # Audio routing system
├── display/
│   └── OLEDWidget.hpp                  # Display rendering
└── EmulatorCore.hpp                    # Core emulator logic
```

### Target Modular Structure After Refactoring

```bash
vcv-plugin/src/
├── NtEmu.cpp                           # ~1500 lines - Core integration only
├── EmulatorConstants.hpp               # NEW - Extracted constants/enums
├── api/
│   └── NTApiWrapper.hpp                # NEW - C API wrapper functions
├── display/
│   ├── DisplayRenderer.hpp             # NEW - Display rendering logic
│   └── BufferConverter.hpp             # NEW - Display buffer conversion
├── ui/
│   └── ContextMenus.hpp                # NEW - Context menu logic
├── input/
│   └── ControlProcessor.hpp            # NEW - Control processing
├── serialization/
│   └── StateManager.hpp                # NEW - JSON serialization logic
├── observers/
│   └── ModuleObserver.hpp              # NEW - Observer pattern methods
└── [existing modular components remain unchanged]
```

### Known Gotchas & Critical Constraints

```cpp
// CRITICAL: VCV Rack plugin architecture constraints
// - Module methods cannot be called simultaneously from multiple threads
// - Audio processing must be real-time safe (no allocations, locks, virtual calls)
// - Parameter changes happen on control thread, audio on audio thread
// - All UI interactions happen on main thread

// CRITICAL: C++11 compatibility required for VCV Rack
// - Cannot use C++14/17/20 features 
// - Smart pointers available, but avoid complex template metaprogramming
// - Use template-based polymorphism instead of virtual calls in audio path

// CRITICAL: Build system dependencies
// - Makefile uses $(wildcard src/*/*.cpp) for automatic source discovery
// - New directories automatically included in build
// - Individual component compilation: make src/component/File.cpp.o

// CRITICAL: Testing infrastructure available
// - 16/16 JSON bridge unit tests currently passing
// - Comprehensive test plugin library for integration testing
// - `make test` runs full unit test suite
// - Mock VCV environment available for component testing

// CRITICAL: Observer pattern used throughout
// - All major components implement Observer pattern for loose coupling
// - IParameterObserver, IPluginStateObserver, IMenuObserver interfaces
// - Component communication via notifications, not direct calls

// GOTCHA: Code removal difficulty
// - Models struggle with removing large blocks of code
// - Use commenting approach with clear delimiters for moved code
// - Example: /* MOVED_TO_FILE: api/NTApiWrapper.hpp - START */ ... /* MOVED_TO_FILE: api/NTApiWrapper.hpp - END */
// - Create cleanup scripts to remove commented blocks after validation
```

## Implementation Blueprint

### Phase 1: Low Risk, High Impact Extractions (~700 lines, 24% reduction)

#### Extract Constants and Enums (50 lines) - EASIEST WIN
```cpp
// CREATE src/EmulatorConstants.hpp
// EXTRACT from NtEmu.cpp lines 549-594:
enum ParamIds { POT_L_PARAM, POT_C_PARAM, POT_R_PARAM, ... };
enum InputIds { AUDIO_INPUT_1, AUDIO_INPUT_2, ... };
enum OutputIds { AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, ... };
enum LightIds { BUTTON_1_LIGHT, BUTTON_2_LIGHT, ... };

// Pattern: Header-only constants file with include guards
#pragma once
namespace EmulatorConstants {
    // All enums and constants here
}
```

#### Extract C API Wrapper Functions (200 lines) - HIGH IMPACT
```cpp
// CREATE src/api/NTApiWrapper.hpp and NTApiWrapper.cpp
// EXTRACT from NtEmu.cpp lines 48-515:
// Functions: NT_drawText(), setNTPixel(), drawNTLine(), NT_sendMidi(), etc.

// Pattern: Standalone utility functions with minimal dependencies
namespace NTApi {
    void NT_drawText(int x, int y, const char* text, int color);
    void setNTPixel(int x, int y, int color);
    void drawNTLine(int x1, int y1, int x2, int y2, int color);
    void NT_sendMidi(unsigned char* data, int length);
    // ... all C API wrapper functions
}

// DEPENDENCY: Only requires NT screen buffer and MIDI output
// TESTING: Create unit tests that verify each API function behavior
```

#### Extract Display System (300 lines) - MODERATE COMPLEXITY
```cpp
// CREATE src/display/DisplayRenderer.hpp and DisplayRenderer.cpp
// EXTRACT ModuleOLEDWidget class from NtEmu.cpp lines 2078-2614

// Pattern: Encapsulate display rendering logic
class DisplayRenderer {
public:
    void syncNTScreenToVCVBuffer(VCVDisplayBuffer& buffer);
    void drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer);
    void renderFrame(NVGcontext* vg);
    
private:
    VCVDisplayBuffer displayBuffer;
    // Display state management
};

// DEPENDENCY: NanoVG, NT screen buffer, FramebufferWidget
// TESTING: Create tests that verify display buffer conversion and rendering
```

#### Extract Display Buffer Conversion (150 lines) - FOCUSED UTILITY
```cpp
// CREATE src/display/BufferConverter.hpp and BufferConverter.cpp
// EXTRACT buffer conversion logic from NtEmu.cpp lines 2250-2400

// Pattern: Pure utility functions for buffer format conversion
namespace DisplayBufferConverter {
    void convertNTBufferToVCV(const uint8_t* ntBuffer, VCVDisplayBuffer& vcvBuffer);
    void applyPixelFormatTransforms(VCVDisplayBuffer& buffer);
    void optimizeBufferForRendering(VCVDisplayBuffer& buffer);
}

// DEPENDENCY: Only buffer format definitions
// TESTING: Create comprehensive tests for different pixel formats and edge cases
```

### Phase 2: Medium Risk Extractions (~400 lines, 14% reduction)

#### Extract Context Menu Logic (200 lines)
```cpp
// CREATE src/ui/ContextMenus.hpp and ContextMenus.cpp
// EXTRACT context menu creation logic from EmulatorWidget class

// Pattern: Separate UI logic from widget structure
class ContextMenuBuilder {
public:
    Menu* createPluginMenu(EmulatorModule* module);
    Menu* createParameterMenu(EmulatorModule* module);
    Menu* createMIDIMenu(EmulatorModule* module);
    
private:
    void addPluginLoadingItems(Menu* menu, EmulatorModule* module);
    void addParameterRoutingItems(Menu* menu, EmulatorModule* module);
};
```

#### Extract Control Processing (100 lines)
```cpp
// CREATE src/input/ControlProcessor.hpp and ControlProcessor.cpp
// EXTRACT control processing methods from NtEmu.cpp lines 1372-1475

// Pattern: Dedicated input handling with clear interface
class ControlProcessor {
public:
    void processButton1Timing(float deltaTime);
    void processControlsExceptButton1();
    void processAllControls(float deltaTime);
    
private:
    EmulatorCore* emulatorCore;
    MenuSystem* menuSystem;
};
```

#### Consolidate Parameter Methods (100 lines)
```cpp
// MOVE remaining parameter methods to existing parameter/ParameterSystem.hpp
// EXTRACT from NtEmu.cpp lines 1080-1233:
// Methods: setParameterValue(), updateParameterRouting(), updateBusRouting()

// Pattern: Complete the existing ParameterSystem with all parameter logic
// Already established in parameter/ParameterSystem.hpp - consolidate remaining methods
```

### List of Tasks to Complete (Phase 1 - Low Risk Extractions)

```yaml
Task 1: Setup Testing Infrastructure
VALIDATE existing tests pass:
  - RUN: make clean && make
  - RUN: make test  
  - VERIFY: All tests pass (16/16 JSON bridge tests)
  - COMMIT: git commit -m "Baseline: All tests passing before refactoring"

Task 2: Extract Constants and Enums  
CREATE src/EmulatorConstants.hpp:
  - EXTRACT lines 549-594 from NtEmu.cpp
  - WRAP in namespace EmulatorConstants
  - ADD include guards and proper formatting
  - COMMENT original code with /* MOVED_TO_FILE: EmulatorConstants.hpp */

MODIFY src/NtEmu.cpp:
  - ADD: #include "EmulatorConstants.hpp"
  - REPLACE all enum references with EmulatorConstants:: prefix
  - VALIDATE: make clean && make (should compile successfully)

VALIDATE functionality:
  - RUN: make test
  - VERIFY: All tests still pass
  - TEST: VCV Rack plugin loads and functions identically
  - COMMIT: git commit -m "Extract constants and enums to EmulatorConstants.hpp"

Task 3: Extract C API Wrapper Functions
CREATE src/api/ directory and src/api/NTApiWrapper.hpp:
  - EXTRACT functions from lines 48-515 in NtEmu.cpp
  - ORGANIZE in namespace NTApi
  - MAINTAIN all function signatures exactly
  - PRESERVE all implementation details

CREATE src/api/NTApiWrapper.cpp:
  - IMPLEMENT all extracted functions
  - MAINTAIN exact same behavior and dependencies
  - ADD comprehensive error handling

CREATE tests/test_nt_api_wrapper.cpp:
  - TEST each API function for expected behavior
  - VALIDATE no regressions in NT screen buffer manipulation
  - TEST MIDI output functionality

MODIFY src/NtEmu.cpp:
  - ADD: #include "api/NTApiWrapper.hpp"  
  - REPLACE function calls with NTApi:: prefix
  - COMMENT moved code blocks with clear markers
  - VALIDATE: make clean && make

VALIDATE functionality:
  - RUN: make test
  - RUN: tests/test_nt_api_wrapper  
  - VERIFY: All API functions work identically
  - TEST: VCV plugin display and MIDI output unchanged
  - COMMIT: git commit -m "Extract C API wrapper functions to api/NTApiWrapper"

Task 4: Extract Display Renderer
CREATE src/display/DisplayRenderer.hpp:
  - EXTRACT ModuleOLEDWidget class definition from lines 2078-2614
  - RENAME to DisplayRenderer for clarity
  - MAINTAIN all method signatures and public interface
  - PRESERVE FramebufferWidget inheritance

CREATE src/display/DisplayRenderer.cpp:
  - IMPLEMENT all display rendering methods
  - MAINTAIN exact rendering behavior
  - PRESERVE NanoVG usage patterns

CREATE tests/test_display_renderer.cpp:
  - TEST display buffer synchronization
  - VALIDATE rendering output consistency  
  - TEST edge cases (empty buffer, max size buffer)

MODIFY src/NtEmu.cpp:
  - ADD: #include "display/DisplayRenderer.hpp"
  - REPLACE ModuleOLEDWidget usage with DisplayRenderer
  - COMMENT moved display code with markers
  - VALIDATE: make clean && make

VALIDATE functionality:
  - RUN: make test
  - RUN: tests/test_display_renderer
  - VERIFY: Display output identical to before
  - TEST: VCV plugin display rendering unchanged
  - COMMIT: git commit -m "Extract display rendering to DisplayRenderer component"

Task 5: Extract Buffer Conversion Logic
CREATE src/display/BufferConverter.hpp:
  - EXTRACT buffer conversion functions from lines 2250-2400
  - ORGANIZE as pure utility functions in namespace
  - MAINTAIN all conversion algorithms exactly

CREATE src/display/BufferConverter.cpp:
  - IMPLEMENT buffer format conversion functions
  - PRESERVE all pixel manipulation logic
  - MAINTAIN performance characteristics

CREATE tests/test_buffer_converter.cpp:
  - TEST various pixel formats and buffer sizes
  - VALIDATE conversion accuracy with known inputs/outputs
  - TEST edge cases and boundary conditions

MODIFY src/display/DisplayRenderer.cpp:
  - ADD: #include "display/BufferConverter.hpp"
  - REPLACE inline buffer conversion with BufferConverter calls
  - VALIDATE: make clean && make

VALIDATE functionality:
  - RUN: make test
  - RUN: tests/test_buffer_converter
  - VERIFY: Display buffer conversion identical
  - TEST: No performance regression in display updates
  - COMMIT: git commit -m "Extract buffer conversion logic to BufferConverter"

Task 6: Validation and Cleanup
VALIDATE complete Phase 1:
  - RUN: make clean && make
  - RUN: make test (all tests pass)
  - TEST: VCV Rack plugin loads and functions identically
  - VERIFY: ~700 lines removed from NtEmu.cpp (24% reduction)
  - MEASURE: File size reduction and compilation time impact

CREATE cleanup script:
  - SCRIPT: tools/remove_commented_code.sh
  - SAFELY remove all /* MOVED_TO_FILE */ comment blocks
  - VALIDATE: Code still compiles after cleanup

DOCUMENT progress:
  - UPDATE: README.md with refactoring progress
  - DOCUMENT: New modular structure and benefits
  - COMMIT: git commit -m "Phase 1 refactoring complete: 700+ lines extracted"
```

## Validation Loop

### Level 1: Compilation and Syntax

```bash
# Run after each extraction step - MUST PASS before proceeding
make clean && make

# Expected: Successful compilation with no errors
# If errors: Fix immediately before proceeding to tests
```

### Level 2: Unit Tests - Comprehensive Validation

```bash
# JSON Bridge tests (existing) - MUST continue to pass
make test
# Expected: 16/16 tests pass ✓

# New component tests - CREATE for each extracted component  
make tests/test_nt_api_wrapper && ./tests/test_nt_api_wrapper
make tests/test_display_renderer && ./tests/test_display_renderer  
make tests/test_buffer_converter && ./tests/test_buffer_converter

# Integration test - Use existing test infrastructure
cd vcv-plugin/test_plugins && make tests && ./test_integration

# Expected: All new tests pass, demonstrating functional equivalence
```

### Level 3: Plugin Integration Testing

```bash
# Build and install plugin to VCV Rack
make clean && make
cp dist/DistingNT/plugin.dylib ~/.Rack2/plugins/DistingNT/

# Manual functional testing
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

# Test checklist:
# - Plugin loads without errors
# - Display renders correctly  
# - Parameters respond to changes
# - MIDI input/output works
# - Menu navigation functions
# - Audio processing unchanged
# - No crashes or glitches
```

### Level 4: Performance and Regression Testing

```bash
# Parameter system validation - use existing comprehensive test
cd emulator/test_plugins/examples && ./test_parameters
# Expected: All parameter extraction and formatting works identically

# Plugin compatibility testing
cd emulator/test_plugins/examples && ./test_all_plugins
# Expected: No API drift, no crashes, all plugins load successfully

# Real-time performance testing (manual)
# - Load complex plugin in VCV Rack
# - Monitor CPU usage during audio processing
# - Verify no audio dropouts or increased latency
# - Compare performance metrics to baseline
```

## Final Validation Checklist (After Each Phase)

- [ ] Build succeeds: `make clean && make`
- [ ] All existing tests pass: `make test` (16/16 ✓)
- [ ] New component tests pass: All extracted component tests ✓  
- [ ] Integration tests pass: `./test_integration` ✓
- [ ] Plugin loads in VCV Rack without errors
- [ ] Display rendering identical to baseline
- [ ] Parameter system functions identically
- [ ] MIDI I/O works unchanged
- [ ] Menu navigation functions correctly
- [ ] No performance regression (audio processing time unchanged)
- [ ] No memory leaks or crashes
- [ ] Git commits created before/after each successful step
- [ ] Code reduction target achieved (Phase 1: 700+ lines, Phase 2: additional 400+ lines)

---

## Anti-Patterns to Avoid

- ❌ Don't extract more than one component at a time - incremental steps only
- ❌ Don't skip testing between extractions - validate every step  
- ❌ Don't modify behavior during extraction - maintain functional equivalence
- ❌ Don't remove original code until tests pass - comment it out first
- ❌ Don't ignore compilation warnings - fix them immediately
- ❌ Don't extract tightly coupled code in early phases - start with loose coupling
- ❌ Don't modify the core audio processing loop early - save for final phases
- ❌ Don't skip git commits - create checkpoint after each successful extraction

## Confidence Score: 9/10

This PRP provides comprehensive context, established patterns, extensive testing infrastructure, and a proven incremental approach. The nt_emu codebase already has successful modular components and comprehensive testing (16/16 unit tests passing, integration tests, plugin compatibility tests). The systematic phase-based approach with validation gates at each step maximizes success probability while minimizing risk of breaking existing functionality.