# TASK PRP: Implement Interactive Controls with State Management

## Goal

Transform the static authentic Disting NT interface into a fully interactive control surface with state management, enabling real-time manipulation of potentiometers, buttons, and encoders that directly drive the hosted plugin parameters.

## Why

**Business Value:**
- **Real-time Control**: Immediate manipulation of plugin parameters during development
- **Hardware Simulation**: Authentic hardware interaction experience for developers
- **Visual Feedback**: Brightness changes and state indicators enhance usability
- **Professional Tool**: Interactive controls suitable for live performance and demonstration

**User Impact:**
- Plugin developers can manipulate parameters in real-time
- Visual feedback provides immediate confirmation of control state
- Authentic hardware feel accelerates development workflow
- No need to use separate control windows - unified interface

## What

Transform the current visual-only interface into a fully interactive control surface with:

### Success Criteria
- [ ] All 3 potentiometers respond to mouse drag with smooth rotation
- [ ] All 4 buttons toggle state on click with visual brightness feedback
- [ ] All 2 encoders respond to mouse wheel/drag with accumulating values
- [ ] All controls support press-and-hold with brightness increase
- [ ] Control state drives actual plugin parameters in real-time
- [ ] Visual feedback shows current control values and states
- [ ] State persists across plugin reloads and hot swaps

### User-Visible Behavior
1. **Potentiometer Interaction**: Click and drag vertically to rotate, click to press
2. **Button Interaction**: Click to toggle, visual brightness indicates state
3. **Encoder Interaction**: Mouse wheel or horizontal drag to increment/decrement
4. **Visual Feedback**: All controls brighten when pressed or active
5. **Real-time Updates**: Plugin parameters change immediately with control movement

## All Needed Context

### Current Architecture Analysis

**Existing Infrastructure:**
- Complete callback system in `HardwareInterface` for parameter changes
- Sophisticated custom ImGui controls with rendering and interaction
- State management with change detection between UI and plugin
- Plugin communication via ApiShim layer
- Hardware state structure supporting all control types

**Current Files:**
- `src/ui/main_window.cpp/.h` - Static visual representation (needs interaction)
- `src/hardware/hardware_interface.cpp/.h` - Interactive controls (separate window)
- `src/core/api_shim.cpp/.h` - Plugin communication layer
- `src/core/emulator.cpp/.h` - Coordination and callback setup

### Dependencies
- **ImGui**: Mouse interaction handling and custom drawing
- **Existing State System**: HardwareState, callback infrastructure
- **Plugin API**: Parameter value setting via ApiShim

### Known Gotchas

**ImGui Interaction Challenges:**
- Custom controls require careful mouse event handling
- Invisible button areas must align precisely with visual elements
- State synchronization between visual appearance and actual values
- Performance with custom drawing every frame

**State Management Issues:**
- Parameter mapping - which controls affect which plugin parameters
- Control value ranges vs plugin parameter ranges
- Concurrent access to state from UI and audio threads
- State persistence during plugin hot reload

**Visual Feedback Complexity:**
- Brightness changes must not conflict with authentic hardware appearance
- Smooth animation of control movements
- Proper highlighting without breaking hardware aesthetic

### Existing Patterns

**Custom ImGui Control Pattern** (`hardware_interface.cpp`):
```cpp
bool HardwareInterface::renderKnob(const char* label, float* value, bool* pressed, 
                                  float min_val, float max_val, float radius) {
    ImVec2 knob_center = ImGui::GetCursorScreenPos();
    
    // Create invisible button for interaction
    ImGui::InvisibleButton(label, ImVec2(radius * 2, radius * 2));
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool value_changed = false;
    
    // Handle mouse dragging for rotation
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        float delta = -mouse_delta.y * 0.01f;
        *value = std::clamp(*value + delta * (max_val - min_val), min_val, max_val);
        value_changed = true;
    }
    
    // Handle clicking for press
    if (ImGui::IsItemClicked()) {
        *pressed = !*pressed;
    }
    
    // Custom rendering with ImDrawList
    renderKnobVisual(knob_center, radius, *value, *pressed, is_hovered);
    
    return value_changed;
}
```

**State Update Pattern** (`hardware_interface.cpp`):
```cpp
void HardwareInterface::update() {
    // Copy current state to previous for change detection
    previous_state_ = state_;
    
    // Copy UI values to hardware state
    for (int i = 0; i < 3; i++) {
        state_.pots[i] = pot_display_values_[i];
        state_.pot_pressed[i] = pot_pressed_states_[i];
    }
    
    // Update API state
    ApiShim::getState().hardware = state_;
    
    // Trigger callbacks for changes
    updateCallbacks();
}
```

**Callback Setup Pattern** (`emulator.cpp`):
```cpp
void Emulator::setupCallbacks() {
    hardware_interface_->setParameterChangeCallback([this](int parameter, float value) {
        onParameterChange(parameter, value);
    });
    
    hardware_interface_->setButtonCallback([this](int button, bool pressed) {
        onButtonChange(button, pressed);
    });
}
```

## Implementation Blueprint

### Task Breakdown

#### TASK 1: Add Interaction Layer to Main Window
**File**: `src/ui/main_window.cpp/.h`
**Duration**: 90 minutes

```cpp
ACTION src/ui/main_window.h:
  - ADD: Interactive state management members
  - ADD: Control interaction handling methods
  - ADD: Reference to hardware interface for state synchronization
  - VALIDATE: Class compiles with new members
  - IF_FAIL: Check forward declarations and includes
  - ROLLBACK: Remove added members, restore original header
```

**Implementation**:
```cpp
class DistingNTMainWindow {
private:
    // Interactive state (synchronized with HardwareInterface)
    std::array<float, 3> pot_values_{0.5f, 0.5f, 0.5f};        // 0.0-1.0
    std::array<bool, 3> pot_pressed_{false, false, false};
    std::array<bool, 4> button_states_{false, false, false, false};
    std::array<int, 2> encoder_values_{0, 0};
    std::array<bool, 2> encoder_pressed_{false, false, false};
    
    // Visual feedback state
    std::array<float, 3> pot_brightness_{1.0f, 1.0f, 1.0f};
    std::array<float, 4> button_brightness_{1.0f, 1.0f, 1.0f, 1.0f};
    std::array<float, 2> encoder_brightness_{1.0f, 1.0f};
    
    // Hardware interface reference for state sync
    std::shared_ptr<HardwareInterface> hardware_interface_;
    
    // Interaction methods
    bool renderInteractivePot(int index, ImVec2 center, float radius);
    bool renderInteractiveButton(int index, ImVec2 center, float radius);
    bool renderInteractiveEncoder(int index, ImVec2 center, float radius);
    void updateBrightness();
    void syncStateToHardware();
};
```

#### TASK 2: Implement Interactive Potentiometer Controls
**File**: `src/ui/main_window.cpp`
**Duration**: 60 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - REPLACE: Static pot rendering with interactive renderInteractivePot
  - ADD: Mouse drag handling for vertical control (authentic hardware feel)
  - ADD: Click handling for pot press state
  - ADD: Brightness feedback for pressed/hovered states
  - VALIDATE: Pots respond to mouse interaction with visual feedback
  - IF_FAIL: Check ImGui::InvisibleButton positioning and sizing
  - ROLLBACK: Restore static pot rendering
```

**Implementation**:
```cpp
bool DistingNTMainWindow::renderInteractivePot(int index, ImVec2 center, float radius) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    
    char id[32];
    snprintf(id, sizeof(id), "pot_%d", index);
    ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));
    
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool value_changed = false;
    
    // Handle vertical mouse drag for pot rotation (like real hardware)
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        float delta = -mouse_delta.y * 0.005f; // Sensitivity adjustment
        pot_values_[index] = std::clamp(pot_values_[index] + delta, 0.0f, 1.0f);
        value_changed = true;
    }
    
    // Handle click for press state
    if (ImGui::IsItemClicked()) {
        pot_pressed_[index] = !pot_pressed_[index];
    }
    
    // Update brightness for visual feedback
    float target_brightness = 1.0f;
    if (pot_pressed_[index]) target_brightness = 1.4f;
    else if (is_hovered) target_brightness = 1.2f;
    
    pot_brightness_[index] = ImLerp(pot_brightness_[index], target_brightness, 0.1f);
    
    // Render visual with brightness adjustment
    renderPotVisual(center, radius, pot_values_[index], pot_pressed_[index], pot_brightness_[index]);
    
    return value_changed;
}

void DistingNTMainWindow::renderPotVisual(ImVec2 center, float radius, float value, bool pressed, float brightness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Brightness-adjusted colors
    ImU32 knob_color = IM_COL32(
        (int)(50 * brightness), (int)(50 * brightness), (int)(50 * brightness), 255
    );
    ImU32 ring_color = IM_COL32(
        (int)(75 * brightness), (int)(75 * brightness), (int)(75 * brightness), 255
    );
    
    // Render knob body with brightness
    draw_list->AddCircleFilled(center, radius, knob_color);
    draw_list->AddCircle(center, radius, ring_color, 0, 2.0f);
    
    // Calculate indicator line angle (7 o'clock to 5 o'clock range)
    float min_angle = -5.0f * M_PI / 6.0f;  // 7 o'clock
    float max_angle = M_PI / 6.0f;          // 5 o'clock
    float angle = min_angle + (max_angle - min_angle) * value;
    
    // Render indicator line
    ImVec2 indicator_start(center.x + cos(angle) * 6, center.y + sin(angle) * 6);
    ImVec2 indicator_end(center.x + cos(angle) * (radius - 3), center.y + sin(angle) * (radius - 3));
    
    ImU32 indicator_color = IM_COL32((int)(255 * brightness), (int)(255 * brightness), (int)(255 * brightness), 255);
    draw_list->AddLine(indicator_start, indicator_end, indicator_color, 5.0f);
}
```

#### TASK 3: Implement Interactive Button Controls
**File**: `src/ui/main_window.cpp`
**Duration**: 45 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - REPLACE: Static button rendering with interactive renderInteractiveButton
  - ADD: Click handling for button toggle
  - ADD: Brightness feedback for pressed/active states
  - ADD: Visual state indication (pressed vs unpressed)
  - VALIDATE: Buttons toggle state and show visual feedback
  - IF_FAIL: Check button positioning around encoders
  - ROLLBACK: Restore static button rendering
```

**Implementation**:
```cpp
bool DistingNTMainWindow::renderInteractiveButton(int index, ImVec2 center, float radius) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    
    char id[32];
    snprintf(id, sizeof(id), "button_%d", index);
    ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));
    
    bool is_hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();
    
    // Handle click for toggle
    if (clicked) {
        button_states_[index] = !button_states_[index];
    }
    
    // Update brightness for visual feedback
    float target_brightness = 1.0f;
    if (button_states_[index]) target_brightness = 1.6f;
    else if (is_hovered) target_brightness = 1.3f;
    
    button_brightness_[index] = ImLerp(button_brightness_[index], target_brightness, 0.15f);
    
    // Render visual with state and brightness
    renderButtonVisual(center, radius, button_states_[index], button_brightness_[index]);
    
    return clicked;
}

void DistingNTMainWindow::renderButtonVisual(ImVec2 center, float radius, bool pressed, float brightness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Brightness and state-adjusted colors
    int base_color = pressed ? 60 : 30;
    ImU32 button_color = IM_COL32(
        (int)(base_color * brightness), (int)(base_color * brightness), (int)(base_color * brightness), 255
    );
    ImU32 ring_color = IM_COL32(
        (int)(80 * brightness), (int)(80 * brightness), (int)(80 * brightness), 255
    );
    
    // Render button with pressed/unpressed appearance
    draw_list->AddCircleFilled(center, radius, button_color);
    draw_list->AddCircle(center, radius, ring_color, 0, pressed ? 2.0f : 1.0f);
    
    // Add inner highlight if pressed
    if (pressed) {
        ImU32 highlight_color = IM_COL32((int)(100 * brightness), (int)(100 * brightness), (int)(100 * brightness), 255);
        draw_list->AddCircle(center, radius - 2, highlight_color, 0, 1.0f);
    }
}
```

#### TASK 4: Implement Interactive Encoder Controls
**File**: `src/ui/main_window.cpp`
**Duration**: 60 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - REPLACE: Static encoder rendering with interactive renderInteractiveEncoder
  - ADD: Mouse wheel handling for encoder increment/decrement
  - ADD: Horizontal drag handling as alternative input method
  - ADD: Click handling for encoder press state
  - ADD: Visual detent indication for encoder values
  - VALIDATE: Encoders respond to wheel and drag with accumulating values
  - IF_FAIL: Check mouse wheel event handling in ImGui
  - ROLLBACK: Restore static encoder rendering
```

**Implementation**:
```cpp
bool DistingNTMainWindow::renderInteractiveEncoder(int index, ImVec2 center, float radius) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    
    char id[32];
    snprintf(id, sizeof(id), "encoder_%d", index);
    ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));
    
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool value_changed = false;
    
    // Handle mouse wheel for encoder increment/decrement
    if (is_hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            encoder_values_[index] += (wheel > 0) ? 1 : -1;
            encoder_values_[index] = std::clamp(encoder_values_[index], -100, 100);
            value_changed = true;
        }
    }
    
    // Handle horizontal drag as alternative
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        static float accumulated_delta = 0.0f;
        accumulated_delta += mouse_delta.x;
        
        if (std::abs(accumulated_delta) > 10.0f) {
            encoder_values_[index] += (accumulated_delta > 0) ? 1 : -1;
            encoder_values_[index] = std::clamp(encoder_values_[index], -100, 100);
            accumulated_delta = 0.0f;
            value_changed = true;
        }
    }
    
    // Handle click for press state
    if (ImGui::IsItemClicked()) {
        encoder_pressed_[index] = !encoder_pressed_[index];
    }
    
    // Update brightness for visual feedback
    float target_brightness = 1.0f;
    if (encoder_pressed_[index]) target_brightness = 1.4f;
    else if (is_hovered) target_brightness = 1.2f;
    
    encoder_brightness_[index] = ImLerp(encoder_brightness_[index], target_brightness, 0.1f);
    
    // Render visual with brightness and value indication
    renderEncoderVisual(center, radius, encoder_values_[index], encoder_pressed_[index], encoder_brightness_[index]);
    
    return value_changed;
}
```

#### TASK 5: State Synchronization with Hardware Interface
**File**: `src/ui/main_window.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - ADD: syncStateToHardware() method to push UI state to hardware interface
  - ADD: State synchronization calls after control updates
  - ADD: Hardware interface reference management
  - VALIDATE: Control changes propagate to plugin parameters
  - IF_FAIL: Check hardware interface callback setup
  - ROLLBACK: Remove synchronization, keep controls visual-only
```

**Implementation**:
```cpp
void DistingNTMainWindow::syncStateToHardware() {
    if (!hardware_interface_) return;
    
    // Sync potentiometer values and press states
    for (int i = 0; i < 3; i++) {
        hardware_interface_->setPotValue(i, pot_values_[i]);
        hardware_interface_->setPotPressed(i, pot_pressed_[i]);
    }
    
    // Sync button states
    for (int i = 0; i < 4; i++) {
        hardware_interface_->setButtonState(i, button_states_[i]);
    }
    
    // Sync encoder values and press states
    for (int i = 0; i < 2; i++) {
        hardware_interface_->setEncoderValue(i, encoder_values_[i]);
        hardware_interface_->setEncoderPressed(i, encoder_pressed_[i]);
    }
}

void DistingNTMainWindow::render() {
    // ... existing window setup ...
    
    bool any_changed = false;
    
    // Render interactive controls
    any_changed |= renderInteractivePots();
    any_changed |= renderInteractiveButtons();
    any_changed |= renderInteractiveEncoders();
    
    // Sync state if any controls changed
    if (any_changed) {
        syncStateToHardware();
    }
    
    // Update brightness animations
    updateBrightness();
    
    // ... rest of rendering ...
}
```

#### TASK 6: Integration with Existing Hardware Interface
**File**: `src/hardware/hardware_interface.h/.cpp`
**Duration**: 45 minutes

```cpp
ACTION src/hardware/hardware_interface.h:
  - ADD: Public setter methods for external state updates
  - ADD: State synchronization interface for main window
  - ENSURE: Thread-safe state updates
  - VALIDATE: External state updates trigger callbacks correctly
  - IF_FAIL: Check for race conditions in state access
  - ROLLBACK: Remove external setters, keep internal state management
```

**Implementation**:
```cpp
// Add to HardwareInterface class
public:
    // External state setters for main window integration
    void setPotValue(int index, float value);
    void setPotPressed(int index, bool pressed);
    void setButtonState(int index, bool state);
    void setEncoderValue(int index, int value);
    void setEncoderPressed(int index, bool pressed);
    
    // State query methods
    float getPotValue(int index) const;
    bool isPotPressed(int index) const;
    bool getButtonState(int index) const;
    int getEncoderValue(int index) const;
    bool isEncoderPressed(int index) const;

private:
    std::mutex state_mutex_; // Thread safety for state updates
```

#### TASK 7: Update Main Application Integration
**File**: `src/main.cpp`
**Duration**: 15 minutes

```cpp
ACTION src/main.cpp:
  - ADD: Hardware interface reference passing to main window
  - ENSURE: Proper initialization order (hardware interface before main window)
  - VALIDATE: Application starts with integrated interactive controls
  - IF_FAIL: Check initialization sequence and shared_ptr usage
  - ROLLBACK: Remove hardware interface connection, run with visual-only controls
```

### Validation Loop

#### Level 1: Control Interaction
```bash
cd emulator/build_gui && ./DistingNTEmulator test_plugins/simple_gain.dylib
# Interactive test:
# - Click and drag each potentiometer vertically
# - Verify knob rotation and brightness feedback
# - Click each button to toggle state
# - Verify button brightness changes
# - Use mouse wheel on encoders
# - Verify encoder value changes
```

#### Level 2: Plugin Parameter Control
```bash
# Load a plugin with known parameters
./DistingNTEmulator test_plugins/simple_gain.dylib
# Verify:
# - Pot changes affect plugin audio output
# - Button states influence plugin behavior  
# - Encoder changes modify plugin parameters
# - Audio processing reflects control changes in real-time
```

#### Level 3: State Persistence
```bash
# Test hot reload behavior
# - Set controls to specific values
# - Trigger plugin hot reload (modify plugin file)
# - Verify control states persist through reload
# - Verify plugin receives current control values
```

#### Level 4: Performance and Responsiveness
```bash
# Monitor during intensive interaction
# - Rapid control manipulation
# - Check frame rate stability (should maintain 60fps)
# - Check audio processing stability (no dropouts)
# - Check memory usage during extended interaction
```

### Debug Strategies

**Interaction Issues**:
```cpp
// Add debug output to control handlers
bool DistingNTMainWindow::renderInteractivePot(int index, ImVec2 center, float radius) {
    // ... control handling ...
    
    #ifdef DEBUG_CONTROLS
    if (value_changed) {
        std::cout << "Pot " << index << " changed to " << pot_values_[index] << std::endl;
    }
    #endif
    
    return value_changed;
}
```

**State Sync Problems**:
```cpp
// Add state validation
void DistingNTMainWindow::syncStateToHardware() {
    #ifdef DEBUG_STATE_SYNC
    std::cout << "Syncing state - Pot 0: " << pot_values_[0] 
              << ", Button 0: " << button_states_[0] << std::endl;
    #endif
    
    // ... synchronization code ...
}
```

**Performance Issues**:
```cpp
// Profile interaction rendering
void DistingNTMainWindow::render() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // ... rendering code ...
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    #ifdef DEBUG_PERFORMANCE
    if (duration.count() > 5000) { // > 5ms
        std::cout << "Slow render: " << duration.count() << " μs" << std::endl;
    }
    #endif
}
```

### Rollback Strategy

**Complete Rollback**:
```bash
git checkout src/ui/main_window.cpp src/ui/main_window.h src/main.cpp
cd build_gui && make -j4
```

**Partial Rollback** (keep visual improvements, remove interaction):
```cpp
// Comment out interaction handling, keep visual enhancements
// #define ENABLE_INTERACTION
#ifdef ENABLE_INTERACTION
    // ... interaction code ...
#endif
```

**Emergency Mode** (visual-only controls):
```cpp
// Disable state synchronization
void DistingNTMainWindow::syncStateToHardware() {
    // Disabled for emergency fallback
    return;
}
```

## Success Metrics

### Functional Requirements
- [ ] **Full Interaction**: All 9 controls (3 pots + 4 buttons + 2 encoders) respond to mouse
- [ ] **Visual Feedback**: Controls brighten when pressed/hovered
- [ ] **State Management**: Control changes propagate to plugin parameters
- [ ] **Real-time Updates**: Audio processing reflects control changes immediately
- [ ] **State Persistence**: Control values maintained across plugin reloads

### User Experience Requirements
- [ ] **Responsive Feel**: Controls respond within 16ms (60fps)
- [ ] **Authentic Behavior**: Pot rotation range matches hardware (7-5 o'clock)
- [ ] **Visual Polish**: Brightness changes smooth and professional
- [ ] **Intuitive Operation**: Mouse interaction feels natural and predictable
- [ ] **Hardware Aesthetic**: Interactive elements maintain authentic appearance

### Technical Requirements
- [ ] **Performance**: No frame rate degradation during interaction
- [ ] **Audio Stability**: No audio dropouts during control manipulation
- [ ] **Memory Efficiency**: No memory leaks during extended interaction
- [ ] **Thread Safety**: Concurrent UI and audio thread access handled correctly
- [ ] **Error Handling**: Graceful degradation if hardware interface unavailable

## Timeline

**Total Estimated Time**: 4-5 hours

1. **Interaction Layer Setup** (90 min): Add interactive state to main window
2. **Potentiometer Controls** (60 min): Mouse drag rotation and press handling
3. **Button Controls** (45 min): Click toggles and brightness feedback  
4. **Encoder Controls** (60 min): Mouse wheel and drag handling
5. **State Synchronization** (30 min): Integration with hardware interface
6. **Hardware Integration** (45 min): Bidirectional state management
7. **Application Integration** (15 min): Main app coordination

## Post-Implementation

### Documentation Updates
- Update README with interactive control usage
- Document control mappings and parameter relationships
- Add developer guide for extending control system

### Future Enhancements
- Parameter metadata integration (names, ranges from plugin)
- Control visibility based on loaded plugin capabilities
- MIDI control surface integration
- Automation recording and playback
- Control presets and state saving