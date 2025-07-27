# TASK PRP: Fix gainMultichannel Plugin Crash

## Goal
Fix null pointer crash in gainMultichannel.dylib by updating factory structure and export symbols to match working plugin patterns.

## Context

### Crash Details
- SIGSEGV at 0x0000000000000000 in calculateRequirements
- Plugin loader expects `NT_getFactoryPtr` symbol but plugin only exports `pluginEntry`
- Factory structure missing required fields causing uninitialized function pointers

### Reference Patterns
- emulator/plugins/gain.cpp:101-123 - Complete factory structure
- emulator/plugins/simple_gain.cpp:211-215 - Dual export pattern

## Tasks

### 1. Add Missing Factory Fields
**ACTION** emulator/plugins/gainMultichannel.cpp:147-159
**OPERATION**: Update factory structure to include all fields:
```cpp
static const _NT_factory factory = 
{
    .guid = NT_MULTICHAR( 'E', 'x', 'g', 'm' ),
    .name = "Gain (multi-channel)",
    .description = "Applies gain",
    .numSpecifications = ARRAY_SIZE(specifications),
    .specifications = specifications,
    .calculateStaticRequirements = nullptr,
    .initialise = nullptr,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = NULL,
    .midiRealtime = nullptr,
    .midiMessage = nullptr,
    .tags = kNT_tagUtility,
    .hasCustomUi = nullptr,
    .customUi = nullptr,
    .setupUi = nullptr,
    .serialise = nullptr,
    .deserialise = nullptr,
};
```
**VALIDATE**: `make` compiles without warnings
**IF_FAIL**: Check _NT_factory definition for field order

### 2. Add Backward Compatibility Export
**ACTION** emulator/plugins/gainMultichannel.cpp:161 (after pluginEntry)
**OPERATION**: Add NT_getFactoryPtr for plugin loader compatibility:
```cpp
extern "C" const _NT_factory* NT_getFactoryPtr()
{
    return &factory;
}
```
**VALIDATE**: `nm -g gainMultichannel.dylib | grep NT_getFactoryPtr`
**IF_FAIL**: Ensure extern "C" linkage is correct

### 3. Test Plugin Loading
**ACTION** Test in VCV Rack:
1. Rebuild: `make clean && make`
2. Load VCV Rack 2 Pro
3. Right-click Disting NT module
4. Select "Load Plugin" → gainMultichannel.dylib
5. Verify no crash and plugin loads

**VALIDATE**: Plugin loads without crash
**IF_FAIL**: Check console logs for symbol resolution errors

## Validation Checklist
- [ ] Factory structure has all 19 fields
- [ ] Both pluginEntry and NT_getFactoryPtr exported
- [ ] No compiler warnings about missing initializers
- [ ] Plugin loads in VCV Rack without crash
- [ ] calculateRequirements executes without SIGSEGV