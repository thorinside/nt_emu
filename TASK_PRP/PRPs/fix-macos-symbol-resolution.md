# TASK PRP: Fix macOS Dynamic Symbol Resolution for VCV Plugin

## Problem Summary
VCV Rack plugin fails to provide API symbols to dynamically loaded NT plugins on macOS, resulting in "symbol not found in flat namespace '_NT_screen'" errors.

## Root Cause
macOS uses two-level namespace by default. When VCV Rack loads DistingNT.vcvplugin, and then DistingNT loads gainCustomUI.dylib, the NT plugin cannot find symbols from its parent plugin due to namespace isolation.

## Solution Design
Implement a **function pointer table** approach that provides explicit API access without relying on global symbol resolution.

## Context

### Architecture Flow
```
VCV Rack -> DistingNT.vcvplugin -> gainCustomUI.dylib
                    |                      |
                    v                      v
              Exports API <------ Needs API symbols
```

### Current Issues
- Direct symbol export with `__attribute__((visibility("default")))` doesn't work
- `dlopen` with `RTLD_GLOBAL` doesn't expose parent plugin symbols
- NT plugins compiled with `-undefined dynamic_lookup` expect runtime resolution

### Documentation
- [Apple Dynamic Library Guidelines](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/DynamicLibraryDesignGuidelines.html)
- VCV Rack Plugin SDK: `vcv-plugin/Rack-SDK/include/`

### Patterns to Follow
- VCV Rack's Model/Module pattern for clean interfaces
- Existing EmulatorCore structure for state management
- Current plugin loading mechanism in `vcv-plugin/src/DistingNT.cpp:loadPlugin()`

### Gotchas
- Must maintain C ABI compatibility (`extern "C"`)
- Thread safety required (audio thread vs UI thread)
- Cannot break existing standalone emulator functionality
- Need to handle version compatibility for future API changes

## Task List

### Phase 1: Create API Interface Structure

**TASK 1: Define API function table structure**
```
CREATE vcv-plugin/src/nt_api_interface.h:
  - DEFINE: NT_API_VERSION constant (start with 1)
  - STRUCT: NT_API_Interface with function pointers for all API functions
  - STRUCT: Include void* for NT_screen buffer pointer
  - VALIDATE: Compile with make
  - IF_FAIL: Check for syntax errors in struct definition
  - ROLLBACK: Delete file
```

**TASK 2: Create API provider implementation**
```
CREATE vcv-plugin/src/nt_api_provider.cpp:
  - IMPLEMENT: getNT_API() function that returns populated API table
  - CONNECT: Link to existing API implementations in DistingNT.cpp
  - ADD: Thread-safe access to display buffer
  - VALIDATE: Compile and check symbols with nm
  - IF_FAIL: Check for undefined references
  - ROLLBACK: Revert to previous state
```

### Phase 2: Modify Plugin Loading

**TASK 3: Update plugin loader to pass API**
```
EDIT vcv-plugin/src/DistingNT.cpp loadPlugin():
  - BEFORE: dlsym for NT_getFactoryPtr
  - ADD: dlsym for optional "NT_setAPI" function
  - IF_FOUND: Call NT_setAPI with API interface pointer
  - MAINTAIN: Backward compatibility with existing plugins
  - VALIDATE: Load existing plugin without errors
  - IF_FAIL: Check dlsym error with dlerror()
  - ROLLBACK: Remove NT_setAPI handling
```

**TASK 4: Export API provider symbol**
```
EDIT vcv-plugin/src/DistingNT.cpp:
  - ADD: extern "C" __attribute__((visibility("default"))) getNT_API declaration
  - ENSURE: Symbol is in global namespace
  - VALIDATE: nm plugin.dylib | grep getNT_API shows T (global text)
  - IF_FAIL: Check visibility attributes
  - ROLLBACK: Remove export
```

### Phase 3: Update NT Plugin Interface

**TASK 5: Create plugin-side API receiver**
```
CREATE emulator/test_plugins/nt_api_receiver.h:
  - DECLARE: Global NT_API_Interface pointer
  - MACRO: Redirect NT_* calls to function pointer table
  - PROVIDE: Fallback to direct linking for standalone
  - VALIDATE: Include in test plugin, check compilation
  - IF_FAIL: Fix macro syntax
  - ROLLBACK: Use direct symbols
```

**TASK 6: Modify test plugin to use API**
```
EDIT emulator/test_plugins/examples/gainCustomUI.cpp:
  - ADD: Include nt_api_receiver.h
  - ADD: extern "C" void NT_setAPI(NT_API_Interface* api)
  - STORE: API pointer for use by drawing functions
  - VALIDATE: Compile plugin
  - IF_FAIL: Check for conflicting symbols
  - ROLLBACK: Remove API usage
```

### Phase 4: Handle Display Buffer Access

**TASK 7: Implement NT_screen access through API**
```
EDIT vcv-plugin/src/nt_api_provider.cpp:
  - MODIFY: getNT_API to include getScreenBuffer() function
  - RETURN: Pointer to current module's display buffer
  - ENSURE: Thread-safe access with proper synchronization
  - VALIDATE: Test pixel writing through API
  - IF_FAIL: Check pointer validity
  - ROLLBACK: Direct buffer access
```

**TASK 8: Update display synchronization**
```
EDIT vcv-plugin/src/DistingNT.cpp:
  - SYNC: NT_screen writes to VCVDisplayBuffer
  - SET: displayDirty flag on any write
  - VALIDATE: Display updates when plugin draws
  - IF_FAIL: Add debug logging to trace updates
  - ROLLBACK: Manual display refresh
```

### Phase 5: Testing and Validation

**TASK 9: Create comprehensive test**
```
CREATE vcv-plugin/test/test_symbol_resolution.cpp:
  - TEST: Load plugin successfully
  - TEST: API functions callable through interface
  - TEST: Display buffer updates visible
  - TEST: Multiple plugins don't conflict
  - VALIDATE: All tests pass
  - IF_FAIL: Debug with lldb
  - ROLLBACK: Identify failing component
```

**TASK 10: Test with actual VCV Rack**
```
TEST vcv-plugin in VCV Rack:
  - BUILD: make install
  - LOAD: DistingNT module in VCV Rack
  - SELECT: gainCustomUI algorithm
  - VERIFY: Display shows text and graphics
  - VERIFY: No console errors about symbols
  - VALIDATE: UI updates with parameter changes
  - IF_FAIL: Check Rack's log.txt
  - ROLLBACK: Previous working version
```

### Phase 6: Backward Compatibility

**TASK 11: Ensure standalone emulator still works**
```
TEST emulator/nt_emu:
  - BUILD: cd emulator && make
  - RUN: ./nt_emu test_plugins/examples/gainCustomUI.cpp
  - VERIFY: Direct symbol linking still works
  - VALIDATE: No regression in functionality
  - IF_FAIL: Conditional compilation for VCV vs standalone
  - ROLLBACK: Separate implementations
```

**TASK 12: Document API versioning**
```
CREATE vcv-plugin/docs/nt_api_versioning.md:
  - DOCUMENT: Current API version 1
  - EXPLAIN: How to check version compatibility
  - PROVIDE: Migration guide for future versions
  - VALIDATE: Markdown renders correctly
  - IF_FAIL: Fix formatting
  - ROLLBACK: Remove documentation
```

## Validation Commands

### Build Validation
```bash
# VCV Plugin
cd vcv-plugin && make clean && make

# Check symbols
nm plugin.dylib | grep -E "(getNT_API|NT_)"

# Standalone emulator
cd emulator && make clean && make
```

### Runtime Validation
```bash
# Test in VCV Rack
cd vcv-plugin && make install
# Launch VCV Rack, add DistingNT module, load gainCustomUI

# Test standalone
cd emulator
./nt_emu test_plugins/examples/gainCustomUI.cpp
```

### Symbol Verification
```bash
# Verify API export
nm vcv-plugin/plugin.dylib | grep getNT_API
# Should show: T _getNT_API

# Verify plugin can find it
otool -L test_plugins/examples/gainCustomUI.dylib
# Should not show unresolved symbols
```

## Success Criteria

1. ✓ gainCustomUI.dylib loads without "symbol not found" errors
2. ✓ Display shows text "Turn centre pot for gain"
3. ✓ Direct pixel manipulation works (NT_screen access)
4. ✓ All drawing functions (NT_drawText, NT_drawShapeI, etc.) work
5. ✓ Standalone emulator continues to work unchanged
6. ✓ No performance regression
7. ✓ Thread-safe operation in VCV Rack

## Risk Mitigation

- **Risk**: Breaking existing plugins
  - **Mitigation**: Maintain backward compatibility with optional API
  
- **Risk**: Thread safety issues
  - **Mitigation**: Use atomic operations for shared state
  
- **Risk**: Version incompatibility
  - **Mitigation**: Include version check in API structure

## Notes

- This approach is recommended by Apple for plugin architectures
- Function pointer tables avoid namespace pollution
- Provides clear API boundary and versioning
- Works with macOS code signing and notarization
- Extensible for future API additions