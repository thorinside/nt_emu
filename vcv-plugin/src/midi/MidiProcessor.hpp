#pragma once
#include <rack.hpp>
#include <functional>

using namespace rack;

// Forward declarations
class PluginExecutor;

// MIDI message observer interface
class IMidiObserver {
public:
    virtual ~IMidiObserver() = default;
    virtual void onMidiInputReceived(const midi::Message& msg) = 0;
    virtual void onMidiOutputSent(const midi::Message& msg) = 0;
};

// MIDI processing system for DistingNT
class MidiProcessor {
public:
    MidiProcessor(PluginExecutor* executor);
    ~MidiProcessor() = default;
    
    // MIDI I/O access
    midi::InputQueue& getInputQueue() { return midiInput; }
    midi::Output& getOutput() { return midiOutput; }
    
    // MIDI processing
    void processInputMessages(const Module::ProcessArgs& args);
    void sendOutputMessage(const midi::Message& msg);
    
    // Activity lights
    float getMidiInputLight() const { return midiInputLight; }
    float getMidiOutputLight() const { return midiOutputLight; }
    void updateActivityLights(float deltaTime);
    
    // MIDI output setup
    void setupMidiOutput();
    void sendMidiMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2);
    void sendMidiMessage(uint8_t byte0, uint8_t byte1);
    void sendMidiMessage(uint8_t byte0);
    
    // Observer pattern
    void addObserver(IMidiObserver* observer);
    void removeObserver(IMidiObserver* observer);
    
    // Configuration
    void setActivityLightDecay(float decay) { lightDecayRate = decay; }
    float getActivityLightDecay() const { return lightDecayRate; }
    
    // Statistics
    struct MidiStats {
        uint32_t messagesReceived = 0;
        uint32_t messagesSent = 0;
        uint32_t realtimeMessagesReceived = 0;
        uint32_t channelMessagesReceived = 0;
        uint32_t lastMessageTimestamp = 0;
    };
    
    const MidiStats& getStats() const { return stats; }
    void resetStats();
    
private:
    PluginExecutor* pluginExecutor;
    
    // MIDI ports
    midi::InputQueue midiInput;
    midi::Output midiOutput;
    
    // Activity indicators
    float midiInputLight = 0.f;
    float midiOutputLight = 0.f;
    float lightDecayRate = 0.9f;
    
    // Statistics
    MidiStats stats;
    
    // Observers
    std::vector<IMidiObserver*> observers;
    
    // Internal processing
    void processSingleMessage(const midi::Message& msg);
    void routeToPlugin(const midi::Message& msg);
    void notifyInputReceived(const midi::Message& msg);
    void notifyOutputSent(const midi::Message& msg);
    
    // Message validation
    bool isValidMidiMessage(const midi::Message& msg) const;
    bool isRealtimeMessage(uint8_t status) const;
    bool isChannelMessage(uint8_t status) const;
    
    // MIDI output helpers
    void setMidiChannel(midi::Message& msg, int channel);
    void triggerOutputLight();
};