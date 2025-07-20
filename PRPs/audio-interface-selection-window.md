# TASK PRP: Audio Interface Selection Window

## Goal

Create a dedicated settings window that allows users to select audio input/output devices, configure buffer sizes, and map physical audio channels to CV buses for professional audio interface support.

## Why

**Business Value:**
- **Professional Workflows**: Support for high-end audio interfaces (MOTU, RME, Focusrite, etc.)
- **Multi-Channel Support**: Route multiple audio channels to different CV inputs/outputs
- **Flexibility**: Users can select optimal devices for their specific setups
- **Hardware Integration**: Better integration with existing studio setups

**User Impact:**
- Plugin developers can use their preferred audio interfaces
- Support for multi-channel audio interfaces with proper routing
- Reduced audio latency with professional drivers (ASIO, Core Audio)
- Better audio quality with high-end converters
- Persistent settings for consistent workflows

## What

Transform the current fixed audio device system into a flexible, user-configurable audio interface selection system:

### Success Criteria
- [ ] Audio settings window accessible from main window
- [ ] List all available PortAudio input/output devices
- [ ] Allow selection of different input and output devices
- [ ] Configure buffer sizes (64, 128, 256, 512, 1024 samples)
- [ ] Map physical audio channels to CV buses (1-12 inputs, 1-6 outputs)
- [ ] Settings persist between sessions (JSON configuration)
- [ ] Real-time device switching without audio dropouts
- [ ] Error handling for device failures and configuration issues

### User-Visible Behavior
1. **Settings Access**: Menu bar or button to open audio settings window
2. **Device Selection**: Dropdown lists of available input/output devices
3. **Channel Mapping**: Visual interface to map audio channels to CV buses
4. **Buffer Configuration**: Slider or dropdown for buffer size selection
5. **Real-time Updates**: Apply settings immediately with audio restart
6. **Status Display**: Show current device status and error messages
7. **Settings Persistence**: Configuration saved automatically on changes

## All Needed Context

### Current Architecture Analysis

**Existing Audio Engine** (`src/core/audio_engine.cpp/.h`):
- PortAudio with default device selection only
- Fixed mono input/output configuration
- 28-bus architecture (12 CV inputs + 6 CV outputs + extras)
- Real-time voltage monitoring with atomic operations
- Buffer size: 64 frames, Sample rate: 48kHz

**Current Device Handling**:
```cpp
// Fixed default device selection
inputParameters.device = Pa_GetDefaultInputDevice();
outputParameters.device = Pa_GetDefaultOutputDevice();
inputParameters.channelCount = 1;  // Mono only
outputParameters.channelCount = 1; // Mono only
```

**Configuration System** (`src/utils/config.cpp/.h`):
- Placeholder implementation with load()/save() methods
- No JSON serialization currently active
- nlohmann/json available in third_party but unused
- Hardcoded values only

**UI Architecture** (`src/ui/main_window.cpp/.h`):
- Single fixed-size window (600x800) with hardware layout
- ImGui rendering with custom draw lists
- No menu system or secondary windows
- Hardware-focused design mimicking physical device

### Dependencies

**PortAudio Integration**:
- Device enumeration: `Pa_GetDeviceCount()`, `Pa_GetDeviceInfo()`
- Host API selection: Core Audio (macOS), WASAPI (Windows), ALSA (Linux)
- Multi-channel support: Modify channel count parameters
- Stream reconfiguration: Stop/restart audio streams for device changes

**JSON Configuration**:
- nlohmann/json library already available in third_party/
- Need to activate JSON serialization in config system
- Settings file location and format definition

**ImGui Window Management**:
- Secondary window creation patterns
- Modal dialog handling for settings
- Menu bar integration for settings access

### Known Gotchas

**PortAudio Device Switching**:
- Must stop audio stream before changing devices
- Device enumeration can change at runtime (USB devices)
- Some devices require specific sample rates or buffer sizes
- ASIO drivers may have exclusive access requirements

**Multi-Channel Audio Complexity**:
- Channel mapping validation (ensure enough channels available)
- Buffer size compatibility across input/output devices
- Sample rate mismatches between devices
- Audio format conversion requirements

**Thread Safety Issues**:
- Device switching affects real-time audio thread
- UI updates during audio reconfiguration
- Voltage monitoring state during device transitions
- Plugin state preservation during audio restart

**Configuration Persistence Challenges**:
- Device IDs may change between sessions (USB interfaces)
- Device names vs. device IDs for identification
- Default fallback when saved devices unavailable
- Migration of existing users with no saved config

### Existing Patterns

**Current Audio Initialization** (`audio_engine.cpp`):
```cpp
bool AudioEngine::initialize() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    // Device parameter setup
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    
    // Stream opening
    err = Pa_OpenStream(&stream_, &inputParameters, &outputParameters, 
                        SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, 
                        audioCallback, this);
}
```

**Voltage Monitoring Pattern** (`audio_engine.cpp`):
```cpp
void AudioEngine::updateVoltageMonitoring(unsigned long frames) {
    auto& voltage_state = ApiShim::getState().voltage;
    if (!voltage_state.monitoring_enabled.load()) return;
    
    // Update input voltages for buses 0-11
    for (int i = 0; i < 12; i++) {
        float rms = calculateRMS(audio_buses_[i].data(), SAMPLES_PER_BLOCK);
        updatePeakDetector(input_peak_detectors_[i], rms);
        voltage_state.input_voltages[i].store(input_peak_detectors_[i] * 10.0f);
    }
}
```

**Configuration Placeholder** (`config.cpp`):
```cpp
bool Config::load() {
    std::cout << "Loading configuration..." << std::endl;
    return true; // Placeholder
}

bool Config::save() {
    std::cout << "Saving configuration..." << std::endl;
    return true; // Placeholder
}
```

## Implementation Blueprint

### Task Breakdown

#### TASK 1: Device Enumeration Service
**File**: `src/core/audio_device_manager.h/.cpp`
**Duration**: 45 minutes

```cpp
ACTION src/core/audio_device_manager.h:
  - CREATE: AudioDeviceManager class for device enumeration and management
  - ADD: Device enumeration methods using PortAudio APIs
  - ADD: Device information structures (name, channels, sample rates)
  - ADD: Device validation and capability checking
  - VALIDATE: Device enumeration works on macOS/Windows/Linux
  - IF_FAIL: Check PortAudio initialization and device detection
  - ROLLBACK: Use existing default device selection as fallback
```

**Implementation**:
```cpp
struct AudioDeviceInfo {
    int device_id;
    std::string name;
    std::string host_api_name;
    int max_input_channels;
    int max_output_channels;
    double default_sample_rate;
    bool is_default_input;
    bool is_default_output;
};

class AudioDeviceManager {
public:
    static bool initialize();
    static std::vector<AudioDeviceInfo> getInputDevices();
    static std::vector<AudioDeviceInfo> getOutputDevices();
    static AudioDeviceInfo getDeviceInfo(int device_id);
    static bool validateDeviceConfiguration(int input_device, int output_device, 
                                          int buffer_size, double sample_rate);
private:
    static std::vector<AudioDeviceInfo> cached_devices_;
    static bool devices_enumerated_;
    static void enumerateDevices();
};
```

#### TASK 2: Enhanced Configuration System
**File**: `src/utils/config.h/.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/utils/config.h:
  - ADD: AudioConfiguration structure with device settings
  - ADD: JSON serialization using nlohmann/json
  - ADD: Configuration validation and migration
  - ADD: Default configuration generation
  - VALIDATE: JSON config saves and loads correctly
  - IF_FAIL: Check file permissions and JSON syntax
  - ROLLBACK: Keep placeholder config for minimal functionality
```

**Implementation**:
```cpp
struct AudioConfiguration {
    int input_device_id = -1;        // -1 = default device
    int output_device_id = -1;       // -1 = default device
    int buffer_size = 64;            // Samples per buffer
    double sample_rate = 48000.0;    // Hz
    std::array<int, 12> input_channel_mapping;   // Physical channel to CV input mapping
    std::array<int, 6> output_channel_mapping;   // CV output to physical channel mapping
    bool voltage_monitoring_enabled = true;
    
    // JSON serialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AudioConfiguration, 
        input_device_id, output_device_id, buffer_size, sample_rate,
        input_channel_mapping, output_channel_mapping, voltage_monitoring_enabled)
};

class Config {
    AudioConfiguration audio;
    std::string config_file_path_;
    
public:
    bool load() override;
    bool save() override;
    AudioConfiguration& getAudioConfig() { return audio; }
    void setAudioConfig(const AudioConfiguration& config);
};
```

#### TASK 3: Audio Settings Dialog
**File**: `src/ui/audio_settings_dialog.h/.cpp`
**Duration**: 60 minutes

```cpp
ACTION src/ui/audio_settings_dialog.h:
  - CREATE: AudioSettingsDialog class following ImGui window patterns
  - ADD: Device selection dropdowns with device enumeration
  - ADD: Buffer size and sample rate configuration
  - ADD: Channel mapping interface for multi-channel support
  - ADD: Apply/Cancel/OK button handling
  - VALIDATE: Settings dialog opens and displays device list
  - IF_FAIL: Check ImGui window flags and device enumeration
  - ROLLBACK: Simple device selection without advanced features
```

**Implementation**:
```cpp
class AudioSettingsDialog {
private:
    bool is_open_ = false;
    AudioConfiguration temp_config_;
    std::vector<AudioDeviceInfo> input_devices_;
    std::vector<AudioDeviceInfo> output_devices_;
    std::string error_message_;
    
    void renderDeviceSelection();
    void renderBufferConfiguration();
    void renderChannelMapping();
    void renderErrorMessages();
    bool validateConfiguration();
    
public:
    void show() { is_open_ = true; }
    void hide() { is_open_ = false; }
    bool isOpen() const { return is_open_; }
    
    void render();  // Called from main render loop
    
    // Callbacks
    std::function<void(const AudioConfiguration&)> on_apply;
    std::function<void()> on_cancel;
};
```

#### TASK 4: Enhanced Audio Engine
**File**: `src/core/audio_engine.h/.cpp`
**Duration**: 50 minutes

```cpp
ACTION src/core/audio_engine.h:
  - ADD: Device configuration methods for runtime switching
  - ADD: Multi-channel input/output support
  - ADD: Channel mapping configuration
  - MODIFY: Audio callback to handle variable channel counts
  - ADD: Safe device switching with stream restart
  - VALIDATE: Audio engine works with different devices and channel counts
  - IF_FAIL: Check PortAudio device compatibility and channel validation
  - ROLLBACK: Keep existing mono default device configuration
```

**Implementation**:
```cpp
class AudioEngine {
private:
    AudioConfiguration current_config_;
    std::vector<float> multi_channel_input_buffer_;
    std::vector<float> multi_channel_output_buffer_;
    
public:
    bool configureDevices(const AudioConfiguration& config);
    bool switchToDevice(int input_device_id, int output_device_id);
    bool setBufferSize(int buffer_size);
    bool setChannelMapping(const std::array<int, 12>& input_mapping,
                          const std::array<int, 6>& output_mapping);
    
    AudioConfiguration getCurrentConfiguration() const { return current_config_; }
    std::string getDeviceStatusString() const;
    
private:
    bool restartAudioStream();
    void mapMultiChannelInput(const float* input, unsigned long frames);
    void mapMultiChannelOutput(float* output, unsigned long frames);
    bool validateDeviceConfiguration(const AudioConfiguration& config);
};
```

#### TASK 5: Main Window Integration
**File**: `src/ui/main_window.h/.cpp`
**Duration**: 30 minutes

```cpp
ACTION src/ui/main_window.cpp:
  - ADD: Menu bar with "Settings" option
  - ADD: AudioSettingsDialog instance and rendering
  - ADD: Settings dialog callback handling
  - ADD: Audio status display in main window
  - MODIFY: Window flags to include menu bar
  - VALIDATE: Settings dialog opens from menu and applies changes
  - IF_FAIL: Check ImGui menu integration and callback setup
  - ROLLBACK: Add simple button instead of menu bar
```

**Implementation**:
```cpp
class DistingNTMainWindow {
private:
    std::unique_ptr<AudioSettingsDialog> audio_settings_dialog_;
    
    void renderMenuBar();
    void handleAudioConfigurationApplied(const AudioConfiguration& config);
    void handleAudioConfigurationCancelled();
    
public:
    void render() override;
    void setEmulator(Emulator* emulator) override;
};

void DistingNTMainWindow::render() {
    renderMenuBar();
    
    // Existing window rendering...
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_MenuBar;  // Add menu bar flag
    
    if (ImGui::Begin("Expert Sleepers Disting NT", nullptr, window_flags)) {
        // Existing hardware rendering...
    }
    ImGui::End();
    
    // Render settings dialog
    if (audio_settings_dialog_) {
        audio_settings_dialog_->render();
    }
}
```

#### TASK 6: Integration and Error Handling
**File**: Multiple files
**Duration**: 40 minutes

```cpp
ACTION src/core/emulator.cpp:
  - ADD: AudioDeviceManager initialization
  - ADD: Configuration loading and audio engine setup
  - ADD: Error handling for device failures
  - ADD: Graceful fallback to default devices
  - VALIDATE: Emulator starts with saved configuration
  - IF_FAIL: Check initialization order and error propagation
  - ROLLBACK: Use existing hardcoded audio initialization

ACTION src/ui/main_window.cpp:
  - ADD: Error message display for audio configuration failures
  - ADD: Device status indicators
  - ADD: Connection between settings dialog and audio engine
  - CLEANUP: Remove any hardcoded audio assumptions
  - VALIDATE: Audio errors are displayed to user properly
  - IF_FAIL: Check error message routing and UI updates
  - ROLLBACK: Keep console error output as primary feedback
```

### Validation Loop

#### Level 1: Device Enumeration Validation
```bash
cd emulator/build_gui && ./DistingNTEmulator
# Manual test:
# 1. Open Settings menu -> Audio Settings
# 2. Verify input devices list populated
# 3. Verify output devices list populated  
# 4. Check device names and channel counts are accurate
# 5. Confirm default devices are pre-selected
```

#### Level 2: Configuration Persistence Validation
```bash
# Test configuration save/load:
# 1. Change audio settings and apply
# 2. Close emulator
# 3. Restart emulator
# 4. Verify settings were restored
# 5. Check JSON config file created in expected location
```

#### Level 3: Multi-Channel Audio Validation
```bash
# Test multi-channel audio interfaces:
# 1. Connect multi-channel audio interface (8+ channels)
# 2. Configure channel mapping (channel 1->CV input 1, etc.)
# 3. Route audio signals to multiple inputs
# 4. Verify CV voltage displays show correct channels
# 5. Test output channel mapping with plugin-generated signals
```

#### Level 4: Device Switching Validation
```bash
# Test real-time device switching:
# 1. Start with default devices
# 2. Switch to different audio interface while audio is running
# 3. Verify smooth transition without crashes
# 4. Test switching back to default devices
# 5. Verify voltage monitoring continues correctly
```

### Debug Strategies

**Device Enumeration Issues**:
```cpp
// Add debug output for device detection
void AudioDeviceManager::enumerateDevices() {
    int device_count = Pa_GetDeviceCount();
    std::cout << "Found " << device_count << " audio devices:" << std::endl;
    
    for (int i = 0; i < device_count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        std::cout << "Device " << i << ": " << info->name 
                  << " (in:" << info->maxInputChannels 
                  << " out:" << info->maxOutputChannels << ")" << std::endl;
    }
}
```

**Configuration Loading Problems**:
```cpp
// Add JSON validation and error reporting
bool Config::load() {
    try {
        std::ifstream file(config_file_path_);
        if (!file.is_open()) {
            std::cerr << "Config file not found, using defaults" << std::endl;
            return true; // Use defaults
        }
        
        nlohmann::json j;
        file >> j;
        audio = j.get<AudioConfiguration>();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return false;
    }
}
```

**Audio Device Failures**:
```cpp
// Add comprehensive error handling
bool AudioEngine::configureDevices(const AudioConfiguration& config) {
    if (!AudioDeviceManager::validateDeviceConfiguration(
            config.input_device_id, config.output_device_id, 
            config.buffer_size, config.sample_rate)) {
        error_message_ = "Invalid device configuration";
        return false;
    }
    
    if (!restartAudioStream()) {
        error_message_ = "Failed to restart audio stream with new configuration";
        // Fall back to default devices
        return initializeWithDefaults();
    }
    
    return true;
}
```

### Rollback Strategy

**Complete Rollback**:
```bash
git checkout src/core/audio_engine.h src/core/audio_engine.cpp src/ui/main_window.h src/ui/main_window.cpp src/utils/config.h src/utils/config.cpp
cd build_gui && make -j4
```

**Partial Rollback** (keep UI, disable device switching):
```cpp
// In audio_settings_dialog.cpp - disable apply button
void AudioSettingsDialog::render() {
    // ... device selection UI ...
    
    if (ImGui::Button("Apply")) {
        // Show "Not implemented yet" message instead of applying
        error_message_ = "Device switching will be available in future version";
    }
}
```

**Emergency Mode** (disable settings dialog entirely):
```cpp
// In main_window.cpp - hide settings menu
void DistingNTMainWindow::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }
        // Comment out audio settings menu
        // if (ImGui::BeginMenu("Settings")) { ... }
        ImGui::EndMenuBar();
    }
}
```

## Success Metrics

### Functional Requirements
- [ ] **Device Selection**: List and select from available audio input/output devices
- [ ] **Multi-Channel Support**: Configure channel mapping for professional interfaces
- [ ] **Buffer Configuration**: Adjustable buffer sizes for latency optimization
- [ ] **Settings Persistence**: Configuration saved and restored between sessions
- [ ] **Real-time Switching**: Change devices without restarting emulator
- [ ] **Error Handling**: Clear error messages for device configuration issues

### Performance Requirements
- [ ] **Audio Quality**: No degradation in audio performance
- [ ] **Latency**: Maintain low-latency performance with user-selected buffer sizes
- [ ] **Stability**: No crashes during device switching or configuration
- [ ] **Responsiveness**: Settings UI remains responsive during device enumeration

### User Experience Requirements
- [ ] **Intuitive Interface**: Clear device selection and configuration options
- [ ] **Professional Workflow**: Support for high-end audio interfaces
- [ ] **Error Recovery**: Graceful fallback when devices become unavailable
- [ ] **Visual Feedback**: Clear indication of current audio device status

## Timeline

**Total Estimated Time**: 4-5 hours

1. **Device Enumeration Service** (45 min): PortAudio device detection and management
2. **Enhanced Configuration System** (30 min): JSON-based settings persistence  
3. **Audio Settings Dialog** (60 min): ImGui-based device selection interface
4. **Enhanced Audio Engine** (50 min): Multi-channel support and device switching
5. **Main Window Integration** (30 min): Menu bar and settings dialog integration
6. **Integration & Error Handling** (40 min): Complete system integration and validation
7. **Testing & Validation** (30-45 min): Comprehensive testing across device types

## Post-Implementation

### Documentation Updates
- Update README with audio interface selection feature
- Document supported audio interface types and requirements
- Add troubleshooting guide for common audio configuration issues

### Future Enhancements
- ASIO driver support for ultra-low latency on Windows
- Audio interface preset management for common studio setups
- Advanced routing matrix for complex multi-channel configurations
- Real-time audio performance monitoring and optimization
- Sample rate conversion for mismatched input/output devices