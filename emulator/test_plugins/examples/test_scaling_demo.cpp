#include <iostream>
#include <distingnt/api.h>
#include <iomanip>

// Demo plugin to show all scaling factors and units
struct ScalingDemoAlgorithm : public _NT_algorithm {
    ScalingDemoAlgorithm() {}
    ~ScalingDemoAlgorithm() {}
};

// Example parameters showcasing different units and scaling
static const _NT_parameter scalingTestParameters[] = {
    // Standard parameters
    { .name = "Basic Value", .min = 0, .max = 100, .def = 50, .unit = kNT_unitNone, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    { .name = "Percentage", .min = 0, .max = 100, .def = 75, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    
    // Scaling demonstrations
    { .name = "Voltage x10", .min = 0, .max = 100, .def = 50, .unit = kNT_unitVolts, .scaling = kNT_scaling10, .enumStrings = nullptr }, // 0.0 to 10.0V
    { .name = "Frequency x100", .min = 1000, .max = 2000, .def = 4400, .unit = kNT_unitHz, .scaling = kNT_scaling100, .enumStrings = nullptr }, // 10.00 to 20.00 Hz
    { .name = "Time x1000", .min = 1000, .max = 5000, .def = 2500, .unit = kNT_unitMs, .scaling = kNT_scaling1000, .enumStrings = nullptr }, // 1.000 to 5.000 ms
    
    // dB units
    { .name = "Gain", .min = -600, .max = 600, .def = 0, .unit = kNT_unitDb, .scaling = kNT_scaling100, .enumStrings = nullptr }, // -6.00 to +6.00 dB
    { .name = "Level MinInf", .min = -1000, .max = 0, .def = -200, .unit = kNT_unitDb_minInf, .scaling = kNT_scaling100, .enumStrings = nullptr },
    
    // Musical units
    { .name = "Pitch", .min = 0, .max = 127, .def = 60, .unit = kNT_unitMIDINote, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    { .name = "Fine Tune", .min = -50, .max = 50, .def = 0, .unit = kNT_unitCents, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    { .name = "Transpose", .min = -24, .max = 24, .def = 0, .unit = kNT_unitSemitones, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    
    // Time units
    { .name = "Delay", .min = 0, .max = 1000, .def = 100, .unit = kNT_unitMs, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    { .name = "Duration", .min = 0, .max = 10, .def = 1, .unit = kNT_unitSeconds, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    { .name = "Tempo", .min = 60, .max = 200, .def = 120, .unit = kNT_unitBPM, .scaling = kNT_scalingNone, .enumStrings = nullptr },
    
    // Enum parameter
    { .name = "Wave Type", .min = 0, .max = 2, .def = 0, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = nullptr }
};

// Enum strings for the wave type parameter
static const char* waveTypeEnums[] = {
    "Sine",
    "Square", 
    "Sawtooth",
    nullptr
};

// Update the enum parameter to point to our strings
static _NT_parameter modifiedParameters[ARRAY_SIZE(scalingTestParameters)];

static const uint8_t page1[] = { 0, 1, 2 };  // Basic values
static const uint8_t page2[] = { 3, 4, 5 };  // Scaled values  
static const uint8_t page3[] = { 6, 7 };     // dB values
static const uint8_t page4[] = { 8, 9, 10 }; // Musical
static const uint8_t page5[] = { 11, 12, 13 }; // Time and tempo
static const uint8_t page6[] = { 14 };       // Enum

static const _NT_parameterPage pages[] = {
    { .name = "Basic", .numParams = ARRAY_SIZE(page1), .params = page1 },
    { .name = "Scaled", .numParams = ARRAY_SIZE(page2), .params = page2 },
    { .name = "Decibels", .numParams = ARRAY_SIZE(page3), .params = page3 },
    { .name = "Musical", .numParams = ARRAY_SIZE(page4), .params = page4 },
    { .name = "Timing", .numParams = ARRAY_SIZE(page5), .params = page5 },
    { .name = "Enum", .numParams = ARRAY_SIZE(page6), .params = page6 },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specifications) {
    (void)specifications;
    req.numParameters = ARRAY_SIZE(scalingTestParameters);
    req.sram = sizeof(ScalingDemoAlgorithm);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications) {
    (void)req;
    (void)specifications;
    
    // Copy parameters and fix up enum strings
    memcpy(modifiedParameters, scalingTestParameters, sizeof(scalingTestParameters));
    modifiedParameters[ARRAY_SIZE(scalingTestParameters) - 1].enumStrings = waveTypeEnums;
    
    ScalingDemoAlgorithm* alg = new (ptrs.sram) ScalingDemoAlgorithm();
    alg->parameters = modifiedParameters;
    alg->parameterPages = &parameterPages;
    return alg;
}

void parameterChanged(_NT_algorithm* self, int p) {
    ScalingDemoAlgorithm* pThis = (ScalingDemoAlgorithm*)self;
    std::cout << "Parameter [" << p << "] changed to: " << pThis->v[p] << std::endl;
}

bool draw(_NT_algorithm* self) {
    (void)self;
    return false;
}

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    (void)self;
    (void)busFrames;
    (void)numFramesBy4;
}

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('S', 'c', 'a', 'l'),
    .name = "Scaling Demo",
    .description = "Demonstrates all parameter scaling and units",
    .numSpecifications = 0,
    .specifications = nullptr,
    .calculateStaticRequirements = nullptr,
    .initialise = nullptr,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = nullptr,
    .midiMessage = nullptr,
    .tags = kNT_tagUtility,
    .hasCustomUi = nullptr,
    .customUi = nullptr,
    .setupUi = nullptr,
    .serialise = nullptr,
    .deserialise = nullptr,
};

extern "C" {
    uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
        switch (selector) {
        case kNT_selector_version:
            return kNT_apiVersionCurrent;
        case kNT_selector_numFactories:
            return 1;
        case kNT_selector_factoryInfo:
            return (uintptr_t)((data == 0) ? &factory : nullptr);
        }
        return 0;
    }
}