# TASK PRP: UI Event Routing to Hosted Plugins

## Goal
Implement complete UI event routing from VCV hardware controls (pots, buttons, encoders) to hosted NT plugins via the customUi API, enabling plugins to provide custom interactive interfaces.

## Why
Currently, the VCV plugin doesn't utilize the NT API's customUi system, preventing hosted plugins from implementing custom control schemes. This implementation will enable plugins like the DnB Sequencer to respond to hardware controls for pattern selection, probability adjustments, and other real-time interactions.

## What
### User-Visible Behavior
1. **Button 1 Behavior Change**:
   - Short press: Sent to plugin if customUi available
   - Long press (>500ms): Enter parameter menu as before
   
2. **Custom UI Event Routing**:
   - All pot turns, button presses, encoder rotations sent to plugin
   - Plugin can respond to control changes in real-time
   - Soft takeover for pot values via setupUi

3. **Hardware Control Mapping**:
   - 3 pots (L/C/R) with normalized values 0.0-1.0
   - 4 buttons with press/release detection
   - 3 pot buttons (pressable pots) with pressed and released events
   - 2 encoders with delta values
   - 2 encoder buttons with pressed and released events

### Technical Requirements
- Track control state changes for change detection
- Maintain previous button states for edge detection
- Call setupUi on plugin load for pot initialization
- Call setupUi on plugin when leaving parameter menu
- Route events only when plugin has customUi capability
- Thread-safe state management between UI and audio threads

## Context

### Documentation
```yaml
context:
  docs:
    - file: emulator/include/distingnt/api.h
      focus: _NT_uiData structure and customUi function signatures

  patterns:
    - file: emulator/src/ui/NtEmu.cpp
      copy: |
        // Current button handling pattern
        if (event.type == Event::Button && event.data0 == 1) {
            if (event.data1 == true) {
                _menuManager.toggleMenu();
            }
        }

    - file: vcv-plugin/src/EmulatorCore.hpp
      copy: |
        // Hardware state tracking
        struct VCVHardwareState {
            float pots[3] = {0.5f, 0.5f, 0.5f};
            bool buttons[4] = {false};
            bool potButtons[3] = {false};
            bool encoderButtons[2] = {false};
        };

  gotchas:
    - issue: "Thread safety between UI and audio threads"
      fix: "Use atomic operations or mutex protection for shared state"
    
    - issue: "Pot soft takeover requires initial values"
      fix: "Call setupUi when plugin loads and sync pot positions"
    
    - issue: "Button edge detection needs previous state"
      fix: "Maintain lastButtons state across frames"
    
    - issue: "Controls bitmask indicates what changed"
      fix: "Set bits only for controls that changed this frame"
```

### NT API Reference
```cpp
// UI data passed to customUi
struct _NT_uiData {
    float pots[3];        // Current pot positions [0.0-1.0]
    uint16_t controls;    // Bitmask of changed controls
    uint16_t lastButtons; // Previous button states
    int8_t encoders[2];   // Encoder deltas: -1, 0, or +1
    uint8_t unused[2];
};

// Control bit definitions
enum _NT_controls {
    kNT_button1 = (1 << 0),
    kNT_button2 = (1 << 1),
    kNT_button3 = (1 << 2),
    kNT_button4 = (1 << 3),
    kNT_potButtonL = (1 << 4),
    kNT_potButtonC = (1 << 5),
    kNT_potButtonR = (1 << 6),
    kNT_encoderButtonL = (1 << 7),
    kNT_encoderButtonR = (1 << 8),
    kNT_encoderL = (1 << 9),
    kNT_encoderR = (1 << 10),
    kNT_potL = (1 << 11),
    kNT_potC = (1 << 12),
    kNT_potR = (1 << 13),
};
```

## Implementation Blueprint

### Core Changes Overview
1. Add customUi state tracking to EmulatorCore
2. Implement control change detection
3. Add long-press detection for Button 1
4. Call customUi during processHardwareChanges
5. Initialize pots via setupUi on plugin load

### Pseudo-code Structure
```cpp
// EmulatorCore additions
class EmulatorCore {
    // UI state tracking
    struct CustomUiState {
        uint16_t lastButtons = 0;
        float lastPots[3] = {0.5f, 0.5f, 0.5f};
        int8_t encoderDeltas[2] = {0, 0};
        bool hasCustomUi = false;
        
        // Button 1 long press tracking
        double button1PressTime = 0.0;
        bool button1LongPressHandled = false;
    };
    CustomUiState uiState;
    
    // Check and cache customUi availability
    void checkCustomUiSupport();
    
    // Build and send UI data to plugin
    void sendCustomUiEvents();
};
```

## Task Breakdown

### ACTION vcv-plugin/src/EmulatorCore.hpp:
  - OPERATION: Add CustomUiState struct and member variable
  - VALIDATE: `grep -q "struct CustomUiState" vcv-plugin/src/EmulatorCore.hpp`
  - IF_FAIL: Check syntax errors in struct definition
  - ROLLBACK: Remove struct definition

### ACTION vcv-plugin/src/EmulatorCore.hpp:
  - OPERATION: Add checkCustomUiSupport() and sendCustomUiEvents() method declarations
  - VALIDATE: `make src/EmulatorCore.cpp.o`
  - IF_FAIL: Check method signatures match implementation
  - ROLLBACK: Remove method declarations

### ACTION vcv-plugin/src/EmulatorCore.cpp:
  - OPERATION: Implement checkCustomUiSupport() to query plugin capabilities
  - VALIDATE: `grep -A 10 "checkCustomUiSupport" vcv-plugin/src/EmulatorCore.cpp`
  - IF_FAIL: Verify plugin pointer and factory access
  - ROLLBACK: Stub out with empty implementation

### ACTION vcv-plugin/src/EmulatorCore.cpp:
  - OPERATION: Implement sendCustomUiEvents() with control change detection
  - VALIDATE: Test with DnB Sequencer plugin loaded
  - IF_FAIL: Add debug logging to trace event flow
  - ROLLBACK: Disable customUi calls

### ACTION vcv-plugin/src/EmulatorCore.cpp:
  - OPERATION: Modify processHardwareChanges() to handle Button 1 long press
  - VALIDATE: Test short vs long press behavior
  - IF_FAIL: Adjust timing threshold
  - ROLLBACK: Revert to immediate menu toggle

### ACTION vcv-plugin/src/EmulatorCore.cpp:
  - OPERATION: Call setupUi during plugin initialization
  - VALIDATE: Check pot values sync correctly
  - IF_FAIL: Debug setupUi parameter passing
  - ROLLBACK: Skip pot initialization

### ACTION vcv-plugin/src/DistingNT.cpp:
  - OPERATION: Update encoder handling to accumulate deltas for customUi
  - VALIDATE: Test encoder response in plugin
  - IF_FAIL: Check delta calculation logic
  - ROLLBACK: Use direct encoder values

### ACTION test/test_custom_ui.cpp:
  - OPERATION: Create test harness for custom UI events
  - VALIDATE: `make test && ./test/test_custom_ui`
  - IF_FAIL: Fix test compilation
  - ROLLBACK: Manual testing only

## Validation Loop

```bash
# 1. Build validation
make clean && make install

# 2. Ask user to test gainCustomUi plugin

# 3. Launch VCV Rack with debug
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

# 4. Test checklist
# - Button 1 long press: Opens parameter menu
# - Center pot: Changes gain

# 5. Debug output verification
grep "customUi called" /Users/nealsanche/Library/Application Support/Rack2/log.txt
grep "setupUi called" /Users/nealsanche/Library/Application Support/Rack2/log.txt
```

## Success Criteria
- [ ] Plugin customUi receives all hardware events
- [ ] Button 1 long press (>500ms) opens menu
- [ ] Button 1 short press sent to plugin
- [ ] Encoder deltas calculated correctly
- [ ] Pot values normalized 0.0-1.0
- [ ] Edge detection works for all buttons
- [ ] Controls bitmask only set for changes
- [ ] setupUi initializes pot positions
- [ ] Thread-safe operation
- [ ] No performance degradation

## Debug Strategies
1. **Event Logging**: Add INFO() for all customUi calls
2. **State Dump**: Print _NT_uiData contents before sending
3. **Timing Debug**: Log button press durations
4. **Control Tracing**: Log which controls bits are set
5. **Thread Analysis**: Use sanitizers for race conditions

## Edge Cases
- Plugin removed while button pressed
- Rapid button presses near threshold
- Multiple simultaneous control changes
- Encoder overflow/underflow
- Plugin without customUi but with setupUi
- Menu open while receiving events
- Preset changes during interaction

## Implementation Notes
- Start with basic event routing, add long press later
- Use atomic operations for thread safety
- Consider caching hasCustomUi result
- Test with multiple plugins
- Document the customUi contract for plugin developers

## Code Snippets

### Control Change Detection Pattern
```cpp
// Build controls bitmask
uint16_t controls = 0;

// Check pot changes
for (int i = 0; i < 3; i++) {
    if (fabsf(currentState.pots[i] - uiState.lastPots[i]) > 0.001f) {
        controls |= (kNT_potL << i);
        uiState.lastPots[i] = currentState.pots[i];
    }
}

// Check button changes (edge detection)
if (currentState.buttons[0] && !wasPressed) {
    controls |= kNT_button1;
}
```

### Long Press Detection
```cpp
if (currentState.buttons[0]) {
    if (uiState.button1PressTime == 0.0) {
        uiState.button1PressTime = system::getTime();
    } else if (!uiState.button1LongPressHandled && 
               (system::getTime() - uiState.button1PressTime) > 0.5) {
        // Long press detected
        toggleParameterMenu();
        uiState.button1LongPressHandled = true;
    }
} else {
    if (uiState.button1PressTime > 0.0 && !uiState.button1LongPressHandled) {
        // Short press - send to customUi
        controls |= kNT_button1;
    }
    uiState.button1PressTime = 0.0;
    uiState.button1LongPressHandled = false;
}
```
