# PRP: Add GUI Display Simulation to Disting NT Emulator

## Goal

Enable the full GUI version of the Disting NT emulator with real-time 256x64 display simulation, interactive hardware controls (3 pots, 4 buttons, 2 encoders), and visual plugin debugging capabilities.

## Why

**Business Value:**
- **Enhanced Developer Experience**: Visual feedback accelerates plugin development
- **Hardware Fidelity**: Accurate simulation of physical Disting NT interface  
- **Debugging Capabilities**: Real-time visualization of plugin display output
- **Professional Tool**: GUI interface suitable for workshops and demonstrations

**User Impact:**
- Plugin developers can see exactly how their code will appear on hardware
- Interactive controls provide immediate feedback during development
- No need for physical hardware during early development phases
- Reduced development cycle time from hours to minutes

## What

Transform the console-only emulator into a full GUI application with:

### Success Criteria
- [ ] ImGui-based window opens showing 256x64 pixel display
- [ ] Plugin display output renders in real-time (text, shapes, graphics)
- [ ] Interactive hardware controls (3 pots, 4 buttons, 2 encoders) 
- [ ] Parameter changes update plugin state immediately
- [ ] Display scales cleanly for different monitor sizes
- [ ] No performance degradation compared to console version
- [ ] Compatible with macOS development environment

### User-Visible Behavior
1. **Application Launch**: GUI window opens with display and controls
2. **Plugin Loading**: Plugin graphics appear on simulated display
3. **Real-time Interaction**: Hardware controls update plugin parameters instantly
4. **Visual Feedback**: Button presses, knob turns, encoder changes visible
5. **Professional Interface**: Clean, usable interface matching hardware layout

## All Needed Context

### Architecture Analysis
**CRITICAL FINDING**: The GUI emulator is **fully implemented** but has a **broken build system**. All components exist:

- ✅ **Display Simulation**: Complete 256x64 pixel rendering with ImGui
- ✅ **Hardware Controls**: All 3 pots, 4 buttons, 2 encoders implemented  
- ✅ **Plugin API**: Full NT_drawText, NT_drawShape integration
- ✅ **UI Framework**: Professional ImGui application structure
- ❌ **Build System**: CMakeLists.txt missing GUI dependencies

### Documentation & References

- **file**: `emulator/src/main.cpp`
  **why**: Complete GUI application implementation with GLFW+ImGui
  **lines**: 1-200+ (full application lifecycle)

- **file**: `emulator/src/hardware/display.cpp`  
  **why**: 256x64 display simulation with pixel-perfect rendering
  **critical**: `updateFromApiState()` copies plugin graphics to display

- **file**: `emulator/src/core/api_shim.cpp`
  **why**: Plugin drawing API implementation (NT_drawText, NT_drawShapeI)
  **critical**: Lines 176-261 - all display functions implemented

- **file**: `emulator/CMakeLists.txt` vs `emulator/CMakeLists_console.txt`
  **why**: Console version works, GUI version missing dependencies
  **problem**: No ImGui, GLFW, OpenGL linking in main CMakeLists.txt

- **directory**: `emulator/third_party/`
  **why**: All required libraries already included (ImGui, json, spdlog)
  **note**: No need to download external dependencies

### Code Examples

```cpp
// Display rendering (already implemented)
void Display::updateFromApiState() {
    auto& display = ApiShim::getState().display;
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            pixels_[y * width_ + x] = display.getPixel(x, y) ? 255 : 0;
        }
    }
}

// Plugin drawing API (already working)
void NT_drawText(int x, int y, const char* text, enum _NT_textSize size, enum _NT_textAlign align) {
    ApiShim::drawText(x, y, text, size, align);
}
```

### Dependencies
- **ImGui**: UI framework (included in third_party/)
- **GLFW**: Window management (system dependency)  
- **OpenGL**: Graphics rendering (system framework)
- **PortAudio**: Audio system (already working)

### Known Gotchas

**CRITICAL**: 
- GUI code is complete but CMakeLists.txt doesn't build it
- Console version works perfectly - don't break it
- macOS OpenGL framework vs Linux libGL differences
- ImGui backends need explicit compilation

**Build System**:
- Must preserve console-only build option
- CMakeLists_console.txt should remain unchanged
- Main CMakeLists.txt needs GUI library integration

**Platform Issues**:
- macOS uses OpenGL framework, not libGL
- GLFW installation via Homebrew vs system
- Retina display scaling considerations

### Existing Patterns

**Working Console Build** (`CMakeLists_console.txt`):
```cmake
# Audio-only dependencies
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
target_link_libraries(${PROJECT_NAME} portaudio)
```

**GUI Main Function** (`main.cpp` - already exists):
```cpp
int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Disting NT Emulator", NULL, NULL);
    // ... complete ImGui setup already implemented
}
```

## Implementation Blueprint

### Task Breakdown

#### SETUP: Build System Prerequisites
**Duration**: 30 minutes

```bash
VERIFY dependencies:
  - brew list | grep glfw
  - ls third_party/imgui/
  - clang++ --version

VALIDATE console build still works:
  - cd build && make -f ../CMakeLists_console.txt
```

#### TASK 1: Add GUI Dependencies to CMakeLists.txt
**File**: `emulator/CMakeLists.txt`
**Duration**: 45 minutes

```cmake
ACTION emulator/CMakeLists.txt:
  - ADD: GLFW dependency discovery
  - ADD: OpenGL framework linking (macOS)
  - ADD: ImGui source compilation  
  - ADD: GUI source files to build
  - VALIDATE: cmake . && make
  - IF_FAIL: Check GLFW installation with brew
  - ROLLBACK: git checkout CMakeLists.txt
```

**Implementation**:
```cmake
# After existing PortAudio setup, add:
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# ImGui sources
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp  
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

# GUI emulator sources (replace console sources)
set(EMULATOR_SOURCES
    src/main.cpp
    src/core/emulator.cpp
    src/core/plugin_loader.cpp
    src/core/api_shim.cpp
    src/core/audio_engine.cpp
    src/core/midi_handler.cpp
    src/hardware/display.cpp
    src/hardware/hardware_interface.cpp
    src/hardware/io_manager.cpp
    src/ui/main_window.cpp
    src/ui/audio_panel.cpp
    src/ui/midi_panel.cpp
    src/ui/plugin_panel.cpp
    src/utils/file_watcher.cpp
    src/utils/logger.cpp
    src/utils/config.cpp
)

# Link GUI libraries
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
    portaudio
    glfw
    ${OPENGL_LIBRARIES}
)
```

#### TASK 2: Verify ImGui Integration
**Files**: `src/main.cpp`, `src/hardware/display.cpp`
**Duration**: 20 minutes

```cpp
ACTION src/main.cpp:
  - VERIFY: GLFW window creation code exists
  - VERIFY: ImGui context initialization  
  - VERIFY: OpenGL renderer setup
  - VALIDATE: Application compiles without errors
  - IF_FAIL: Check ImGui backend includes
  - ROLLBACK: Review third_party/imgui/examples/
```

#### TASK 3: Test Display Simulation
**Files**: Plugin integration test
**Duration**: 30 minutes

```bash
ACTION test display rendering:
  - BUILD: make -j4
  - RUN: ./DistingNTEmulator test_plugins/simple_gain.dylib  
  - VERIFY: GUI window opens with display panel
  - VERIFY: Plugin text/graphics appear on 256x64 display
  - VALIDATE: Hardware controls respond to mouse interaction
  - IF_FAIL: Check API shim display buffer connection
  - ROLLBACK: Use console version for comparison
```

#### TASK 4: Hardware Controls Integration
**Files**: `src/hardware/hardware_interface.cpp`, `src/ui/`
**Duration**: 15 minutes

```cpp
ACTION hardware controls:
  - VERIFY: Pot knob rendering and mouse interaction
  - VERIFY: Button click visualization and callbacks  
  - VERIFY: Encoder wheel interaction and parameter updates
  - VALIDATE: Parameter changes affect plugin immediately
  - IF_FAIL: Check callback registration in main.cpp
  - ROLLBACK: Compare with working console parameter system
```

#### TASK 5: Performance Validation
**Duration**: 15 minutes

```bash
ACTION performance check:
  - MEASURE: Frame rate with Activity Monitor
  - MEASURE: CPU usage compared to console version
  - VERIFY: Audio processing maintains 48kHz without dropouts
  - VALIDATE: No memory leaks over 5-minute run
  - IF_FAIL: Profile with Instruments.app
  - ROLLBACK: Disable display updates if needed
```

### Validation Loop

#### Level 1: Build Verification
```bash
cd emulator && mkdir build_gui && cd build_gui
cmake .. && make -j4
echo "Exit code: $?"
ls -la DistingNTEmulator
```

#### Level 2: Basic Functionality
```bash
./DistingNTEmulator test_plugins/simple_gain.dylib &
sleep 2
kill %1  # Should exit cleanly
```

#### Level 3: Display Integration  
```bash
# Interactive test - verify plugin graphics appear
./DistingNTEmulator test_plugins/simple_gain.dylib
# Manual verification:
# - GUI window opens
# - 256x64 display panel visible
# - Hardware controls panel visible  
# - Plugin text appears on display
```

#### Level 4: Hardware Interaction
```bash
# Interactive test - parameter control
# - Click pot knobs and verify plugin responds
# - Press buttons and verify visual feedback
# - Turn encoders and verify parameter changes
# - Verify audio processing continues smoothly
```

### Debug Strategies

**Build Failures**:
```bash
# Check dependencies
brew list | grep -E "(glfw|cmake)"
pkg-config --modversion glfw3

# Verbose build output  
make VERBOSE=1

# Check ImGui includes
find . -name "imgui.h" -o -name "imgui_impl_glfw.h"
```

**Runtime Issues**:
```bash
# Check OpenGL support
system_profiler SPDisplaysDataType | grep OpenGL

# Verify plugin symbols
nm -gU test_plugins/simple_gain.dylib | grep NT_

# Audio system check
./DistingNTEmulator --audio-test  # If implemented
```

**Display Problems**:
```cpp
// Add debug output to display.cpp
void Display::updateFromApiState() {
    auto& display = ApiShim::getState().display;
    static int frame = 0;
    if (++frame % 60 == 0) {
        std::cout << "Display update frame " << frame << std::endl;
    }
    // ... existing code
}
```

### Rollback Strategy

**Complete Rollback**:
```bash
git checkout CMakeLists.txt
cd build && make -f ../CMakeLists_console.txt  # Restore console version
```

**Partial Rollback**:
```bash
# Keep GUI build but disable specific features
cmake -DENABLE_GUI=OFF ..
```

**Emergency Console Mode**:
```bash  
# Always keep console version working
cp DistingNTEmulator_console DistingNTEmulator_backup
```

## Success Metrics

### Functional Requirements
- [ ] **Window Opens**: GUI application launches without crashes
- [ ] **Display Renders**: 256x64 pixel display shows plugin graphics  
- [ ] **Controls Work**: All hardware controls affect plugin parameters
- [ ] **Audio Maintains**: 48kHz audio processing continues smoothly
- [ ] **Performance**: <5% CPU overhead compared to console version

### Quality Requirements  
- [ ] **Build Time**: Complete build finishes under 2 minutes
- [ ] **Memory Usage**: <50MB additional RAM compared to console
- [ ] **Compatibility**: Works on macOS 10.15+ with standard dev setup
- [ ] **Stability**: Runs for >30 minutes without crashes
- [ ] **User Experience**: Intuitive interface requiring no documentation

### Risk Mitigation
- [ ] **Console Preserved**: Original console version still builds and works
- [ ] **Dependencies**: All required libraries available via Homebrew
- [ ] **Fallback**: Can disable GUI and revert to console mode
- [ ] **Documentation**: Clear build instructions for future developers

## Timeline

**Total Estimated Time**: 2-3 hours

1. **Setup & Dependencies** (30 min): Verify environment and libraries
2. **Build System Fix** (45 min): Complete CMakeLists.txt implementation  
3. **Integration Test** (20 min): Verify GUI components work together
4. **Display Validation** (30 min): Test plugin graphics rendering
5. **Controls Test** (15 min): Verify hardware interface interaction
6. **Performance Check** (15 min): Ensure no regressions
7. **Documentation** (15 min): Update README with GUI instructions

## Post-Implementation

### Documentation Updates
- Update `emulator/README.md` with GUI build instructions
- Add GUI usage guide with hardware control mapping
- Document troubleshooting for common GUI issues

### Future Enhancements
- Add display recording/screenshot capabilities
- Implement preset loading for hardware control states  
- Add plugin hot-reload with GUI state preservation
- Create automated GUI testing framework