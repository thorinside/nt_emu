# VCV Rack Parameter Update Patterns

This document outlines best practices for handling parameter updates in VCV Rack modules, especially for parameters that affect audio routing.

## Key Principles

### 1. Thread Safety Guarantee
VCV Rack guarantees that module methods are mutually exclusive:
- `process()` 
- `onReset()`, `onRandomize()`
- `dataToJson()`, `dataFromJson()`
- Parameter change events

This means no explicit locking is needed in module code.

### 2. Immediate vs Deferred Updates

**Immediate Updates** (Discrete Parameters):
- Routing switches
- On/off states
- Trigger responses
- Any parameter where delay would be confusing

**Deferred/Smoothed Updates** (Continuous Parameters):
- Volume faders
- Filter cutoffs
- Parameters that could cause clicks/pops

## Common Patterns

### Direct Parameter Reading
```cpp
// Best practice: Read parameters directly in process()
void process(const ProcessArgs& args) override {
    // Read routing parameter every sample
    int route = params[ROUTE_PARAM].getValue();
    
    // Apply routing immediately
    switch(route) {
        case 0:
            outputs[OUT_A].setVoltage(inputs[IN].getVoltage());
            outputs[OUT_B].setVoltage(0.f);
            break;
        case 1:
            outputs[OUT_A].setVoltage(0.f);
            outputs[OUT_B].setVoltage(inputs[IN].getVoltage());
            break;
    }
}
```

### Cached State Pattern
```cpp
// Only cache if computation is expensive
struct MyModule : Module {
    // Cache expensive calculations
    float expensiveResult = 0.f;
    int lastRouteParam = -1;
    
    void process(const ProcessArgs& args) override {
        int route = params[ROUTE_PARAM].getValue();
        
        // Update cache only when parameter changes
        if (route != lastRouteParam) {
            expensiveResult = calculateExpensiveThing(route);
            lastRouteParam = route;
        }
        
        // Use cached result
        outputs[OUT].setVoltage(inputs[IN].getVoltage() * expensiveResult);
    }
};
```

### Parameter Change Detection
```cpp
// For parameters that trigger actions on change
struct MyModule : Module {
    float lastParamValue = 0.f;
    
    void process(const ProcessArgs& args) override {
        float paramValue = params[MY_PARAM].getValue();
        
        // Detect change
        if (paramValue != lastParamValue) {
            onParameterChange(paramValue);
            lastParamValue = paramValue;
        }
    }
    
    void onParameterChange(float newValue) {
        // Handle the change
    }
};
```

## Anti-Patterns to Avoid

### 1. Deferred Routing Updates
```cpp
// BAD: Don't defer routing updates
void process(const ProcessArgs& args) override {
    if (updatePending) {
        updateRouting();  // Too late!
        updatePending = false;
    }
    // User expects immediate response
}
```

### 2. Complex State Management
```cpp
// BAD: Avoid complex intermediate state
struct RouteMap {
    int source;
    int dest;
    float level;
};
std::vector<RouteMap> routingTable;  // Can get out of sync

// GOOD: Read parameters directly
float getRouteLevel(int src, int dst) {
    return params[LEVEL_PARAM + src * 4 + dst].getValue();
}
```

### 3. Blocking Operations
```cpp
// BAD: Never block in process()
void process(const ProcessArgs& args) override {
    mutex.lock();  // NO!
    // ...
    mutex.unlock();
}
```

## Performance Considerations

### When Caching Makes Sense
- Complex mathematical operations (FFT, convolution)
- String parsing or formatting
- Large lookup tables
- File I/O results (though avoid in process())

### When Direct Reading is Better
- Simple arithmetic
- Routing decisions
- Boolean flags
- Most parameter scaling

## Example: Matrix Router
```cpp
// Efficient matrix router reading parameters directly
void process(const ProcessArgs& args) override {
    // Clear outputs
    for (int out = 0; out < NUM_OUTPUTS; out++) {
        outputs[out].setVoltage(0.f);
    }
    
    // Matrix routing - read all connections
    for (int in = 0; in < NUM_INPUTS; in++) {
        if (!inputs[in].isConnected()) continue;
        
        float inputVoltage = inputs[in].getVoltage();
        
        for (int out = 0; out < NUM_OUTPUTS; out++) {
            // Direct parameter read for each matrix point
            float level = params[MATRIX_PARAM + in * NUM_OUTPUTS + out].getValue();
            
            if (level > 0.f) {
                // Accumulate routed signal
                float current = outputs[out].getVoltage();
                outputs[out].setVoltage(current + inputVoltage * level);
            }
        }
    }
}
```

## Testing Parameter Updates

### Manual Test Protocol
1. Change parameter while audio is playing
2. Verify immediate response (routing should switch instantly)
3. Check for audio artifacts (clicks, pops)
4. Test rapid parameter changes
5. Verify state persistence on save/load

### Automated Testing
```cpp
// Test immediate parameter response
void testParameterResponse() {
    MyModule module;
    
    // Set initial state
    module.params[ROUTE_PARAM].setValue(0);
    module.inputs[IN].setVoltage(5.f);
    
    // Process one sample
    module.process(ProcessArgs{});
    assert(module.outputs[OUT_A].getVoltage() == 5.f);
    assert(module.outputs[OUT_B].getVoltage() == 0.f);
    
    // Change parameter
    module.params[ROUTE_PARAM].setValue(1);
    
    // Process next sample - should immediately reflect change
    module.process(ProcessArgs{});
    assert(module.outputs[OUT_A].getVoltage() == 0.f);
    assert(module.outputs[OUT_B].getVoltage() == 5.f);
}
```