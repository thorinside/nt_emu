# PRP: MIDI Input/Output Integration for DistingNT

## Goal
Enable bidirectional MIDI communication between VCV Rack and Disting NT plugins by adding MIDI input/output ports with device selection menus, routing MIDI data to/from plugins, and ensuring proper timing and persistence.

## Why
The Disting NT hardware supports MIDI I/O via USB and breakout connections. Plugins can generate MIDI output (as seen in midiLFO) and need to receive MIDI input for full functionality. Without MIDI integration, many plugin features are unusable and the emulation is incomplete.

## What

### User-Visible Behavior
1. **Context Menu MIDI Settings**: Right-click menu shows MIDI Input and MIDI Output submenus with driver/device/channel selection
2. **MIDI Output from Plugins**: Plugin-generated MIDI messages are sent to the selected VCV MIDI output device
3. **MIDI Input to Plugins**: Incoming MIDI from the selected device is routed to plugins for processing
4. **Visual Feedback**: Optional LED indicators for MIDI activity
5. **Persistence**: MIDI settings are saved with the patch and restored on load

### Technical Requirements
1. Add `midi::InputQueue` and `midi::Output` to DistingNT module
2. Implement MIDI device selection menus following VCV patterns
3. Connect MidiHandler output callback to VCV MIDI output
4. Extend plugin API to support MIDI input (currently output-only)
5. Handle timing and buffering for real-time performance
6. Support MIDI destination routing (USB vs Breakout)

## All Needed Context

### Documentation References
- **VCV MIDI Implementation Guide**: `PRPs/ai_docs/vcv-rack-midi-implementation.md`
- **NT Plugin MIDI API**: `PRPs/ai_docs/nt-plugin-midi-api.md`
- **VCV Core MIDI Modules**: https://github.com/VCVRack/Rack/tree/v2/src/core
- **MIDI Protocol**: https://www.midi.org/specifications-old/item/table-1-summary-of-midi-message

### Key Code Examples

#### VCV MIDI Port Setup (from Core modules)
```cpp
struct MidiModule : Module {
    midi::InputQueue midiInput;
    midi::Output midiOutput;
    
    MidiModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }
    
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "midiInput", midiInput.toJson());
        json_object_set_new(rootJ, "midiOutput", midiOutput.toJson());
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        json_t* midiInputJ = json_object_get(rootJ, "midiInput");
        if (midiInputJ) midiInput.fromJson(midiInputJ);
        json_t* midiOutputJ = json_object_get(rootJ, "midiOutput");
        if (midiOutputJ) midiOutput.fromJson(midiOutputJ);
    }
};
```

#### MidiHandler Callback Pattern (emulator/src/core/midi_handler.cpp)
```cpp
class MidiHandler {
    std::function<void(const uint8_t*, size_t)> midi_output_callback_;
    
    void setMidiOutputCallback(std::function<void(const uint8_t*, size_t)> callback) {
        midi_output_callback_ = callback;
    }
    
    void sendMidi3ByteMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2, _NT_midiDestination dest) {
        if (midi_output_callback_) {
            uint8_t msg[3] = {byte0, byte1, byte2};
            midi_output_callback_(msg, 3);
        }
    }
};
```

#### Plugin MIDI API Usage (test_plugins/examples/midiLFO.cpp)
```cpp
// Output MIDI from plugin
NT_sendMidi3ByteMessage(~0, 0xB0, cc_number, cc_value);  // Control Change
NT_sendMidi2ByteMessage(~0, 0xD0, pressure);             // Aftertouch
NT_sendMidiByte(~0, 0xF8);                               // MIDI Clock
```

### Gotchas and Patterns

1. **Thread Safety**: MIDI callbacks happen on separate thread - use `InputQueue` not direct callbacks
2. **Frame Timing**: VCV uses frame timestamps for sample-accurate MIDI timing
3. **Channel Encoding**: MIDI channel is in lower nibble of status byte (0x90 = Note On Ch1)
4. **Device Disconnection**: Handle gracefully - devices can be removed while running
5. **Destination Routing**: NT uses bit flags, need to map to VCV MIDI ports
6. **API Extension**: Current NT API is output-only, need to add input callbacks

### Existing Infrastructure

1. **MidiHandler**: Already exists in emulator core with output methods
2. **API Shim**: Routes plugin MIDI calls to MidiHandler
3. **EmulatorCore**: Has MidiHandler instance ready to use
4. **Menu System**: Context menu infrastructure in place

## Implementation Blueprint

### Phase 1: Add MIDI Ports to Module

```cpp
// In DistingNT class definition
class DistingNT : Module {
    // Add MIDI ports
    midi::InputQueue midiInput;
    midi::Output midiOutput;
    
    // MIDI activity lights
    dsp::ClockDivider midiActivityDivider;
    float midiInputLight = 0.f;
    float midiOutputLight = 0.f;
    
    // Add to existing constructor
    DistingNT() {
        // ... existing code ...
        
        // Configure MIDI activity divider
        midiActivityDivider.setDivision(512);
    }
};
```

### Phase 2: Connect MidiHandler to VCV Output

```cpp
// In DistingNT constructor or initialization
void setupMidiOutput() {
    // Connect MidiHandler callback to VCV MIDI output
    emulatorCore.getMidiHandler().setMidiOutputCallback(
        [this](const uint8_t* data, size_t size) {
            // Convert raw bytes to VCV Message
            midi::Message msg;
            msg.bytes.assign(data, data + size);
            
            // Check destination routing
            uint8_t status = data[0] & 0xF0;
            uint8_t channel = data[0] & 0x0F;
            
            // Apply channel filter if set
            if (midiOutput.channel >= 0 && channel != midiOutput.channel) {
                return;  // Skip if wrong channel
            }
            
            // Send to VCV MIDI output
            midiOutput.sendMessage(msg);
            
            // Flash activity light
            midiOutputLight = 1.f;
        }
    );
}
```

### Phase 3: Process MIDI Input

```cpp
// In process() method
void process(const ProcessArgs& args) override {
    // Process MIDI input
    midi::Message msg;
    while (midiInput.tryPop(&msg, args.frame)) {
        processMidiMessage(msg);
        midiInputLight = 1.f;
    }
    
    // Update activity lights
    if (midiActivityDivider.process()) {
        midiInputLight *= 0.9f;  // Decay
        midiOutputLight *= 0.9f;
    }
    
    // ... rest of process code ...
}

void processMidiMessage(const midi::Message& msg) {
    // Route to plugin if loaded
    if (isPluginLoaded() && pluginAlgorithm) {
        // TODO: Need to extend plugin API for MIDI input
        // For now, could store in buffer for plugin to poll
        
        // Example structure for future API:
        // if (pluginFactory->midiReceived) {
        //     pluginFactory->midiReceived(pluginAlgorithm, 
        //         msg.bytes.data(), msg.bytes.size());
        // }
    }
}
```

### Phase 4: Add Context Menu MIDI Options

```cpp
// In DistingNTWidget::appendContextMenu
void appendContextMenu(Menu* menu) override {
    DistingNT* module = dynamic_cast<DistingNT*>(this->module);
    if (!module) return;
    
    menu->addChild(new MenuSeparator);
    
    // MIDI Input submenu
    menu->addChild(createSubmenuItem("MIDI Input", "", [=](Menu* menu) {
        appendMidiMenu(menu, &module->midiInput);
    }));
    
    // MIDI Output submenu
    menu->addChild(createSubmenuItem("MIDI Output", "", [=](Menu* menu) {
        appendMidiMenu(menu, &module->midiOutput);
    }));
    
    // ... existing plugin menu ...
}
```

### Phase 5: Implement Destination Routing

```cpp
// Add routing preference to module
enum MidiRoutingMode {
    ROUTING_BOTH,      // Send to both USB and Breakout
    ROUTING_USB_ONLY,  // USB only
    ROUTING_BREAKOUT_ONLY  // Breakout only
};

MidiRoutingMode midiRouting = ROUTING_BOTH;

// Modify output callback to check routing
void setupMidiOutput() {
    emulatorCore.getMidiHandler().setMidiOutputCallback(
        [this](const uint8_t* data, size_t size, uint32_t destination) {
            // Check if we should send based on routing mode
            bool sendUSB = (destination & kNT_destinationUSB) && 
                          (midiRouting == ROUTING_BOTH || midiRouting == ROUTING_USB_ONLY);
            bool sendBreakout = (destination & kNT_destinationBreakout) && 
                               (midiRouting == ROUTING_BOTH || midiRouting == ROUTING_BREAKOUT_ONLY);
            
            // For VCV, we only have one output, so send if either is true
            if (sendUSB || sendBreakout) {
                midi::Message msg;
                msg.bytes.assign(data, data + size);
                midiOutput.sendMessage(msg);
                midiOutputLight = 1.f;
            }
        }
    );
}
```

### Phase 6: Extend Plugin API for MIDI Input

```cpp
// Add to _NT_factory structure (in api.h)
struct _NT_factory {
    // ... existing fields ...
    
    // New MIDI input callback
    void (*midiReceived)(struct _NT_algorithm* algorithm, 
                        const uint8_t* data, size_t size);
};

// Add buffer for plugins without callback
struct MidiInputBuffer {
    static constexpr size_t MAX_MESSAGES = 256;
    struct Message {
        uint8_t data[4];  // Max 4 bytes per message
        uint8_t size;
        uint32_t timestamp;
    };
    Message messages[MAX_MESSAGES];
    size_t writeIndex = 0;
    size_t readIndex = 0;
    
    void push(const uint8_t* data, size_t size) {
        if (size > 4) return;  // Too large
        
        Message& msg = messages[writeIndex % MAX_MESSAGES];
        memcpy(msg.data, data, size);
        msg.size = size;
        msg.timestamp = /* current sample count */;
        
        writeIndex++;
    }
    
    bool pop(Message& out) {
        if (readIndex >= writeIndex) return false;
        out = messages[readIndex % MAX_MESSAGES];
        readIndex++;
        return true;
    }
};
```

## Validation Loop

### Level 1: Compilation and Syntax
```bash
cd vcv-plugin
make clean && make
# Ensure MIDI includes are found
```

### Level 2: Unit Tests
```bash
# Test MIDI message handling
cd vcv-plugin
./test_midi_integration

# Test with MIDI plugin
./test_distingnt ./midiLFO.dylib
```

### Level 3: Integration Testing
```bash
# In VCV Rack:
# 1. Add DistingNT module
# 2. Right-click → MIDI Input → Select device
# 3. Right-click → MIDI Output → Select device
# 4. Load midiLFO plugin
# 5. Connect MIDI monitor to verify output
# 6. Send MIDI to input and verify reception
```

### Level 4: MIDI Loopback Test
```python
# Python script to test MIDI I/O
import mido
import time

# Open MIDI ports
inport = mido.open_input('IAC Driver Bus 1')
outport = mido.open_output('IAC Driver Bus 1')

# Send test messages
test_messages = [
    mido.Message('note_on', note=60, velocity=64),
    mido.Message('control_change', control=7, value=100),
    mido.Message('clock'),
]

for msg in test_messages:
    outport.send(msg)
    time.sleep(0.1)
    
# Verify reception
for msg in inport.iter_pending():
    print(f"Received: {msg}")
```

## Checklist

- [ ] Add midi::InputQueue and midi::Output to DistingNT module
- [ ] Implement dataToJson/dataFromJson for MIDI persistence
- [ ] Connect MidiHandler callback to VCV MIDI output
- [ ] Process incoming MIDI messages in process()
- [ ] Add MIDI device selection to context menu
- [ ] Implement destination routing (USB/Breakout modes)
- [ ] Add MIDI activity LED indicators
- [ ] Extend plugin API to support MIDI input
- [ ] Create MIDI input buffer for legacy plugins
- [ ] Handle device disconnection gracefully
- [ ] Test with midiLFO and other MIDI plugins
- [ ] Document MIDI routing behavior

## Success Metrics

1. MIDI output from plugins appears at selected VCV MIDI device
2. MIDI input from VCV is received by plugins
3. Device selection persists across patch save/load
4. No audio dropouts or timing issues
5. Activity lights indicate MIDI traffic
6. Supports all MIDI message types (notes, CC, clock, sysex)

**Confidence Score: 8/10**

The implementation is well-understood with clear patterns from VCV Core modules. The main uncertainty is extending the plugin API for MIDI input, which may require coordination with the plugin format specification. The output path is straightforward since the infrastructure already exists.