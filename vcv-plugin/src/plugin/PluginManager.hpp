#pragma once
#include <rack.hpp>
#include <string>
#include <vector>
#include <functional>
#include "../nt_api_interface.h"

#ifdef ARCH_WIN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using namespace rack;

// Observer interface for plugin state changes
class IPluginStateObserver {
public:
    virtual ~IPluginStateObserver() = default;
    virtual void onPluginLoaded(const std::string& path) = 0;
    virtual void onPluginUnloaded() = 0;
    virtual void onPluginError(const std::string& error) = 0;
};

// Plugin management system extracted from DistingNT
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    // Plugin lifecycle
    bool loadPlugin(const std::string& path);
    bool loadPlugin(const std::string& path, const std::vector<int32_t>& customSpecifications);
    void unloadPlugin();
    void reloadPlugin();
    bool isLoaded() const;
    
    // Plugin access
    _NT_factory* getFactory() const { return pluginFactory; }
    _NT_algorithm* getAlgorithm() const { return pluginAlgorithm; }
    void* getSharedMemory() const { return pluginSharedMemory; }
    void* getInstanceMemory() const { return pluginInstanceMemory; }
    const std::string& getPluginPath() const { return pluginPath; }
    const std::vector<int32_t>& getSpecifications() const { return pluginSpecifications; }
    
    // Observer pattern
    void addObserver(IPluginStateObserver* observer);
    void removeObserver(IPluginStateObserver* observer);
    
    // Loading status
    const std::string& getLoadingMessage() const { return loadingMessage; }
    float getLoadingMessageTimer() const { return loadingMessageTimer; }
    void updateLoadingTimer(float deltaTime);
    
    // Safe execution wrapper
    template<typename Func>
    void safeExecute(Func&& func, const char* context) {
        if (!isLoaded()) return;
        try {
            func();
        } catch (const std::exception& e) {
            INFO("Plugin crashed in %s: %s", context, e.what());
            notifyError("Plugin crashed in " + std::string(context) + ": " + e.what());
            unloadPlugin();
        } catch (...) {
            INFO("Plugin crashed in %s: unknown error", context);
            notifyError("Plugin crashed in " + std::string(context) + ": unknown error");
            unloadPlugin();
        }
    }
    
    // Plugin state restoration methods
    void restorePluginState(const std::string& pluginStateJson);
    void callSetupUi(float potValues[3]);
    void initializeParameterSystem(class ParameterSystem* parameterSystem);
    
private:
    // Plugin state
    void* pluginHandle = nullptr;
    _NT_factory* pluginFactory = nullptr;
    _NT_algorithm* pluginAlgorithm = nullptr;
    void* pluginSharedMemory = nullptr;
    void* pluginInstanceMemory = nullptr;
    std::string pluginPath;
    std::string lastPluginFolder;
    
    // Plugin specifications
    std::vector<int32_t> currentSpecifications;
    bool useCustomSpecifications = false;
    std::vector<int32_t> pluginSpecifications;
    
    // Status
    std::string loadingMessage;
    float loadingMessageTimer = 0.f;
    
    // Observers
    std::vector<IPluginStateObserver*> observers;
    
    // Internal helpers
    bool validatePlugin();
    bool initializePlugin();
    void cleanupPlugin();
    void notifyLoaded();
    void notifyUnloaded();
    void notifyError(const std::string& error);
    
    // Pointer validation helper
    bool isValidPointer(void* ptr) const;
};