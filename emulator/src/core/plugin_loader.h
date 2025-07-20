#pragma once

#include <string>
#include <memory>
#include <distingnt/api.h>

struct PluginInstance {
    void* handle = nullptr;
    _NT_factory* factory = nullptr;
    _NT_algorithm* algorithm = nullptr;
    void* shared_memory = nullptr;
    void* instance_memory = nullptr;
    std::string path;
    time_t last_modified = 0;
    bool is_loaded = false;
};

class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();
    
    bool loadPlugin(const std::string& path);
    void unloadPlugin();
    bool isLoaded() const { return plugin_.is_loaded; }
    
    _NT_algorithm* getAlgorithm() const { return plugin_.algorithm; }
    _NT_factory* getFactory() const { return plugin_.factory; }
    
    bool needsReload() const;
    bool reload();
    
    const std::string& getPath() const { return plugin_.path; }
    
private:
    PluginInstance plugin_;
    
    bool validatePlugin(void* handle);
    void cleanup();
    time_t getFileModTime(const std::string& path) const;
};