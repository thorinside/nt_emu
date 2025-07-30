#include "MidiProcessor.hpp"
#include "../plugin/PluginExecutor.hpp"
#include <rack.hpp>
#include <algorithm>

using namespace rack;

MidiProcessor::MidiProcessor(PluginExecutor* executor) : pluginExecutor(executor) {
    resetStats();
}

void MidiProcessor::processInputMessages(const Module::ProcessArgs& args) {
    midi::Message msg;
    
    // Process all available MIDI messages
    while (midiInput.tryPop(&msg, args.frame)) {
        if (isValidMidiMessage(msg)) {
            processSingleMessage(msg);
            midiInputLight = 1.f;
            stats.messagesReceived++;
            stats.lastMessageTimestamp = args.frame;
            notifyInputReceived(msg);
        }
    }
}

void MidiProcessor::sendOutputMessage(const midi::Message& msg) {
    if (!isValidMidiMessage(msg)) {
        INFO("MidiProcessor: Invalid MIDI message, size=%d", (int)msg.bytes.size());
        return;
    }
    
    // Debug: Log MIDI output messages
    if (msg.bytes.size() >= 3) {
        INFO("MidiProcessor: Sending MIDI message: %02X %02X %02X", 
             msg.bytes[0], msg.bytes[1], msg.bytes[2]);
    } else if (msg.bytes.size() == 2) {
        INFO("MidiProcessor: Sending MIDI message: %02X %02X", 
             msg.bytes[0], msg.bytes[1]);
    } else if (msg.bytes.size() == 1) {
        INFO("MidiProcessor: Sending MIDI message: %02X", msg.bytes[0]);
    }
    
    midiOutput.sendMessage(msg);
    stats.messagesSent++;
    triggerOutputLight();
    notifyOutputSent(msg);
    
    INFO("MidiProcessor: MIDI message sent, total sent: %d, output light triggered", stats.messagesSent);
}

void MidiProcessor::updateActivityLights(float deltaTime) {
    if (midiInputLight > 0.f) {
        midiInputLight *= lightDecayRate;
        if (midiInputLight < 0.001f) {
            midiInputLight = 0.f;
        }
    }
    
    if (midiOutputLight > 0.f) {
        midiOutputLight *= lightDecayRate;
        if (midiOutputLight < 0.001f) {
            midiOutputLight = 0.f;
        }
    }
}

void MidiProcessor::setupMidiOutput() {
    // Setup MIDI output callback for plugin
    if (pluginExecutor && pluginExecutor->isPluginValid()) {
        // Note: MIDI output callback will be connected through the plugin API
        // when the plugin system is fully integrated
    }
}

void MidiProcessor::sendMidiMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    midi::Message msg;
    msg.bytes = {byte0, byte1, byte2};
    
    // Apply channel mapping if needed
    if (midiOutput.channel >= 0) {
        setMidiChannel(msg, midiOutput.channel);
    }
    
    sendOutputMessage(msg);
}

void MidiProcessor::sendMidiMessage(uint8_t byte0, uint8_t byte1) {
    midi::Message msg;
    msg.bytes = {byte0, byte1};
    
    // Apply channel mapping if needed
    if (midiOutput.channel >= 0) {
        setMidiChannel(msg, midiOutput.channel);
    }
    
    sendOutputMessage(msg);
}

void MidiProcessor::sendMidiMessage(uint8_t byte0) {
    midi::Message msg;
    msg.bytes = {byte0};
    sendOutputMessage(msg);
}

void MidiProcessor::addObserver(IMidiObserver* observer) {
    if (observer) {
        observers.push_back(observer);
    }
}

void MidiProcessor::removeObserver(IMidiObserver* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void MidiProcessor::resetStats() {
    stats = MidiStats{};
}

void MidiProcessor::processSingleMessage(const midi::Message& msg) {
    // Route to plugin if available
    routeToPlugin(msg);
    
    // Update statistics
    uint8_t status = msg.bytes[0] & 0xF0;
    if (isRealtimeMessage(status)) {
        stats.realtimeMessagesReceived++;
    } else if (isChannelMessage(status)) {
        stats.channelMessagesReceived++;
    }
}

void MidiProcessor::routeToPlugin(const midi::Message& msg) {
    if (!pluginExecutor || !pluginExecutor->isPluginValid() || msg.bytes.empty()) {
        return;
    }
    
    uint8_t status = msg.bytes[0] & 0xF0;
    
    // Route real-time messages (0xF0-0xFF)
    if (isRealtimeMessage(msg.bytes[0])) {
        pluginExecutor->safeMidiRealtime(msg.bytes[0]);
    }
    // Route channel messages (Note On/Off, CC, Program Change, etc.)
    else if (isChannelMessage(status) && msg.bytes.size() >= 2) {
        uint8_t byte0 = msg.bytes[0];
        uint8_t byte1 = msg.bytes[1];
        uint8_t byte2 = (msg.bytes.size() >= 3) ? msg.bytes[2] : 0;
        
        pluginExecutor->safeMidiMessage(byte0, byte1, byte2);
    }
}

void MidiProcessor::notifyInputReceived(const midi::Message& msg) {
    for (auto* observer : observers) {
        observer->onMidiInputReceived(msg);
    }
}

void MidiProcessor::notifyOutputSent(const midi::Message& msg) {
    for (auto* observer : observers) {
        observer->onMidiOutputSent(msg);
    }
}

bool MidiProcessor::isValidMidiMessage(const midi::Message& msg) const {
    if (msg.bytes.empty() || msg.bytes.size() > 3) {
        return false;
    }
    
    uint8_t status = msg.bytes[0];
    
    // Check if first byte is a valid status byte
    if (status < 0x80) {
        return false;
    }
    
    // Validate message length based on status
    uint8_t statusNibble = status & 0xF0;
    switch (statusNibble) {
        case 0x80: // Note Off
        case 0x90: // Note On
        case 0xA0: // Aftertouch
        case 0xB0: // Control Change
        case 0xE0: // Pitch Bend
            return msg.bytes.size() == 3;
            
        case 0xC0: // Program Change
        case 0xD0: // Channel Pressure
            return msg.bytes.size() == 2;
            
        case 0xF0: // System messages
            if (status >= 0xF8) {
                // Real-time messages are single byte
                return msg.bytes.size() == 1;
            } else {
                // Other system messages vary
                return msg.bytes.size() >= 1;
            }
            
        default:
            return false;
    }
}

bool MidiProcessor::isRealtimeMessage(uint8_t status) const {
    return status >= 0xF8;
}

bool MidiProcessor::isChannelMessage(uint8_t status) const {
    uint8_t statusNibble = status & 0xF0;
    return statusNibble >= 0x80 && statusNibble <= 0xE0;
}

void MidiProcessor::setMidiChannel(midi::Message& msg, int channel) {
    if (msg.bytes.empty() || channel < 0 || channel > 15) {
        return;
    }
    
    uint8_t status = msg.bytes[0] & 0xF0;
    
    // Only apply channel to channel messages
    if (isChannelMessage(status)) {
        msg.bytes[0] = status | (channel & 0x0F);
    }
}

void MidiProcessor::triggerOutputLight() {
    midiOutputLight = 1.f;
}