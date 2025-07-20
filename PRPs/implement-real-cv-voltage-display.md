# TASK PRP: Implement Real CV Voltage Display

## Goal

Replace the dummy CV voltage arrays in the main window with real-time audio signal values from the plugin I/O buffers, providing accurate voltage visualization for the 12 input and 6 output CV jacks.

## Why

**Business Value:**
- **Real-time Monitoring**: Developers can see actual signal levels flowing through their plugins
- **Debug Capability**: Visual feedback helps identify signal flow issues and parameter ranges
- **Hardware Accuracy**: Emulator behavior matches real Disting NT hardware voltage monitoring
- **Professional Tool**: Essential feature for serious plugin development workflows

**User Impact:**
- Plugin developers can monitor input/output signal levels in real-time
- Visual feedback confirms plugin is processing audio correctly
- Debugging becomes much easier with live voltage displays
- Authentic hardware experience enhances development workflow

## What

Transform the current static dummy voltage displays into live signal monitoring by:

### Success Criteria
- [ ] All 12 input CV jacks show real-time input signal voltages
- [ ] All 6 output CV jacks show real-time output signal voltages  
- [ ] Voltage values update at smooth frame rate (30-60 FPS)
- [ ] Color coding accurately reflects signal polarity and magnitude
- [ ] Zero voltage displays as neutral gray color
- [ ] Voltage range properly maps to -10V to +10V scale
- [ ] No performance impact on audio processing

### User-Visible Behavior
1. **Input Monitoring**: CV input jacks glow with colors representing input signal levels
2. **Output Monitoring**: CV output jacks glow with colors representing plugin output levels
3. **Real-time Updates**: Colors change smoothly as audio signals change
4. **Accurate Mapping**: Voltage colors match actual signal voltages from plugin
5. **Performance**: No audio dropouts or GUI lag during voltage monitoring

## All Needed Context

### Current Architecture Analysis

**Existing CV Jack Rendering:**
- Complete visual CV jack rendering in `renderCVJack()` method
- Color-coded voltage visualization with `getVoltageColor()` 
- 18 total jacks: 12 inputs (3 rows × 4) + 6 outputs (3 rows × 2)
- Dummy voltage arrays currently used for demonstration

**Audio Processing Pipeline:**
- Audio engine receives input buffers from PortAudio
- Plugin processes audio via `algorithm->step(buffers, numSamples)`
- Output buffers contain processed audio from plugin
- ApiShim manages state between UI thread and audio thread

**Current Files:**
- `src/ui/main_window.cpp/.h` - CV jack rendering and dummy voltage arrays
- `src/core/audio_engine.cpp/.h` - Audio processing and plugin execution
- `src/core/api_shim.cpp/.h` - State management between threads
- `src/core/emulator.cpp/.h` - Coordination between components

### Dependencies

**Audio Engine Integration:**
- Need access to current audio buffer values from plugin processing
- Must handle thread safety between audio and UI threads
- Should use peak detection or RMS calculation for stable readings

**State Management:**
- ApiShim already handles state synchronization
- Need to add voltage monitoring to state structure
- Must handle variable buffer sizes and sample rates

### Known Gotchas

**Thread Safety Issues:**
- Audio processing runs on separate high-priority thread
- UI updates on main thread at different frequency (60 FPS vs 48kHz)
- Must avoid blocking audio thread with UI operations
- Race conditions possible when reading voltage values

**Performance Considerations:**
- Audio processing is time-critical (cannot add latency)
- Voltage calculations must be efficient
- UI updates should not impact audio buffer processing
- Need to balance update frequency vs performance

**Signal Processing Challenges:**
- Audio signals change rapidly (sample-level resolution)
- Need smoothing/averaging for stable visual display
- Peak detection vs RMS vs instantaneous values
- Handling silence vs very low-level signals

### Existing Patterns

**Current Dummy Implementation** (`main_window.cpp`):
```cpp
float DistingNTMainWindow::getInputVoltage(int index) {
    // TODO: Get actual input voltage from emulator
    // For now, return dummy values for demonstration
    static float dummy_voltages[12] = {
        2.5f, -1.8f, 0.0f, 4.2f, -3.1f, 1.0f,
        0.5f, -0.8f, 3.7f, -2.2f, 0.0f, 1.5f
    };
    
    if (index >= 0 && index < 12) {
        return dummy_voltages[index];
    }
    return 0.0f;
}
```

**Audio Engine Buffer Access** (`audio_engine.cpp`):
```cpp
void AudioEngine::audioCallback(const float* input, float* output, unsigned long frameCount) {
    if (algorithm_ && algorithm_->step) {
        // Prepare buffers for plugin
        float* buffers[18]; // 12 inputs + 6 outputs
        
        // Set up input buffers
        for (int i = 0; i < 12; i++) {
            buffers[i] = input_buffers_[i];
        }
        
        // Set up output buffers  
        for (int i = 0; i < 6; i++) {
            buffers[12 + i] = output_buffers_[i];
        }
        
        // Process audio through plugin
        algorithm_->step(algorithm_, buffers, frameCount);
    }
}
```

**State Management Pattern** (`api_shim.cpp`):
```cpp
struct ApiState {
    HardwareState hardware;
    DisplayState display;
    // Could add: VoltageState voltage;
};

static ApiState& getState() {
    static ApiState state;
    return state;
}
```

## Implementation Blueprint

### Task Breakdown

#### TASK 1: Add Voltage Monitoring to State Management
**File**: `src/core/api_shim.h/.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/core/api_shim.h:
  - ADD: VoltageState structure with input/output voltage arrays
  - ADD: Voltage state to main ApiState structure
  - ENSURE: Thread-safe access patterns for voltage data
  - VALIDATE: State structure compiles and initializes correctly
  - IF_FAIL: Check structure alignment and threading annotations
  - ROLLBACK: Remove voltage state additions, keep existing API
```

**Implementation**:
```cpp
struct VoltageState {
    std::atomic<float> input_voltages[12];   // Thread-safe voltage readings
    std::atomic<float> output_voltages[6];   // Thread-safe voltage readings
    std::atomic<bool> monitoring_enabled;    // Enable/disable monitoring
    
    VoltageState() : monitoring_enabled(true) {
        // Initialize all voltages to zero
        for (int i = 0; i < 12; i++) input_voltages[i] = 0.0f;
        for (int i = 0; i < 6; i++) output_voltages[i] = 0.0f;
    }
};

struct ApiState {
    HardwareState hardware;
    DisplayState display;
    VoltageState voltage;  // Add voltage monitoring
};
```

#### TASK 2: Implement Voltage Calculation in Audio Engine
**File**: `src/core/audio_engine.cpp/.h`
**Duration**: 45 minutes

```cpp
ACTION src/core/audio_engine.h:
  - ADD: Peak detection variables for input/output channels
  - ADD: Voltage calculation methods (peak detection, RMS, smoothing)
  - ADD: Voltage update frequency control (avoid UI thread blocking)
  - VALIDATE: Audio engine builds with new voltage monitoring code
  - IF_FAIL: Check atomic operations and audio thread safety
  - ROLLBACK: Remove voltage calculation code, restore audio-only processing
```

**Implementation**:
```cpp
class AudioEngine {
private:
    // Voltage monitoring state
    float input_peak_detectors_[12];     // Peak hold values for inputs
    float output_peak_detectors_[6];     // Peak hold values for outputs
    float peak_decay_rate_;              // How fast peaks decay
    int voltage_update_counter_;         // Update voltage state every N buffers
    static constexpr int VOLTAGE_UPDATE_INTERVAL = 16; // Update every 16 buffers (~21ms at 64 samples)
    
    void updateVoltageMonitoring(const float* input, float* output, unsigned long frameCount);
    float calculateRMS(const float* buffer, unsigned long frameCount);
    float calculatePeak(const float* buffer, unsigned long frameCount);
    void updatePeakDetector(float& detector, float new_value);
};

void AudioEngine::updateVoltageMonitoring(const float* input, float* output, unsigned long frameCount) {
    // Only update voltage monitoring every N buffers to avoid UI thread pressure
    voltage_update_counter_++;
    if (voltage_update_counter_ < VOLTAGE_UPDATE_INTERVAL) return;
    voltage_update_counter_ = 0;
    
    auto& voltage_state = ApiShim::getState().voltage;
    if (!voltage_state.monitoring_enabled) return;
    
    // Update input voltage monitoring
    for (int i = 0; i < 12; i++) {
        if (i < input_channels_) {
            float rms = calculateRMS(&input[i * frameCount], frameCount);
            updatePeakDetector(input_peak_detectors_[i], rms);
            voltage_state.input_voltages[i] = input_peak_detectors_[i] * 10.0f; // Scale to ±10V
        } else {
            voltage_state.input_voltages[i] = 0.0f;
        }
    }
    
    // Update output voltage monitoring  
    for (int i = 0; i < 6; i++) {
        if (i < output_channels_) {
            float rms = calculateRMS(&output[i * frameCount], frameCount);
            updatePeakDetector(output_peak_detectors_[i], rms);
            voltage_state.output_voltages[i] = output_peak_detectors_[i] * 10.0f; // Scale to ±10V
        } else {
            voltage_state.output_voltages[i] = 0.0f;
        }
    }
}
```

#### TASK 3: Update Main Window to Use Real Voltage Data
**File**: `src/ui/main_window.cpp/.h`
**Duration**: 30 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - REPLACE: Dummy voltage arrays with ApiShim voltage state access
  - UPDATE: getInputVoltage() and getOutputVoltage() methods
  - ADD: Voltage monitoring enable/disable toggle (for performance)
  - VALIDATE: CV jacks display real voltage values from audio processing
  - IF_FAIL: Check thread safety and ApiShim integration
  - ROLLBACK: Restore dummy voltage arrays for visual functionality
```

**Implementation**:
```cpp
float DistingNTMainWindow::getInputVoltage(int index) {
    if (index >= 0 && index < 12) {
        auto& voltage_state = ApiShim::getState().voltage;
        return voltage_state.input_voltages[index].load();
    }
    return 0.0f;
}

float DistingNTMainWindow::getOutputVoltage(int index) {
    if (index >= 0 && index < 6) {
        auto& voltage_state = ApiShim::getState().voltage;
        return voltage_state.output_voltages[index].load();
    }
    return 0.0f;
}

// Add voltage monitoring controls to UI
void DistingNTMainWindow::renderVoltageControls() {
    auto& voltage_state = ApiShim::getState().voltage;
    bool monitoring = voltage_state.monitoring_enabled.load();
    
    if (ImGui::Checkbox("CV Voltage Monitoring", &monitoring)) {
        voltage_state.monitoring_enabled.store(monitoring);
    }
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable/disable real-time CV voltage display\n(disable for better performance)");
    }
}
```

#### TASK 4: Add Voltage Calculation Utilities
**File**: `src/core/audio_engine.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/core/audio_engine.cpp:
  - ADD: RMS calculation for smooth voltage readings
  - ADD: Peak detection with decay for responsive but stable display
  - ADD: Voltage scaling from audio range to ±10V hardware range
  - VALIDATE: Voltage calculations produce reasonable values (0-10V range)
  - IF_FAIL: Check audio buffer formats and scaling factors
  - ROLLBACK: Use simple absolute value instead of complex calculations
```

**Implementation**:
```cpp
float AudioEngine::calculateRMS(const float* buffer, unsigned long frameCount) {
    if (!buffer || frameCount == 0) return 0.0f;
    
    float sum = 0.0f;
    for (unsigned long i = 0; i < frameCount; i++) {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / frameCount);
}

float AudioEngine::calculatePeak(const float* buffer, unsigned long frameCount) {
    if (!buffer || frameCount == 0) return 0.0f;
    
    float peak = 0.0f;
    for (unsigned long i = 0; i < frameCount; i++) {
        peak = std::max(peak, std::abs(buffer[i]));
    }
    return peak;
}

void AudioEngine::updatePeakDetector(float& detector, float new_value) {
    // Peak detection with exponential decay
    const float attack_rate = 0.99f;   // Fast attack
    const float decay_rate = 0.992f;   // Slow decay for readable display
    
    if (new_value > detector) {
        detector = detector * attack_rate + new_value * (1.0f - attack_rate);
    } else {
        detector *= decay_rate;
    }
    
    // Prevent underflow
    if (detector < 0.001f) detector = 0.0f;
}
```

#### TASK 5: Integration and Cleanup
**File**: Multiple files
**Duration**: 30 minutes

```cpp
ACTION src/core/emulator.cpp:
  - ENSURE: Audio engine voltage monitoring is initialized
  - ADD: Voltage monitoring start/stop with audio engine
  - VALIDATE: Emulator initializes voltage monitoring correctly
  - IF_FAIL: Check component initialization order
  - ROLLBACK: Remove voltage monitoring integration

ACTION src/ui/main_window.cpp:
  - REMOVE: Dummy voltage array definitions and TODO comments
  - ADD: Voltage monitoring controls to debug UI
  - CLEANUP: Remove obsolete voltage generation code
  - VALIDATE: No dummy voltage code remains in codebase
  - IF_FAIL: Ensure real voltage display works before removing dummy code
  - ROLLBACK: Keep dummy arrays as fallback option
```

### Validation Loop

#### Level 1: Voltage Calculation Validation
```bash
cd emulator/build_gui && ./DistingNTEmulator test_plugins/simple_gain.dylib
# Manual test:
# 1. Load plugin successfully
# 2. Observe CV jack color changes 
# 3. Verify input jacks respond to audio input (microphone)
# 4. Verify output jacks show plugin output activity
# 5. Check voltage values are in reasonable range (-10V to +10V)
```

#### Level 2: Performance Validation
```bash
# Monitor audio engine performance:
# 1. Check audio CPU load remains stable (< 10%)
# 2. Verify no audio dropouts during voltage monitoring
# 3. Test voltage monitoring enable/disable toggle
# 4. Confirm UI frame rate remains smooth (> 30 FPS)
```

#### Level 3: Accuracy Validation
```bash
# Test voltage accuracy:
# 1. Generate known test signals (sine waves, DC offsets)
# 2. Verify voltage display matches expected signal levels
# 3. Test positive and negative voltage display
# 4. Check zero-signal displays as neutral (gray color)
# 5. Verify color coding matches voltage polarity
```

#### Level 4: Thread Safety Validation
```bash
# Test concurrent access:
# 1. Run extended session (10+ minutes) with active audio
# 2. Toggle voltage monitoring repeatedly during audio playback
# 3. Verify no crashes or race conditions
# 4. Check memory usage remains stable
```

### Debug Strategies

**Voltage Display Issues**:
```cpp
// Add debug output to voltage calculations
void AudioEngine::updateVoltageMonitoring(...) {
    #ifdef DEBUG_VOLTAGE_MONITORING
    static int debug_counter = 0;
    if (++debug_counter % 100 == 0) { // Log every ~2 seconds
        std::cout << "Input voltages: ";
        for (int i = 0; i < 4; i++) {
            std::cout << input_peak_detectors_[i] << " ";
        }
        std::cout << std::endl;
    }
    #endif
}
```

**Thread Safety Problems**:
```cpp
// Add atomic operation validation
auto& voltage_state = ApiShim::getState().voltage;
float voltage = voltage_state.input_voltages[index].load(std::memory_order_relaxed);

#ifdef DEBUG_THREAD_SAFETY
if (voltage < -15.0f || voltage > 15.0f) {
    std::cerr << "WARNING: Voltage out of range: " << voltage << std::endl;
}
#endif
```

**Performance Issues**:
```cpp
// Add timing measurements
void AudioEngine::audioCallback(...) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // ... audio processing ...
    updateVoltageMonitoring(input, output, frameCount);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    #ifdef DEBUG_PERFORMANCE
    if (duration.count() > 1000) { // > 1ms is concerning
        std::cout << "Slow audio callback: " << duration.count() << " μs" << std::endl;
    }
    #endif
}
```

### Rollback Strategy

**Complete Rollback**:
```bash
git checkout src/core/api_shim.h src/core/api_shim.cpp src/core/audio_engine.h src/core/audio_engine.cpp src/ui/main_window.cpp
cd build_gui && make -j4
```

**Partial Rollback** (keep structure, disable monitoring):
```cpp
// In api_shim.h
struct VoltageState {
    std::atomic<bool> monitoring_enabled{false}; // Disable by default
    // ... rest of structure
};

// In main_window.cpp - fallback to dummy values
float DistingNTMainWindow::getInputVoltage(int index) {
    auto& voltage_state = ApiShim::getState().voltage;
    if (!voltage_state.monitoring_enabled.load()) {
        // Fall back to dummy values
        static float dummy_voltages[12] = { /* original dummy data */ };
        return (index >= 0 && index < 12) ? dummy_voltages[index] : 0.0f;
    }
    // Use real voltage data
    return voltage_state.input_voltages[index].load();
}
```

**Emergency Mode** (disable voltage monitoring entirely):
```cpp
// Quick disable without code changes
void AudioEngine::updateVoltageMonitoring(...) {
    return; // Disable all voltage monitoring
}
```

## Success Metrics

### Functional Requirements
- [ ] **Real Voltage Display**: All 18 CV jacks show actual audio signal levels
- [ ] **Smooth Updates**: Voltage display updates smoothly at 30+ FPS
- [ ] **Accurate Scaling**: Voltage values properly represent ±10V hardware range
- [ ] **Color Coding**: Red for positive, blue for negative, gray for zero
- [ ] **Thread Safety**: No crashes or race conditions during extended use

### Performance Requirements
- [ ] **Audio Performance**: No audio dropouts or increased CPU load
- [ ] **UI Responsiveness**: GUI remains responsive during voltage monitoring
- [ ] **Memory Efficiency**: No memory leaks during extended monitoring
- [ ] **Toggle Control**: Voltage monitoring can be disabled for performance

### User Experience Requirements
- [ ] **Visual Clarity**: Voltage changes are clearly visible and intuitive
- [ ] **Real-time Feedback**: Voltage display responds immediately to audio changes
- [ ] **Professional Appearance**: Voltage visualization looks polished and hardware-like
- [ ] **Debug Utility**: Developers can easily monitor signal flow through plugins

## Timeline

**Total Estimated Time**: 2-3 hours

1. **State Management Setup** (30 min): Add voltage state structure
2. **Audio Engine Integration** (45 min): Implement voltage calculation and monitoring
3. **UI Integration** (30 min): Connect real voltage data to CV jack display
4. **Utility Functions** (30 min): Add RMS, peak detection, and scaling
5. **Integration & Cleanup** (30 min): Remove dummy code and add controls
6. **Testing & Validation** (15-30 min): Verify functionality and performance

## Post-Implementation

### Documentation Updates
- Update README with voltage monitoring feature description
- Document voltage monitoring performance impact and controls
- Add developer guide for interpreting voltage displays

### Future Enhancements
- Configurable voltage scaling and display ranges
- Voltage history/oscilloscope-like display
- Voltage logging and recording capabilities
- Advanced signal analysis (spectrum, FFT displays)
- Custom voltage display themes and color schemes