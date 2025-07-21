#include "plugin.hpp"

struct TestModule : Module {
    enum ParamId {
        PARAMS_LEN
    };
    enum InputId {
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    TestModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    void process(const ProcessArgs& args) override {
        // Empty processing
    }
};

struct TestModuleWidget : ModuleWidget {
    TestModuleWidget(TestModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/TestModule.svg")));
    }
};

Model* modelTestModule = createModel<TestModule, TestModuleWidget>("TestModule");