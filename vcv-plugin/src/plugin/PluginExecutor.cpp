#include "PluginExecutor.hpp"
#include "PluginManager.hpp"
#include <rack.hpp>
#include <atomic>

using namespace rack;

PluginExecutor::PluginExecutor(PluginManager* manager) : pluginManager(manager) {
    resetErrorStats();
}

void PluginExecutor::safeStep(float* buses, int numFrames) {
    if (!checkPluginPointers()) return;
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    if (!factory->step) return;
    
    safeExecute("step", [&]() {
        factory->step(algorithm, buses, numFrames);
    });
}

void PluginExecutor::safeMidiMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    if (!checkPluginPointers()) return;
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    if (!factory->midiMessage) return;
    
    safeExecute("midiMessage", [&]() {
        factory->midiMessage(algorithm, byte0, byte1, byte2);
    });
}

void PluginExecutor::safeMidiRealtime(uint8_t byte) {
    if (!checkPluginPointers()) return;
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    if (!factory->midiRealtime) return;
    
    safeExecute("midiRealtime", [&]() {
        factory->midiRealtime(algorithm, byte);
    });
}

void PluginExecutor::safeMidiSysEx(const uint8_t* data, uint32_t count) {
    if (!checkPluginPointers() || !data || count == 0) return;

    _NT_factory* factory = pluginManager->getFactory();

    if (!factory->midiSysEx) return;

    safeExecute("midiSysEx", [&]() {
        factory->midiSysEx(data, count);
    });
}

void PluginExecutor::safeParameterChanged(int paramIndex) {
    if (!checkPluginPointers()) return;
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    if (!factory->parameterChanged) return;
    
    safeExecute("parameterChanged", [&]() {
        factory->parameterChanged(algorithm, paramIndex);
    });
}

bool PluginExecutor::safeDraw() {
    if (!checkPluginPointers()) return false;
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    if (!factory->draw) return false;
    
    return safeExecuteWithReturn<bool>("draw", [&]() -> bool {
        return factory->draw(algorithm);
    }, false);
}

bool PluginExecutor::safeSerialise(uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesWritten) {
    // TODO: Implement proper serialization with correct API types
    if (bytesWritten) *bytesWritten = 0;
    return false;
}

bool PluginExecutor::safeDeserialise(const uint8_t* buffer, uint32_t bufferSize) {
    // TODO: Implement proper deserialization with correct API types
    return false;
}

void PluginExecutor::resetErrorStats() {
    errorStats = ErrorStats{};
}

void PluginExecutor::updateErrorTimer(float deltaTime) {
    if (errorStats.lastErrorTime > 0.0f) {
        errorStats.lastErrorTime -= deltaTime;
    }
}

bool PluginExecutor::isPluginValid() const {
    return pluginManager && pluginManager->isLoaded() && checkPluginPointers();
}

void PluginExecutor::handleException(const char* context, const char* error) {
    errorStats.totalErrors++;
    errorStats.lastError = std::string(context) + ": " + error;
    errorStats.lastErrorTime = 5.0f; // Show error for 5 seconds
    
    incrementErrorCounter(context);
    
    // Use real-time safe logging if we're in audio context
    if (isAudioContext()) {
        rtSafeLog(context, error);
    } else {
        INFO("Plugin error in %s: %s", context, error);
    }
    
    // Unload plugin if too many errors in critical functions
    if (strcmp(context, "step") == 0 && errorStats.stepErrors > 3) {
        INFO("Too many step errors, unloading plugin");
        if (pluginManager) {
            pluginManager->unloadPlugin();
        }
    }
}

void PluginExecutor::incrementErrorCounter(const char* context) {
    if (strcmp(context, "step") == 0) {
        errorStats.stepErrors++;
    } else if (strstr(context, "midi") != nullptr) {
        errorStats.midiErrors++;
    } else if (strcmp(context, "parameterChanged") == 0) {
        errorStats.parameterErrors++;
    } else if (strcmp(context, "draw") == 0) {
        errorStats.drawErrors++;
    } else if (strstr(context, "serialise") != nullptr || strstr(context, "deserialise") != nullptr) {
        errorStats.stateErrors++;
    }
}

bool PluginExecutor::checkPluginPointers() const {
    if (!pluginManager) return false;
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    return factory && algorithm;
}

bool PluginExecutor::isAudioContext() const {
    // Simple heuristic: if we're called from a thread with high priority,
    // we're likely in the audio thread. This is implementation-dependent.
    // For now, we'll assume any call could be from audio thread.
    return true;
}

void PluginExecutor::rtSafeLog(const char* context, const char* error) {
    // Real-time safe logging - use atomic operations where possible
    // and avoid dynamic allocation
    
    static std::atomic<uint32_t> logCounter{0};
    uint32_t count = logCounter.fetch_add(1);
    
    // Only log first few errors to avoid spam
    if (count < 10) {
        // VCV Rack's INFO should be relatively safe
        INFO("Plugin RT error #%u in %s: %s", count, context, error);
    }
}