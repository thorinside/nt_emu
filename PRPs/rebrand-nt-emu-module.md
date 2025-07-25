# PRP: Rebrand DistingNT Module to nt_emu

## Goal
Rebrand the DistingNT VCV Rack module to "nt_emu" with "No Such Device" branding, update the panel design with a dark blue color scheme, add two additional outputs (for 8 total), and update all metadata to reflect the new ownership and branding.

## Why
The current branding could cause confusion with the actual Disting NT hardware from Expert Sleepers. This rebranding establishes a clear identity as an emulation project while maintaining the functionality and adding the missing two outputs present on the hardware.

## What

### User-Visible Changes
1. **Module Name**: "nt_emu" displayed centered at top of panel
2. **Brand Name**: "No Such Device" at bottom of panel
3. **Panel Color**: Dark blue background (#1A365D suggested)
4. **Additional Outputs**: Add outputs 7-8 below existing 6 outputs
5. **Input Visual Cues**: White background strips behind input rows for clarity
6. **Typography**: Professional Eurorack-style fonts (Bebas Neue for headers, Roboto for labels)

### Technical Changes
1. Update plugin.json with new metadata
2. Create new panel SVG with updated design
3. Add OUTPUT_7 and OUTPUT_8 to module code
4. Update widget layout to accommodate new outputs
5. Maintain existing code structure and filenames

## All Needed Context

### Documentation References
- **VCV Panel Design Guide**: `PRPs/ai_docs/vcv-panel-design-guide.md`
- **VCV Panel Standards**: https://vcvrack.com/manual/Panel
- **SVG Text to Path**: Essential for font compatibility

### Current Implementation

#### Panel SVG Structure (res/panels/DistingNT.svg)
```xml
<!-- Current structure -->
<svg width="71.12mm" height="128.5mm" viewBox="0 0 71.12 128.5">
  <rect width="71.12" height="128.5" fill="#3A3A3F"/> <!-- Current gray -->
  <!-- Brand text at lines 39-40 and 124-125 -->
  <text x="35.56" y="26.0" fill="#00FFFF">expert sleepers</text>
  <text x="35.56" y="28.5" fill="#00FFFF">disting NT</text>
</svg>
```

#### Module Definition (src/DistingNT.cpp)
```cpp
enum OutputIds {
    AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3,
    AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
    NUM_OUTPUTS  // Currently 6
};

// Widget creation pattern
for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 2; col++) {
        int index = row * 2 + col;
        // Creates 6 outputs in 3x2 grid
    }
}
```

#### Plugin Metadata (plugin.json)
```json
{
  "slug": "DistingNT",
  "name": "Disting NT",
  "brand": "Expert Sleepers",
  "author": "Expert Sleepers Ltd",
  "authorEmail": "support@expert-sleepers.co.uk",
  "authorUrl": "https://www.expert-sleepers.co.uk/",
}
```

### Design Specifications

#### Color Palette
- **Background**: #1A365D (Dark Blue)
- **Text Primary**: #E2E8F0 (Light Gray)
- **Text Secondary**: #CBD5E0 (Medium Gray)
- **Input Strips**: #FFFFFF with 10% opacity
- **Jack Markers**: #606060 (existing)

#### Typography
- **Module Name**: Bebas Neue, 3.5mm height, centered
- **Brand Name**: Roboto Medium, 2.5mm height, centered
- **Labels**: Roboto Regular, 1.8mm height

#### Layout Adjustments
- Current output area: Y positions 70, 80, 90
- New outputs 7-8: Y position 100
- Bottom branding: Move from Y 110-115 to Y 118-123

### Gotchas and Patterns

1. **Text to Path Conversion**: All SVG text must be converted to paths
2. **Module Slug**: Keep as "DistingNT" to maintain patch compatibility
3. **Output Enumeration**: Must update NUM_OUTPUTS and all related loops
4. **Panel Height**: Fixed at 128.5mm, work within constraints
5. **Jack Spacing**: Maintain 9mm horizontal spacing for consistency

## Implementation Blueprint

### Phase 1: Update Module Code for 8 Outputs

```cpp
// In DistingNT class
enum OutputIds {
    AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3,
    AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
    AUDIO_OUTPUT_7, AUDIO_OUTPUT_8,  // Add new outputs
    NUM_OUTPUTS  // Now 8
};

// Update constructor
for (int i = 0; i < 8; i++) {  // Changed from 6
    configOutput(AUDIO_OUTPUT_1 + i, string::f("Output %d", i + 1));
}

// Update bus routing (if needed for hardware compatibility)
// Buses 20-27 can now map to outputs 0-7
```

### Phase 2: Update Widget Layout

```cpp
// In DistingNTWidget constructor
// Outputs now in 4 rows of 2
float outputStartX = 55.0;
for (int row = 0; row < 4; row++) {  // Changed from 3
    for (int col = 0; col < 2; col++) {
        int index = row * 2 + col;
        float x = outputStartX + col * jackSpacing;
        float y = inputStartY + row * rowSpacing;
        
        DistingNTOutputPort* outputPort = createOutputCentered<DistingNTOutputPort>(
            mm2px(Vec(x, y)), module, DistingNT::AUDIO_OUTPUT_1 + index);
        if (module) {
            outputPort->distingModule = module;
            outputPort->outputIndex = index;
        }
        addOutput(outputPort);
    }
}
```

### Phase 3: Create New Panel SVG

```xml
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg width="71.12mm" height="128.5mm" viewBox="0 0 71.12 128.5"
     version="1.1" xmlns="http://www.w3.org/2000/svg">
  
  <!-- Dark Blue Background -->
  <rect width="71.12" height="128.5" fill="#1A365D"/>
  
  <!-- Input Row Backgrounds (white strips) -->
  <rect x="5" y="68" width="40" height="5" fill="#FFFFFF" opacity="0.1" rx="0.5"/>
  <rect x="5" y="78" width="40" height="5" fill="#FFFFFF" opacity="0.1" rx="0.5"/>
  <rect x="5" y="88" width="40" height="5" fill="#FFFFFF" opacity="0.1" rx="0.5"/>
  
  <!-- Module Name (convert to path in Inkscape) -->
  <text x="35.56" y="12" text-anchor="middle" 
        font-family="Bebas Neue" font-size="3.5" fill="#E2E8F0">nt_emu</text>
  
  <!-- Brand Name at bottom (convert to path) -->
  <text x="35.56" y="122" text-anchor="middle" 
        font-family="Roboto" font-weight="500" font-size="2.5" fill="#CBD5E0">No Such Device</text>
  
  <!-- [Include all other elements with updated colors] -->
</svg>
```

### Phase 4: Update Plugin Metadata

```json
{
  "slug": "DistingNT",  // Keep for compatibility
  "name": "nt_emu",
  "version": "2.0.0",
  "license": "MIT",
  "brand": "No Such Device",
  "author": "Neal Sanche",
  "authorEmail": "thorinside@gmail.com",
  "authorUrl": "https://nosuch.dev/nt-emu",
  "pluginUrl": "https://github.com/thorinside/nt_emu",
  "manualUrl": "https://nosuch.dev/nt-emu",
  "sourceUrl": "https://github.com/thorinside/nt_emu",
  "modules": [
    {
      "slug": "DistingNT",
      "name": "nt_emu",
      "description": "Multi-algorithm DSP module emulator with plugin support",
      "tags": ["Effect", "Oscillator", "Utility", "Multiple"]
    }
  ]
}
```

### Phase 5: Update Makefile

```makefile
# Update SLUG if needed for distribution
SLUG = nt_emu  # Or keep as DistingNT for compatibility
VERSION = 2.0.0
```

## Validation Loop

### Level 1: Compilation
```bash
cd vcv-plugin
make clean && make
# Verify 8 outputs compile correctly
```

### Level 2: Visual Validation
```bash
# After creating SVG
inkscape res/panels/DistingNT.svg
# 1. Convert all text to paths (Path → Object to Path)
# 2. Save as Plain SVG
# 3. Verify in browser/preview
```

### Level 3: Module Testing
```bash
# In VCV Rack
# 1. Load module - verify new branding displays
# 2. Check all 8 outputs appear
# 3. Test signal routing to outputs 7-8
# 4. Verify input visual strips are visible
```

### Level 4: Metadata Validation
```bash
# Check plugin browser
# 1. Module shows as "nt_emu" 
# 2. Brand shows as "No Such Device"
# 3. Author info is updated
```

## Checklist

- [ ] Update OutputIds enum to include AUDIO_OUTPUT_7 and AUDIO_OUTPUT_8
- [ ] Change NUM_OUTPUTS from 6 to 8
- [ ] Update output configuration loop to handle 8 outputs
- [ ] Modify widget layout from 3x2 to 4x2 output grid
- [ ] Create new panel SVG with dark blue background (#1A365D)
- [ ] Add "nt_emu" text at top of panel
- [ ] Add "No Such Device" text at bottom of panel
- [ ] Add white background strips behind input rows
- [ ] Convert all text to paths in SVG
- [ ] Update plugin.json with new metadata
- [ ] Update output labels in SVG (add 7, 8)
- [ ] Test all 8 outputs work correctly
- [ ] Verify visual design in VCV Rack

## Success Metrics

1. Module displays "nt_emu" and "No Such Device" branding
2. Panel has professional dark blue appearance
3. All 8 outputs are functional
4. Input rows are visually distinct with white backgrounds
5. Plugin metadata shows correct author and URLs
6. No references to Expert Sleepers remain visible

**Confidence Score: 9/10**

The implementation is straightforward with clear patterns to follow. The only minor complexity is ensuring the SVG text is properly converted to paths, which is a standard Inkscape operation. All code changes follow existing patterns in the codebase.