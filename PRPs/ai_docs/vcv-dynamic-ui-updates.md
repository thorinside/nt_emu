# VCV Rack Dynamic UI Updates

This guide covers implementing dynamic UI updates in VCV Rack, particularly for tooltips and parameter displays.

## ParamQuantity System

### Overview
ParamQuantity is VCV's abstraction for parameter metadata and formatting:
- Controls how parameters are displayed in tooltips
- Handles value formatting and unit display
- Can be customized per parameter

### Basic Configuration
```cpp
// In module constructor
configParam(PARAM_ID, minValue, maxValue, defaultValue, "Label", "Unit");

// Access later
ParamQuantity* pq = getParamQuantity(PARAM_ID);
pq->displayBase = 10;        // Decimal display
pq->displayMultiplier = 100; // Show as percentage
pq->displayOffset = 0;       // No offset
```

### Custom ParamQuantity
```cpp
struct CustomParamQuantity : ParamQuantity {
    // Override display methods
    std::string getDisplayValueString() override {
        float v = getValue();
        return string::f("%.2f dB", v);
    }
    
    std::string getLabel() override {
        // Dynamic label based on module state
        Module* m = module;
        if (MyModule* myModule = dynamic_cast<MyModule*>(m)) {
            if (myModule->isInSpecialMode()) {
                return "Special Parameter";
            }
        }
        return ParamQuantity::getLabel();
    }
    
    std::string getUnit() override {
        return "dB";
    }
};
```

### Installing Custom Quantities
```cpp
// Method 1: Replace after configuration (hacky but works)
configParam(PARAM_ID, min, max, def, "Label");
delete paramQuantities[PARAM_ID];
paramQuantities[PARAM_ID] = new CustomParamQuantity();
paramQuantities[PARAM_ID]->module = this;
paramQuantities[PARAM_ID]->paramId = PARAM_ID;

// Method 2: Use configButton/configSwitch helpers
configButton(PARAM_ID, "Label");
configSwitch(PARAM_ID, min, max, def, "Label", {"Off", "On"});
```

## Input/Output Tooltips

### Static Configuration
```cpp
configInput(INPUT_ID, "Audio Input");
configOutput(OUTPUT_ID, "Filtered Output");
configLight(LIGHT_ID, "Signal Present");
```

### Dynamic Updates
VCV doesn't support dynamic input/output labels, but you can:
1. Use the module's context menu to show routing
2. Display information on the module panel
3. Use custom widgets with hover text

## Custom Widgets with Dynamic Text

### Hover Text Widget
```cpp
struct HoverLabel : TransparentWidget {
    std::string text;
    std::function<std::string()> textCallback;
    
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {
            if (textCallback) {
                text = textCallback();
            }
            // Hover detection
            if (APP->event->hoveredWidget == this) {
                // Draw tooltip
                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 200));
                nvgRect(args.vg, 0, -20, 100, 20);
                nvgFill(args.vg);
                
                nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 255));
                nvgText(args.vg, 2, -5, text.c_str(), NULL);
            }
        }
        Widget::drawLayer(args, layer);
    }
};
```

### Display Widget for Parameters
```cpp
struct ParameterDisplay : LedDisplay {
    MyModule* module;
    
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1 && module) {
            // Get current parameter info
            int paramIdx = module->currentParamIndex;
            if (paramIdx >= 0) {
                const auto& param = module->parameters[paramIdx];
                
                // Draw parameter name
                nvgFontSize(args.vg, 10);
                nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 255));
                nvgText(args.vg, 2, 10, param.name, NULL);
                
                // Draw value
                char valueStr[32];
                module->formatValue(valueStr, param);
                nvgText(args.vg, 2, 22, valueStr, NULL);
            }
        }
        LedDisplay::drawLayer(args, layer);
    }
};
```

## Update Triggers

### When to Update UI
1. **Parameter Changes**: Detected automatically by VCV
2. **Module State Changes**: Use dirty flags
3. **Menu/Mode Changes**: Trigger updates manually

### Dirty Flag Pattern
```cpp
struct MyModule : Module {
    bool displayDirty = true;
    
    void process(const ProcessArgs& args) override {
        if (stateChanged()) {
            displayDirty = true;
        }
    }
};

struct MyModuleWidget : ModuleWidget {
    void draw(const DrawArgs& args) override {
        ModuleWidget::draw(args);
        
        MyModule* module = dynamic_cast<MyModule*>(this->module);
        if (module && module->displayDirty) {
            // Update displays
            module->displayDirty = false;
        }
    }
};
```

## Best Practices

### 1. Performance
- Cache formatted strings when possible
- Only update when state changes
- Avoid heavy computation in draw()

### 2. Thread Safety
- UI runs on main thread
- DSP runs on audio thread
- Use atomic variables for communication

### 3. User Experience
- Keep tooltips concise
- Update promptly on state changes
- Provide visual feedback for modes

## Example: Dynamic Output Labels

```cpp
struct DynamicOutputPort : SvgPort {
    MyModule* module;
    int outputIndex;
    
    void onHover(const HoverEvent& e) override {
        SvgPort::onHover(e);
        
        if (module && !settings::tooltips) {
            // Create custom tooltip
            Tooltip* tooltip = new Tooltip();
            tooltip->text = getLabel();
            APP->scene->addChild(tooltip);
        }
    }
    
    std::string getLabel() {
        if (!module) return "Output";
        
        // Get label based on current routing
        for (const auto& route : module->outputRouting) {
            if (route.outputIndex == outputIndex) {
                return module->parameters[route.paramIndex].name;
            }
        }
        return "Output";
    }
};
```

## References

- VCV Rack API documentation: https://vcvrack.com/docs
- Example modules with dynamic UI: Fundamental, Befaco
- ParamQuantity source: include/engine/ParamQuantity.hpp