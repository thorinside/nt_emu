#pragma once

#include "../nt_api_interface.h"
#include <memory>
#include <string>
#include <functional>

// Forward declarations
struct VCVDisplayBuffer;
class PluginManager;
class ParameterSystem;

// Interface to break circular dependency between DisplayRenderer and EmulatorModule
class IDisplayDataProvider {
public:
    virtual ~IDisplayDataProvider() = default;
    
    // Display state
    virtual bool isDisplayDirty() const = 0;
    virtual void setDisplayDirty(bool dirty) = 0;
    virtual const VCVDisplayBuffer& getDisplayBuffer() const = 0;
    virtual void updateDisplay() = 0;
    
    // Menu system
    virtual int getMenuMode() const = 0;
    
    // Plugin system
    virtual bool hasLoadedPlugin() const = 0;
    virtual PluginManager* getPluginManagerPtr() const = 0;
    
    // Parameter system
    virtual ParameterSystem* getParameterSystemPtr() const = 0;
    
    // Safe plugin execution
    virtual void safeExecutePlugin(std::function<void()> func, const std::string& operation) = 0;
};