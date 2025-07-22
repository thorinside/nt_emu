#include <distingnt/api.h>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <new>

// Simple gain plugin for testing the emulator
class SimpleGain {
public:
    SimpleGain() : gain_(1.0f) {}
    
    void step(float** buffers, unsigned int numSamples) {
        // Apply gain to bus 0 (input -> output)
        for (unsigned int i = 0; i < numSamples; i++) {
            buffers[0][i] *= gain_;
        }
    }
    
    void draw() {
        // The drawing will be handled by the emulator
        // For now, just a simple placeholder that doesn't call NT functions
    }
    
    void setParameterValue(unsigned int parameterIndex, float value) {
        if (parameterIndex == 0) {
            gain_ = value;
        }
    }
    
    float getParameterValue(unsigned int parameterIndex) {
        if (parameterIndex == 0) {
            return gain_;
        } else {
            return 0.0f;
        }
    }
    
private:
    float gain_;
};

// Simple factory helper functions (no longer need a class)
_NT_algorithm* construct_simple_gain(_NT_algorithmMemoryPtrs memPtrs, _NT_algorithmRequirements /* reqs */, const int32_t* /* specifications */) {
    // Need to place both SimpleGain and _NT_algorithm in memory
    // Put SimpleGain at the beginning of SRAM
    new(memPtrs.sram) SimpleGain();
    
    // Put _NT_algorithm structure right after SimpleGain
    uint8_t* algoPtr = (uint8_t*)memPtrs.sram + sizeof(SimpleGain);
    _NT_algorithm* algorithm = (_NT_algorithm*)algoPtr;
    
    // Initialize algorithm structure for current API (no function pointers, just data)
    algorithm->parameters = nullptr;      // No parameters for simple gain
    algorithm->parameterPages = nullptr;  // No parameter pages
    algorithm->vIncludingCommon = nullptr; // Will be managed by system
    algorithm->v = nullptr;               // Will be managed by system
    
    return algorithm;
}

// Create factory wrapper for current API
static struct _NT_factory g_factory_wrapper = {
    NT_MULTICHAR('S', 'G', 'A', 'N'),  // guid
    "Simple Gain",                      // name  
    "A simple gain plugin for testing", // description
    0,                                  // numSpecifications
    nullptr,                           // specifications
    
    // calculateStaticRequirements
    [](_NT_staticRequirements &req) {
        req.dram = 0; // No shared memory needed
    },
    
    // initialise  
    [](_NT_staticMemoryPtrs & /* ptrs */, const _NT_staticRequirements & /* req */) {
        // Nothing to initialize
    },
    
    // calculateRequirements
    [](_NT_algorithmRequirements &req, const int32_t * /* specifications */) {
        req.numParameters = 1;  // One gain parameter
        req.sram = sizeof(SimpleGain) + sizeof(_NT_algorithm);
        req.dram = 0;
        req.dtc = 0; 
        req.itc = 0;
    },
    
    // construct
    [](const _NT_algorithmMemoryPtrs &ptrs, const _NT_algorithmRequirements &req, const int32_t *specifications) -> _NT_algorithm* {
        return construct_simple_gain(const_cast<_NT_algorithmMemoryPtrs&>(ptrs), const_cast<_NT_algorithmRequirements&>(req), specifications);
    },
    
    // parameterChanged
    [](_NT_algorithm * /* self */, int /* p */) {
        // Parameter changes will be handled by VCV system
    },
    
    // step
    [](_NT_algorithm *self, float *busFrames, int numFramesBy4) {
        // The SimpleGain plugin is at the start of SRAM, before the _NT_algorithm structure
        uint8_t* sramPtr = (uint8_t*)self - sizeof(SimpleGain);
        SimpleGain* plugin = (SimpleGain*)sramPtr;
        float* buffers[1] = { busFrames };
        plugin->step(buffers, numFramesBy4 * 4);
    },
    
    // draw
    [](_NT_algorithm *self) -> bool {
        // The SimpleGain plugin is at the start of SRAM, before the _NT_algorithm structure
        uint8_t* sramPtr = (uint8_t*)self - sizeof(SimpleGain);
        SimpleGain* plugin = (SimpleGain*)sramPtr;
        plugin->draw();
        return false; // Don't suppress standard parameter display
    },
    
    // midiRealtime
    [](_NT_algorithm * /* self */, uint8_t /* byte */) {
        // No MIDI realtime handling
    },
    
    // midiMessage
    [](_NT_algorithm * /* self */, uint8_t /* byte0 */, uint8_t /* byte1 */, uint8_t /* byte2 */) {
        // No MIDI message handling
    },
    
    0, // tags (no special tags)
    
    // hasCustomUi
    [](_NT_algorithm * /* self */) -> uint32_t {
        return 0; // No custom UI
    },
    
    // customUi
    [](_NT_algorithm * /* self */, const _NT_uiData & /* data */) {
        // No custom UI handling
    },
    
    // setupUi
    [](_NT_algorithm * /* self */, _NT_float3 & /* pots */) {
        // No UI setup needed
    },
    
    // serialise
    [](_NT_algorithm * /* self */, class _NT_jsonStream & /* stream */) {
        // No serialization needed
    },
    
    // deserialise
    [](_NT_algorithm * /* self */, class _NT_jsonParse & /* parse */) -> bool {
        return true; // Always succeed (no data to deserialize)
    }
};

// Export the plugin entry function for current API
extern "C" {
    uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
        switch (selector) {
            case kNT_selector_version:
                return kNT_apiVersionCurrent;
            case kNT_selector_numFactories:
                return 1;  // We have one factory
            case kNT_selector_factoryInfo:
                if (data == 0) {
                    return (uintptr_t)&g_factory_wrapper;
                }
                return 0;
            default:
                return 0;
        }
    }
    
    // Keep old function for backward compatibility
    struct _NT_factory* NT_getFactoryPtr() {
        return &g_factory_wrapper;
    }
}