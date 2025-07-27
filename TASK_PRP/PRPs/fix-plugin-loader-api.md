# TASK PRP: Fix Plugin Loader API Mismatch

## Goal
Fix plugin loader crash by updating it to use the correct pluginEntry API instead of the non-existent NT_getFactoryPtr.

## Context

### Root Cause
- Plugin loader expects `NT_getFactoryPtr` symbol (doesn't exist in API)
- Plugins actually export `pluginEntry` with selector-based access
- Null factory pointer causes crash at offset 0
- API version constant mismatch (`kNT_apiVersion` vs `kNT_apiVersionCurrent`)

### API Pattern
```cpp
// Correct plugin API from api.h
uintptr_t pluginEntry(_NT_selector selector, uint32_t data);
// Selectors: kNT_selector_version, kNT_selector_numFactories, kNT_selector_factoryInfo
```

## Tasks

### 1. Update Symbol Loading
**ACTION** emulator/src/core/plugin_loader.cpp:31
**OPERATION**: Change symbol from NT_getFactoryPtr to pluginEntry:
```cpp
// Old: dlsym(handle, "NT_getFactoryPtr");
auto pluginEntry = (uintptr_t (*)(_NT_selector, uint32_t))dlsym(handle, "pluginEntry");
if (!pluginEntry) {
    fprintf(stderr, "Failed to find pluginEntry symbol\n");
    dlclose(handle);
    return nullptr;
}
```
**VALIDATE**: Symbol resolves without error
**IF_FAIL**: Check extern "C" linkage in plugins

### 2. Implement Selector-Based Access
**ACTION** emulator/src/core/plugin_loader.cpp:38-55
**OPERATION**: Replace getFactory() logic with selector API:
```cpp
// Check API version
uintptr_t version = pluginEntry(kNT_selector_version, 0);
if (version != kNT_apiVersionCurrent) {
    fprintf(stderr, "API version mismatch: %lu vs %d\n", version, kNT_apiVersionCurrent);
    dlclose(handle);
    return nullptr;
}

// Get number of factories (should be 1)
uintptr_t numFactories = pluginEntry(kNT_selector_numFactories, 0);
if (numFactories < 1) {
    fprintf(stderr, "No factories in plugin\n");
    dlclose(handle);
    return nullptr;
}

// Get factory pointer for index 0
const _NT_factory* factory = (const _NT_factory*)pluginEntry(kNT_selector_factoryInfo, 0);
if (!factory) {
    fprintf(stderr, "Failed to get factory\n");
    dlclose(handle);
    return nullptr;
}
```
**VALIDATE**: Factory pointer is non-null
**IF_FAIL**: Add debug prints for each selector call

### 3. Fix API Version Constant
**ACTION** emulator/src/core/plugin_loader.cpp:53
**OPERATION**: Change kNT_apiVersion to kNT_apiVersionCurrent
**VALIDATE**: Compiles without undefined symbol errors

### 4. Add Safety Checks
**ACTION** emulator/src/core/plugin_loader.cpp (throughout)
**OPERATION**: Add null checks before dereferencing:
```cpp
if (factory && factory->calculateRequirements) {
    factory->calculateRequirements(req, parameters);
} else {
    fprintf(stderr, "calculateRequirements is null\n");
}
```
**VALIDATE**: No crashes on null function pointers

### 5. Test Loading
**ACTION** Test with multiple plugins:
1. `make clean && make`
2. Test gainMultichannel.dylib loading
3. Test other example plugins
4. Verify no crashes at offset 0

**VALIDATE**: All plugins load without crash
**IF_FAIL**: Check dlopen error with dlerror()

## Validation Checklist
- [ ] pluginEntry symbol resolves correctly
- [ ] API version matches kNT_apiVersionCurrent
- [ ] Factory pointer obtained via selectors
- [ ] No null pointer dereferences
- [ ] gainMultichannel loads successfully
- [ ] Other plugins continue to work