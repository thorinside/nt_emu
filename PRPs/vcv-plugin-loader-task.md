name: "Add Plugin Loading to VCV Rack DistingNT Module"
description: |
  Add the ability to load Disting NT plugins (.so/.dylib/.dll) via right-click context menu, with folder memory, preset persistence, and OLED loading notification.

---

# Task: VCV Rack Plugin Loading Feature

> Enable loading of native Disting NT algorithm plugins through VCV Rack's context menu system

## Critical Context

```yaml
context:
  docs:
    - url: https://vcvrack.com/docs-v2/stringsplit/MenuItem
      focus: Context menu implementation and file browser
    
    - url: https://vcvrack.com/docs-v2/rack/Module#dataToJson
      focus: Preset serialization for persistent settings
    
    - url: https://github.com/VCVRack/Rack/blob/v2/include/ui/MenuItem.hpp
      focus: Menu item types and file dialog patterns

  patterns:
    - file: /emulator/src/core/plugin_loader.cpp
      copy: Existing plugin loading implementation to adapt
    
    - file: https://github.com/clone45/voxglitch/blob/main/src/WavBank/WavBank.cpp
      copy: File browser dialog and path persistence pattern
    
    - file: https://github.com/bogaudio/BogaudioModules/blob/master/src/Pressor.cpp
      copy: Context menu with submenu organization

  gotchas:
    - issue: "VCV Rack runs in audio thread - no blocking operations"
      fix: "Load plugins asynchronously or defer to idle callback"
    
    - issue: "File paths must be cross-platform compatible"
      fix: "Use rack::system::join() for path construction"
    
    - issue: "Native plugins may crash if incompatible"
      fix: "Wrap loading in try-catch, validate API version"
    
    - issue: "Module state must handle NULL algorithm gracefully"
      fix: "Always check currentAlgorithm before use"
```

## Implementation Tasks

### 1. Setup Plugin Infrastructure

```
READ vcv-plugin/src/DistingNT.cpp:
  - UNDERSTAND: Current module structure
  - FIND: Algorithm management approach
  - NOTE: How algorithms are currently switched

UPDATE vcv-plugin/src/DistingNT.cpp:
  - ADD: Plugin loader member variable
    ```cpp
    #include <dlfcn.h> // or Windows equivalent
    
    struct DistingNT : Module {
        // ... existing members ...
        
        // Plugin loading
        std::unique_ptr<PluginLoader> pluginLoader;
        std::string pluginPath;
        std::string lastPluginFolder;
        
        // Display notification
        std::string loadingMessage;
        float loadingMessageTimer = 0.f;
        
        DistingNT() {
            // ... existing constructor ...
            pluginLoader = std::make_unique<PluginLoader>();
        }
    };
    ```
  - VALIDATE: make && echo "Infrastructure added"
  - IF_FAIL: Check include paths for plugin_loader.h
```

### 2. Create Plugin Loader Adapter

```
CREATE vcv-plugin/src/plugin_loader_vcv.cpp:
  - ADAPT: Original plugin_loader.cpp for VCV context
  - IMPLEMENT:
    ```cpp
    #include "plugin_loader_vcv.hpp"
    #include <rack.hpp>
    
    #ifdef ARCH_WIN
      #include <windows.h>
      #define PLUGIN_EXT ".dll"
    #elif ARCH_MAC
      #include <dlfcn.h>
      #define PLUGIN_EXT ".dylib"
    #else
      #include <dlfcn.h>
      #define PLUGIN_EXT ".so"
    #endif
    
    class PluginLoaderVCV {
    private:
        void* handle = nullptr;
        _NT_factory* factory = nullptr;
        _NT_algorithm* algorithm = nullptr;
        std::string currentPath;
        
    public:
        ~PluginLoaderVCV() {
            unload();
        }
        
        bool load(const std::string& path) {
            unload();
            
            // Platform-specific loading
            #ifdef ARCH_WIN
                handle = LoadLibraryA(path.c_str());
            #else
                handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
            #endif
            
            if (!handle) {
                WARN("Failed to load plugin: %s", getError().c_str());
                return false;
            }
            
            // Get factory function
            #ifdef ARCH_WIN
                factory = (_NT_factory*)GetProcAddress((HMODULE)handle, "_NT_factory");
            #else
                factory = (_NT_factory*)dlsym(handle, "_NT_factory");
            #endif
            
            if (!factory) {
                unload();
                return false;
            }
            
            // Validate and create algorithm
            if (!validatePlugin()) {
                unload();
                return false;
            }
            
            currentPath = path;
            return true;
        }
        
        void unload() {
            if (algorithm && factory) {
                // Cleanup algorithm
                factory(k_algorithm_destroy, algorithm);
            }
            
            if (handle) {
                #ifdef ARCH_WIN
                    FreeLibrary((HMODULE)handle);
                #else
                    dlclose(handle);
                #endif
            }
            
            handle = nullptr;
            factory = nullptr;
            algorithm = nullptr;
            currentPath.clear();
        }
        
        bool validatePlugin() {
            // Check API version
            int version = 0;
            factory(k_api_version, &version);
            if (version != 0x00010000) {
                WARN("Incompatible plugin API version: %x", version);
                return false;
            }
            
            // Create algorithm instance
            algorithm = (_NT_algorithm*)factory(k_algorithm_create, nullptr);
            return algorithm != nullptr;
        }
        
        std::string getError() {
            #ifdef ARCH_WIN
                return "Windows error"; // GetLastError() formatting
            #else
                return dlerror() ? dlerror() : "Unknown error";
            #endif
        }
        
        _NT_algorithm* getAlgorithm() { return algorithm; }
        const std::string& getPath() { return currentPath; }
    };
    ```
  - VALIDATE: make && echo "Plugin loader implemented"
  - IF_FAIL: Check platform defines and linking
```

### 3. Add Context Menu

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: struct DistingNTWidget : ModuleWidget {
  - ADD: Menu implementation after constructor
    ```cpp
    void appendContextMenu(Menu* menu) override {
        DistingNT* module = dynamic_cast<DistingNT*>(this->module);
        if (!module) return;
        
        menu->addChild(new MenuSeparator);
        
        // Plugin loading submenu
        menu->addChild(createSubmenuItem("Plugin", "", [=](Menu* menu) {
            // Load plugin option
            menu->addChild(createMenuItem("Load Plugin...", "", [=]() {
                loadPluginDialog(module);
            }));
            
            // Recent folder shortcut
            if (!module->lastPluginFolder.empty()) {
                menu->addChild(createMenuItem("Load from Recent Folder...", 
                    module->lastPluginFolder, [=]() {
                    loadPluginDialog(module, module->lastPluginFolder);
                }));
            }
            
            // Unload option
            if (module->pluginLoader && module->pluginLoader->isLoaded()) {
                menu->addChild(new MenuSeparator);
                menu->addChild(createMenuItem("Unload Plugin", "", [=]() {
                    module->unloadPlugin();
                }));
                
                // Show current plugin
                std::string filename = rack::system::getFilename(module->pluginPath);
                menu->addChild(createMenuLabel(string::f("Current: %s", filename.c_str())));
            }
        }));
    }
    
    void loadPluginDialog(DistingNT* module, std::string startPath = "") {
        if (startPath.empty()) {
            startPath = module->lastPluginFolder.empty() ? 
                asset::user("") : module->lastPluginFolder;
        }
        
        osdialog_filters* filters = osdialog_filters_parse(
            "Disting NT Plugin:dylib,so,dll"
        );
        
        char* pathC = osdialog_file(OSDIALOG_OPEN, startPath.c_str(), NULL, filters);
        if (pathC) {
            std::string path = pathC;
            free(pathC);
            
            // Remember folder
            module->lastPluginFolder = rack::system::getDirectory(path);
            
            // Load plugin
            module->loadPlugin(path);
        }
        
        osdialog_filters_free(filters);
    }
    ```
  - VALIDATE: make && echo "Context menu added"
  - IF_FAIL: Check osdialog include and linking
```

### 4. Implement Plugin Loading in Module

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: struct DistingNT : Module {
  - ADD: Loading methods
    ```cpp
    void loadPlugin(const std::string& path) {
        if (pluginLoader->load(path)) {
            pluginPath = path;
            
            // Replace current algorithm
            if (currentAlgorithm) {
                // Save any state if needed
            }
            
            // Wrap native algorithm
            currentAlgorithm = std::make_unique<NativeAlgorithmWrapper>(
                pluginLoader->getAlgorithm(), 
                pluginLoader->getFactory()
            );
            
            // Show loading message
            std::string filename = rack::system::getFilename(path);
            loadingMessage = string::f("Loaded: %s", filename.c_str());
            loadingMessageTimer = 2.0f; // Show for 2 seconds
            
            // Reset parameters
            onReset();
        } else {
            // Show error
            loadingMessage = "Failed to load plugin!";
            loadingMessageTimer = 3.0f;
        }
    }
    
    void unloadPlugin() {
        pluginLoader->unload();
        pluginPath.clear();
        currentAlgorithm = nullptr;
        
        loadingMessage = "Plugin unloaded";
        loadingMessageTimer = 1.5f;
        
        // Reset to default algorithm if any
        currentAlgorithmIndex = 0;
        if (!algorithms.empty()) {
            currentAlgorithm = algorithms[0].get();
        }
    }
    
    void process(const ProcessArgs& args) override {
        // Update loading message timer
        if (loadingMessageTimer > 0) {
            loadingMessageTimer -= args.sampleTime;
        }
        
        // ... existing process code ...
    }
    ```
  - VALIDATE: make && echo "Loading methods implemented"
```

### 5. Add Native Algorithm Wrapper

```
CREATE vcv-plugin/src/native_algorithm_wrapper.hpp:
  - IMPLEMENT: Adapter between native plugin and VCV interface
    ```cpp
    class NativeAlgorithmWrapper : public IDistingAlgorithm {
    private:
        _NT_algorithm* native;
        _NT_factory* factory;
        float sampleRate;
        
    public:
        NativeAlgorithmWrapper(_NT_algorithm* alg, _NT_factory* fact) 
            : native(alg), factory(fact) {}
        
        ~NativeAlgorithmWrapper() {
            // Destructor handled by PluginLoader
        }
        
        void prepare(float sr) override {
            sampleRate = sr;
            // Initialize algorithm with sample rate
            int sr_int = (int)sr;
            factory(k_algorithm_set_sample_rate, native, &sr_int);
        }
        
        void step(float* buses, int frames) override {
            // Call native step function
            factory(k_algorithm_step, native, buses, frames);
        }
        
        void parameterChanged(int param, float value) override {
            NT_PARAMETER p = { param, value };
            factory(k_algorithm_set_parameter, native, &p);
        }
        
        void buttonPressed(int button) override {
            factory(k_algorithm_button_pressed, native, button);
        }
        
        void drawDisplay(NVGcontext* vg, int width, int height) override {
            // Native plugins use different display API
            // Would need conversion layer or skip display
        }
        
        const char* getName() override {
            static char name[64];
            factory(k_algorithm_get_name, native, name, sizeof(name));
            return name;
        }
    };
    ```
  - VALIDATE: Compiles with make
```

### 6. Implement Preset Persistence

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - FIND: json_t* dataToJson() override {
  - MODIFY: Add plugin state serialization
    ```cpp
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        
        // Save current algorithm index
        json_object_set_new(rootJ, "algorithm", json_integer(currentAlgorithmIndex));
        
        // Save plugin info if loaded
        if (!pluginPath.empty()) {
            json_object_set_new(rootJ, "pluginPath", json_string(pluginPath.c_str()));
            json_object_set_new(rootJ, "lastFolder", json_string(lastPluginFolder.c_str()));
            
            // Save plugin state if it supports it
            if (currentAlgorithm) {
                json_t* pluginState = currentAlgorithm->dataToJson();
                if (pluginState) {
                    json_object_set_new(rootJ, "pluginState", pluginState);
                }
            }
        }
        
        // Save parameter values
        json_t* paramsJ = json_array();
        for (int i = 0; i < 3; i++) {
            json_array_append_new(paramsJ, json_real(params[POT_L_PARAM + i].getValue()));
        }
        json_object_set_new(rootJ, "params", paramsJ);
        
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        // Restore algorithm index
        json_t* algorithmJ = json_object_get(rootJ, "algorithm");
        if (algorithmJ) {
            currentAlgorithmIndex = json_integer_value(algorithmJ);
        }
        
        // Restore plugin if saved
        json_t* pluginPathJ = json_object_get(rootJ, "pluginPath");
        if (pluginPathJ) {
            std::string path = json_string_value(pluginPathJ);
            
            json_t* folderJ = json_object_get(rootJ, "lastFolder");
            if (folderJ) {
                lastPluginFolder = json_string_value(folderJ);
            }
            
            // Try to load plugin
            if (!path.empty() && rack::system::exists(path)) {
                loadPlugin(path);
                
                // Restore plugin state
                json_t* stateJ = json_object_get(rootJ, "pluginState");
                if (stateJ && currentAlgorithm) {
                    currentAlgorithm->dataFromJson(stateJ);
                }
            }
        }
        
        // Restore parameters
        json_t* paramsJ = json_object_get(rootJ, "params");
        if (paramsJ) {
            for (int i = 0; i < 3; i++) {
                json_t* paramJ = json_array_get(paramsJ, i);
                if (paramJ) {
                    params[POT_L_PARAM + i].setValue(json_real_value(paramJ));
                }
            }
        }
    }
    ```
  - VALIDATE: Save/load preset in VCV Rack
  - IF_FAIL: Check JSON structure in saved .vcvm file
```

### 7. Update OLED Display Widget

```
UPDATE vcv-plugin/src/display/OLEDWidget.cpp:
  - FIND: void draw(const DrawArgs& args) override {
  - ADD: Loading message display
    ```cpp
    void draw(const DrawArgs& args) override {
        if (!module) return;
        
        DistingNT* distingModule = dynamic_cast<DistingNT*>(module);
        
        // Set up 256x64 coordinate system
        nvgSave(args.vg);
        nvgScale(args.vg, box.size.x / 256.f, box.size.y / 64.f);
        
        // Clear display
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, 256, 64);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        // Show loading message if active
        if (distingModule->loadingMessageTimer > 0) {
            nvgFillColor(args.vg, nvgRGB(0, 255, 255)); // Cyan
            nvgFontSize(args.vg, 16);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            
            // Fade out effect
            float alpha = std::min(1.0f, distingModule->loadingMessageTimer);
            nvgFillColor(args.vg, nvgRGBA(0, 255, 255, (int)(255 * alpha)));
            
            nvgText(args.vg, 128, 32, distingModule->loadingMessage.c_str(), NULL);
        } else if (distingModule->currentAlgorithm) {
            // Normal algorithm display
            distingModule->currentAlgorithm->drawDisplay(args.vg, 256, 64);
        } else {
            // No algorithm loaded
            nvgFillColor(args.vg, nvgRGB(100, 100, 100));
            nvgFontSize(args.vg, 14);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, 128, 32, "No Algorithm Loaded", NULL);
        }
        
        nvgRestore(args.vg);
    }
    ```
  - VALIDATE: Visual check in VCV Rack
```

### 8. Add Error Handling

```
UPDATE vcv-plugin/src/DistingNT.cpp:
  - WRAP: Plugin loading in try-catch
    ```cpp
    void loadPlugin(const std::string& path) {
        try {
            if (pluginLoader->load(path)) {
                // ... success case ...
            } else {
                throw std::runtime_error(pluginLoader->getLastError());
            }
        } catch (const std::exception& e) {
            WARN("Plugin load failed: %s", e.what());
            loadingMessage = string::f("Error: %s", e.what());
            loadingMessageTimer = 4.0f;
        } catch (...) {
            WARN("Plugin load failed: Unknown error");
            loadingMessage = "Error: Unknown failure";
            loadingMessageTimer = 4.0f;
        }
    }
    ```
  - VALIDATE: Try loading invalid file
```

### 9. Test Loading Functionality

```
CHECKPOINT integration:
  - BUILD: make clean && make install
  - RUN: Rack -d
  - TEST: Right-click DistingNT module
  - VERIFY: "Plugin" submenu appears
  - TEST: Load test_plugins/simple_gain.dylib
  - VERIFY: Loading message appears on OLED
  - VERIFY: Plugin functions correctly
  - TEST: Save preset
  - TEST: Reload Rack and load preset
  - VERIFY: Plugin auto-loads from saved path
  - TEST: Load non-existent/invalid file
  - VERIFY: Error message displays
```

### 10. Add Safety Features

```
CREATE vcv-plugin/src/plugin_sandbox.cpp:
  - IMPLEMENT: Basic sandboxing for plugin crashes
    ```cpp
    class PluginSandbox {
    public:
        template<typename Func>
        static bool safeExecute(Func func, const char* operation) {
            try {
                func();
                return true;
            } catch (const std::exception& e) {
                WARN("%s failed: %s", operation, e.what());
                return false;
            } catch (...) {
                WARN("%s failed: Unknown exception", operation);
                return false;
            }
        }
    };
    
    // Use in process():
    void process(const ProcessArgs& args) override {
        if (currentAlgorithm) {
            PluginSandbox::safeExecute([&]() {
                currentAlgorithm->step(buses, 4);
            }, "Plugin step");
        }
    }
    ```
  - VALIDATE: Load buggy plugin without crashing
```

## Validation Checklist

- [ ] Context menu shows "Plugin" submenu
- [ ] File browser opens with correct filters (.so/.dylib/.dll)
- [ ] Plugin loads successfully and shows notification
- [ ] Loading message appears on OLED for 2 seconds
- [ ] Last folder is remembered between uses
- [ ] Plugin path saved in preset
- [ ] Plugin auto-loads when preset loaded
- [ ] Unload option works correctly
- [ ] Error messages display for invalid plugins
- [ ] No crashes with incompatible plugins
- [ ] Performance unchanged when no plugin loaded
- [ ] Native algorithms still work as before

## Rollback Strategy

```bash
# If plugin loading causes instability:
git stash
git checkout HEAD -- vcv-plugin/src/DistingNT.cpp
make clean && make install

# Gradual implementation:
# 1. First implement without persistence
# 2. Add preset save/load
# 3. Add error handling
# 4. Add sandboxing
```

## Performance Notes

- Plugin loading is one-time cost, not in audio thread
- Use defer if needed for heavy operations
- Consider caching plugin metadata
- Monitor CPU usage with loaded plugins

## Security Considerations

- Validate plugin API version before use
- Never execute arbitrary code from plugins
- Sandbox plugin calls where possible
- Log all plugin operations for debugging