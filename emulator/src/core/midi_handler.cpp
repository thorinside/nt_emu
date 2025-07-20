#include "midi_handler.h"
#include <iostream>

MidiHandler::MidiHandler() {
}

MidiHandler::~MidiHandler() {
    shutdown();
}

bool MidiHandler::initialize() {
    if (initialized_) return true;
    
    // For now, just a stub implementation
    initialized_ = true;
    std::cout << "MIDI handler initialized (stub)" << std::endl;
    return true;
}

void MidiHandler::shutdown() {
    if (!initialized_) return;
    
    initialized_ = false;
    std::cout << "MIDI handler shutdown" << std::endl;
}

void MidiHandler::setMidiOutputCallback(std::function<void(const uint8_t*, size_t)> callback) {
    midi_output_callback_ = callback;
}

void MidiHandler::sendControllerChange(const struct _NT_controllerChange& cc, enum _NT_midiDestination dest) {
    (void)dest;
    
    if (midi_output_callback_) {
        uint8_t midi_msg[3] = {
            static_cast<uint8_t>(0xB0 | (cc.channel & 0x0F)),
            cc.controller,
            cc.value
        };
        midi_output_callback_(midi_msg, 3);
    }
    
    std::cout << "MIDI CC: ch=" << (int)cc.channel << " cc=" << (int)cc.controller << " val=" << (int)cc.value << std::endl;
}

void MidiHandler::sendNoteOn(const struct _NT_noteOn& note, enum _NT_midiDestination dest) {
    (void)dest;
    
    if (midi_output_callback_) {
        uint8_t midi_msg[3] = {
            static_cast<uint8_t>(0x90 | (note.channel & 0x0F)),
            note.note,
            note.velocity
        };
        midi_output_callback_(midi_msg, 3);
    }
    
    std::cout << "MIDI Note On: ch=" << (int)note.channel << " note=" << (int)note.note << " vel=" << (int)note.velocity << std::endl;
}

void MidiHandler::sendNoteOff(const struct _NT_noteOff& note, enum _NT_midiDestination dest) {
    (void)dest;
    
    if (midi_output_callback_) {
        uint8_t midi_msg[3] = {
            static_cast<uint8_t>(0x80 | (note.channel & 0x0F)),
            note.note,
            note.velocity
        };
        midi_output_callback_(midi_msg, 3);
    }
    
    std::cout << "MIDI Note Off: ch=" << (int)note.channel << " note=" << (int)note.note << " vel=" << (int)note.velocity << std::endl;
}