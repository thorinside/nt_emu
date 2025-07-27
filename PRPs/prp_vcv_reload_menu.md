# TASK PRP: Add Reload Plugin Menu Item to VCV Rack

## Context

### Documentation
```yaml
context:
  existing_patterns:
    - file: vcv-plugin/src/DistingNT.cpp
      focus: appendContextMenu() method at line 1550
      description: Shows existing menu structure with Load/Unload options
    
    - file: emulator/src/core/plugin_loader.cpp
      focus: reload() method at line 190
      description: Reference implementation of plugin reload logic
  
  menu_patterns:
    - pattern: "createMenuItem(label, rightText, callback)"
      usage: Standard VCV Rack menu item creation
    
    - pattern: "module->loadPlugin() / module->unloadPlugin()"
      usage: Existing plugin management methods
  
  gotchas:
    - issue: "Plugin state must be preserved"
      fix: "Store current plugin path before unload"
    
    - issue: "UI must update after reload"
      fix: "Set displayDirty = true and show loading message"
    
    - issue: "Plugin specification might change"
      fix: "Re-discover specifications after reload"
```

## Task Breakdown

### TASK 1: Add reloadPlugin() method to DistingNT class
**File**: `vcv-plugin/src/DistingNT.cpp`

**OPERATION**: Add method after unloadPlugin() (around line 718)
```cpp
void reloadPlugin() {
    if (!isPluginLoaded() || pluginPath.empty()) {
        loadingMessage = "No plugin to reload";
        loadingMessageTimer = 1.5f;
        displayDirty = true;
        return;
    }
    
    // Store current state
    std::string currentPath = pluginPath;
    uint32_t currentSpecIndex = selectedSpecificationIndex;
    
    // Unload current plugin
    unloadPlugin();
    
    // Reload from same path
    if (loadPlugin(currentPath)) {
        // Try to restore specification if still valid
        if (currentSpecIndex < pluginSpecifications.size()) {
            selectedSpecificationIndex = currentSpecIndex;
            onSpecificationChanged();
        }
        loadingMessage = "Plugin reloaded";
    } else {
        loadingMessage = "Reload failed";
    }
    loadingMessageTimer = 1.5f;
    displayDirty = true;
}
```

**VALIDATE**: 
```bash
cd vcv-plugin && make
```

**IF_FAIL**: Check for syntax errors, missing semicolons, or undefined variables

**ROLLBACK**: Remove the added method

### TASK 2: Add Reload menu item to context menu
**File**: `vcv-plugin/src/DistingNT.cpp`

**OPERATION**: Modify appendContextMenu() method (around line 1574)
Find the section after "Unload Plugin" menu item and add:
```cpp
// After the Unload Plugin menu item (around line 1574)
menu->addChild(createMenuItem("Reload Plugin", 
    RACK_MOD_CTRL_NAME "+R", [=]() {
    module->reloadPlugin();
}));
```

**VALIDATE**:
```bash
cd vcv-plugin && make
```

**IF_FAIL**: Ensure the menu item is added within the correct scope and lambda capture is correct

**ROLLBACK**: Remove the added menu item

### TASK 3: Add keyboard shortcut support (optional enhancement)
**File**: `vcv-plugin/src/DistingNT.cpp`

**OPERATION**: Add to onAction() or key handling (if exists)
```cpp
// In the module's key handling section
if (e.action == GLFW_PRESS && e.mods & RACK_MOD_CTRL) {
    if (e.key == GLFW_KEY_R) {
        reloadPlugin();
        e.consume(this);
    }
}
```

**VALIDATE**: Test keyboard shortcut manually after build

**IF_FAIL**: Check if key handling is done differently in this module

**ROLLBACK**: Remove keyboard shortcut code

### TASK 4: Test the implementation
**OPERATION**: Build and test in VCV Rack

```bash
# Build the plugin
cd vcv-plugin
make clean
make

# Install to VCV Rack (adjust path as needed)
make install

# Test steps:
# 1. Launch VCV Rack
# 2. Add DistingNT module
# 3. Load a plugin via menu
# 4. Right-click → Plugin → Reload Plugin
# 5. Verify plugin reloads successfully
```

**VALIDATE**: 
- Plugin reloads without crashing
- Current specification is preserved if possible
- Loading message appears
- Plugin functionality works after reload

**IF_FAIL**: 
- Check VCV Rack log for errors
- Add debug logging to track reload process
- Verify plugin path is preserved correctly

## Implementation Checklist

- [ ] reloadPlugin() method added to DistingNT class
- [ ] Method handles edge cases (no plugin loaded)
- [ ] Reload menu item added to context menu
- [ ] Menu item appears only when plugin is loaded
- [ ] Optional: Keyboard shortcut (Ctrl+R) works
- [ ] Loading message displays during reload
- [ ] Plugin specification restored if still valid
- [ ] Build completes without errors
- [ ] Manual testing confirms reload works
- [ ] No memory leaks or crashes during reload

## Validation Commands

```bash
# Level 1: Syntax Check
cd vcv-plugin && make clean && make

# Level 2: Installation
make install

# Level 3: Runtime Test (manual)
# 1. Start VCV Rack
# 2. Load DistingNT module
# 3. Load a plugin
# 4. Test reload menu item
# 5. Verify plugin works after reload

# Level 4: Memory Check (optional)
# Run VCV Rack with memory checker
# Perform multiple reload operations
# Check for memory leaks
```

## Success Criteria

1. **Reload menu item appears** when a plugin is loaded
2. **Plugin reloads successfully** maintaining functionality
3. **UI updates correctly** with loading message
4. **No crashes or errors** during reload operation
5. **Specification preserved** when possible

## Notes

- The reload functionality mimics the emulator's hot reload but triggered manually
- Consider adding auto-reload in future (monitor file changes)
- Reload is useful for plugin developers iterating quickly
- The Ctrl+R shortcut follows standard reload conventions