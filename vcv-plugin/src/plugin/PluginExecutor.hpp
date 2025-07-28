#pragma once
#include <rack.hpp>
#include <functional>
#include "../nt_api_interface.h"

using namespace rack;

// Forward declaration
class PluginManager;

// Safe plugin execution wrapper with comprehensive error handling
class PluginExecutor {
public:
    PluginExecutor(PluginManager* manager);
    ~PluginExecutor() = default;
    
    // Audio processing - most critical, must be real-time safe
    void safeStep(float* buses, int numFrames);
    
    // MIDI handling
    void safeMidiMessage(uint8_t byte0, uint8_t byte1, uint8_t byte2);
    void safeMidiRealtime(uint8_t byte);
    
    // Parameter handling
    void safeParameterChanged(int paramIndex);
    
    // Display rendering
    bool safeDraw();
    
    // State persistence
    bool safeSerialise(uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesWritten);
    bool safeDeserialise(const uint8_t* buffer, uint32_t bufferSize);
    
    // General safe execution template for any plugin function
    template<typename Func>
    void safeExecute(const char* context, Func&& func) {
        if (!isPluginValid()) {
            return;
        }
        
        try {
            func();
        } catch (const std::exception& e) {
            handleException(context, e.what());
        } catch (...) {
            handleException(context, "unknown error");
        }
    }
    
    // Safe execution template with return value
    template<typename RetType, typename Func>
    RetType safeExecuteWithReturn(const char* context, Func&& func, RetType defaultReturn = RetType()) {
        if (!isPluginValid()) {
            return defaultReturn;
        }
        
        try {
            return func();
        } catch (const std::exception& e) {
            handleException(context, e.what());
            return defaultReturn;
        } catch (...) {
            handleException(context, "unknown error");
            return defaultReturn;
        }
    }
    
    // Error statistics
    struct ErrorStats {
        uint32_t totalErrors = 0;
        uint32_t stepErrors = 0;
        uint32_t midiErrors = 0;
        uint32_t parameterErrors = 0;
        uint32_t drawErrors = 0;
        uint32_t stateErrors = 0;
        std::string lastError;
        float lastErrorTime = 0.0f;
    };
    
    const ErrorStats& getErrorStats() const { return errorStats; }
    void resetErrorStats();
    void updateErrorTimer(float deltaTime);
    
    // Plugin validation
    bool isPluginValid() const;
    
private:
    PluginManager* pluginManager;
    ErrorStats errorStats;
    
    // Exception handling
    void handleException(const char* context, const char* error);
    void incrementErrorCounter(const char* context);
    
    // Safety checks
    bool checkPluginPointers() const;
    bool isAudioContext() const;
    
    // Real-time safe logging (minimal allocation)
    void rtSafeLog(const char* context, const char* error);
};