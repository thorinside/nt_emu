# Disting NT Plugin MIDI API

This document describes the MIDI API available to Disting NT plugins and how it integrates with the emulator.

## Plugin MIDI Output Functions

Plugins can send MIDI messages using these NT API functions:

### 1. Single Byte Messages
```cpp
void NT_sendMidiByte(uint32_t destination, uint8_t byte);
```
- Used for real-time messages (Clock, Start, Stop, etc.)
- Example: `NT_sendMidiByte(~0, 0xF8);` // MIDI Clock

### 2. Two-Byte Messages
```cpp
void NT_sendMidi2ByteMessage(uint32_t destination, uint8_t byte0, uint8_t byte1);
```
- Used for Program Change, Channel Aftertouch
- Example: `NT_sendMidi2ByteMessage(~0, 0xD0, 64);` // Channel aftertouch

### 3. Three-Byte Messages
```cpp
void NT_sendMidi3ByteMessage(uint32_t destination, uint8_t byte0, uint8_t byte1, uint8_t byte2);
```
- Used for Note On/Off, Control Change, Pitch Bend
- Example: `NT_sendMidi3ByteMessage(~0, 0x90, 60, 127);` // Note On C4

### 4. System Exclusive
```cpp
void NT_sendMidiSysEx(uint32_t destination, const uint8_t* data, size_t size);
```
- Sends complete SysEx message (including F0/F7 bytes)

## Destination Parameter

The `destination` parameter uses these flags:
```cpp
enum _NT_midiDestination {
    kNT_destinationBreakout = (1 << 0),  // Hardware breakout
    kNT_destinationUSB = (1 << 1)         // USB MIDI
};
```
- Use `~0` to send to all destinations
- Can OR flags together for multiple destinations

## Integration with Emulator

### Current Implementation

1. **API Shim Layer**: Plugin calls are routed through the API shim
2. **MidiHandler Class**: Formats and dispatches MIDI messages
3. **Output Callback**: Registered callback sends MIDI to external system

### Data Flow
```
Plugin → NT API → API Shim → MidiHandler → Output Callback → VCV MIDI
```

### MidiHandler Methods

```cpp
class MidiHandler {
    // High-level message methods
    void sendControllerChange(const _NT_controllerChange& cc, _NT_midiDestination dest);
    void sendNoteOn(const _NT_noteOn& note, _NT_midiDestination dest);
    void sendNoteOff(const _NT_noteOff& note, _NT_midiDestination dest);
    
    // Raw byte methods (used by API shim)
    void sendMidiByte(uint8_t byte, _NT_midiDestination dest);
    void sendMidi2ByteMessage(uint8_t byte0, uint8_t byte1, _NT_midiDestination dest);
    void sendMidi3ByteMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2, _NT_midiDestination dest);
    void sendMidiSysEx(const uint8_t* data, size_t size, _NT_midiDestination dest);
};
```

## Message Format Examples

### Note Messages
```cpp
// Note On: Channel 1, Middle C, Full velocity
NT_sendMidi3ByteMessage(~0, 0x90, 60, 127);

// Note Off: Channel 1, Middle C
NT_sendMidi3ByteMessage(~0, 0x80, 60, 0);
```

### Control Change
```cpp
// CC#7 (Volume) on Channel 1, value 64
NT_sendMidi3ByteMessage(~0, 0xB0, 7, 64);

// CC#1 (Mod Wheel) on Channel 1, value 0
NT_sendMidi3ByteMessage(~0, 0xB0, 1, 0);
```

### Real-Time Messages
```cpp
// MIDI Clock
NT_sendMidiByte(~0, 0xF8);

// Start
NT_sendMidiByte(~0, 0xFA);

// Stop
NT_sendMidiByte(~0, 0xFC);
```

### Channel Aftertouch
```cpp
// Channel 1, pressure 100
NT_sendMidi2ByteMessage(~0, 0xD0, 100);
```

## Plugin Example Usage

From the midiLFO plugin:
```cpp
void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    _midiLFO* pThis = (_midiLFO*)self;
    
    // Generate MIDI at 2Hz
    pThis->countdown -= 4 * numFramesBy4;
    if (pThis->countdown <= 0) {
        pThis->countdown = NT_globals.sampleRate / 2;
        
        switch (pThis->v[kParamType]) {
        case 0:  // CC mode
            NT_sendMidi3ByteMessage(~0, 0xB0, pThis->v[kParamCC], pThis->value);
            break;
        case 1:  // Aftertouch mode
            NT_sendMidi2ByteMessage(~0, 0xD0, pThis->value);
            break;
        case 2:  // Clock mode
            NT_sendMidiByte(~0, 0xF8);
            break;
        }
        
        pThis->value = (pThis->value + 1) & 127;
    }
}
```

## Current Limitations

1. **Output Only**: No MIDI input API for plugins currently
2. **No Channel Selection**: Channel is encoded in status byte
3. **Destination Routing**: Currently ignored in implementation
4. **No Timestamps**: Messages sent immediately

## Required Integration Points

To connect plugin MIDI to VCV:
1. Register MidiHandler callback with VCV MIDI output
2. Map destination flags to VCV MIDI ports
3. Handle timing/buffering for real-time performance
4. Implement MIDI input routing from VCV to plugins