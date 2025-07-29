// Test plugin for customUi debugging
// Displays all pot values, encoder deltas, and button events on screen

#include <math.h>
#include <new>
#include <cstring>
#include <cstdio>
#include <distingnt/api.h>

struct TestCustomUiAlgorithm : public _NT_algorithm
{
    // Current values
    float pots[3];
    int encoders[2];
    uint16_t buttons;
    uint16_t lastButtons;
    
    // Event history (circular buffer)
    static const int MAX_EVENTS = 8;
    char events[MAX_EVENTS][64];
    int eventIndex;
    
    // Control flags for what changed
    uint16_t lastControls;
    
    TestCustomUiAlgorithm() {
        pots[0] = pots[1] = pots[2] = 0.5f;
        encoders[0] = encoders[1] = 0;
        buttons = 0;
        lastButtons = 0;
        eventIndex = 0;
        lastControls = 0;
        for (int i = 0; i < MAX_EVENTS; i++) {
            events[i][0] = '\0';
        }
    }
    
    void addEvent(const char* event) {
        strncpy(events[eventIndex], event, 63);
        events[eventIndex][63] = '\0';
        eventIndex = (eventIndex + 1) % MAX_EVENTS;
    }
};

// Parameters
enum {
    kParamInput,
    kParamOutput,
    kParamOutputMode,
};

static const _NT_parameter parameters[] = {
    NT_PARAMETER_AUDIO_INPUT("Input", 1, 1)
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE("Output", 1, 1)
};

static const uint8_t page1[] = { kParamInput, kParamOutput, kParamOutputMode };

static const _NT_parameterPage pages[] = {
    { .name = "Routing", .numParams = ARRAY_SIZE(page1), .params = page1 },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

// Calculate memory requirements
void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specifications)
{
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof(TestCustomUiAlgorithm);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

// Construct algorithm
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications)
{
    TestCustomUiAlgorithm* alg = new (ptrs.sram) TestCustomUiAlgorithm();
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    return alg;
}

// Setup UI (initialize pot values)
void setupUi(_NT_algorithm* algorithm, _NT_float3& potValues)
{
    TestCustomUiAlgorithm* alg = (TestCustomUiAlgorithm*)algorithm;
    
    // Log the call with pot values
    char buf[128];
    snprintf(buf, sizeof(buf), "setupUi: %.3f %.3f %.3f", potValues[0], potValues[1], potValues[2]);
    alg->addEvent(buf);
    
    // Print to console for debugging
    printf("TEST PLUGIN: setupUi called with pots: %.3f %.3f %.3f\n", potValues[0], potValues[1], potValues[2]);
    
    alg->pots[0] = potValues[0];
    alg->pots[1] = potValues[1];
    alg->pots[2] = potValues[2];
}

// Declare that we handle all controls
uint32_t hasCustomUi(_NT_algorithm* self)
{
    return kNT_potL | kNT_potC | kNT_potR | 
           kNT_encoderL | kNT_encoderR |
           kNT_button1 | kNT_button2 | kNT_button3 | kNT_button4 |
           kNT_potButtonL | kNT_potButtonC | kNT_potButtonR |
           kNT_encoderButtonL | kNT_encoderButtonR;
}

// Handle custom UI events
void customUi(_NT_algorithm* algorithm, const _NT_uiData& uiData)
{
    TestCustomUiAlgorithm* alg = (TestCustomUiAlgorithm*)algorithm;
    
    // Update pot values
    alg->pots[0] = uiData.pots[0];
    alg->pots[1] = uiData.pots[1];
    alg->pots[2] = uiData.pots[2];
    
    // Update encoder values
    alg->encoders[0] = uiData.encoders[0];
    alg->encoders[1] = uiData.encoders[1];
    
    // Store control flags
    alg->lastControls = uiData.controls;
    
    // Check for pot changes
    if (uiData.controls & kNT_potL) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Pot L: %.3f", uiData.pots[0]);
        alg->addEvent(buf);
    }
    if (uiData.controls & kNT_potC) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Pot C: %.3f", uiData.pots[1]);
        alg->addEvent(buf);
    }
    if (uiData.controls & kNT_potR) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Pot R: %.3f", uiData.pots[2]);
        alg->addEvent(buf);
    }
    
    // Check for encoder changes
    if (uiData.controls & kNT_encoderL) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Encoder L: %+d", uiData.encoders[0]);
        alg->addEvent(buf);
    }
    if (uiData.controls & kNT_encoderR) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Encoder R: %+d", uiData.encoders[1]);
        alg->addEvent(buf);
    }
    
    // Check for button changes
    uint16_t currentButtons = uiData.lastButtons;
    uint16_t buttonChanges = currentButtons ^ alg->lastButtons;
    
    for (int i = 0; i < 4; i++) {
        uint16_t buttonMask = kNT_button1 << i;
        if (uiData.controls & buttonMask) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Button %d: %s", i + 1, 
                     (currentButtons & buttonMask) ? "PRESS" : "RELEASE");
            alg->addEvent(buf);
        }
    }
    
    // Check for pot button changes
    for (int i = 0; i < 3; i++) {
        uint16_t buttonMask = kNT_potButtonL << i;
        if (uiData.controls & buttonMask) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Pot %c btn: %s", 
                     i == 0 ? 'L' : (i == 1 ? 'C' : 'R'),
                     (currentButtons & buttonMask) ? "PRESS" : "RELEASE");
            alg->addEvent(buf);
        }
    }
    
    // Check for encoder button changes
    for (int i = 0; i < 2; i++) {
        uint16_t buttonMask = kNT_encoderButtonL << i;
        if (uiData.controls & buttonMask) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Enc %c btn: %s", 
                     i == 0 ? 'L' : 'R',
                     (currentButtons & buttonMask) ? "PRESS" : "RELEASE");
            alg->addEvent(buf);
        }
    }
    
    alg->lastButtons = currentButtons;
}

// Draw the display
bool draw(_NT_algorithm* algorithm)
{
    TestCustomUiAlgorithm* alg = (TestCustomUiAlgorithm*)algorithm;
    
    // Clear screen
    memset(NT_screen, 0, 256 * 64 / 2);
    
    // Title
    NT_drawText(128, 2, "CustomUI Test", 15, kNT_textCentre, kNT_textNormal);
    
    // Draw current pot values
    char buf[64];
    snprintf(buf, sizeof(buf), "Pots: %.2f %.2f %.2f", 
             alg->pots[0], alg->pots[1], alg->pots[2]);
    NT_drawText(2, 12, buf, 15, kNT_textLeft, kNT_textNormal);
    
    // Draw last encoder deltas
    snprintf(buf, sizeof(buf), "Enc: %+d %+d", 
             alg->encoders[0], alg->encoders[1]);
    NT_drawText(2, 22, buf, 15, kNT_textLeft, kNT_textNormal);
    
    // Draw button states
    snprintf(buf, sizeof(buf), "Btn: %c%c%c%c", 
             (alg->lastButtons & kNT_button1) ? '1' : '-',
             (alg->lastButtons & kNT_button2) ? '2' : '-',
             (alg->lastButtons & kNT_button3) ? '3' : '-',
             (alg->lastButtons & kNT_button4) ? '4' : '-');
    NT_drawText(2, 32, buf, 15, kNT_textLeft, kNT_textNormal);
    
    // Draw control flags
    snprintf(buf, sizeof(buf), "Ctrl: 0x%04X", alg->lastControls);
    NT_drawText(128, 32, buf, 15, kNT_textLeft, kNT_textNormal);
    
    // Draw event history
    NT_drawText(2, 42, "Events:", 10, kNT_textLeft, kNT_textNormal);
    int y = 50;
    for (int i = 0; i < 5 && i < TestCustomUiAlgorithm::MAX_EVENTS; i++) {
        int idx = (alg->eventIndex - 1 - i + TestCustomUiAlgorithm::MAX_EVENTS) % TestCustomUiAlgorithm::MAX_EVENTS;
        if (alg->events[idx][0] != '\0') {
            NT_drawText(2, y, alg->events[idx], 10, kNT_textLeft, kNT_textTiny);
            y += 6;
        }
    }
    
    return true; // Suppress default display
}

// Process audio (minimal implementation)
void step(_NT_algorithm* algorithm, float* buses, int frames)
{
    TestCustomUiAlgorithm* alg = (TestCustomUiAlgorithm*)algorithm;
    
    // Simple passthrough
    int numFrames = frames * 4;
    const float* in = buses + (alg->v[kParamInput] - 1) * numFrames;
    float* out = buses + (alg->v[kParamOutput] - 1) * numFrames;
    
    if (alg->v[kParamOutputMode]) {
        // Replace mode
        memcpy(out, in, numFrames * sizeof(float));
    } else {
        // Add mode
        for (int i = 0; i < numFrames; i++) {
            out[i] += in[i];
        }
    }
}

// Factory structure
static _NT_factory testCustomUiFactory = {
    .guid = NT_MULTICHAR('T', 'C', 'U', 'I'),
    .name = "CustomUI Test",
    .description = "Test plugin for customUi debugging",
    .numSpecifications = 0,
    .specifications = nullptr,
    .calculateStaticRequirements = nullptr,
    .initialise = nullptr,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = nullptr,
    .step = step,
    .draw = draw,
    .midiRealtime = nullptr,
    .midiMessage = nullptr,
    .tags = kNT_tagUtility,
    .hasCustomUi = hasCustomUi,
    .customUi = customUi,
    .setupUi = setupUi,
    .serialise = nullptr,
    .deserialise = nullptr,
};

// Plugin entry point
extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t param)
{
    switch (selector) {
        case kNT_selector_factoryInfo:
            return (uintptr_t)&testCustomUiFactory;
            
        default:
            return 0;
    }
}