#include "plugin.hpp"
#include "widgets/DistingNTPressablePot.hpp"
#include "widgets/DistingNTPressableEncoder.hpp"
#include <componentlibrary.hpp>

struct PressableWidgetsDebugTest : Module {
    enum ParamId {
        POT_PARAM,
        ENCODER_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        INPUTS_LEN
    };
    enum OutputId {
        POT_OUTPUT,
        ENCODER_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        POT_PRESS_LIGHT,
        ENCODER_PRESS_LIGHT,
        ENCODER_STEP_LIGHT,
        LIGHTS_LEN
    };

    // Dummy emulator core for testing
    EmulatorCore dummyCore;
    
    // Track encoder output voltage
    float encoderOutputVoltage = 0.0f;
    float stepLightTimer = 0.0f;
    
    PressableWidgetsDebugTest() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(POT_PARAM, 0.0f, 1.0f, 0.5f, "Test Pot");
        configParam(ENCODER_PARAM, -INFINITY, INFINITY, 0.0f, "Test Encoder");
        
        configOutput(POT_OUTPUT, "Pot Value");
        configOutput(ENCODER_OUTPUT, "Encoder Value");
        
        configLight(POT_PRESS_LIGHT, "Pot Press");
        configLight(ENCODER_PRESS_LIGHT, "Encoder Press");
        configLight(ENCODER_STEP_LIGHT, "Encoder Step");
        
        // Initialize dummy core
        dummyCore.initialize(APP->engine->getSampleRate());
    }

    void process(const ProcessArgs& args) override {
        // Output pot value
        outputs[POT_OUTPUT].setVoltage(params[POT_PARAM].getValue() * 10.0f);
        
        // Update lights based on hardware state
        const auto& hwState = dummyCore.getHardwareState();
        lights[POT_PRESS_LIGHT].setBrightness(hwState.pot_pressed[0] ? 1.0f : 0.0f);
        lights[ENCODER_PRESS_LIGHT].setBrightness(hwState.encoder_pressed[0] ? 1.0f : 0.0f);
        
        // Process encoder delta and update output voltage
        int encoderDelta = hwState.encoder_deltas[0];
        if (encoderDelta != 0) {
            // Increment/decrement output voltage by delta (0.1V per step)
            encoderOutputVoltage += encoderDelta * 0.1f;
            // Clamp to reasonable range
            encoderOutputVoltage = clamp(encoderOutputVoltage, -5.0f, 5.0f);
            
            // Flash step light
            stepLightTimer = 0.1f; // 100ms flash
            
            // Clear the delta after processing
            dummyCore.turnEncoder(0, 0); // Reset delta to 0
        }
        
        // Set encoder output voltage
        outputs[ENCODER_OUTPUT].setVoltage(encoderOutputVoltage);
        
        // Update step light
        stepLightTimer -= args.sampleTime;
        lights[ENCODER_STEP_LIGHT].setBrightness(stepLightTimer > 0.0f ? 1.0f : 0.0f);
    }
};


struct PressableWidgetsDebugTestWidget : ModuleWidget {
    PressableWidgetsDebugTestWidget(PressableWidgetsDebugTest* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PressableWidgetsDebugTest.svg")));
        
        // Layout for custom 60.96mm wide panel
        // Center horizontally around 30.48mm (half of 60.96mm)
        
        // Add press indicator lights (match SVG light labels)
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15, 32)), module, PressableWidgetsDebugTest::POT_PRESS_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(30.48, 32)), module, PressableWidgetsDebugTest::ENCODER_PRESS_LIGHT));
        addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(46, 32)), module, PressableWidgetsDebugTest::ENCODER_STEP_LIGHT));
        
        // Add test pot (centered in "Pressable Pot" section)
        auto* pot = createParamCentered<DistingNTPressablePot>(mm2px(Vec(30.48, 58)), module, PressableWidgetsDebugTest::POT_PARAM);
        if (module) {
            pot->setEmulatorCore(&module->dummyCore, 0);
        }
        addParam(pot);
        
        // Add test encoder (centered in "Pressable Encoder" section)
        auto* encoder = createParamCentered<DistingNTPressableEncoder>(mm2px(Vec(30.48, 88)), module, PressableWidgetsDebugTest::ENCODER_PARAM);
        if (module) {
            encoder->setEmulatorCore(&module->dummyCore, 0);
        }
        addParam(encoder);
        
        // Add output ports (match SVG output labels)
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20, 115)), module, PressableWidgetsDebugTest::POT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41, 115)), module, PressableWidgetsDebugTest::ENCODER_OUTPUT));
        
        // Add labels
        addChild(createWidget<Widget>(mm2px(Vec(5, 10))));
    }
};

Model* modelPressableWidgetsDebugTest = createModel<PressableWidgetsDebugTest, PressableWidgetsDebugTestWidget>("PressableWidgetsDebugTest");