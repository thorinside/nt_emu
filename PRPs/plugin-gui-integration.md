# TASK PRP: Plugin GUI Integration

## Goal
Integrate loaded plugins with the GUI by implementing 30fps display updates, full hardware interface connectivity, and visual feedback for active plugin connections.

## Why
Currently plugins can be loaded but have no visual output or interaction capability. This connects the loaded plugin's draw function to the display and wires up all hardware controls (buttons, pots, encoders) for full plugin interaction.

## What
- Plugin draw function execution at 30fps when plugin loaded
- Complete hardware interface integration (buttons, pots, encoders)
- Visual indication of active CV jack connections
- Real-time display updates synchronized with plugin state

## All Needed Context

### Existing Architecture
- `Emulator` class manages plugin lifecycle via `PluginLoader`
- `ApiShim` provides plugin API implementation with display buffer
- `HardwareInterface` handles UI input events
- `Display` class renders plugin graphics
- Audio processing calls plugin `step()` function in audio thread

### Key Files and Patterns
```cpp
// Plugin interface (from NT SDK)
struct _NT_algorithm {
    void (*step)(_NT_algorithm* algorithm, float** buses, unsigned long numSamples);
    void (*draw)(_NT_algorithm* algorithm);
    // ... other functions
};

// Current audio processing pattern
algorithm->step(algorithm_, bus_ptrs, samples_to_process);

// Display update pattern (currently in emulator.cpp:updateDisplay)
algorithm->draw(algorithm);
display_->updateFromApiState();
```

### Hardware Interface Events
- Button presses: `onButtonPress(int button, bool pressed)`
- Pot changes: `onParameterChange(int parameter, float value)`  
- Encoder changes: `onEncoderChange(int delta)`

### CV Connection State
- Input voltages: `ApiShim::getState().voltage.input_voltages[i]`
- Output voltages: `ApiShim::getState().voltage.output_voltages[i]`
- Connection threshold: voltage > 0.1V indicates active connection

## Implementation Blueprint

### TASK 1: Display Timer and Draw Loop
**File: `src/ui/main_window.h`**
```cpp
class DistingNTMainWindow {
    // Add display update timer
    std::chrono::steady_clock::time_point last_display_update_;
    static constexpr int DISPLAY_FPS = 30;
    static constexpr auto DISPLAY_INTERVAL = std::chrono::milliseconds(1000 / DISPLAY_FPS);
    
    void updatePluginDisplay();
    bool shouldUpdateDisplay();
};
```

**File: `src/ui/main_window.cpp`**
```cpp
void DistingNTMainWindow::render() {
    // ... existing code ...
    
    // Update plugin display at 30fps
    if (shouldUpdateDisplay()) {
        updatePluginDisplay();
    }
    
    // ... rest of render code ...
}
```

### TASK 2: Hardware Interface Integration
**File: `src/ui/main_window.cpp`**
```cpp
// In constructor, set up hardware callbacks
hardware_interface_->setParameterChangeCallback([this](int param, float value) {
    if (emulator_ && emulator_->isPluginLoaded()) {
        emulator_->onParameterChange(param, value);
    }
});

hardware_interface_->setButtonCallback([this](int button, bool pressed) {
    if (emulator_ && emulator_->isPluginLoaded()) {
        emulator_->onButtonPress(button, pressed);
    }
});

hardware_interface_->setEncoderCallback([this](int delta) {
    if (emulator_ && emulator_->isPluginLoaded()) {
        emulator_->onEncoderChange(delta);
    }
});
```

### TASK 3: CV Jack Visual Feedback
**File: `src/ui/main_window.cpp`**
```cpp
void DistingNTMainWindow::renderCVJack(ImVec2 pos, float size, float voltage, bool is_input, int number) {
    // ... existing jack rendering ...
    
    // Add connection indicator
    bool is_connected = std::abs(voltage) > 0.1f;
    if (is_connected) {
        // Draw connection ring around jack
        ImU32 connection_color = IM_COL32(0, 255, 0, 128); // Green glow
        draw_list->AddCircle(pos, size + 8, connection_color, 0, 3.0f);
    }
    
    // ... rest of function ...
}
```

### TASK 4: Interactive Control Updates
**File: `src/ui/main_window.cpp`**
```cpp
bool DistingNTMainWindow::renderInteractivePot(int index, ImVec2 center, float radius) {
    bool changed = false;
    
    // Handle mouse interaction
    if (ImGui::IsMouseHoveringRect(/* pot bounds */)) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            // Calculate new pot value from mouse delta
            float delta = ImGui::GetIO().MouseDelta.y * -0.01f;
            pot_values_[index] = std::clamp(pot_values_[index] + delta, 0.0f, 1.0f);
            changed = true;
        }
        
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            pot_pressed_[index] = true;
            changed = true;
        }
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        pot_pressed_[index] = false;
        changed = true;
    }
    
    return changed;
}
```

## Validation Loop

### Level 1: Compilation and Basic Function
```bash
# Build the project
make -j$(sysctl -n hw.ncpu)

# Verify no compilation errors
echo "Build status: $?"
```

### Level 2: Plugin Loading and Display
```bash
# Test plugin loading
./DistingNTEmulator
# 1. File -> Load Plugin -> select test plugin
# 2. Verify display updates at 30fps
# 3. Check console for "Plugin display updating at 30fps" messages
```

### Level 3: Hardware Interaction
```bash
# Test hardware controls
# 1. Load plugin
# 2. Click pots/buttons in GUI
# 3. Verify parameter changes reflected in plugin state
# 4. Check console for parameter change messages
```

### Level 4: CV Connection Feedback
```bash
# Test CV visualization  
# 1. Start audio
# 2. Send CV signals via VCV Rack
# 3. Verify green rings appear around active CV jacks
# 4. Test both input and output jacks
```

### Level 5: Performance Validation
```bash
# Monitor performance
# 1. Load plugin with complex graphics
# 2. Verify 30fps display updates maintained
# 3. Check CPU usage stays reasonable
# 4. Verify audio thread not affected
```

## Gotchas and Edge Cases

### Threading Considerations
- Display updates run on GUI thread
- Plugin draw() must be thread-safe
- Audio step() runs on audio thread
- Use atomic operations for shared state

### Plugin Validation
- Check plugin has draw function before calling
- Handle plugin crashes gracefully
- Validate parameter indices before forwarding

### Performance Issues
- 30fps updates can be CPU intensive
- Limit display updates only when plugin loaded
- Consider adaptive frame rate based on complexity

### Memory Management
- Plugin lifetime managed by PluginLoader
- Don't cache plugin pointers across load/unload cycles
- Clean up display state when plugin unloaded

## Success Criteria

1. **Display Updates**: Plugin graphics render at stable 30fps
2. **Hardware Integration**: All controls (3 pots, 2 buttons, 1 encoder) functional
3. **Visual Feedback**: CV jacks show green rings when signals present
4. **Performance**: No audio dropouts, smooth GUI updates
5. **Error Handling**: Graceful handling of plugin crashes/unloads

## Technical Implementation Details

### Display Update Strategy
- Use `std::chrono` for precise timing
- Only update when plugin loaded and has draw function
- Separate display timer from audio processing

### Parameter Mapping
- Pot 0 -> Parameter 0 (typically algorithm selection)
- Pot 1 -> Parameter 1 (typically main parameter)
- Pot 2 -> Parameter 2 (typically secondary parameter)
- Button 0 -> Special function (shift/alt mode)
- Button 1 -> Menu/selection
- Encoder -> Navigation/fine adjustment

### CV Connection Detection
- Threshold: |voltage| > 0.1V indicates connection
- Update connection state every display frame
- Visual feedback: animated green ring around connected jacks

This PRP provides complete plugin GUI integration with real-time display updates, full hardware control, and visual feedback systems.