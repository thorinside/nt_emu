# VCV Rack MIDI Implementation Guide

This guide provides essential information for implementing MIDI input/output in VCV Rack modules.

## Core MIDI Classes

### 1. MIDI Message Structure
```cpp
struct midi::Message {
    std::vector<uint8_t> bytes;  // MIDI bytes (usually 1-3)
    int64_t frame = -1;          // Engine frame for timing
    
    // Helper methods
    uint8_t getChannel() const;  // 0-15
    uint8_t getStatus() const;   // Status nibble (e.g., 0x9 for Note On)
    uint8_t getNote() const;     // Note number 0-127
    uint8_t getValue() const;    // Velocity/value 0-127
};
```

### 2. MIDI Port Classes
```cpp
// Base class for MIDI ports
struct midi::Port {
    int channel = -1;  // -1 = all channels, 0-15 = specific channel
    
    // Device management
    Driver* getDriver();
    void setDriverId(int driverId);
    Device* getDevice();
    void setDeviceId(int deviceId);
    
    // Serialization
    json_t* toJson();
    void fromJson(json_t* rootJ);
};

// Input port with message queue
struct midi::InputQueue : midi::Input {
    bool tryPop(Message* messageOut, int64_t maxFrame);
    size_t size();
};

// Output port for sending MIDI
struct midi::Output : midi::Port {
    void sendMessage(const Message& message);
};
```

## UI Components

### MidiDisplay Widget
```cpp
// Complete MIDI configuration display
MidiDisplay* display = createWidget<MidiDisplay>(Vec(x, y));
display->box.size = Vec(100, 50);
display->setMidiPort(&module->midiInput);
```

### MidiButton Widget
```cpp
// Compact button that opens MIDI menu
MidiButton* button = createWidget<MidiButton>(Vec(x, y));
button->setMidiPort(&module->midiInput);
```

### Context Menu Integration
```cpp
void appendContextMenu(Menu* menu) override {
    menu->addChild(new MenuSeparator);
    appendMidiMenu(menu, &module->midiInput);
}
```

## Implementation Patterns

### Basic MIDI Input
```cpp
class MyModule : Module {
    midi::InputQueue midiInput;
    
    void process(const ProcessArgs& args) override {
        midi::Message msg;
        while (midiInput.tryPop(&msg, args.frame)) {
            // Process message
            switch (msg.getStatus()) {
                case 0x9: // Note On
                    handleNoteOn(msg.getNote(), msg.getValue());
                    break;
                case 0x8: // Note Off
                    handleNoteOff(msg.getNote());
                    break;
                case 0xB: // Control Change
                    handleCC(msg.getNote(), msg.getValue());
                    break;
            }
        }
    }
    
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "midi", midiInput.toJson());
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        json_t* midiJ = json_object_get(rootJ, "midi");
        if (midiJ) midiInput.fromJson(midiJ);
    }
};
```

### MIDI Output
```cpp
class MyModule : Module {
    midi::Output midiOutput;
    
    void sendNoteOn(uint8_t note, uint8_t velocity) {
        midi::Message msg;
        msg.setStatus(0x9);
        msg.setChannel(0);  // Channel 1
        msg.setNote(note);
        msg.setValue(velocity);
        midiOutput.sendMessage(msg);
    }
    
    void sendCC(uint8_t cc, uint8_t value) {
        midi::Message msg;
        msg.setStatus(0xB);
        msg.setChannel(0);
        msg.setNote(cc);     // CC number
        msg.setValue(value);
        midiOutput.sendMessage(msg);
    }
};
```

## MIDI Message Types

```cpp
// Status bytes (without channel)
const uint8_t NOTE_OFF = 0x8;
const uint8_t NOTE_ON = 0x9;
const uint8_t POLY_PRESSURE = 0xA;
const uint8_t CONTROL_CHANGE = 0xB;
const uint8_t PROGRAM_CHANGE = 0xC;
const uint8_t CHANNEL_PRESSURE = 0xD;
const uint8_t PITCH_BEND = 0xE;
const uint8_t SYSTEM_EXCLUSIVE = 0xF0;
const uint8_t MIDI_CLOCK = 0xF8;
const uint8_t START = 0xFA;
const uint8_t CONTINUE = 0xFB;
const uint8_t STOP = 0xFC;
```

## Device Selection Menu Pattern

The standard VCV pattern creates a hierarchical menu:
1. **Driver Selection**: Core Audio, ALSA, Windows MIDI, etc.
2. **Device Selection**: Available devices for selected driver
3. **Channel Selection**: 1-16 or All

Example menu structure:
```
MIDI Input
├── Driver: Core Audio
├── Device: IAC Driver Bus 1
└── Channel: All channels
```

## Best Practices

1. **Timing**: Use frame timestamps for accurate timing
2. **Threading**: MIDI callbacks happen on separate thread - use InputQueue
3. **Channel Filtering**: Let users select specific channel or all
4. **Persistence**: Save/load MIDI settings with patch
5. **Error Handling**: Handle device disconnection gracefully
6. **Performance**: Process multiple messages per audio callback

## Common Pitfalls

1. **Not saving MIDI settings**: Always implement dataToJson/dataFromJson
2. **Blocking in audio thread**: Use InputQueue, not direct callbacks
3. **Ignoring timestamps**: Can cause timing issues
4. **Hardcoded channels**: Always make channel selectable
5. **Not handling device changes**: Devices can be added/removed while running