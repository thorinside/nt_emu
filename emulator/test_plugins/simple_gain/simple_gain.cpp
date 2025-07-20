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
    
    void setParameterValue(unsigned int parameterIndex, _NT_parameterValue value) {
        if (parameterIndex == 0) {
            gain_ = value.asFloat;
        }
    }
    
    _NT_parameterValue getParameterValue(unsigned int parameterIndex) {
        _NT_parameterValue value;
        if (parameterIndex == 0) {
            value.asFloat = gain_;
        } else {
            value.asFloat = 0.0f;
        }
        return value;
    }
    
private:
    float gain_;
};

// Plugin factory implementation
class SimpleGainFactory {
public:
    SimpleGainFactory() {}
    
    unsigned int getAPIVersion() {
        return kNT_apiVersion;
    }
    
    int getValue(enum _NT_selector selector, void* value, unsigned int maxLength) {
        switch (selector) {
            case kNT_selector_factoryName:
                strncpy((char*)value, "Simple Gain Factory", maxLength);
                return 0;
            case kNT_selector_factoryVersion:
                strncpy((char*)value, "1.0.0", maxLength);
                return 0;
            case kNT_selector_algorithmName:
                strncpy((char*)value, "Simple Gain", maxLength);
                return 0;
            case kNT_selector_algorithmAuthor:
                strncpy((char*)value, "Test Author", maxLength);
                return 0;
            case kNT_selector_algorithmDescription:
                strncpy((char*)value, "A simple gain plugin for testing", maxLength);
                return 0;
            case kNT_selector_numInputs:
                *(int*)value = 1;
                return 0;
            case kNT_selector_numOutputs:
                *(int*)value = 1;
                return 0;
            case kNT_selector_numParameters:
                *(int*)value = 1;
                return 0;
            case kNT_selector_canReceiveMIDI:
                *(int*)value = 0;
                return 0;
            case kNT_selector_canTransmitMIDI:
                *(int*)value = 0;
                return 0;
            default:
                return -1;
        }
    }
    
    struct _NT_staticRequirements getStaticRequirements() {
        struct _NT_staticRequirements reqs;
        reqs.memorySize = 0;
        reqs.numRequirements = 0;
        reqs.requirements = nullptr;
        return reqs;
    }
    
    int initialise(void* sharedMemory) {
        (void)sharedMemory;
        return 0;
    }
    
    struct _NT_memoryRequirements getRequirements() {
        struct _NT_memoryRequirements reqs;
        reqs.memorySize = sizeof(SimpleGain);
        reqs.numRequirements = 0;
        reqs.requirements = nullptr;
        return reqs;
    }
    
    struct _NT_algorithm* construct(void* memory) {
        // Placement new to construct the plugin in provided memory
        SimpleGain* plugin = new(memory) SimpleGain();
        
        // Create algorithm wrapper
        static struct _NT_algorithm algorithm;
        algorithm.refCon = plugin;
        
        algorithm.getAPIVersion = [](struct _NT_algorithm* self) -> unsigned int {
            return kNT_apiVersion;
        };
        
        algorithm.getValue = [](struct _NT_algorithm* self, enum _NT_selector selector, void* value, unsigned int maxLength) -> int {
            (void)self; (void)selector; (void)value; (void)maxLength;
            return -1; // Not implemented
        };
        
        algorithm.getRequirements = [](struct _NT_algorithm* self) -> struct _NT_memoryRequirements {
            (void)self;
            struct _NT_memoryRequirements reqs;
            reqs.memorySize = 0;
            reqs.numRequirements = 0;
            reqs.requirements = nullptr;
            return reqs;
        };
        
        algorithm.setParameterValue = [](struct _NT_algorithm* self, unsigned int parameterIndex, _NT_parameterValue value) {
            SimpleGain* plugin = static_cast<SimpleGain*>(self->refCon);
            plugin->setParameterValue(parameterIndex, value);
        };
        
        algorithm.getParameterValue = [](struct _NT_algorithm* self, unsigned int parameterIndex) -> _NT_parameterValue {
            SimpleGain* plugin = static_cast<SimpleGain*>(self->refCon);
            return plugin->getParameterValue(parameterIndex);
        };
        
        algorithm.step = [](struct _NT_algorithm* self, float** buffers, unsigned int numSamples) {
            SimpleGain* plugin = static_cast<SimpleGain*>(self->refCon);
            plugin->step(buffers, numSamples);
        };
        
        algorithm.draw = [](struct _NT_algorithm* self) {
            SimpleGain* plugin = static_cast<SimpleGain*>(self->refCon);
            plugin->draw();
        };
        
        // Null out MIDI handlers
        algorithm.controllerChange = nullptr;
        algorithm.noteOn = nullptr;
        algorithm.noteOff = nullptr;
        algorithm.pitchBend = nullptr;
        algorithm.programChange = nullptr;
        algorithm.channelPressure = nullptr;
        algorithm.polyKeyPressure = nullptr;
        algorithm.systemExclusive = nullptr;
        algorithm.clockTick = nullptr;
        algorithm.clockStart = nullptr;
        algorithm.clockStop = nullptr;
        algorithm.clockContinue = nullptr;
        algorithm.activeSense = nullptr;
        algorithm.systemReset = nullptr;
        algorithm.getPresetChunk = nullptr;
        algorithm.setPresetChunk = nullptr;
        algorithm.postPresetLoad = nullptr;
        
        return &algorithm;
    }
    
    void destruct(struct _NT_algorithm* algorithm) {
        if (algorithm && algorithm->refCon) {
            SimpleGain* plugin = static_cast<SimpleGain*>(algorithm->refCon);
            plugin->~SimpleGain();
        }
    }
    
    void terminate() {
        // Nothing to do
    }
};

// Global factory instance
static SimpleGainFactory g_factory;

// Create factory wrapper
static struct _NT_factory g_factory_wrapper = {
    &g_factory,  // refCon
    
    [](struct _NT_factory* self) -> unsigned int {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        return factory->getAPIVersion();
    },
    
    [](struct _NT_factory* self, enum _NT_selector selector, void* value, unsigned int maxLength) -> int {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        return factory->getValue(selector, value, maxLength);
    },
    
    [](struct _NT_factory* self) -> struct _NT_staticRequirements {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        return factory->getStaticRequirements();
    },
    
    [](struct _NT_factory* self, void* sharedMemory) -> int {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        return factory->initialise(sharedMemory);
    },
    
    [](struct _NT_factory* self, void* memory) -> struct _NT_algorithm* {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        return factory->construct(memory);
    },
    
    [](struct _NT_factory* self, struct _NT_algorithm* algorithm) {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        factory->destruct(algorithm);
    },
    
    [](struct _NT_factory* self) {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        factory->terminate();
    },
    
    [](struct _NT_factory* self) -> struct _NT_memoryRequirements {
        SimpleGainFactory* factory = static_cast<SimpleGainFactory*>(self->refCon);
        return factory->getRequirements();
    }
};

// Export the factory function
extern "C" {
    struct _NT_factory* NT_getFactoryPtr() {
        return &g_factory_wrapper;
    }
}