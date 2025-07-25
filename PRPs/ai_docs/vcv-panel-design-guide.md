# VCV Rack Panel Design Guide

This guide provides essential information for creating and modifying VCV Rack module panels.

## Panel Standards

### Dimensions
- **Height**: 128.5mm (standard 3U Eurorack)
- **Width**: Multiples of 5.08mm (1 HP)
  - 14HP module = 71.12mm width

### Color Schemes
- Dark panels are increasingly popular
- Use sufficient contrast for readability
- Consider ThemedSvgPanel for light/dark theme support

### Typography
**Popular Font Combinations:**
- **Headers**: Bebas Neue (display font, all-caps)
- **Body/Labels**: Roboto (clean, readable)
- **Technical**: Arial, Helvetica (widely available)

**Important**: All text must be converted to paths in SVG files

### Layout Best Practices
1. **Signal Flow**: Top-to-bottom or left-to-right
2. **Spacing**: Adequate room between controls
3. **Grouping**: Related controls together
4. **Input/Output Convention**:
   - Inputs: Left side or top
   - Outputs: Right side or bottom
   - Visual distinction (inverted backgrounds for outputs)

### Branding Guidelines
- Brand/Logo: Top or bottom of panel
- Module Name: Prominently displayed
- Version info: Smaller text at bottom
- Parameter labels: Close to controls

## SVG Technical Requirements

### Software
- **Recommended**: Inkscape (free, cross-platform)
- Alternative: Adobe Illustrator, Affinity Designer

### File Preparation
1. Convert all text to paths (Path → Object to Path)
2. Use simple linear gradients only
3. Group related elements
4. Keep file organized with layers

### Common Elements
```xml
<!-- Panel Background -->
<rect width="71.12" height="128.5" fill="#1A365D"/>

<!-- Mounting Screws -->
<circle cx="7.62" cy="3.0" r="1.6" fill="#C0C0C0"/>

<!-- Input/Output Markers -->
<circle cx="x" cy="y" r="0.4" fill="#606060" opacity="0.5"/>
```

## Color Palette Suggestions

### Dark Blue Theme
```
Background: #1A365D (Dark Blue)
Secondary: #2C5282 (Medium Blue)
Accent: #3182CE (Light Blue)
Text: #E2E8F0 (Light Gray)
Labels: #CBD5E0 (Medium Gray)
```

### Visual Hierarchy
1. **Primary Text**: High contrast (#FFFFFF or #E2E8F0)
2. **Secondary Text**: Medium contrast (#CBD5E0)
3. **Background Elements**: Low contrast (#2D3748)

## Font Conversion in Inkscape

1. Select all text elements
2. Path → Object to Path (Ctrl+Shift+C)
3. Save as Plain SVG (not Inkscape SVG)

## Testing Your Panel

1. Load in VCV Rack at various zoom levels
2. Check readability at 100% zoom
3. Verify all controls are accessible
4. Test with both light and dark room settings