# TASK PRP: Reproduce Disting NT UI Faithfully

## Goal

Transform the current generic hardware controls interface into a pixel-perfect reproduction of the authentic Expert Sleepers Disting NT hardware, including proper layout, visual styling, colors, and CV jack voltage indicators.

## Why

**Business Value:**
- **Authentic Experience**: Users get the exact look and feel of real hardware
- **Professional Appeal**: Pixel-perfect reproduction demonstrates quality and attention to detail
- **Hardware Familiarity**: Real Disting NT users will immediately recognize the interface
- **Visual Debugging**: CV voltage indicators provide immediate feedback for plugin development

**User Impact:**
- Developers can work with familiar hardware layout
- CV connections visualized with color-coded voltage indicators
- Authentic experience reduces learning curve for existing Disting NT users
- Professional appearance suitable for demos and workshops

## What

Transform the current generic UI into an authentic Disting NT reproduction with:

### Success Criteria
- [ ] Dark charcoal background matching hardware
- [ ] White "Expert Sleepers" branding and "disting NT" logo
- [ ] Proper hardware layout: display top-center, 3 knobs below, 4 buttons + 2 encoders bottom
- [ ] CV jack indicators: 12 inputs (left), 6 outputs (right) with voltage visualization
- [ ] Accurate knob and button styling matching hardware
- [ ] Proper proportions and spacing matching reference image
- [ ] CV voltage display: red shades for positive, blue shades for negative voltages
- [ ] Clean, professional interface with no debug information visible

### User-Visible Behavior
1. **Hardware Layout**: Interface matches physical Disting NT exactly
2. **CV Visualization**: Jack indicators show real-time voltage with color coding
3. **Authentic Controls**: Knobs and buttons behave like real hardware
4. **Professional Styling**: Clean, hardware-accurate appearance

## All Needed Context

### Reference Analysis
From the provided reference image, the authentic Disting NT layout features:

**Top Section:**
- Expert Sleepers logo (stylized "Z" with lines)
- Large rectangular display (256x64, cyan text on black)
- "expert sleepers disting NT" branding
- "v1.0.0" version text (bottom left of display)

**Middle Section:**
- 3 large potentiometers with white indicator lines
- 2 smaller encoder knobs (no visible indicators)
- 1 small button on far left

**Bottom Section:**
- **Left side**: 12 CV input jacks arranged in 3 columns (numbered 1-12)
- **Right side**: 6 CV output jacks arranged in 2 columns (numbered 1-6)
- All jacks are 3.5mm with silver/metallic appearance

**Color Scheme:**
- Background: Dark charcoal/black
- Text: White and cyan (display)
- Hardware: Dark gray knobs, silver jacks
- Branding: White text

### Current Implementation Analysis

**Files to modify:**
- `src/main.cpp` - Main UI layout and window management
- `src/hardware/hardware_interface.cpp/.h` - Controls rendering
- `src/hardware/display.cpp/.h` - Display styling
- `src/ui/main_window.cpp/.h` - Overall layout coordination

**Current Issues:**
- Generic ImGui styling instead of hardware-accurate appearance
- Controls laid out vertically instead of matching hardware layout
- No CV jack visualization
- Missing Expert Sleepers branding
- No proper color scheme

### Dependencies
- **ImGui**: Custom drawing for CV jacks and knobs
- **OpenGL**: For custom rendering if needed
- **Hardware state**: CV voltage values for jack indicators

### Known Gotchas

**Layout Challenges:**
- ImGui default styling doesn't match hardware
- Need custom knob rendering for authentic look
- CV jack positioning must be precise
- Display aspect ratio must match 256x64

**Styling Issues:**
- ImGui dark theme needs customization
- Custom colors for voltage indicators
- Font selection for branding text
- Proper spacing and proportions

**Performance Considerations:**
- Custom drawing for 18 CV jacks updated every frame
- Efficient voltage value updates
- Smooth knob interaction

### Existing Patterns

**Current Hardware Interface Pattern** (`hardware_interface.cpp`):
```cpp
void HardwareInterface::renderControlsWindow() {
    ImGui::Begin("Disting NT Hardware", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // Generic layout with vertical arrangement
}
```

**Current Display Pattern** (`display.cpp`):
```cpp
void Display::renderDisplayWindow() {
    ImGui::Begin("Disting NT Display");
    // Basic display with minimal styling
}
```

## Implementation Blueprint

### Task Breakdown

#### TASK 1: Create Authentic Main Window Layout
**File**: `src/ui/main_window.cpp`
**Duration**: 60 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - CREATE: Authentic Disting NT main window class
  - ADD: Fixed window size matching hardware proportions
  - ADD: Dark charcoal background styling
  - ADD: Expert Sleepers branding header
  - VALIDATE: Window opens with proper dimensions
  - IF_FAIL: Check ImGui window flags and styling
  - ROLLBACK: Revert to main.cpp implementation
```

**Implementation**:
```cpp
class DistingNTMainWindow {
public:
    void render() {
        // Set window style
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        
        // Fixed size window matching hardware proportions
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Always);
        ImGui::Begin("Expert Sleepers Disting NT", nullptr, 
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        renderHeader();
        renderDisplay();
        renderControlsSection();
        renderCVSection();
        
        ImGui::End();
        ImGui::PopStyleColor(2);
    }
};
```

#### TASK 2: Implement Header with Branding
**File**: `src/ui/main_window.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - ADD: Expert Sleepers logo rendering
  - ADD: "disting NT" text with proper styling
  - ADD: Centered layout matching hardware
  - VALIDATE: Header appears correctly styled
  - IF_FAIL: Check font rendering and positioning
  - ROLLBACK: Use simple text fallback
```

#### TASK 3: Redesign Display Integration
**File**: `src/hardware/display.cpp`
**Duration**: 45 minutes

```cpp
ACTION src/hardware/display.cpp:
  - MODIFY: Display rendering to match hardware appearance
  - ADD: Cyan text on black background
  - ADD: Proper 256x64 aspect ratio
  - ADD: "v1.0.0" version text overlay
  - VALIDATE: Display shows cyan text with proper proportions
  - IF_FAIL: Check color values and scaling
  - ROLLBACK: Restore original display colors
```

**Implementation**:
```cpp
void Display::renderEmbedded(ImVec2 position, ImVec2 size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Black background
    draw_list->AddRectFilled(
        position, 
        ImVec2(position.x + size.x, position.y + size.y),
        IM_COL32(0, 0, 0, 255)
    );
    
    // Cyan pixels
    ImU32 cyan_color = IM_COL32(0, 255, 255, 255);
    renderPixelsWithColor(draw_list, position, size, cyan_color);
    
    // Version text
    draw_list->AddText(
        ImVec2(position.x + 10, position.y + size.y - 20),
        IM_COL32(100, 100, 100, 255),
        "v1.0.0"
    );
}
```

#### TASK 4: Recreate Hardware Controls Layout
**File**: `src/hardware/hardware_interface.cpp`
**Duration**: 90 minutes

```cpp
ACTION src/hardware/hardware_interface.cpp:
  - REPLACE: Current layout with authentic hardware layout
  - ADD: 3 potentiometers in horizontal row
  - ADD: 2 encoders positioned like hardware
  - ADD: 4 buttons with proper placement
  - ADD: Custom knob rendering with white indicators
  - VALIDATE: Controls match reference image layout
  - IF_FAIL: Check positioning calculations
  - ROLLBACK: Restore original control layout
```

**Layout Implementation**:
```cpp
void HardwareInterface::renderEmbedded(ImVec2 position, ImVec2 size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Calculate positions based on hardware layout
    float knob_y = position.y + 20;
    float knob_spacing = size.x / 4.0f;
    
    // 3 Potentiometers
    for (int i = 0; i < 3; i++) {
        ImVec2 knob_pos(position.x + knob_spacing * (i + 0.5f), knob_y);
        renderAuthenticKnob(draw_list, knob_pos, 30.0f, pot_display_values_[i]);
    }
    
    // 2 Encoders positioned lower
    float encoder_y = knob_y + 100;
    for (int i = 0; i < 2; i++) {
        ImVec2 enc_pos(position.x + size.x * 0.3f + i * 100, encoder_y);
        renderAuthenticEncoder(draw_list, enc_pos, 20.0f, encoder_display_values_[i]);
    }
    
    // 4 Buttons positioned around encoders
    renderAuthenticButton(draw_list, ImVec2(position.x + 50, encoder_y), 0);
    renderAuthenticButton(draw_list, ImVec2(position.x + 50, encoder_y + 40), 1);
    renderAuthenticButton(draw_list, ImVec2(position.x + size.x - 80, encoder_y), 2);
    renderAuthenticButton(draw_list, ImVec2(position.x + size.x - 80, encoder_y + 40), 3);
}
```

#### TASK 5: Implement CV Jack Visualization
**File**: `src/hardware/cv_interface.cpp` (new)
**Duration**: 75 minutes

```cpp
ACTION src/hardware/cv_interface.cpp:
  - CREATE: New CV interface class
  - ADD: 12 input jacks (left side)
  - ADD: 6 output jacks (right side)
  - ADD: Voltage visualization with color coding
  - ADD: Real-time voltage value updates
  - VALIDATE: Jacks display with proper colors
  - IF_FAIL: Check voltage value mapping
  - ROLLBACK: Display placeholder jacks
```

**CV Implementation**:
```cpp
class CVInterface {
public:
    void renderEmbedded(ImVec2 position, ImVec2 size) {
        // Left side: 12 inputs in 3x4 grid
        float jack_size = 25.0f;
        float spacing = 35.0f;
        
        for (int i = 0; i < 12; i++) {
            int row = i / 3;
            int col = i % 3;
            ImVec2 jack_pos(
                position.x + col * spacing,
                position.y + row * spacing
            );
            
            float voltage = getInputVoltage(i);
            renderCVJack(jack_pos, jack_size, voltage, true, i + 1);
        }
        
        // Right side: 6 outputs in 2x3 grid
        float right_x = position.x + size.x - 3 * spacing;
        for (int i = 0; i < 6; i++) {
            int row = i / 2;
            int col = i % 2;
            ImVec2 jack_pos(
                right_x + col * spacing,
                position.y + row * spacing
            );
            
            float voltage = getOutputVoltage(i);
            renderCVJack(jack_pos, jack_size, voltage, false, i + 1);
        }
    }
    
private:
    void renderCVJack(ImVec2 pos, float size, float voltage, bool is_input, int number) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Jack body (metallic silver)
        ImU32 jack_color = IM_COL32(180, 180, 180, 255);
        draw_list->AddCircleFilled(pos, size/2, jack_color);
        
        // Voltage indicator ring
        ImU32 voltage_color = getVoltageColor(voltage);
        draw_list->AddCircle(pos, size/2 + 3, voltage_color, 0, 3.0f);
        
        // Number label
        char label[4];
        snprintf(label, sizeof(label), "%d", number);
        draw_list->AddText(
            ImVec2(pos.x - 5, pos.y + size/2 + 5),
            IM_COL32(200, 200, 200, 255),
            label
        );
    }
    
    ImU32 getVoltageColor(float voltage) {
        if (voltage > 0) {
            // Red shades for positive
            int intensity = (int)(voltage * 50) + 100;
            intensity = std::min(255, std::max(100, intensity));
            return IM_COL32(intensity, 0, 0, 255);
        } else if (voltage < 0) {
            // Blue shades for negative
            int intensity = (int)(-voltage * 50) + 100;
            intensity = std::min(255, std::max(100, intensity));
            return IM_COL32(0, 0, intensity, 255);
        } else {
            // Gray for zero
            return IM_COL32(80, 80, 80, 255);
        }
    }
};
```

#### TASK 6: Integrate All Components
**File**: `src/main.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/main.cpp:
  - MODIFY: Main render loop to use new layout
  - INTEGRATE: All components in single authentic window
  - REMOVE: Separate debug windows
  - VALIDATE: Complete interface renders as single window
  - IF_FAIL: Check component initialization order
  - ROLLBACK: Restore separate window layout
```

#### TASK 7: Custom Styling and Polish
**File**: `src/ui/styling.cpp` (new)
**Duration**: 45 minutes

```cpp
ACTION src/ui/styling.cpp:
  - CREATE: Custom ImGui styling for hardware look
  - ADD: Proper fonts and colors
  - ADD: Custom knob and button rendering
  - ADD: Professional appearance polish
  - VALIDATE: Interface looks professional and authentic
  - IF_FAIL: Revert to default ImGui styling
  - ROLLBACK: Use basic styling
```

### Validation Loop

#### Level 1: Layout Verification
```bash
cd emulator/build_gui && ./DistingNTEmulator
# Manual verification:
# - Single window opens with hardware layout
# - Display positioned at top center
# - Controls match reference image positions
# - CV jacks visible on left and right sides
```

#### Level 2: Visual Accuracy
```bash
# Compare against reference image:
# - Background color matches hardware
# - Knob and button styling authentic
# - CV jack layout matches hardware
# - Branding text positioned correctly
```

#### Level 3: Functionality Test
```bash
./DistingNTEmulator test_plugins/simple_gain.dylib
# Interactive verification:
# - All controls respond to mouse interaction
# - Display updates with plugin content
# - CV voltage indicators change color
# - Professional appearance maintained
```

#### Level 4: Performance Check
```bash
# Monitor performance during interaction:
# - Smooth knob dragging
# - Real-time CV voltage updates
# - No frame rate drops
# - Memory usage stable
```

### Debug Strategies

**Layout Issues**:
```cpp
// Add debug overlays to check positioning
#ifdef DEBUG_LAYOUT
draw_list->AddRect(expected_pos, expected_size, IM_COL32(255, 0, 0, 128));
#endif
```

**Color Problems**:
```cpp
// Test voltage color mapping
void testVoltageColors() {
    for (float v = -10.0f; v <= 10.0f; v += 0.5f) {
        ImU32 color = getVoltageColor(v);
        printf("Voltage %.1f -> Color 0x%08X\n", v, color);
    }
}
```

**Performance Issues**:
```cpp
// Profile custom drawing
auto start = std::chrono::high_resolution_clock::now();
renderAllCVJacks();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
if (duration.count() > 1000) {
    printf("CV rendering took %ld μs\n", duration.count());
}
```

### Rollback Strategy

**Complete Rollback**:
```bash
git checkout src/main.cpp src/hardware/hardware_interface.cpp src/hardware/display.cpp
cd build_gui && make -j4
```

**Partial Rollback**:
```bash
# Keep new layout but disable custom styling
cmake -DUSE_CUSTOM_STYLING=OFF ..
```

**Emergency Fallback**:
```bash
# Always preserve working version
cp DistingNTEmulator DistingNTEmulator_backup_before_ui_changes
```

## Success Metrics

### Visual Accuracy
- [ ] **Hardware Match**: Layout matches reference image within 95% accuracy
- [ ] **Color Scheme**: Authentic dark background with proper accent colors
- [ ] **Branding**: Expert Sleepers logo and text properly positioned
- [ ] **Proportions**: Display and controls sized correctly relative to each other

### Functional Requirements
- [ ] **CV Visualization**: All 18 jacks display with color-coded voltage indicators
- [ ] **Real-time Updates**: Voltage colors change smoothly during plugin operation
- [ ] **Control Response**: All knobs, buttons, and encoders respond authentically
- [ ] **Performance**: Maintains 60fps with all visual elements active

### User Experience
- [ ] **Professional Look**: Interface suitable for demonstrations and workshops
- [ ] **Hardware Familiarity**: Real Disting NT users immediately recognize layout
- [ ] **Visual Clarity**: All elements clearly visible and properly labeled
- [ ] **Intuitive Operation**: Controls behave as expected without explanation

## Timeline

**Total Estimated Time**: 5-6 hours

1. **Main Window Layout** (60 min): Create authentic window structure
2. **Header/Branding** (30 min): Add Expert Sleepers branding
3. **Display Integration** (45 min): Style display to match hardware
4. **Controls Layout** (90 min): Recreate authentic control positions
5. **CV Visualization** (75 min): Implement voltage-indicating jacks
6. **Integration** (30 min): Combine all components
7. **Styling/Polish** (45 min): Final appearance refinements

## Post-Implementation

### Documentation Updates
- Add authentic UI screenshots to README
- Document CV voltage visualization feature
- Create user guide showing hardware control mapping

### Future Enhancements
- Add animation effects for button presses
- Implement CV cable visualization connecting jacks
- Add plugin-specific branding on display
- Create themes for different hardware variants