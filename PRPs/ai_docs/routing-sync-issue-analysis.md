# Routing Sync Issue Analysis

This document provides a detailed analysis of the parameter synchronization issue in the nt_emu module.

## The Problem

When users change routing parameters in the menu (e.g., changing "Output" from 1 to 2), the audio doesn't immediately route to the new output. The routing only updates when exiting the menu.

## Root Cause Analysis

### Current Flow
1. User changes parameter in menu → `setParameterValue()` called
2. `setParameterValue()` updates `routingMatrix[paramIdx]` 
3. Plugin is notified via `parameterChanged()` callback
4. But `paramInputRouting`/`paramOutputRouting` maps are NOT updated
5. Audio processing uses stale routing maps
6. Maps only update when menu exits → `applyRoutingChanges()` → `updateParameterRouting()`

### Code Flow Diagram
```
Parameter Change in Menu
    ↓
setParameterValue(paramIdx, value)
    ├─→ routingMatrix[paramIdx] = value ✓
    ├─→ pluginFactory->parameterChanged() ✓
    └─→ updateParameterRouting() ✗ (NOT CALLED)
    
Audio Processing (process())
    ↓
applyCustomOutputRouting()
    ├─→ Reads routingMatrix[paramIdx] (current value) ✓
    └─→ Uses paramOutputRouting map (stale data) ✗
```

## The Mismatch

### In `updateParameterRouting()`
```cpp
// This stores the bus index at the time of update
paramOutputRouting[i] = {
    .paramIndex = (int)i,
    .busIndex = routingMatrix[i] > 0 ? routingMatrix[i] - 1 : -1,  // STORED VALUE
    .isCV = (param.unit == kNT_unitCvOutput)
};
```

### In `applyCustomOutputRouting()`
```cpp
// Gets fresh value from matrix
int sourceBus = routingMatrix[paramIdx] - 1;  // CURRENT VALUE

// But uses stale routing info to find which output
for (const auto& [paramIdx, routing] : paramOutputRouting) {
    // routing.busIndex is STALE!
}
```

## Why This Design Exists

The routing maps were likely created to:
1. Avoid scanning all parameters every audio sample
2. Pre-compute which parameters control routing
3. Cache parameter metadata (isCV, hasOutputMode)

However, the bus index shouldn't be cached since it changes with the parameter.

## Solutions

### Quick Fix
Update routing maps immediately when routing parameters change:
```cpp
if (isRoutingParameter(param)) {
    updateParameterRouting();  // Add this line
}
```

### Better Fix
Don't cache the bus index, only cache parameter metadata:
```cpp
struct OutputRouting {
    int paramIndex;
    // Remove: int busIndex;  // Don't cache this!
    bool isCV;
    bool hasOutputMode;
    int outputModeParamIndex;
};
```

### Best Fix
Follow VCV best practices and read parameters directly in process() without intermediate maps.

## Impact

- **User Experience**: Confusing when changes don't apply immediately
- **Performance**: Negligible - routing parameters change infrequently
- **Correctness**: Current behavior is technically a bug
- **Compatibility**: Fix won't affect saved patches