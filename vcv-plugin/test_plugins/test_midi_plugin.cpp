/*
MIT License

MIDI Delay Plugin for DistingNT - Test Plugin for VCV Rack Integration
This plugin creates a MIDI delay effect with configurable repeats, delay time,
velocity decay, and channel routing.
*/

#include <math.h>
#include <new>
#include <distingnt/api.h>

enum
{
    kParamDelayTime,     // 0-100 mapping to 0.1-2.0 seconds
    kParamRepeats,       // 1-8 repeats
    kParamVelocityDecay, // 0-90% decay per repeat
    kParamInputChannel,  // 1-16 or 17 for "All"
    kParamOutputChannel, // 1-16
};

struct _midiDelayAlgorithm : public _NT_algorithm
{
    struct DelayedNote {
        uint8_t status;      // MIDI status byte (with channel)
        uint8_t data1;       // Note number or CC number
        uint8_t data2;       // Velocity or CC value
        uint8_t originalVel; // Original velocity for decay calculation
        float timeLeft;      // Time remaining until playback (seconds)
        int repeatsLeft;     // Number of repeats remaining
        int totalRepeats;    // Total repeats (for decay calculation)
        bool active;         // Whether this slot is in use
    };
    
    _midiDelayAlgorithm() : writeIndex(0) {
        // Initialize ring buffer
        for (int i = 0; i < 64; i++) {
            ringBuffer[i].active = false;
        }
        
        // Initialize with default values (will be updated when parameters are set)
        delayTime = 0.5f;        // Default 0.5 seconds
        numRepeats = 3;          // Default 3 repeats
        velocityDecay = 0.2f;    // Default 20% decay
        inputChannel = 16;       // Default "All" channels
        outputChannel = 0;       // Default channel 1 (0-indexed)
    }
    ~_midiDelayAlgorithm() {}
    
    DelayedNote ringBuffer[64];  // Ring buffer for delayed notes
    int writeIndex;              // Next slot to write to
    float delayTime;            // Current delay time in seconds
    int numRepeats;             // Current number of repeats
    float velocityDecay;        // Velocity decay factor per repeat (0.0-0.9)
    int inputChannel;           // Input MIDI channel (0-15, or 16 for all)
    int outputChannel;          // Output MIDI channel (0-15)
    
    void updateParameters() {
        // Convert parameter values to working values
        delayTime = v[kParamDelayTime] / 100.0f * 1.9f + 0.1f; // 0.1 to 2.0 seconds
        numRepeats = v[kParamRepeats];
        velocityDecay = v[kParamVelocityDecay] / 100.0f * 0.9f; // 0% to 90%
        inputChannel = v[kParamInputChannel] - 1; // 0-15 or 16 for all
        outputChannel = v[kParamOutputChannel] - 1; // 0-15
    }
    
    int findFreeSlot() {
        // Find first free slot in ring buffer
        for (int i = 0; i < 64; i++) {
            int index = (writeIndex + i) % 64;
            if (!ringBuffer[index].active) {
                writeIndex = (index + 1) % 64;
                return index;
            }
        }
        // If no free slots, overwrite the oldest (writeIndex)
        int index = writeIndex;
        writeIndex = (writeIndex + 1) % 64;
        return index;
    }
    
    void addDelayedNote(uint8_t status, uint8_t data1, uint8_t data2) {
        int slot = findFreeSlot();
        DelayedNote& note = ringBuffer[slot];
        
        // Map to output channel
        uint8_t outputStatus = (status & 0xF0) | (outputChannel & 0x0F);
        
        note.status = outputStatus;
        note.data1 = data1;
        note.data2 = data2;
        note.originalVel = data2; // Store original velocity
        note.timeLeft = delayTime;
        note.repeatsLeft = numRepeats;
        note.totalRepeats = numRepeats;
        note.active = true;
    }
};

static char const * const enumStringsInputChannel[] = {
    "1", "2", "3", "4", "5", "6", "7", "8",
    "9", "10", "11", "12", "13", "14", "15", "16", "All"
};

static char const * const enumStringsOutputChannel[] = {
    "1", "2", "3", "4", "5", "6", "7", "8",
    "9", "10", "11", "12", "13", "14", "15", "16"
};

static const _NT_parameter parameters[] = {
    { .name = "Delay", .min = 0, .max = 100, .def = 25, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Repeats", .min = 1, .max = 8, .def = 3, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Decay", .min = 0, .max = 90, .def = 20, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "In Ch", .min = 1, .max = 17, .def = 17, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = enumStringsInputChannel },
    { .name = "Out Ch", .min = 1, .max = 16, .def = 1, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = enumStringsOutputChannel },
};

static const uint8_t page1[] = { kParamDelayTime, kParamRepeats, kParamVelocityDecay };
static const uint8_t page2[] = { kParamInputChannel, kParamOutputChannel };

static const _NT_parameterPage pages[] = {
    { .name = "Delay", .numParams = ARRAY_SIZE(page1), .params = page1 },
    { .name = "MIDI", .numParams = ARRAY_SIZE(page2), .params = page2 },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specifications)
{
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof(_midiDelayAlgorithm);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications)
{
    _midiDelayAlgorithm* alg = new (ptrs.sram) _midiDelayAlgorithm();
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    return alg;
}

void parameterChanged(_NT_algorithm* self, int p)
{
    _midiDelayAlgorithm* pThis = (_midiDelayAlgorithm*)self;
    pThis->updateParameters();
}

void midiMessage(_NT_algorithm* self, uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
    _midiDelayAlgorithm* pThis = (_midiDelayAlgorithm*)self;
    
    // Extract channel from incoming message
    uint8_t status = byte0 & 0xF0;
    uint8_t channel = byte0 & 0x0F;
    
    // Filter by input channel (if not "All")
    if (pThis->inputChannel < 16) { // Not "All"
        if (channel != pThis->inputChannel) {
            return; // Ignore this message
        }
    }
    
    // Only process Note On, Note Off, and CC messages
    if (status == 0x80 || status == 0x90 || status == 0xB0) {
        // For Note Off or Note On with velocity 0, we still want to delay it
        pThis->addDelayedNote(byte0, byte1, byte2);
    }
}

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4)
{
    _midiDelayAlgorithm* pThis = (_midiDelayAlgorithm*)self;
    
    // Calculate time step in seconds
    float dt = (4.0f * numFramesBy4) / NT_globals.sampleRate;
    
    // Process all active delayed notes
    for (int i = 0; i < 64; i++) {
        _midiDelayAlgorithm::DelayedNote& note = pThis->ringBuffer[i];
        
        if (!note.active) continue;
        
        note.timeLeft -= dt;
        
        if (note.timeLeft <= 0.0f) {
            // Time to play this note
            
            // Calculate decayed velocity
            int repeatsDone = note.totalRepeats - note.repeatsLeft;
            float decayFactor = powf(1.0f - pThis->velocityDecay, (float)repeatsDone);
            uint8_t newVelocity = (uint8_t)(note.originalVel * decayFactor);
            
            // Ensure velocity doesn't go to zero unless it was originally zero
            if (newVelocity == 0 && note.originalVel > 0) {
                newVelocity = 1;
            }
            
            // Send the MIDI message with decayed velocity
            uint8_t status = note.status & 0xF0;
            if (status == 0x90 || status == 0x80) {
                // Note message - apply velocity decay to data2
                NT_sendMidi3ByteMessage(~0, note.status, note.data1, newVelocity);
            } else {
                // CC or other message - apply decay to data2
                uint8_t newValue = (uint8_t)(note.data2 * decayFactor);
                NT_sendMidi3ByteMessage(~0, note.status, note.data1, newValue);
            }
            
            // Check if more repeats are needed
            note.repeatsLeft--;
            if (note.repeatsLeft > 0) {
                note.timeLeft = pThis->delayTime; // Reset timer
            } else {
                note.active = false; // Done with this note
            }
        }
    }
}

static const _NT_factory factory = 
{
    .guid = NT_MULTICHAR('M', 'D', 'L', 'Y'),
    .name = "MIDI Delay",
    .description = "MIDI delay with velocity decay",
    .numSpecifications = 0,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .step = step,
    .parameterChanged = parameterChanged,
    .midiMessage = midiMessage,
    .tags = kNT_tagUtility,
};

uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
    switch (selector)
    {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)((data == 0) ? &factory : NULL);
    }
    return 0;
}