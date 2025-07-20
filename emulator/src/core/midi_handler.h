#pragma once

#include <distingnt/api.h>
#include <functional>

class MidiHandler {
public:
    MidiHandler();
    ~MidiHandler();
    
    bool initialize();
    void shutdown();
    
    // MIDI output callbacks
    void setMidiOutputCallback(std::function<void(const uint8_t*, size_t)> callback);
    
    // Send MIDI messages
    void sendControllerChange(const struct _NT_controllerChange& cc, enum _NT_midiDestination dest);
    void sendNoteOn(const struct _NT_noteOn& note, enum _NT_midiDestination dest);
    void sendNoteOff(const struct _NT_noteOff& note, enum _NT_midiDestination dest);
    
private:
    std::function<void(const uint8_t*, size_t)> midi_output_callback_;
    bool initialized_ = false;
};