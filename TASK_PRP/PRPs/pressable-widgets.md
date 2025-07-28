# TASK PRP: Pressable VCV Widgets Implementation

## Goal
Create two custom VCV Rack widgets: a pressable pot (potentiometer) and a pressable continuous encoder, both with visual feedback for press states.

## Why
The DistingNT module requires interactive controls that combine rotation and press functionality to match the hardware interface. These widgets will enable more intuitive parameter control and menu navigation.

## What
### User-Visible Behavior
1. **Pressable Pot Widget**:
   - Rotates from -160° to +160° (320° total range)
   - Outputs 0.0 to 1.0 (0.5 when pointing up at 0°)
   - Dark grey appearance, lighter when pressed
   - Momentary press behavior
   - Visual line indicator for position

2. **Pressable Continuous Encoder**:
   - Infinite rotation (no limits)
   - Outputs -1 for CCW, +1 for CW rotation
   - Dark grey appearance, lighter when pressed
   - Momentary press behavior
   - Visual rotation feedback

### Technical Requirements
- Extend existing VCV widget classes (app::Knob)
- Handle both drag (rotation) and click (press) events
- Thread-safe state management
- Efficient rendering with visual feedback
- C++11 compatible (VCV standard)

## Context

### Documentation
```yaml
context:
  docs:
    - url: https://vcvrack.com/manual/PluginDevelopmentTutorial
      focus: Widget event handling and custom widgets
    
    - url: https://github.com/VCVRack/Rack/blob/v2/include/app/Knob.hpp
      focus: Base Knob class interface and event methods

  patterns:
    - file: vcv-plugin/src/InfiniteEncoder.hpp
      copy: |
        // Existing encoder pattern - extends SvgKnob
        struct InfiniteEncoder : app::SvgKnob {
            int currentStep = 0;
            float accumulatedDelta = 0.0f;
            
            void onDragMove(const DragMoveEvent& e) override {
                // Accumulate rotation and convert to steps
            }
        };

    - file: vcv-plugin/Rack-SDK/include/app/SvgButton.hpp
      copy: |
        // Button press handling pattern
        void onButton(const ButtonEvent& e) {
            if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT)
                // Handle press
        }

  gotchas:
    - issue: "VCV widgets must handle events in specific order"
      fix: "Always call parent class event handlers first"
    
    - issue: "Visual updates require dirty flag"
      fix: "Call fb->dirty = true after state changes"
    
    - issue: "Thread safety for audio vs UI threads"
      fix: "Use atomic operations or proper synchronization"
    
    - issue: "SVG rendering performance"
      fix: "Consider FramebufferWidget for complex visuals"
```

### Existing Infrastructure
- EmulatorCore already has encoder press methods:
  - `pressEncoder(int encoder)`
  - `releaseEncoder(int encoder)`
  - `turnEncoder(int encoder, int delta)`
- DistingNT.cpp has encoder handling infrastructure
- ComponentLibrary provides base widget styles

## Implementation Blueprint

### File Structure
```
vcv-plugin/src/widgets/
├── PressablePot.hpp       # Pressable potentiometer widget
├── PressablePot.cpp       # Implementation
├── PressableEncoder.hpp   # Pressable continuous encoder
└── PressableEncoder.cpp   # Implementation
```

### Core Classes
```cpp
// PressablePot.hpp
struct PressablePot : app::Knob {
    bool pressed = false;
    float pressValue = 0.0f;
    
    // Visual components
    widget::FramebufferWidget* fb;
    widget::SvgWidget* knobSvg;
    
    // Event handlers
    void onButton(const ButtonEvent& e) override;
    void onDragMove(const DragMoveEvent& e) override;
    void draw(const DrawArgs& args) override;
    
    // State management
    void setPressed(bool state);
    float getRotationAngle();
};

// PressableEncoder.hpp  
struct PressableEncoder : app::Knob {
    bool pressed = false;
    float lastAngle = 0.0f;
    int stepOutput = 0;
    
    // Event handlers
    void onButton(const ButtonEvent& e) override;
    void onDragMove(const DragMoveEvent& e) override;
    
    // Encoder logic
    int calculateSteps(float deltaAngle);
};
```

## Task Breakdown

### ACTION vcv-plugin/src/widgets/:
  - OPERATION: Create widgets directory
  - VALIDATE: `test -d vcv-plugin/src/widgets`
  - IF_FAIL: Check current directory with `pwd`
  - ROLLBACK: `rm -rf vcv-plugin/src/widgets`

### ACTION vcv-plugin/src/widgets/PressablePot.hpp:
  - OPERATION: Create pressable pot header with class definition
  - VALIDATE: `grep -q "struct PressablePot" vcv-plugin/src/widgets/PressablePot.hpp`
  - IF_FAIL: Check file creation permissions
  - ROLLBACK: Remove file

### ACTION vcv-plugin/src/widgets/PressablePot.cpp:
  - OPERATION: Implement pot behavior with press handling
  - VALIDATE: `make src/widgets/PressablePot.cpp.o`
  - IF_FAIL: Check compilation errors, missing includes
  - ROLLBACK: Revert to header only

### ACTION vcv-plugin/src/widgets/PressableEncoder.hpp:
  - OPERATION: Create encoder header with infinite rotation
  - VALIDATE: `grep -q "struct PressableEncoder" vcv-plugin/src/widgets/PressableEncoder.hpp`
  - IF_FAIL: Check syntax errors
  - ROLLBACK: Remove file

### ACTION vcv-plugin/src/widgets/PressableEncoder.cpp:
  - OPERATION: Implement encoder step calculation and press
  - VALIDATE: `make src/widgets/PressableEncoder.cpp.o`
  - IF_FAIL: Debug step calculation logic
  - ROLLBACK: Simplify to basic rotation

### ACTION vcv-plugin/src/DistingNT.cpp:
  - OPERATION: Add widget includes and usage examples
  - VALIDATE: `make clean && make`
  - IF_FAIL: Check integration points and includes
  - ROLLBACK: Comment out widget usage

### ACTION test/manual_widget_test.cpp:
  - OPERATION: Create visual test module for widgets
  - VALIDATE: Load in VCV Rack and test interaction
  - IF_FAIL: Add debug output to trace events
  - ROLLBACK: Use existing test infrastructure

## Validation Loop

```bash
# 1. Syntax validation
make clean && make

# 2. Component tests
make src/widgets/PressablePot.cpp.o
make src/widgets/PressableEncoder.cpp.o

# 3. Integration test
cp dist/DistingNT/plugin.dylib ~/.Rack2/plugins/DistingNT/
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

# 4. Visual validation
# - Check pot rotation range (-160° to +160°)
# - Verify 0.5 output at center position
# - Test press visual feedback (color change)
# - Confirm encoder infinite rotation
# - Verify encoder step outputs (-1/+1)

# 5. Performance check
# Monitor CPU usage during widget interaction
```

## Success Criteria
- [ ] Pot rotates smoothly with correct range
- [ ] Pot outputs 0.0-1.0 with 0.5 at center
- [ ] Pot shows visual press feedback
- [ ] Encoder rotates infinitely
- [ ] Encoder outputs discrete -1/+1 steps
- [ ] Encoder shows visual press feedback
- [ ] Both widgets handle simultaneous press+drag
- [ ] No performance degradation
- [ ] Thread-safe operation
- [ ] Clean integration with existing module

## Debug Strategies
1. **Event Tracing**: Add INFO() logs to trace event flow
2. **Visual Debug**: Draw debug overlays showing internal state
3. **Step Testing**: Create unit tests for encoder step calculation
4. **Thread Analysis**: Use sanitizers to check thread safety
5. **Performance Profile**: Use Tracy or similar for bottlenecks

## Edge Cases
- Rapid clicking while dragging
- Very fast rotation (encoder step accumulation)
- Window focus loss during interaction
- Parameter automation conflicts
- Preset loading during interaction
- Multiple simultaneous widget interactions

## Implementation Notes
- Start with basic functionality, add visuals later
- Use existing InfiniteEncoder as reference
- Consider FramebufferWidget for efficient rendering
- Test on both macOS and other platforms
- Document widget API for future use