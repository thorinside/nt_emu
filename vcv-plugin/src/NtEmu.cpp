#define _DISTINGNT_SERIALISATION_INTERNAL
#include "plugin.hpp"
#include "algorithms/Algorithm.hpp"
#include "dsp/BusSystem.hpp"
#include "EmulatorCore.hpp"
#include "json_bridge.h"
#include "EncoderParamQuantity.hpp"
#include "InfiniteEncoder.hpp"
#include "widgets/PressablePot.hpp"
#include "widgets/PressableEncoder.hpp"
#include "widgets/EmulatorPressablePot.hpp"
#include "widgets/EmulatorPressableEncoder.hpp"
#include "widgets/SimpleEncoder.hpp"
#include "nt_api_interface.h"
#include "../../emulator/src/core/fonts.h"
// New modular components
#include "plugin/PluginManager.hpp"
#include "plugin/PluginExecutor.hpp"
#include "parameter/ParameterSystem.hpp"
#include "menu/MenuSystem.hpp"
#include "midi/MidiProcessor.hpp"
#include "EmulatorConstants.hpp"
#include "api/NTApiWrapper.hpp"
#include "display/IDisplayDataProvider.hpp"
#include "display/DisplayRenderer.hpp"
#include <componentlibrary.hpp>
#include <osdialog.h>
#include <map>
#include <cstring>

struct EmulatorModule;
static EmulatorModule* g_currentModule = nullptr;

// NT_screen buffer definition for the VCV plugin
__attribute__((visibility("default"))) uint8_t NT_screen[128 * 64];

// NT_globals structure for the VCV plugin
static _NT_globals vcv_nt_globals = {
    .sampleRate = 48000,
    .maxFramesPerStep = 256,
    .workBuffer = nullptr,
    .workBufferSizeBytes = 0
};

__attribute__((visibility("default"))) const _NT_globals NT_globals = vcv_nt_globals;

// Simple C API wrapper functions for plugins to use (minimal implementation)
// External API provider function
extern "C" const NT_API_Interface* getNT_API(void);

// MOVED_TO_FILE block - code moved to separate files

// Include JSON serialization support
#define _DISTINGNT_SERIALISATION_INTERNAL
#include "../../emulator/include/distingnt/serialisation.h"
#include "json_bridge.h"

// Thread-local storage for JSON bridge instances moved to api/NTApiWrapper.cpp
// thread_local std::unique_ptr<JsonStreamBridge> g_currentStream;
// thread_local std::unique_ptr<JsonParseBridge> g_currentParse;

// Moved code block - code moved to separate files

// Moved code block - code moved to separate files

// Moved code block - code moved to separate files

// For plugin loading
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

// Legacy API structures for compatibility
struct _NT_memoryRequirements {
    uint32_t memorySize;
    uint32_t numRequirements;
    void* requirements;
};

// Legacy _NT_staticRequirements with memorySize field
struct _NT_staticRequirements_old {
    uint32_t memorySize;
    uint32_t numRequirements;
    void* requirements;
};

// Forward declaration
struct EmulatorModule;

// Context-aware encoder parameter quantity
struct ContextAwareEncoderQuantity : EncoderParamQuantity {
    EmulatorModule* distingModule = nullptr;
    bool isLeftEncoder = true;
    
    std::string getDisplayValueString() override;
};

// Custom output port widget with dynamic tooltips
struct TooltipOutputPort : PJ301MPort {
    EmulatorModule* distingModule = nullptr;
    int outputIndex = 0;
    
    void onHover(const HoverEvent& e) override;
};

struct EmulatorModule : Module, IParameterObserver, IPluginStateObserver, IDisplayDataProvider {
// MOVED_TO_FILE block - code moved to separate files
    
    // Static constants for easy access throughout the class
    static constexpr int NUM_PARAMS = EmulatorConstants::NUM_PARAMS;
    static constexpr int NUM_INPUTS = EmulatorConstants::NUM_INPUTS;
    static constexpr int NUM_OUTPUTS = EmulatorConstants::NUM_OUTPUTS;
    static constexpr int NUM_LIGHTS = EmulatorConstants::NUM_LIGHTS;
    
    // Parameter IDs
    static constexpr int POT_L_PARAM = EmulatorConstants::POT_L_PARAM;
    static constexpr int POT_C_PARAM = EmulatorConstants::POT_C_PARAM;
    static constexpr int POT_R_PARAM = EmulatorConstants::POT_R_PARAM;
    static constexpr int BUTTON_1_PARAM = EmulatorConstants::BUTTON_1_PARAM;
    static constexpr int BUTTON_2_PARAM = EmulatorConstants::BUTTON_2_PARAM;
    static constexpr int BUTTON_3_PARAM = EmulatorConstants::BUTTON_3_PARAM;
    static constexpr int BUTTON_4_PARAM = EmulatorConstants::BUTTON_4_PARAM;
    static constexpr int ENCODER_L_PARAM = EmulatorConstants::ENCODER_L_PARAM;
    static constexpr int ENCODER_R_PARAM = EmulatorConstants::ENCODER_R_PARAM;
    static constexpr int ENCODER_L_PRESS_PARAM = EmulatorConstants::ENCODER_L_PRESS_PARAM;
    static constexpr int ENCODER_R_PRESS_PARAM = EmulatorConstants::ENCODER_R_PRESS_PARAM;
    static constexpr int ALGORITHM_PARAM = EmulatorConstants::ALGORITHM_PARAM;
    
    // Input IDs
    static constexpr int AUDIO_INPUT_1 = EmulatorConstants::AUDIO_INPUT_1;
    static constexpr int AUDIO_INPUT_2 = EmulatorConstants::AUDIO_INPUT_2;
    static constexpr int AUDIO_INPUT_3 = EmulatorConstants::AUDIO_INPUT_3;
    static constexpr int AUDIO_INPUT_4 = EmulatorConstants::AUDIO_INPUT_4;
    static constexpr int AUDIO_INPUT_5 = EmulatorConstants::AUDIO_INPUT_5;
    static constexpr int AUDIO_INPUT_6 = EmulatorConstants::AUDIO_INPUT_6;
    static constexpr int AUDIO_INPUT_7 = EmulatorConstants::AUDIO_INPUT_7;
    static constexpr int AUDIO_INPUT_8 = EmulatorConstants::AUDIO_INPUT_8;
    static constexpr int AUDIO_INPUT_9 = EmulatorConstants::AUDIO_INPUT_9;
    static constexpr int AUDIO_INPUT_10 = EmulatorConstants::AUDIO_INPUT_10;
    static constexpr int AUDIO_INPUT_11 = EmulatorConstants::AUDIO_INPUT_11;
    static constexpr int AUDIO_INPUT_12 = EmulatorConstants::AUDIO_INPUT_12;
    
    // Output IDs
    static constexpr int AUDIO_OUTPUT_1 = EmulatorConstants::AUDIO_OUTPUT_1;
    static constexpr int AUDIO_OUTPUT_2 = EmulatorConstants::AUDIO_OUTPUT_2;
    static constexpr int AUDIO_OUTPUT_3 = EmulatorConstants::AUDIO_OUTPUT_3;
    static constexpr int AUDIO_OUTPUT_4 = EmulatorConstants::AUDIO_OUTPUT_4;
    static constexpr int AUDIO_OUTPUT_5 = EmulatorConstants::AUDIO_OUTPUT_5;
    static constexpr int AUDIO_OUTPUT_6 = EmulatorConstants::AUDIO_OUTPUT_6;
    static constexpr int AUDIO_OUTPUT_7 = EmulatorConstants::AUDIO_OUTPUT_7;
    static constexpr int AUDIO_OUTPUT_8 = EmulatorConstants::AUDIO_OUTPUT_8;
    
    // Light IDs
    static constexpr int BUTTON_1_LIGHT = EmulatorConstants::BUTTON_1_LIGHT;
    static constexpr int BUTTON_2_LIGHT = EmulatorConstants::BUTTON_2_LIGHT;
    static constexpr int BUTTON_3_LIGHT = EmulatorConstants::BUTTON_3_LIGHT;
    static constexpr int BUTTON_4_LIGHT = EmulatorConstants::BUTTON_4_LIGHT;
    static constexpr int INPUT_LIGHT_1 = EmulatorConstants::INPUT_LIGHT_1;
    static constexpr int INPUT_LIGHT_2 = EmulatorConstants::INPUT_LIGHT_2;
    static constexpr int INPUT_LIGHT_3 = EmulatorConstants::INPUT_LIGHT_3;
    static constexpr int INPUT_LIGHT_4 = EmulatorConstants::INPUT_LIGHT_4;
    static constexpr int INPUT_LIGHT_5 = EmulatorConstants::INPUT_LIGHT_5;
    static constexpr int INPUT_LIGHT_6 = EmulatorConstants::INPUT_LIGHT_6;
    static constexpr int INPUT_LIGHT_7 = EmulatorConstants::INPUT_LIGHT_7;
    static constexpr int INPUT_LIGHT_8 = EmulatorConstants::INPUT_LIGHT_8;
    static constexpr int INPUT_LIGHT_9 = EmulatorConstants::INPUT_LIGHT_9;
    static constexpr int INPUT_LIGHT_10 = EmulatorConstants::INPUT_LIGHT_10;
    static constexpr int INPUT_LIGHT_11 = EmulatorConstants::INPUT_LIGHT_11;
    static constexpr int INPUT_LIGHT_12 = EmulatorConstants::INPUT_LIGHT_12;
    static constexpr int OUTPUT_LIGHT_1 = EmulatorConstants::OUTPUT_LIGHT_1;
    static constexpr int OUTPUT_LIGHT_2 = EmulatorConstants::OUTPUT_LIGHT_2;
    static constexpr int OUTPUT_LIGHT_3 = EmulatorConstants::OUTPUT_LIGHT_3;
    static constexpr int OUTPUT_LIGHT_4 = EmulatorConstants::OUTPUT_LIGHT_4;
    static constexpr int OUTPUT_LIGHT_5 = EmulatorConstants::OUTPUT_LIGHT_5;
    static constexpr int OUTPUT_LIGHT_6 = EmulatorConstants::OUTPUT_LIGHT_6;
    static constexpr int OUTPUT_LIGHT_7 = EmulatorConstants::OUTPUT_LIGHT_7;
    static constexpr int OUTPUT_LIGHT_8 = EmulatorConstants::OUTPUT_LIGHT_8;
    static constexpr int MIDI_INPUT_LIGHT = EmulatorConstants::MIDI_INPUT_LIGHT;
    static constexpr int MIDI_OUTPUT_LIGHT = EmulatorConstants::MIDI_OUTPUT_LIGHT;

    // Core components
    BusSystem busSystem;
    EmulatorCore emulatorCore;
    
    // New modular components
    std::unique_ptr<PluginManager> pluginManager;
    std::unique_ptr<PluginExecutor> pluginExecutor;
    std::unique_ptr<ParameterSystem> parameterSystem;
    std::unique_ptr<MenuSystem> menuSystem;
    std::unique_ptr<MidiProcessor> midiProcessor;
    
    // MIDI activity divider
    dsp::ClockDivider midiActivityDivider;
    
    // Compatibility helpers
    bool isPluginLoaded() const {
        return pluginManager && pluginManager->isLoaded();
    }
    
    // MIDI access getters
    midi::InputQueue& getMidiInput() { return midiProcessor->getInputQueue(); }
    midi::Output& getMidiOutput() { return midiProcessor->getOutput(); }
    
    // Member variables for two-phase plugin loading with specifications
    // Plugin loading state
    std::string lastPluginFolder;
    std::string pendingPluginState; // JSON string for plugin state to be restored after setupUi
    json_t* pendingParameterValues = nullptr; // Parameter values to be restored after parameter extraction
    
    // Display state
    bool displayDirty = true;
    
    // Menu system state
    enum MenuMode {
        MENU_OFF,
        MENU_PAGE_SELECT,
        MENU_PARAM_SELECT,
        MENU_VALUE_EDIT
    };
    
    MenuMode menuMode = MENU_OFF;
    int parameterEditValue = 0;
    
    // Button 1 long press timing
    double button1PressTime = 0.0;
    bool button1LongPressHandled = false;
    bool button1CurrentlyPressed = false;
    
    
    // UI interaction tracking
    bool leftEncoderPressed = false;
    bool rightEncoderPressed = false;
    float lastLeftPotValue = 0.5f;
    float lastLeftEncoderValue = 0.f;
    float lastRightEncoderValue = 0.f;
    
    // Encoder state tracking for discrete step detection
    float lastEncoderLParam = 0.f;
    float lastEncoderRParam = 0.f;
    int encoderLSteps = 0;  // Accumulated steps to process
    int encoderRSteps = 0;  // Accumulated steps to process
    
    // Debug data for display
    
    // Bus routing maps
    std::array<int, 28> busInputMap;
    std::array<int, 28> busOutputMap;
    bool routingDirty = false;
    
    // Parameter-based routing structures
    struct InputRouting {
        int paramIndex;      // Which parameter controls this routing
        int busIndex;        // Target bus (0-27)
        bool isCV;           // Audio or CV input
    };
    
    struct OutputRouting {
        int paramIndex;           // Which parameter controls this routing
        int busIndex;             // Source bus (0-27)
        bool isCV;                // Audio or CV output
        bool hasOutputMode;       // Has associated output mode parameter
        int outputModeParamIndex; // Index of output mode parameter
    };
    
    // Maps from parameter index to routing info
    std::map<int, InputRouting> paramInputRouting;
    std::map<int, OutputRouting> paramOutputRouting;
    
    // Sample accumulation for 4-sample processing blocks
    int sampleCounter = 0;
    static constexpr int BLOCK_SIZE = 4;
    
    // Trigger detectors for buttons and encoders
    dsp::SchmittTrigger buttonTriggers[4];
    dsp::SchmittTrigger encoderLPressTrigger, encoderRPressTrigger;
    
    // Button state tracking for proper press/release events
    bool lastButtonStates[4] = {false, false, false, false};
    
    EmulatorModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        g_currentModule = this;
        
        // Configure parameters
        configParam(POT_L_PARAM, 0.f, 1.f, 0.5f, "Pot L");
        configParam(POT_C_PARAM, 0.f, 1.f, 0.5f, "Pot C");  
        configParam(POT_R_PARAM, 0.f, 1.f, 0.5f, "Pot R");
        
        configParam(BUTTON_1_PARAM, 0.f, 1.f, 0.f, "Button 1");
        configParam(BUTTON_2_PARAM, 0.f, 1.f, 0.f, "Button 2");
        configParam(BUTTON_3_PARAM, 0.f, 1.f, 0.f, "Button 3");
        configParam(BUTTON_4_PARAM, 0.f, 1.f, 0.f, "Button 4");
        
        // Configure infinite encoders with special handling for infinite rotation
        configParam(ENCODER_L_PARAM, -INFINITY, INFINITY, 0.f, "Left Encoder");
        configParam(ENCODER_R_PARAM, -INFINITY, INFINITY, 0.f, "Right Encoder");
        
        // Disable snapping so encoders don't snap back to center
        getParamQuantity(ENCODER_L_PARAM)->snapEnabled = false;
        getParamQuantity(ENCODER_R_PARAM)->snapEnabled = false;
        
        // Replace with context-aware encoder quantities
        paramQuantities[ENCODER_L_PARAM] = new ContextAwareEncoderQuantity();
        paramQuantities[ENCODER_L_PARAM]->module = this;
        paramQuantities[ENCODER_L_PARAM]->paramId = ENCODER_L_PARAM;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_L_PARAM])->distingModule = this;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_L_PARAM])->isLeftEncoder = true;
        
        paramQuantities[ENCODER_R_PARAM] = new ContextAwareEncoderQuantity();
        paramQuantities[ENCODER_R_PARAM]->module = this;
        paramQuantities[ENCODER_R_PARAM]->paramId = ENCODER_R_PARAM;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_R_PARAM])->distingModule = this;
        ((ContextAwareEncoderQuantity*)paramQuantities[ENCODER_R_PARAM])->isLeftEncoder = false;
        
        configParam(ENCODER_L_PRESS_PARAM, 0.f, 1.f, 0.f, "Encoder L Press");
        configParam(ENCODER_R_PRESS_PARAM, 0.f, 1.f, 0.f, "Encoder R Press");
        
        configParam(ALGORITHM_PARAM, 0.f, 1.f, 0.f, "Algorithm");
        
        // Configure 12 inputs
        for (int i = 0; i < 12; i++) {
            configInput(AUDIO_INPUT_1 + i, string::f("Input %d", i + 1));
        }
        
        // Configure 8 outputs  
        for (int i = 0; i < 8; i++) {
            configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
        }
        
        // Initialize bus system
        busSystem.init();
        
        // Initialize emulator core
        emulatorCore.initialize(APP->engine->getSampleRate());
        
        // Initialize bus routing matrices
        busInputMap.fill(-1);
        busOutputMap.fill(-1);
        
        // Initialize encoder step tracking to current parameter values
        // (This ensures first delta calculation works correctly)
        encoderLSteps = (int)params[ENCODER_L_PARAM].getValue();
        encoderRSteps = (int)params[ENCODER_R_PARAM].getValue();
        
        // Configure MIDI activity divider
        midiActivityDivider.setDivision(512);
        
        // Initialize new modular components (C++11 compatible)
        pluginManager.reset(new PluginManager());
        pluginManager->addObserver(this);  // Register for plugin state notifications
        pluginExecutor.reset(new PluginExecutor(pluginManager.get()));
        parameterSystem.reset(new ParameterSystem(pluginManager.get()));
        menuSystem.reset(new MenuSystem(parameterSystem.get()));
        midiProcessor.reset(new MidiProcessor(pluginExecutor.get()));
        
        // Initialize parameter system routing matrix with parameter defaults
        // NOTE: At construction time, no plugin is loaded yet, so parameterSystem will have no parameters
        INFO("Initializing parameter system routing matrix: paramCount=%zu, routingSize=%zu", 
             parameterSystem->getParameterCount(), parameterSystem->getRoutingMatrix().size());
        
        // Only initialize if we actually have parameters (which we won't at construction time)
        if (parameterSystem->hasParameters()) {
            for (size_t i = 0; i < parameterSystem->getParameterCount() && i < parameterSystem->getRoutingMatrix().size(); i++) {
                INFO("Setting routing matrix[%zu] = %d", i, parameterSystem->getParameters()[i].def);
                parameterSystem->getRoutingMatrix()[i] = parameterSystem->getParameters()[i].def;
            }
        } else {
            INFO("No parameters to initialize - parameter system will be initialized when plugin is loaded");
        }
        
        // Sync algorithm's v[] array with the initialized parameter defaults
        if (pluginManager->getAlgorithm()) {
            pluginManager->getAlgorithm()->v = parameterSystem->getRoutingMatrix().data();
        }
        
        // Register as observer for parameter changes
        parameterSystem->addObserver(this);
        
        // Setup MIDI output callback
        setupMidiOutput();
        
        INFO("EmulatorModule constructor completed successfully");
    }
    
    ~EmulatorModule() {
        if (g_currentModule == this) {
            g_currentModule = nullptr;
        }
        
        // Clean up pending parameter values if any
        if (pendingParameterValues) {
            json_decref(pendingParameterValues);
            pendingParameterValues = nullptr;
        }
        
        // Unregister observers
        if (parameterSystem) {
            parameterSystem->removeObserver(this);
        }
        if (pluginManager) {
            pluginManager->removeObserver(this);
        }
    }
    
    // MIDI setup method - simplified for modular architecture
    void setupMidiOutput() {
        // Setup MIDI output using new MidiProcessor
        if (midiProcessor) {
            midiProcessor->setupMidiOutput();
        }
        
    }
    
    // Plugin helper methods now handled by PluginManager
    
    bool isValidPointer(void* ptr, size_t size = sizeof(void*)) {
        if (!ptr) return false;
        uintptr_t addr = (uintptr_t)ptr;
        if (addr < 0x1000) return false;
        #if defined(__aarch64__) || defined(_M_ARM64)
            if (addr > 0x800000000000ULL) return false;
        #else
            if (addr > 0x7FFFFFFFFFFF) return false;
        #endif
        try {
            volatile char test = *((volatile char*)ptr);
            (void)test;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void unloadPlugin() {
        menuMode = MENU_OFF;
        parameterSystem->clearParameters();
        pluginManager->unloadPlugin();
        displayDirty = true;
    }
    
    void reloadPlugin() {
        if (!isPluginLoaded() || pluginManager->getPluginPath().empty()) {
            displayDirty = true;
            return;
        }
        
        // Use PluginManager's reload method which properly handles observer notifications
        pluginManager->reloadPlugin();
        displayDirty = true;
    }
    
    // Plugin specification discovery (phase 1 of loading)
    struct PluginSpecificationInfo {
        std::string path;
        std::string name;
        std::string description;
        std::vector<_NT_specification> specifications;
        bool hasSpecifications() const { return !specifications.empty(); }
    };
    
    PluginSpecificationInfo discoverPluginSpecifications(const std::string& path) {
        PluginSpecificationInfo info;
        info.path = path;
        
        // Load the dynamic library temporarily
        void* tempHandle = nullptr;
        #ifdef ARCH_WIN
            tempHandle = LoadLibraryA(path.c_str());
        #else
            tempHandle = dlopen(path.c_str(), RTLD_LAZY);
        #endif
        
        if (!tempHandle) {
            INFO("Failed to load plugin for specification discovery: %s", dlerror());
            return info;
        }
        
        // Get pluginEntry function
        typedef uintptr_t (*PluginEntryFunc)(_NT_selector, uint32_t);
        PluginEntryFunc pluginEntry;
        #ifdef ARCH_WIN
            pluginEntry = (PluginEntryFunc)GetProcAddress((HMODULE)tempHandle, "pluginEntry");
        #else
            pluginEntry = (PluginEntryFunc)dlsym(tempHandle, "pluginEntry");
        #endif
        
        if (pluginEntry) {
            try {
                // Get factory info
                _NT_factory* factory = (_NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
                if (factory) {
                    info.name = factory->name ? factory->name : "Unknown";
                    info.description = factory->description ? factory->description : "";
                    
                    // Copy specifications if they exist
                    if (factory->numSpecifications > 0 && factory->specifications) {
                        info.specifications.resize(factory->numSpecifications);
                        for (uint32_t i = 0; i < factory->numSpecifications; i++) {
                            info.specifications[i] = factory->specifications[i];
                        }
                        INFO("Discovered plugin '%s' with %u specifications", info.name.c_str(), factory->numSpecifications);
                    } else {
                        INFO("Discovered plugin '%s' with no specifications", info.name.c_str());
                    }
                }
            } catch (...) {
                WARN("Error during plugin specification discovery");
            }
        }
        
        // Clean up temporary handle
        #ifdef ARCH_WIN
            FreeLibrary((HMODULE)tempHandle);
        #else
            dlclose(tempHandle);
        #endif
        
        return info;
    }
    
    
    // Plugin loading methods
    bool loadPlugin(const std::string& path) {
        // Observer will handle initialization automatically via onPluginLoaded()
        return pluginManager->loadPlugin(path);
    }
    
    // Overloaded loadPlugin that accepts custom specifications  
    bool loadPlugin(const std::string& path, const std::vector<int32_t>& customSpecifications) {
        // Observer will handle initialization automatically via onPluginLoaded()
        return pluginManager->loadPlugin(path, customSpecifications);
    }
    
    
    // extractParameterData now handled by ParameterSystem
    
    // Sync VCV pot positions to menu navigation state when entering menu
    void syncPotsToMenuState() {
        if (!parameterSystem->hasParameterPages() || !pluginManager->getAlgorithm()) {
            return;
        }
        
        // Get current navigation state
        int currentPageIdx = parameterSystem->getCurrentPageIndex();
        int currentParamIdx = parameterSystem->getCurrentParamIndex();
        
        if (currentPageIdx < 0 || currentPageIdx >= static_cast<int>(parameterSystem->getPageCount())) {
            return;
        }
        
        const auto& pages = parameterSystem->getParameterPages();
        const auto& parameters = parameterSystem->getParameters();
        
        if (currentPageIdx >= static_cast<int>(pages.size())) {
            return;
        }
        
        const _NT_parameterPage& currentPage = pages[currentPageIdx];
        
        // POT_L: Page selection (normalized position showing which page is selected)
        float pagePosition = 0.0f;
        if (parameterSystem->getPageCount() > 1) {
            pagePosition = static_cast<float>(currentPageIdx) / static_cast<float>(parameterSystem->getPageCount() - 1);
        }
        params[POT_L_PARAM].setValue(std::max(0.0f, std::min(1.0f, pagePosition)));
        
        // POT_C: Parameter selection (normalized position showing which parameter on page is selected)
        float paramPosition = 0.0f;
        if (currentPage.numParams > 1) {
            paramPosition = static_cast<float>(currentParamIdx) / static_cast<float>(currentPage.numParams - 1);
        }
        params[POT_C_PARAM].setValue(std::max(0.0f, std::min(1.0f, paramPosition)));
        
        // POT_R: Parameter value (normalized position of actual parameter value)
        float valuePosition = 0.5f; // Default to center
        if (currentParamIdx >= 0 && currentParamIdx < static_cast<int>(currentPage.numParams)) {
            int actualParamIdx = currentPage.params[currentParamIdx];
            if (actualParamIdx >= 0 && actualParamIdx < static_cast<int>(parameters.size())) {
                const _NT_parameter& param = parameters[actualParamIdx];
                int16_t currentValue = parameterSystem->getParameterValue(actualParamIdx);
                
                if (param.max != param.min) {
                    valuePosition = static_cast<float>(currentValue - param.min) / static_cast<float>(param.max - param.min);
                }
            }
        }
        params[POT_R_PARAM].setValue(std::max(0.0f, std::min(1.0f, valuePosition)));
        
        INFO("Synced pot positions to menu state: page %d/%zu, param %d/%d, value %.3f", 
             currentPageIdx, parameterSystem->getPageCount(), currentParamIdx, currentPage.numParams, valuePosition);
    }
    
    void toggleMenuMode() {
        // Delegate to the new MenuSystem
        bool wasActive = menuSystem->isMenuActive();
        menuSystem->toggleMenu();
        
        // Sync legacy menuMode with MenuSystem state for backward compatibility
        if (menuSystem->isMenuActive()) {
            // If we just entered the menu (was not active before), sync pot positions to menu navigation state
            if (!wasActive) {
                syncPotsToMenuState();
            }
            
            switch (menuSystem->getCurrentState()) {
                case MenuSystem::State::PAGE_SELECT:
                    menuMode = MENU_PAGE_SELECT;
                    break;
                case MenuSystem::State::PARAM_SELECT:
                    menuMode = MENU_PARAM_SELECT;
                    break;
                case MenuSystem::State::VALUE_EDIT:
                    menuMode = MENU_VALUE_EDIT;
                    break;
                default:
                    menuMode = MENU_OFF;
                    break;
            }
        } else {
            menuMode = MENU_OFF;
            
            // When exiting menu, call setupUi if plugin supports it
            if (wasActive && pluginManager->getAlgorithm()) {
                applyRoutingChanges();
                
                // Call setupUi when exiting parameter menu
                if (pluginManager->getFactory() && pluginManager->getFactory()->setupUi) {
                    float potValues[3] = {
                        params[POT_L_PARAM].getValue(),
                        params[POT_C_PARAM].getValue(),
                        params[POT_R_PARAM].getValue()
                    };
                    INFO("NtEmu::toggleMenuMode calling setupUi on menu exit with pot values: %.3f %.3f %.3f", 
                         potValues[0], potValues[1], potValues[2]);
                    try {
                        pluginManager->getFactory()->setupUi(pluginManager->getAlgorithm(), potValues);
                        // Set VCV pot parameters to match what the plugin wants
                        params[POT_L_PARAM].setValue(potValues[0]);
                        params[POT_C_PARAM].setValue(potValues[1]);
                        params[POT_R_PARAM].setValue(potValues[2]);
                        INFO("NtEmu::toggleMenuMode set pot values to: %.3f %.3f %.3f", 
                             potValues[0], potValues[1], potValues[2]);
                    } catch (...) {
                        WARN("Plugin crashed during setupUi on menu exit");
                    }
                }
            }
        }
        displayDirty = true;
    }
    
    
    // State update methods - single point of control
    void setCurrentPage(int pageIndex) {
        parameterSystem->setCurrentPage(pageIndex);
        displayDirty = true;
    }
    
    void setCurrentParam(int paramIndex) {
        parameterSystem->setCurrentParam(paramIndex);
        displayDirty = true;
    }
    
    bool isRoutingParameter(const _NT_parameter& param) {
        return param.unit == kNT_unitAudioInput || 
               param.unit == kNT_unitCvInput ||
               param.unit == kNT_unitAudioOutput || 
               param.unit == kNT_unitCvOutput ||
               param.unit == kNT_unitOutputMode;
    }
    
    void setParameterValue(int paramIdx, int value) {
        // Delegate to ParameterSystem
        parameterSystem->setParameterValue(paramIdx, (int16_t)value);
        
        // Update routing immediately for routing parameters if this is a routing parameter
        if (paramIdx >= 0 && paramIdx < (int)parameterSystem->getParameterCount()) {
            const _NT_parameter& param = parameterSystem->getParameters()[paramIdx];
            if (isRoutingParameter(param)) {
                updateParameterRouting();
            }
        }
        
        // Update VCV pot positions to reflect parameter changes
        if (pluginManager->isLoaded() && pluginManager->getFactory() && pluginManager->getFactory()->setupUi) {
            try {
                float potValues[3] = { 0.0f, 0.0f, 0.0f };
                pluginManager->getFactory()->setupUi(pluginManager->getAlgorithm(), potValues);
                params[POT_L_PARAM].setValue(potValues[0]);
                params[POT_C_PARAM].setValue(potValues[1]);
                params[POT_R_PARAM].setValue(potValues[2]);
            } catch (...) {
                WARN("NtEmu: Failed to update pot positions after parameter change");
            }
        }
        
        displayDirty = true;
    }
    
    void handleSetParameterFromUi(uint32_t parameter, int16_t value) {
        // Called from plugin via NT_setParameterFromUi
        INFO("NtEmu: handleSetParameterFromUi called: param=%d, value=%d", parameter, value);
        
        // Convert parameter index (parameter might have offset)
        int paramIdx = (int)parameter;
        
        // Delegate to existing setParameterValue method
        setParameterValue(paramIdx, value);
        
        // Also call parameterChanged to notify plugin
        if (pluginManager->isLoaded() && pluginManager->getFactory() && pluginManager->getFactory()->parameterChanged) {
            try {
                pluginManager->getFactory()->parameterChanged(pluginManager->getAlgorithm(), paramIdx);
            } catch (...) {
                WARN("NtEmu: Plugin crashed during parameterChanged callback");
            }
        }
    }
    
    int getCurrentParameterIndex() {
        if (parameterSystem->getCurrentPageIndex() >= (int)parameterSystem->getPageCount() || !parameterSystem->hasParameterPages()) return -1;
        
        const _NT_parameterPage& page = parameterSystem->getParameterPages()[parameterSystem->getCurrentPageIndex()];
        if (parameterSystem->getCurrentParamIndex() >= page.numParams) return -1;
        
        // Handle default page case (no parameter index array)
        if (page.params == nullptr) {
            // For default pages, assume sequential parameter indexing
            if (parameterSystem->getCurrentParamIndex() >= (int)parameterSystem->getParameterCount()) return -1;
            return parameterSystem->getCurrentParamIndex();
        }
        
        // Handle normal pages with parameter index arrays
        int paramIdx = page.params[parameterSystem->getCurrentParamIndex()];
        if (paramIdx < 0 || paramIdx >= (int)parameterSystem->getParameterCount()) return -1;
        
        return paramIdx;
    }
    
    int getCurrentParameterValue() {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0 || paramIdx >= 256) return 0;
        
        return parameterSystem->getRoutingMatrix()[paramIdx];
    }
    
    void confirmParameterValue() {
        int paramIdx = getCurrentParameterIndex();
        if (paramIdx < 0) return;
        
        // Delegate to ParameterSystem which will handle plugin notification through observer pattern
        parameterSystem->setParameterValue(paramIdx, parameterEditValue);
        
        routingDirty = true;
    }
    
    void applyRoutingChanges() {
        // Update bus routing based on parameter values
        updateBusRouting();
        
        // Save routing to module state
        routingDirty = true;
    }
    
    void updateParameterRouting() {
        // Clear existing routing
        paramInputRouting.clear();
        paramOutputRouting.clear();
        
        // Scan parameters for input/output types
        for (size_t i = 0; i < parameterSystem->getParameterCount(); i++) {
            const _NT_parameter& param = parameterSystem->getParameters()[i];
            
            if (param.unit == kNT_unitAudioInput || param.unit == kNT_unitCvInput) {
                // This parameter controls an input routing
                paramInputRouting[i] = {
                    .paramIndex = (int)i,
                    .busIndex = parameterSystem->getRoutingMatrix()[i] > 0 ? parameterSystem->getRoutingMatrix()[i] - 1 : -1,  // Convert to 0-based
                    .isCV = (param.unit == kNT_unitCvInput)
                };
            }
            else if (param.unit == kNT_unitAudioOutput || param.unit == kNT_unitCvOutput) {
                // This parameter controls an output routing
                paramOutputRouting[i] = {
                    .paramIndex = (int)i,
                    .busIndex = parameterSystem->getRoutingMatrix()[i] > 0 ? parameterSystem->getRoutingMatrix()[i] - 1 : -1,  // Convert to 0-based
                    .isCV = (param.unit == kNT_unitCvOutput),
                    .hasOutputMode = false,
                    .outputModeParamIndex = -1
                };
                
                // Check if next parameter is output mode
                if (i + 1 < parameterSystem->getParameterCount() && parameterSystem->getParameters()[i + 1].unit == kNT_unitOutputMode) {
                    paramOutputRouting[i].hasOutputMode = true;
                    paramOutputRouting[i].outputModeParamIndex = i + 1;
                }
            }
        }
        
        // Update the busInputMap and busOutputMap for compatibility
        updateBusRouting();
    }
    
    void updateBusRouting() {
        // Validate plugin state before updating routing
        if (!pluginManager || !pluginManager->getAlgorithm() || !parameterSystem->hasParameters()) {
            return;
        }
        
        // This method is now called from updateParameterRouting()
        // and uses the paramInputRouting and paramOutputRouting maps
    }
    
    int getVCVInputForParameter(int paramIndex) {
        // Map parameter index to VCV input based on naming convention
        // This would need to be customized based on how parameters are named
        // For now, simple sequential mapping
        return paramIndex < 12 ? paramIndex : -1;
    }
    
    int getVCVOutputForParameter(int paramIndex) {
        // Map parameter index to VCV output
        // Assuming outputs come after inputs in parameter list
        return (paramIndex >= 12 && paramIndex < 18) ? paramIndex - 12 : -1;
    }
    
    void applyCustomRouting() {
        // For custom routing, just use the default routing for now
        // This ensures all 12 inputs are properly routed to buses 0-11
        busSystem.routeInputs(this);
    }
    
    void applyCustomOutputRouting() {
        // Get current sample from the block
        int currentSample = busSystem.getCurrentSampleIndex();
        
        // Route buses 12-17 to outputs 1-6 for current sample
        for (int i = 0; i < 6 && i < NUM_OUTPUTS; i++) {
            float busValue = busSystem.getBus(12 + i, currentSample);
            outputs[AUDIO_OUTPUT_1 + i].setVoltage(busValue);
        }
        
        // Advance sample index
        busSystem.nextSample();
    }
    
    // Plugin sandboxing utility
    template<typename Func>
    bool safeExecutePlugin(Func func, const char* operation) {
        if (!isPluginLoaded()) return false;
        
        try {
            func();
            return true;
        } catch (const std::exception& e) {
            WARN("Plugin %s failed: %s", operation, e.what());
            return false;
        } catch (...) {
            WARN("Plugin %s failed: Unknown exception", operation);
            return false;
        }
    }
    
    void process(const ProcessArgs& args) override {
        try {
        
        // Update plugin manager timers
        pluginManager->updateLoadingTimer(args.sampleTime);
        
        // Process MIDI input using new MidiProcessor
        midiProcessor->processInputMessages(args);
        
        // Update MIDI activity lights
        if (midiActivityDivider.process()) {
            midiProcessor->updateActivityLights(args.sampleTime);
        }
        
        // Menu toggle is handled by Button 1, not encoder press
        // Update encoder press state tracking
        if (params[ENCODER_L_PRESS_PARAM].getValue() > 0) {
            leftEncoderPressed = true;
        } else if (params[ENCODER_L_PRESS_PARAM].getValue() == 0) {
            leftEncoderPressed = false;
        }
        
        // Always process Button 1 timing logic regardless of menu state
        processButton1Timing();
        
        if (menuSystem->isMenuActive()) {
            // Collect current input states for menu processing
            std::array<float, 3> potValues = {
                params[POT_L_PARAM].getValue(),
                params[POT_C_PARAM].getValue(),
                params[POT_R_PARAM].getValue()
            };
            
            // Get encoder deltas from hardware state (set by encoder widgets)
            const auto& hwState = emulatorCore.getHardwareState();
            std::array<int, 2> encoderDeltas = {
                hwState.encoder_deltas[0],
                hwState.encoder_deltas[1]
            };
            
            // Debug log encoder deltas if non-zero
            if (encoderDeltas[0] != 0 || encoderDeltas[1] != 0) {
                INFO("EmulatorModule: Encoder deltas before menu: L=%d R=%d", 
                     encoderDeltas[0], encoderDeltas[1]);
            }
            
            std::array<bool, 2> encoderPressed = {
                hwState.encoder_pressed[0],
                hwState.encoder_pressed[1]
            };
            
            menuSystem->processNavigation(potValues, encoderDeltas, encoderPressed);
            
            // Clear encoder deltas after processing
            emulatorCore.clearEncoderDeltas();
        } else {
            // Process control inputs when not in menu mode (excluding pot changes - handled directly by widgets)
            processControlsExceptButton1();
        }
        
        // Button 1 timing is handled in processControls() - no need for separate handling
        
        // DISABLED: Sync routing matrix with plugin algorithm values 
        // This was overriding VCV pot values with plugin v[] array (all zeros)
        // For customUi, VCV controls should drive the plugin, not the other way around
        // if (isPluginLoaded() && pluginManager && pluginManager->getAlgorithm() && pluginManager->getAlgorithm()->v && menuMode == MENU_OFF) {
        //     for (size_t i = 0; i < parameterSystem->getParameterCount() && i < parameterSystem->getRoutingMatrix().size(); i++) {
        //         parameterSystem->getRoutingMatrix()[i] = pluginManager->getAlgorithm()->v[i];
        //     }
        // }
        
        // Clear buses at the start of a new block
        if (sampleCounter == 0) {
            busSystem.clear();
        }
        
        // Route inputs to buses for current sample
        busSystem.routeInputs(this);
        
        // Check if we've accumulated a full block (after routing input)
        if (sampleCounter == BLOCK_SIZE - 1) {
            // Process algorithm with 4-sample blocks
            printf("Audio block complete, isPluginLoaded()=%s\n", isPluginLoaded() ? "true" : "false");
            if (isPluginLoaded()) {
                // Process hardware changes for loaded plugins
                printf("Processing hardware changes for loaded plugin\n");
                emulatorCore.processHardwareChanges(pluginManager->getFactory(), pluginManager->getAlgorithm());
                
                // Use plugin for audio processing
                safeExecutePlugin([&]() {
                    // Add comprehensive null checks for plugin reload safety
                    if (pluginManager && pluginManager->getFactory() && pluginManager->getAlgorithm() && pluginManager->getFactory()->step) {
                        // Debug logging disabled for performance
                        
                        pluginManager->getFactory()->step(pluginManager->getAlgorithm(), busSystem.getBuses(), 1);
                        
                        // Debug logging disabled for performance
                    }
                }, "step");
            } else {
                // Use built-in emulator
                emulatorCore.processAudio(busSystem.getBuses(), 1); // 1 = numFramesBy4
            }
            
            // Don't reset - we'll output samples in sequence
        }
        
        // Route outputs for current sample
        busSystem.routeOutputs(this);
        
        // Increment sample counter and wrap at block size
        sampleCounter = (sampleCounter + 1) % BLOCK_SIZE;
        
        // Update lights
        updateLights();
        
        } catch (const std::exception& e) {
            WARN("EmulatorModule process() exception: %s", e.what());
        } catch (...) {
            WARN("EmulatorModule process() unknown exception");
        }
    }
    
    void processButton1Timing() {
        // Special handling for Button 1 (long press detection)
        bool button1Pressed = params[BUTTON_1_PARAM].getValue() > 0.5f;
        
        if (button1Pressed && !button1CurrentlyPressed) {
            // Button 1 just pressed
            button1CurrentlyPressed = true;
            button1PressTime = system::getTime();
            button1LongPressHandled = false;
        } else if (!button1Pressed && button1CurrentlyPressed) {
            // Button 1 just released
            button1CurrentlyPressed = false;
            if (!button1LongPressHandled) {
                // Short press
                if (menuSystem->isMenuActive()) {
                    // If menu is active, short press exits menu
                    toggleMenuMode();
                } else {
                    // If menu is not active, send to customUi
                    onButtonPress(0, true);  // Press
                    onButtonPress(0, false); // Release
                }
            }
            button1PressTime = 0.0;
            button1LongPressHandled = false;
            displayDirty = true;
        } else if (button1Pressed && button1CurrentlyPressed && !button1LongPressHandled) {
            // Check for long press (>500ms)
            if (system::getTime() - button1PressTime > 0.5) {
                // Long press detected - toggle menu
                toggleMenuMode();
                button1LongPressHandled = true;
                displayDirty = true;
            }
        }
    }
    
    void processControlsExceptButton1() {
        // Process other button triggers normally (2-4)
        for (int i = 1; i < 4; i++) {
            bool buttonPressed = params[BUTTON_1_PARAM + i].getValue() > 0.5f;
            
            // Detect press event
            if (buttonPressed && !lastButtonStates[i]) {
                onButtonPress(i, true);  // Press event
                displayDirty = true;
            }
            // Detect release event
            else if (!buttonPressed && lastButtonStates[i]) {
                onButtonPress(i, false); // Release event
                displayDirty = true;
            }
            
            lastButtonStates[i] = buttonPressed;
        }
        
        // Process encoder press triggers
        // Encoder press handling now done by MenuSystem in menu mode
        // Legacy encoder press handling for non-menu mode (emulator core)
        if (encoderLPressTrigger.process(params[ENCODER_L_PRESS_PARAM].getValue())) {
            onEncoderPress(0);
            displayDirty = true;
        }
        if (encoderRPressTrigger.process(params[ENCODER_R_PRESS_PARAM].getValue())) {
            onEncoderPress(1);
            displayDirty = true;
        }
    }
    
    void processControls() {
        // processButton1Timing(); // Already called in main process() loop
        processControlsExceptButton1();
        
        // Update hardware state for emulator core
        VCVHardwareState hwState;
        
        // Update pot values
        for (int i = 0; i < 3; i++) {
            float potValue = params[POT_L_PARAM + i].getValue();
            hwState.pots[i] = potValue;
        }
        
        // Update button states
        for (int i = 0; i < 4; i++) {
            hwState.buttons[i] = params[BUTTON_1_PARAM + i].getValue() > 0.5f;
        }
        
        // Update encoder states
        hwState.pot_pressed[0] = params[ENCODER_L_PRESS_PARAM].getValue() > 0.5f;
        hwState.pot_pressed[2] = params[ENCODER_R_PRESS_PARAM].getValue() > 0.5f;
        
        // Encoder deltas are now set by the encoder widgets themselves via turnEncoder()
        
        emulatorCore.updateHardwareState(hwState);
        
        // Process hardware changes immediately after updating state
        if (isPluginLoaded()) {
            // Only process hardware changes for non-customUi plugins
            if (!pluginManager->getFactory()->customUi) {
                emulatorCore.processHardwareChanges(pluginManager->getFactory(), pluginManager->getAlgorithm());
            }
        }
    }
    
    void updateLights() {
        // Update button lights based on state
        for (int i = 0; i < 4; i++) {
            lights[BUTTON_1_LIGHT + i].setBrightness(params[BUTTON_1_PARAM + i].getValue());
        }
        
        // Update port lights based on routing
        updatePortLights();
    }
    
    void updatePortLights() {
        // Light up connected inputs
        for (int i = 0; i < 12; i++) {
            bool isRouted = false;
            for (int bus = 0; bus < 28; bus++) {
                if (busInputMap[bus] == i) {
                    isRouted = true;
                    break;
                }
            }
            lights[INPUT_LIGHT_1 + i].setBrightness(isRouted ? 1.0f : 0.0f);
        }
        
        // Light up connected outputs
        for (int i = 0; i < 6; i++) {
            bool isRouted = false;
            for (int bus = 0; bus < 28; bus++) {
                if (busOutputMap[bus] == i) {
                    isRouted = true;
                    break;
                }
            }
            lights[OUTPUT_LIGHT_1 + i].setBrightness(isRouted ? 1.0f : 0.0f);
        }
        
        // Update MIDI activity lights using new MidiProcessor
        lights[MIDI_INPUT_LIGHT].setBrightness(midiProcessor->getMidiInputLight());
        lights[MIDI_OUTPUT_LIGHT].setBrightness(midiProcessor->getMidiOutputLight());
    }
    
    void onParameterChange(int parameter, float value) {
        if (isPluginLoaded()) {
            // Forward parameter changes to plugin
            safeExecutePlugin([&]() {
                if (pluginManager->getFactory()->parameterChanged) {
                    pluginManager->getFactory()->parameterChanged(pluginManager->getAlgorithm(), parameter);
                }
            }, "parameterChanged");
        } else {
            emulatorCore.setParameter(parameter, value);
        }
    }
    
    void onButtonPress(int button, bool pressed) {
        INFO("Button %d %s", button + 1, pressed ? "pressed" : "released");
        
        // If menu is active, don't send to plugin
        if (menuSystem->isMenuActive()) {
            return;
        }
        
        // Check if plugin has customUi
        if (isPluginLoaded() && pluginManager->getFactory() && pluginManager->getFactory()->customUi) {
            // Send directly to customUi
            sendCustomUiButtonEvent(button, pressed);
        } else {
            // Use emulator core for non-customUi plugins
            if (pressed) {
                emulatorCore.pressButton(button);
            } else {
                emulatorCore.releaseButton(button);
            }
        }
    }
    
    void onEncoderChange(int encoder, int delta) {
        // Check if plugin has customUi
        if (isPluginLoaded() && pluginManager->getFactory() && pluginManager->getFactory()->customUi) {
            // Send directly to customUi
            sendCustomUiEncoderEvent(encoder, delta);
        } else {
            // Use emulator core for non-customUi plugins
            emulatorCore.turnEncoder(encoder, delta);
        }
        displayDirty = true;
    }
    
    void onEncoderPress(int encoder) {
        emulatorCore.pressEncoder(encoder);
        displayDirty = true;
    }
    
    void selectAlgorithm(int index) {
        if (emulatorCore.selectAlgorithm(index)) {
            displayDirty = true;
        }
    }
    
    // VCV Rack lifecycle methods
    void onSampleRateChange() override {
        emulatorCore.initialize(APP->engine->getSampleRate());
    }
    
    void onReset() override {
        emulatorCore.selectAlgorithm(0);
        displayDirty = true;
    }
    
    void onRandomize() override {
        // Randomize pot positions
        params[POT_L_PARAM].setValue(random::uniform());
        params[POT_C_PARAM].setValue(random::uniform());
        params[POT_R_PARAM].setValue(random::uniform());
        displayDirty = true;
    }
    
    json_t* dataToJson() override {
        json_t* rootJ = emulatorCore.saveState();
        
        // Save plugin info if loaded
        if (!pluginManager->getPluginPath().empty()) {
            json_object_set_new(rootJ, "pluginPath", json_string(pluginManager->getPluginPath().c_str()));
        }
        
        // Save parameter values
        json_t* paramsJ = json_array();
        for (int i = 0; i < 3; i++) {
            json_array_append_new(paramsJ, json_real(params[POT_L_PARAM + i].getValue()));
        }
        json_object_set_new(rootJ, "params", paramsJ);
        
        // Save routing matrix
        json_t* routingJ = json_array();
        for (int i = 0; i < 256; i++) {
            if (parameterSystem->getRoutingMatrix()[i] != 0) {
                json_t* routeJ = json_object();
                json_object_set_new(routeJ, "param", json_integer(i));
                json_object_set_new(routeJ, "bus", json_integer(parameterSystem->getRoutingMatrix()[i]));
                json_array_append_new(routingJ, routeJ);
            }
        }
        json_object_set_new(rootJ, "routing", routingJ);
        
        // Save plugin parameter values (separate from VCV pot params)
        if (parameterSystem->hasParameters()) {
            json_object_set_new(rootJ, "parameterValues", parameterSystem->saveParameterState());
            INFO("NtEmu: Saved plugin parameter values");
        }
        
        // Save MIDI settings
        json_object_set_new(rootJ, "midiInput", midiProcessor->getInputQueue().toJson());
        json_object_set_new(rootJ, "midiOutput", midiProcessor->getOutput().toJson());
        
        // Save plugin-specific state if plugin is loaded and supports serialization
        if (pluginManager->getAlgorithm() && pluginManager->getFactory() && pluginManager->getFactory()->serialise) {
            INFO("NtEmu: Saving plugin state via pluginManager serialization");
            
            // Use the same pattern as PluginExecutor - thread-local bridge
            setCurrentJsonStream(std::unique_ptr<JsonStreamBridge>(new JsonStreamBridge()));
            _NT_jsonStream dummy_stream(nullptr);
            
            pluginManager->getFactory()->serialise(pluginManager->getAlgorithm(), dummy_stream);
            
            nlohmann::json pluginJson = getCurrentJsonStream()->getJson();
            std::string pluginJsonStr = pluginJson.dump();
            INFO("NtEmu: Plugin state JSON: %s", pluginJsonStr.c_str());
            json_object_set_new(rootJ, "pluginState", json_string(pluginJsonStr.c_str()));
            
            clearCurrentJsonStream();
        } else {
            INFO("NtEmu: No plugin serialization - algorithm: %p, factory: %p, serialise: %p", 
                 pluginManager->getAlgorithm(), 
                 pluginManager->getFactory(), 
                 pluginManager->getFactory() ? pluginManager->getFactory()->serialise : nullptr);
        }
        
        return rootJ;
    }
    
    void dataFromJson(json_t* rootJ) override {
        emulatorCore.loadState(rootJ);
        
        // First, store plugin state for restoration BEFORE loading plugin
        json_t* pluginStateJ = json_object_get(rootJ, "pluginState");
        if (pluginStateJ && json_is_string(pluginStateJ)) {
            pendingPluginState = json_string_value(pluginStateJ);
            INFO("NtEmu: Found plugin state in JSON (%zu chars)", pendingPluginState.length());
            INFO("NtEmu: Plugin state content: %s", pendingPluginState.c_str());
            INFO("NtEmu: Stored plugin state for restoration BEFORE loading plugin");
        } else {
            pendingPluginState.clear();
            INFO("NtEmu: No plugin state found in JSON (pluginStateJ=%p, is_string=%s)", 
                 (void*)pluginStateJ, 
                 pluginStateJ ? (json_is_string(pluginStateJ) ? "true" : "false") : "N/A");
        }
        
        // Now restore plugin if saved - state will be restored in onPluginLoaded
        json_t* pluginPathJ = json_object_get(rootJ, "pluginPath");
        if (pluginPathJ) {
            std::string path = json_string_value(pluginPathJ);
            
            json_t* folderJ = json_object_get(rootJ, "lastFolder");
            if (folderJ) {
                lastPluginFolder = json_string_value(folderJ);
            }
            
            // Try to load plugin - onPluginLoaded will handle state restoration
            if (!path.empty() && rack::system::exists(path)) {
                INFO("NtEmu: Loading plugin from path: %s (pending state: %s)", 
                     path.c_str(), pendingPluginState.empty() ? "NO" : "YES");
                loadPlugin(path);
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
        
        // Load routing matrix
        json_t* routingJ = json_object_get(rootJ, "routing");
        if (routingJ) {
            // Reset to defaults first
            for (size_t i = 0; i < parameterSystem->getParameterCount(); i++) {
                if (i < parameterSystem->getRoutingMatrix().size()) {
                    parameterSystem->getRoutingMatrix()[i] = parameterSystem->getParameters()[i].def;
                }
            }
            
            // Apply saved routing
            size_t routingSize = json_array_size(routingJ);
            for (size_t i = 0; i < routingSize; i++) {
                json_t* routeJ = json_array_get(routingJ, i);
                if (routeJ) {
                    int param = json_integer_value(json_object_get(routeJ, "param"));
                    int bus = json_integer_value(json_object_get(routeJ, "bus"));
                    if (param >= 0 && param < 256) {
                        parameterSystem->getRoutingMatrix()[param] = bus;
                    }
                }
            }
            
            // Apply routing
            updateBusRouting();
        }
        
        // Handle parameter values restoration
        json_t* parameterValuesJ = json_object_get(rootJ, "parameterValues");
        if (parameterValuesJ && json_is_array(parameterValuesJ)) {
            INFO("NtEmu: Found %zu parameter values to restore (plugin loaded: %s)", 
                 json_array_size(parameterValuesJ),
                 pluginManager->isLoaded() ? "YES" : "NO");
            
            if (pluginManager->isLoaded()) {
                // Plugin is already loaded, restore immediately
                INFO("NtEmu: Plugin already loaded, restoring parameters via ParameterSystem");
                parameterSystem->loadParameterState(parameterValuesJ);
                
                // Note: Plugin state should have been restored in onPluginLoaded, but check anyway
                if (!pendingPluginState.empty()) {
                    WARN("NtEmu: Unexpected pending plugin state in already-loaded scenario - this should not happen");
                    INFO("NtEmu: About to restore plugin state via PluginManager (%zu chars)", pendingPluginState.length());
                    INFO("NtEmu: Plugin state being restored: %s", pendingPluginState.c_str());
                    pluginManager->restorePluginState(pendingPluginState);
                    INFO("NtEmu: Plugin state restoration call completed, clearing pending state");
                    pendingPluginState.clear();
                } else {
                    INFO("NtEmu: No pending plugin state (already loaded scenario - this is expected)");
                }
                
                // Call setupUi to get pot positions from plugin parameters
                float potValues[3] = { 0.0f, 0.0f, 0.0f };
                INFO("NtEmu: Calling setupUi via PluginManager to get pot positions from plugin parameters");
                pluginManager->callSetupUi(potValues);
                
                // Update VCV pot positions based on plugin parameters
                params[POT_L_PARAM].setValue(potValues[0]);
                params[POT_C_PARAM].setValue(potValues[1]);
                params[POT_R_PARAM].setValue(potValues[2]);
                INFO("NtEmu: Updated VCV pot positions to: %.3f %.3f %.3f", 
                     potValues[0], potValues[1], potValues[2]);
                
                // Clean up any existing pending parameter values
                if (pendingParameterValues) {
                    json_decref(pendingParameterValues);
                    pendingParameterValues = nullptr;
                }
            } else {
                // Plugin not loaded yet, store for later restoration in onPluginLoaded
                INFO("NtEmu: Plugin not loaded yet, storing for later restoration");
                if (pendingParameterValues) {
                    json_decref(pendingParameterValues);
                }
                pendingParameterValues = json_incref(parameterValuesJ);
            }
        } else {
            // Clean up if no parameter values to restore
            if (pendingParameterValues) {
                json_decref(pendingParameterValues);
                pendingParameterValues = nullptr;
            }
            INFO("NtEmu: No parameter values found in JSON (plugin loaded: %s)", 
                 pluginManager->isLoaded() ? "YES" : "NO");
        }
        
        // Restore MIDI settings
        json_t* midiInputJ = json_object_get(rootJ, "midiInput");
        if (midiInputJ) {
            midiProcessor->getInputQueue().fromJson(midiInputJ);
        }
        
        json_t* midiOutputJ = json_object_get(rootJ, "midiOutput");
        if (midiOutputJ) {
            midiProcessor->getOutput().fromJson(midiOutputJ);
        }
        
        displayDirty = true;
    }
    
    // IParameterObserver interface implementation
    void onParameterChanged(int index, int16_t value) override {
        // Notify plugin of parameter change
        if (pluginManager->getFactory() && pluginManager->getFactory()->parameterChanged) {
            pluginManager->getFactory()->parameterChanged(pluginManager->getAlgorithm(), index);
        }
        displayDirty = true;
    }
    
    void onParameterPageChanged(int pageIndex) override {
        displayDirty = true;
    }
    
    void onParametersExtracted() override {
        // Sync algorithm's v[] pointer with ParameterSystem's routing matrix
        if (pluginManager->getAlgorithm()) {
            pluginManager->getAlgorithm()->v = parameterSystem->getRoutingMatrix().data();
        }
        displayDirty = true;
    }
    
    // IPluginStateObserver implementation
    void onPluginLoaded(const std::string& path) override {
        INFO("NtEmu: Plugin loaded: %s", path.c_str());
        
        // Initialize parameter system when plugin loads
        INFO("NtEmu: Extracting parameters from loaded plugin");
        parameterSystem->extractParameterData();
        
        // Handle pending parameter restoration
        if (pendingParameterValues) {
            INFO("NtEmu: Restoring pending parameter values");
            parameterSystem->loadParameterState(pendingParameterValues);
            json_decref(pendingParameterValues);
            pendingParameterValues = nullptr;
        }
        
        if (!pendingPluginState.empty()) {
            INFO("NtEmu: About to restore plugin state via PluginManager in onPluginLoaded (%zu chars)", pendingPluginState.length());
            INFO("NtEmu: Plugin state being restored: %s", pendingPluginState.c_str());
            pluginManager->restorePluginState(pendingPluginState);
            INFO("NtEmu: Plugin state restoration call completed in onPluginLoaded, clearing pending state");
            pendingPluginState.clear();
        } else {
            INFO("NtEmu: No pending plugin state to restore in onPluginLoaded");
        }
        
        // Call setupUi to get pot positions from plugin parameters
        float potValues[3] = { 0.0f, 0.0f, 0.0f };
        INFO("NtEmu: Calling setupUi via PluginManager in onPluginLoaded to get pot positions from plugin parameters");
        pluginManager->callSetupUi(potValues);
        
        // Update VCV pot positions based on plugin parameters
        params[POT_L_PARAM].setValue(potValues[0]);
        params[POT_C_PARAM].setValue(potValues[1]);
        params[POT_R_PARAM].setValue(potValues[2]);
        INFO("NtEmu: Updated VCV pot positions to: %.3f %.3f %.3f", 
             potValues[0], potValues[1], potValues[2]);
        
        displayDirty = true;
    }
    
    void onPluginUnloaded() override {
        // Plugin unloaded - could add any cleanup here if needed
        displayDirty = true;
    }
    
    void onPluginError(const std::string& error) override {
        WARN("Plugin error: %s", error.c_str());
        displayDirty = true;
    }
    
    // Direct customUi event delivery for pot changes
    void sendCustomUiPotEvent(int potIndex, float newValue) {
        INFO("sendCustomUiPotEvent called: pot %d = %.3f", potIndex, newValue);
        
        if (!isPluginLoaded() || !pluginManager->getFactory() || !pluginManager->getAlgorithm()) {
            INFO("sendCustomUiPotEvent: plugin not loaded or no factory/algorithm");
            return;
        }
        
        _NT_factory* factory = pluginManager->getFactory();
        _NT_algorithm* algorithm = pluginManager->getAlgorithm();
        
        // Check if plugin supports customUi
        if (!factory->customUi) {
            return;
        }
        
        // Get current pot values directly from VCV parameters
        std::array<float, 3> potValues = {
            params[POT_L_PARAM].getValue(),
            params[POT_C_PARAM].getValue(),
            params[POT_R_PARAM].getValue()
        };
        
        // Build _NT_uiData structure
        _NT_uiData uiData = {};
        
        // Set pot values
        for (int i = 0; i < 3; i++) {
            uiData.pots[i] = potValues[i];
        }
        
        // Set control bitmask to indicate which pot changed
        uint16_t controls = (kNT_potL << potIndex);
        uiData.controls = controls;
        
        // Current button state (for completeness)
        uint16_t currentButtons = 0;
        for (int i = 0; i < 4; i++) {
            if (params[BUTTON_1_PARAM + i].getValue() > 0.5f) {
                currentButtons |= (kNT_button1 << i);
            }
        }
        uiData.lastButtons = currentButtons;
        
        // Include current encoder states (from hardware state or direct reading)
        // For now, encoders are handled separately, but we should include their current deltas
        const auto& hwState = emulatorCore.getHardwareState();
        uiData.encoders[0] = hwState.encoder_deltas[0];
        uiData.encoders[1] = hwState.encoder_deltas[1];
        
        // Send customUi event
        try {
            INFO("Direct CustomUi pot event: pot %d = %.3f, controls=0x%04X", potIndex, newValue, controls);
            factory->customUi(algorithm, uiData);
        } catch (...) {
            INFO("Plugin crashed during direct customUi call");
        }
    }
    
    // Direct customUi event for encoder changes
    void sendCustomUiEncoderEvent(int encoderIndex, int delta) {
        INFO("sendCustomUiEncoderEvent called: encoder %d, delta=%d", encoderIndex, delta);
        
        if (!isPluginLoaded() || !pluginManager->getFactory() || !pluginManager->getAlgorithm()) {
            return;
        }
        
        _NT_factory* factory = pluginManager->getFactory();
        _NT_algorithm* algorithm = pluginManager->getAlgorithm();
        
        if (!factory->customUi) {
            return;
        }
        
        // Build _NT_uiData structure
        _NT_uiData uiData = {};
        
        // Set current pot values
        for (int i = 0; i < 3; i++) {
            uiData.pots[i] = params[POT_L_PARAM + i].getValue();
        }
        
        // Set encoder deltas
        uiData.encoders[0] = (encoderIndex == 0) ? delta : 0;
        uiData.encoders[1] = (encoderIndex == 1) ? delta : 0;
        
        // Set control bitmask
        uint16_t controls = (kNT_encoderL << encoderIndex);
        uiData.controls = controls;
        
        // Current button state
        uint16_t currentButtons = 0;
        for (int i = 0; i < 4; i++) {
            if (params[BUTTON_1_PARAM + i].getValue() > 0.5f) {
                currentButtons |= (kNT_button1 << i);
            }
        }
        uiData.lastButtons = currentButtons;
        
        // Send customUi event
        try {
            INFO("Direct CustomUi encoder event: encoder %d, delta=%d, controls=0x%04X", 
                 encoderIndex, delta, controls);
            factory->customUi(algorithm, uiData);
        } catch (...) {
            INFO("Plugin crashed during customUi encoder event");
        }
    }
    
    // Direct customUi event for button changes
    void sendCustomUiButtonEvent(int buttonIndex, bool pressed) {
        INFO("sendCustomUiButtonEvent called: button %d %s", buttonIndex, pressed ? "pressed" : "released");
        
        if (!isPluginLoaded() || !pluginManager->getFactory() || !pluginManager->getAlgorithm()) {
            return;
        }
        
        _NT_factory* factory = pluginManager->getFactory();
        _NT_algorithm* algorithm = pluginManager->getAlgorithm();
        
        if (!factory->customUi) {
            return;
        }
        
        // Build _NT_uiData structure
        _NT_uiData uiData = {};
        
        // Set current pot values
        for (int i = 0; i < 3; i++) {
            uiData.pots[i] = params[POT_L_PARAM + i].getValue();
        }
        
        // Clear encoder deltas
        uiData.encoders[0] = 0;
        uiData.encoders[1] = 0;
        
        // Build current button state
        uint16_t currentButtons = 0;
        for (int i = 0; i < 4; i++) {
            if (params[BUTTON_1_PARAM + i].getValue() > 0.5f) {
                currentButtons |= (kNT_button1 << i);
            }
        }
        
        // Update for the changing button
        if (pressed) {
            currentButtons |= (kNT_button1 << buttonIndex);
        } else {
            currentButtons &= ~(kNT_button1 << buttonIndex);
        }
        
        // Set control to indicate button change (edge detection)
        uint16_t controls = (kNT_button1 << buttonIndex);
        uiData.controls = controls;
        uiData.lastButtons = currentButtons;
        
        // Send customUi event
        try {
            INFO("Direct CustomUi button event: button %d %s, controls=0x%04X, buttons=0x%04X", 
                 buttonIndex, pressed ? "pressed" : "released", controls, currentButtons);
            factory->customUi(algorithm, uiData);
        } catch (...) {
            INFO("Plugin crashed during customUi button event");
        }
    }
    
    // IDisplayDataProvider interface implementation
    bool isDisplayDirty() const override { return displayDirty; }
    void setDisplayDirty(bool dirty) override { displayDirty = dirty; }
    const VCVDisplayBuffer& getDisplayBuffer() const override { return emulatorCore.getDisplayBuffer(); }
    void updateDisplay() override { emulatorCore.updateDisplay(); }
    
    int getMenuMode() const override { return static_cast<int>(menuMode); }
    
    bool hasLoadedPlugin() const override { 
        return pluginManager && pluginManager->getFactory() != nullptr; 
    }
    PluginManager* getPluginManagerPtr() const override { 
        return pluginManager.get(); 
    }
    
    ParameterSystem* getParameterSystemPtr() const override { 
        return parameterSystem.get(); 
    }
    
    void safeExecutePlugin(std::function<void()> func, const std::string& operation) override {
        // Delegate to the existing safeExecutePlugin method
        this->safeExecutePlugin(func, operation.c_str());
    }
};


// MIDI API functions - now properly implemented after EmulatorModule definition
extern "C" {
    __attribute__((visibility("default"))) void NT_sendMidiByte(uint32_t destination, uint8_t b0) {
        if (g_currentModule && g_currentModule->midiProcessor) {
            g_currentModule->midiProcessor->sendMidiMessage(b0);
        }
    }
    
    __attribute__((visibility("default"))) void NT_sendMidi2ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1) {
        if (g_currentModule && g_currentModule->midiProcessor) {
            g_currentModule->midiProcessor->sendMidiMessage(b0, b1);
        }
    }
    
    __attribute__((visibility("default"))) void NT_sendMidi3ByteMessage(uint32_t destination, uint8_t b0, uint8_t b1, uint8_t b2) {
        INFO("NT_sendMidi3ByteMessage: %02X %02X %02X, destination=%08X", b0, b1, b2, destination);
        if (g_currentModule && g_currentModule->midiProcessor) {
            g_currentModule->midiProcessor->sendMidiMessage(b0, b1, b2);
        } else {
            INFO("NT_sendMidi3ByteMessage: No module or midiProcessor available");
        }
    }
    
    __attribute__((visibility("default"))) void NT_sendMidiSysEx(uint32_t destination, const uint8_t* data, uint32_t count, bool end) {
        if (g_currentModule && g_currentModule->midiProcessor && data && count > 0) {
            // Send SysEx messages byte by byte since VCV's MIDI system handles messages up to 3 bytes
            // For SysEx, we send 0xF0 start, data bytes, and 0xF7 end
            g_currentModule->midiProcessor->sendMidiMessage(0xF0); // SysEx start
            
            for (uint32_t i = 0; i < count; i++) {
                g_currentModule->midiProcessor->sendMidiMessage(data[i]);
            }
            
            if (end) {
                g_currentModule->midiProcessor->sendMidiMessage(0xF7); // SysEx end
            }
        }
    }
}


// Implementation of ContextAwareEncoderQuantity
std::string ContextAwareEncoderQuantity::getDisplayValueString() {
    if (!distingModule || distingModule->menuMode == EmulatorModule::MENU_OFF) {
        return EncoderParamQuantity::getDisplayValueString();
    }
    
    // Show current parameter name and value
    int paramIdx = distingModule->parameterSystem->getCurrentParamIndex();
    if (paramIdx >= 0 && paramIdx < (int)distingModule->parameterSystem->getParameterCount()) {
        const _NT_parameter& param = distingModule->parameterSystem->getParameters()[paramIdx];
        int value = distingModule->parameterSystem->getRoutingMatrix()[paramIdx];
        
        // Simple value display for now
        return std::string(param.name) + ": " + std::to_string(value);
    }
    
    return EncoderParamQuantity::getDisplayValueString();
}

// Implementation of TooltipOutputPort
void TooltipOutputPort::onHover(const HoverEvent& e) {
    PJ301MPort::onHover(e);
    
    // Update tooltip based on current routing
    if (distingModule && distingModule->isPluginLoaded()) {
        // Find parameter that routes to this output
        for (const auto& pair : distingModule->paramOutputRouting) {
            size_t paramIdx = pair.first;
            // Check if this parameter's routing would send to this output
            const std::string& paramName = distingModule->parameterSystem->getParameters()[paramIdx].name;
            int targetOutput = -1;
            
            // Extract output number from parameter name
            if (sscanf(paramName.c_str(), "Output %d", &targetOutput) == 1 ||
                sscanf(paramName.c_str(), "Out %d", &targetOutput) == 1) {
                targetOutput--; // Convert to 0-based
                
                if (targetOutput == outputIndex) {
                    // Tooltip will be displayed through VCV's mechanism
                    return;
                }
            }
        }
    }
}

struct EmulatorWidget : ModuleWidget {
    EmulatorWidget(EmulatorModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DistingNT.svg")));

        // Add mounting screws (14HP module = 71.12mm width)
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // OLED Display (larger, centered at top)
        DisplayRenderer::ModuleOLEDWidget* display = new DisplayRenderer::ModuleOLEDWidget();
        display->box.pos = mm2px(Vec(8.0, 8.0));
        display->box.size = mm2px(Vec(55.12, 15.0));
        if (module) {
            display->dataProvider = module;
        }
        addChild(display);

        // 3 Pressable Pots (large knobs with press capability)
        auto* potL = createParamCentered<EmulatorPressablePot>(mm2px(Vec(17.78, 35.0)), module, EmulatorModule::POT_L_PARAM);
        if (module) {
            potL->setEmulatorCore(&module->emulatorCore, 0);
            potL->setPotChangedCallback([module](int index, float value) {
                module->sendCustomUiPotEvent(index, value);
            });
        }
        addParam(potL);
        
        auto* potC = createParamCentered<EmulatorPressablePot>(mm2px(Vec(35.56, 35.0)), module, EmulatorModule::POT_C_PARAM);
        if (module) {
            potC->setEmulatorCore(&module->emulatorCore, 1);
            potC->setPotChangedCallback([module](int index, float value) {
                module->sendCustomUiPotEvent(index, value);
            });
        }
        addParam(potC);
        
        auto* potR = createParamCentered<EmulatorPressablePot>(mm2px(Vec(53.34, 35.0)), module, EmulatorModule::POT_R_PARAM);
        if (module) {
            potR->setEmulatorCore(&module->emulatorCore, 2);
            potR->setPotChangedCallback([module](int index, float value) {
                module->sendCustomUiPotEvent(index, value);
            });
        }
        addParam(potR);

        // 2 Simple Encoders
        auto* encoderL = createParamCentered<SimpleEncoder>(mm2px(Vec(26.67, 52.0)), module, EmulatorModule::ENCODER_L_PARAM);
        if (module) {
            encoderL->setEmulatorCore(&module->emulatorCore, 0);
            encoderL->setEncoderChangedCallback([module](int index, int delta) {
                module->onEncoderChange(index, delta);
            });
        }
        addParam(encoderL);
        
        auto* encoderR = createParamCentered<SimpleEncoder>(mm2px(Vec(44.45, 52.0)), module, EmulatorModule::ENCODER_R_PARAM);
        if (module) {
            encoderR->setEmulatorCore(&module->emulatorCore, 1);
            encoderR->setEncoderChangedCallback([module](int index, int delta) {
                module->onEncoderChange(index, delta);
            });
        }
        addParam(encoderR);

        // 4 Buttons (vertical pairs - no LEDs)
        addParam(createParamCentered<TL1105>(mm2px(Vec(14.5, 48.0)), module, EmulatorModule::BUTTON_1_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(14.5, 56.0)), module, EmulatorModule::BUTTON_2_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(56.5, 48.0)), module, EmulatorModule::BUTTON_3_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(56.5, 56.0)), module, EmulatorModule::BUTTON_4_PARAM));

        // 12 Inputs (3 rows of 4) - balanced margins
        float inputStartY = 70.0;
        float jackSpacing = 9.0;
        float rowSpacing = 10.0;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                int index = row * 4 + col;
                float x = 10.0 + col * jackSpacing;  // Balanced positioning
                float y = inputStartY + row * rowSpacing;
                addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, EmulatorModule::AUDIO_INPUT_1 + index));
            }
        }

        // 8 Outputs (4 rows of 2) - balanced margins
        float outputStartX = 55.0;  // Balanced positioning
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 2; col++) {
                int index = row * 2 + col;
                float x = outputStartX + col * jackSpacing;
                float y = inputStartY + row * rowSpacing;
                
                // Create custom output port with dynamic tooltips
                TooltipOutputPort* outputPort = createOutputCentered<TooltipOutputPort>(
                    mm2px(Vec(x, y)), module, EmulatorModule::AUDIO_OUTPUT_1 + index);
                if (module) {
                    outputPort->distingModule = module;
                    outputPort->outputIndex = index;
                }
                addOutput(outputPort);
            }
        }
        
        // MIDI activity LEDs (near the display)
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(5.0, 8.0)), module, EmulatorModule::MIDI_INPUT_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(66.0, 8.0)), module, EmulatorModule::MIDI_OUTPUT_LIGHT));
    }
    
    void appendContextMenu(Menu* menu) override {
        EmulatorModule* module = dynamic_cast<EmulatorModule*>(this->module);
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
            if (module->isPluginLoaded()) {
                menu->addChild(new MenuSeparator);
                menu->addChild(createMenuItem("Reload Plugin", "", [=]() {
                    module->reloadPlugin();
                }));
                menu->addChild(createMenuItem("Unload Plugin", "", [=]() {
                    module->unloadPlugin();
                }));
                
                // Show current plugin
                std::string filename = rack::system::getFilename(module->pluginManager->getPluginPath());
                menu->addChild(createMenuLabel(string::f("Current: %s", filename.c_str())));
            }
        }));
        
        menu->addChild(new MenuSeparator);
        
        // MIDI Input submenu
        menu->addChild(createSubmenuItem("MIDI Input", "", [=](Menu* menu) {
            appendMidiMenu(menu, &module->midiProcessor->getInputQueue());
        }));
        
        // MIDI Output submenu
        menu->addChild(createSubmenuItem("MIDI Output", "", [=](Menu* menu) {
            appendMidiMenu(menu, &module->midiProcessor->getOutput());
        }));
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Algorithm"));
        
        // This will be implemented when we have actual algorithms loaded
    }
    
    void loadPluginDialog(EmulatorModule* module, std::string startPath = "") {
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
            
            // Phase 1: Discover plugin specifications
            auto specInfo = module->discoverPluginSpecifications(path);
            
            if (specInfo.hasSpecifications()) {
                // Show specification dialog
                showSpecificationDialog(module, specInfo);
            } else {
                // No specifications needed, load directly
                module->loadPlugin(path);
            }
        }
        
        osdialog_filters_free(filters);
    }
    
    // Simplified specification dialog - using default values for now
    
    // Custom text field for numeric input with validation
    struct NumericTextField : ui::TextField {
        int32_t* valuePtr = nullptr;
        int32_t minValue = 0;
        int32_t maxValue = 0;
        
        NumericTextField(int32_t* valuePtr, int32_t minVal, int32_t maxVal) 
            : valuePtr(valuePtr), minValue(minVal), maxValue(maxVal) {
            box.size.x = 60.0f;
            if (valuePtr) {
                text = std::to_string(*valuePtr);
            } else {
                text = "0";
            }
        }
        
        void onSelectKey(const SelectKeyEvent& e) override {
            TextField::onSelectKey(e);
            
            // Update value when Enter is pressed or focus is lost
            if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
                updateValue();
            }
        }
        
        void onDeselect(const DeselectEvent& e) override {
            TextField::onDeselect(e);
            updateValue();
        }
        
        void updateValue() {
            if (valuePtr) {
                try {
                    int32_t newValue = std::stoi(text);
                    // Clamp to valid range
                    newValue = std::max(minValue, std::min(maxValue, newValue));
                    *valuePtr = newValue;
                    // Update text field to show clamped value
                    text = std::to_string(newValue);
                } catch (...) {
                    // Invalid input, restore previous value
                    text = std::to_string(*valuePtr);
                }
            }
        }
    };

    // Specification dialog window  
    struct SpecificationDialog : ui::MenuOverlay {
        EmulatorModule* module = nullptr;
        EmulatorModule::PluginSpecificationInfo specInfo;
        std::vector<int32_t> specificationValues;
        std::vector<NumericTextField*> textFields;
        
        SpecificationDialog(EmulatorModule* module, const EmulatorModule::PluginSpecificationInfo& info) 
            : module(module), specInfo(info) {
            
            // Validate specifications vector before using it
            if (specInfo.specifications.empty() || specInfo.specifications.size() > 64) {
                // Defensive fallback - don't create dialog if specifications are invalid
                return;
            }
            
            specificationValues.resize(specInfo.specifications.size());
            
            // Initialize with default values with bounds checking
            for (size_t i = 0; i < specInfo.specifications.size(); i++) {
                try {
                    // Validate that we can access this element safely
                    const auto& spec = specInfo.specifications.at(i);
                    specificationValues[i] = spec.def;
                } catch (const std::exception&) {
                    // If we can't access the specification safely, use default
                    specificationValues[i] = 0;
                }
            }
            
            // Create menu content
            ui::Menu* menu = new ui::Menu;
            menu->box.size.x = 400.0f; // Make menu wider to accommodate input fields
            
            // Title
            std::string titleText = specInfo.name.empty() ? "Configure Plugin" : ("Configure " + specInfo.name);
            menu->addChild(createMenuLabel(titleText));
            menu->addChild(new ui::MenuSeparator);
            
            if (!specInfo.description.empty()) {
                menu->addChild(createMenuLabel(specInfo.description));
                menu->addChild(new ui::MenuSeparator);
            }
            
            // Create input fields for each specification
            for (size_t i = 0; i < specInfo.specifications.size(); i++) {
                const auto& spec = specInfo.specifications[i];
                
                // Create a container for label and input field
                ui::MenuEntry* entry = new ui::MenuEntry;
                entry->box.size.x = menu->box.size.x;
                entry->box.size.y = 30.0f;
                
                // Create label with safe string handling - avoid dereferencing bad pointers
                ui::Label* label = new ui::Label;
                std::string specName = "Spec " + std::to_string(i); // Safe default
                
                // Skip any unsafe pointer access entirely for now
                // The spec.name pointer appears to be corrupted, so use fallback naming
                label->text = specName + " (" + std::to_string(spec.min) + " to " + std::to_string(spec.max) + "):";
                label->box.pos = math::Vec(10.0f, 8.0f);
                label->color = nvgRGB(0xFF, 0xFF, 0xFF);
                entry->addChild(label);
                
                // Create text field for input
                NumericTextField* textField = new NumericTextField(&specificationValues[i], spec.min, spec.max);
                textField->box.pos = math::Vec(menu->box.size.x - 80.0f, 5.0f);
                entry->addChild(textField);
                textFields.push_back(textField);
                
                menu->addChild(entry);
            }
            
            menu->addChild(new ui::MenuSeparator);
            
            // Buttons
            menu->addChild(createMenuItem("Load Plugin", "", [=]() {
                // Update all values from text fields before loading
                for (auto* field : textFields) {
                    field->updateValue();
                }
                // Load plugin with configured specifications
                module->loadPlugin(specInfo.path, specificationValues);
                requestDelete();
            }));
            
            menu->addChild(createMenuItem("Cancel", "", [=]() {
                requestDelete();
            }));
            
            addChild(menu);
        }
    };
    
    void showSpecificationDialog(EmulatorModule* module, const EmulatorModule::PluginSpecificationInfo& specInfo) {
        SpecificationDialog* dialog = new SpecificationDialog(module, specInfo);
        APP->scene->addChild(dialog);
    }
};

Model* modelNtEmu = createModel<EmulatorModule, EmulatorWidget>("nt_emu");

extern "C" void emulatorHandleSetParameterFromUi(uint32_t parameter, int16_t value) {
    if (g_currentModule) {
        g_currentModule->handleSetParameterFromUi(parameter, value);
    }
}
