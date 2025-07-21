#include "plugin.hpp"
#include "algorithms/Algorithm.hpp"
#include "dsp/BusSystem.hpp"
#include "EmulatorCore.hpp"
#include <componentlibrary.hpp>

struct DistingNT : Module {
    enum ParamIds {
        // Pots
        POT_L_PARAM, POT_C_PARAM, POT_R_PARAM,
        
        // Buttons 
        BUTTON_1_PARAM, BUTTON_2_PARAM, BUTTON_3_PARAM, BUTTON_4_PARAM,
        
        // Encoders (simplified for widget placement)
        ENCODER_L_PARAM, ENCODER_R_PARAM,
        ENCODER_L_PRESS_PARAM, ENCODER_R_PRESS_PARAM,
        
        // Algorithm selection
        ALGORITHM_PARAM,
        
        NUM_PARAMS
    };
    
    enum InputIds {
        // 12 inputs (as per hardware specification)
        AUDIO_INPUT_1, AUDIO_INPUT_2, AUDIO_INPUT_3, AUDIO_INPUT_4,
        AUDIO_INPUT_5, AUDIO_INPUT_6, AUDIO_INPUT_7, AUDIO_INPUT_8,
        AUDIO_INPUT_9, AUDIO_INPUT_10, AUDIO_INPUT_11, AUDIO_INPUT_12,
        
        NUM_INPUTS
    };
    
    enum OutputIds {
        // 6 outputs (as per hardware specification)
        AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3, 
        AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
        
        NUM_OUTPUTS
    };
    
    enum LightIds {
        BUTTON_1_LIGHT, BUTTON_2_LIGHT, BUTTON_3_LIGHT, BUTTON_4_LIGHT,
        NUM_LIGHTS
    };

    // Core components
    BusSystem busSystem;
    EmulatorCore emulatorCore;
    
    // Display state
    bool displayDirty = true;
    
    // Sample accumulation for 4-sample processing blocks
    int sampleCounter = 0;
    static constexpr int BLOCK_SIZE = 4;
    
    // Trigger detectors for buttons and encoders
    dsp::SchmittTrigger buttonTriggers[4];
    dsp::SchmittTrigger encoderLPressTrigger, encoderRPressTrigger;
    
    DistingNT() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        // Configure parameters
        configParam(POT_L_PARAM, 0.f, 1.f, 0.5f, "Pot L");
        configParam(POT_C_PARAM, 0.f, 1.f, 0.5f, "Pot C");  
        configParam(POT_R_PARAM, 0.f, 1.f, 0.5f, "Pot R");
        
        configParam(BUTTON_1_PARAM, 0.f, 1.f, 0.f, "Button 1");
        configParam(BUTTON_2_PARAM, 0.f, 1.f, 0.f, "Button 2");
        configParam(BUTTON_3_PARAM, 0.f, 1.f, 0.f, "Button 3");
        configParam(BUTTON_4_PARAM, 0.f, 1.f, 0.f, "Button 4");
        
        // Configure infinite encoders with detent mode
        configParam(ENCODER_L_PARAM, -INFINITY, INFINITY, 0.f, "Encoder L");
        configParam(ENCODER_R_PARAM, -INFINITY, INFINITY, 0.f, "Encoder R");
        getParamQuantity(ENCODER_L_PARAM)->snapEnabled = true;
        getParamQuantity(ENCODER_R_PARAM)->snapEnabled = true;
        configParam(ENCODER_L_PRESS_PARAM, 0.f, 1.f, 0.f, "Encoder L Press");
        configParam(ENCODER_R_PRESS_PARAM, 0.f, 1.f, 0.f, "Encoder R Press");
        
        configParam(ALGORITHM_PARAM, 0.f, 1.f, 0.f, "Algorithm");
        
        // Configure 12 inputs
        for (int i = 0; i < 12; i++) {
            configInput(AUDIO_INPUT_1 + i, string::f("Input %d", i + 1));
        }
        
        // Configure 6 outputs  
        for (int i = 0; i < 6; i++) {
            configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
        }
        
        // Initialize bus system
        busSystem.init();
        
        // Initialize emulator core
        emulatorCore.initialize(APP->engine->getSampleRate());
    }
    
    // This method is no longer needed as EmulatorCore handles algorithm loading
    
    void process(const ProcessArgs& args) override {
        // Handle control inputs
        processControls();
        
        // Route inputs to buses
        busSystem.routeInputs(this);
        
        // Accumulate samples for block processing
        sampleCounter++;
        if (sampleCounter >= BLOCK_SIZE) {
            // Process algorithm with 4-sample blocks
            emulatorCore.processAudio(busSystem.getBuses(), 1); // 1 = numFramesBy4
            sampleCounter = 0;
        }
        
        // Route buses to outputs
        busSystem.routeOutputs(this);
        
        // Update lights
        updateLights();
    }
    
    void processControls() {
        // Process button triggers
        for (int i = 0; i < 4; i++) {
            if (buttonTriggers[i].process(params[BUTTON_1_PARAM + i].getValue())) {
                onButtonPress(i, true);
                displayDirty = true;
            }
        }
        
        // Process encoder press triggers
        if (encoderLPressTrigger.process(params[ENCODER_L_PRESS_PARAM].getValue())) {
            onEncoderPress(0);
            displayDirty = true;
        }
        if (encoderRPressTrigger.process(params[ENCODER_R_PRESS_PARAM].getValue())) {
            onEncoderPress(1);
            displayDirty = true;
        }
        
        // Update hardware state for emulator core
        VCVHardwareState hwState;
        
        // Update pot values
        for (int i = 0; i < 3; i++) {
            hwState.pots[i] = params[POT_L_PARAM + i].getValue();
        }
        
        // Update button states
        for (int i = 0; i < 4; i++) {
            hwState.buttons[i] = params[BUTTON_1_PARAM + i].getValue() > 0.5f;
        }
        
        // Update encoder states
        hwState.pot_pressed[0] = params[ENCODER_L_PRESS_PARAM].getValue() > 0.5f;
        hwState.pot_pressed[2] = params[ENCODER_R_PRESS_PARAM].getValue() > 0.5f;
        
        // Set encoder values from params
        hwState.encoder_deltas[0] = 0;
        hwState.encoder_deltas[1] = 0;
        
        emulatorCore.updateHardwareState(hwState);
    }
    
    void updateLights() {
        // Update button lights based on state
        for (int i = 0; i < 4; i++) {
            lights[BUTTON_1_LIGHT + i].setBrightness(params[BUTTON_1_PARAM + i].getValue());
        }
    }
    
    void onParameterChange(int parameter, float value) {
        emulatorCore.setParameter(parameter, value);
    }
    
    void onButtonPress(int button, bool pressed) {
        if (pressed) {
            emulatorCore.pressButton(button);
        } else {
            emulatorCore.releaseButton(button);
        }
    }
    
    void onEncoderChange(int encoder, int delta) {
        emulatorCore.turnEncoder(encoder, delta);
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
        return emulatorCore.saveState();
    }
    
    void dataFromJson(json_t* rootJ) override {
        emulatorCore.loadState(rootJ);
        displayDirty = true;
    }
};

// Custom OLED Widget with access to DistingNT module
struct DistingNTOLEDWidget : FramebufferWidget {
    DistingNT* module = nullptr;
    
    // Display dimensions matching Disting NT OLED
    static constexpr int DISPLAY_WIDTH = 256;
    static constexpr int DISPLAY_HEIGHT = 64;
    
    DistingNTOLEDWidget() {}
    
    void step() override {
        // Check if we need to redraw
        if (module && (module->displayDirty || module->emulatorCore.getDisplayBuffer().dirty)) {
            // Mark framebuffer as dirty to trigger redraw
            FramebufferWidget::dirty = true;
            module->displayDirty = false;
        }
        FramebufferWidget::step();
    }
    
    void draw(const DrawArgs& args) override {
        if (!module) {
            drawPlaceholder(args);
            return;
        }
        
        // Set up coordinate system for 256x64 display
        nvgSave(args.vg);
        nvgScale(args.vg, box.size.x / DISPLAY_WIDTH, box.size.y / DISPLAY_HEIGHT);
        
        // Clear display background (black)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);
        
        // Update emulator display
        module->emulatorCore.updateDisplay();
        
        // Draw the display buffer
        drawDisplayBuffer(args.vg, module->emulatorCore.getDisplayBuffer());
        
        nvgRestore(args.vg);
    }
    
    void drawPlaceholder(const DrawArgs& args) {
        // Draw placeholder when module is not available (browser preview)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(20, 20, 20));
        nvgFill(args.vg);
        
        // Draw border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgStrokeColor(args.vg, nvgRGB(100, 100, 100));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);
        
        // Draw "OLED" text
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgFontSize(args.vg, 14);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, "OLED DISPLAY", nullptr);
    }
    
    void drawDisplayBuffer(NVGcontext* vg, const VCVDisplayBuffer& buffer) {
        // Draw the display buffer pixel by pixel
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                if (buffer.getPixel(x, y)) {
                    nvgBeginPath(vg);
                    nvgRect(vg, x, y, 1, 1);
                    nvgFill(vg);
                }
            }
        }
    }
};

struct DistingNTWidget : ModuleWidget {
    DistingNTWidget(DistingNT* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DistingNT.svg")));

        // Add mounting screws (14HP module = 71.12mm width)
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // OLED Display (larger, centered at top)
        DistingNTOLEDWidget* display = new DistingNTOLEDWidget();
        display->box.pos = mm2px(Vec(8.0, 8.0));
        display->box.size = mm2px(Vec(55.12, 15.0));
        if (module) {
            display->module = module;
        }
        addChild(display);

        // 3 Pots (large knobs)
        addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(17.78, 35.0)), module, DistingNT::POT_L_PARAM));
        addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(35.56, 35.0)), module, DistingNT::POT_C_PARAM));
        addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(53.34, 35.0)), module, DistingNT::POT_R_PARAM));

        // 2 Encoders (infinite encoders - no position indicators)
        auto* encoderL = createParamCentered<BefacoTinyKnob>(mm2px(Vec(26.67, 52.0)), module, DistingNT::ENCODER_L_PARAM);
        encoderL->speed = 2.0f;  // Increased sensitivity
        addParam(encoderL);
        
        auto* encoderR = createParamCentered<BefacoTinyKnob>(mm2px(Vec(44.45, 52.0)), module, DistingNT::ENCODER_R_PARAM);
        encoderR->speed = 2.0f;  // Increased sensitivity
        addParam(encoderR);

        // 4 Buttons (vertical pairs - no LEDs)
        addParam(createParamCentered<TL1105>(mm2px(Vec(14.5, 48.0)), module, DistingNT::BUTTON_1_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(14.5, 56.0)), module, DistingNT::BUTTON_2_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(56.5, 48.0)), module, DistingNT::BUTTON_3_PARAM));
        addParam(createParamCentered<TL1105>(mm2px(Vec(56.5, 56.0)), module, DistingNT::BUTTON_4_PARAM));

        // 12 Inputs (3 rows of 4) - balanced margins
        float inputStartY = 70.0;
        float jackSpacing = 9.0;
        float rowSpacing = 10.0;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                int index = row * 4 + col;
                float x = 10.0 + col * jackSpacing;  // Balanced positioning
                float y = inputStartY + row * rowSpacing;
                addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, DistingNT::AUDIO_INPUT_1 + index));
            }
        }

        // 6 Outputs (3 rows of 2) - balanced margins
        float outputStartX = 55.0;  // Balanced positioning
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 2; col++) {
                int index = row * 2 + col;
                float x = outputStartX + col * jackSpacing;
                float y = inputStartY + row * rowSpacing;
                addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, DistingNT::AUDIO_OUTPUT_1 + index));
            }
        }
    }
    
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Algorithm"));
        
        // TODO: Add algorithm selection items
        // This will be implemented when we have actual algorithms loaded
    }
};

Model* modelDistingNT = createModel<DistingNT, DistingNTWidget>("DistingNT");