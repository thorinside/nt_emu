name: "VCV Rack Panel Validation MCP Server"
description: |
  MCP server for automated visual validation of VCV Rack module panels against reference designs

---

# VCV Rack Panel Validation MCP

## Purpose

Create an MCP server that provides tools for automated visual validation of VCV Rack module panels, enabling pixel-perfect comparison with reference hardware images and coordinate extraction from running modules.

## Core Features

### 1. Screenshot Capture
```typescript
interface ScreenshotTool {
  name: "vcv_screenshot";
  description: "Capture screenshot of VCV Rack module";
  parameters: {
    module_name: string;  // "DistingNT"
    window_title?: string;  // VCV Rack window
    crop_to_module?: boolean;  // Auto-crop to module bounds
  };
  returns: {
    image_path: string;
    dimensions: { width: number; height: number };
    module_bounds?: { x: number; y: number; width: number; height: number };
  };
}
```

### 2. Visual Comparison
```typescript
interface CompareToolsGroup {
  // Pixel-by-pixel comparison
  "vcv_compare_images": {
    parameters: {
      reference_image: string;  // Path to reference hardware image
      actual_image: string;     // Path to VCV screenshot
      threshold?: number;       // Similarity threshold (0-1)
      highlight_differences?: boolean;
    };
    returns: {
      similarity: number;  // 0-1 score
      difference_image?: string;  // Path to diff visualization
      mismatch_regions: Array<{x: number; y: number; w: number; h: number}>;
    };
  };

  // Component detection and validation
  "vcv_detect_components": {
    parameters: {
      image_path: string;
      component_type: "knob" | "jack" | "button" | "display" | "all";
    };
    returns: {
      components: Array<{
        type: string;
        center: { x: number; y: number };
        radius?: number;
        confidence: number;
      }>;
    };
  };
}
```

### 3. Coordinate Extraction
```typescript
interface CoordinateExtractor {
  name: "vcv_extract_layout";
  description: "Extract component coordinates from running VCV module";
  parameters: {
    module_name: string;
    extract_from: "screenshot" | "svg" | "source";
  };
  returns: {
    panel_size: { width_hp: number; width_mm: number; height_mm: number };
    components: {
      inputs: Array<{ id: string; x: number; y: number }>;
      outputs: Array<{ id: string; x: number; y: number }>;
      params: Array<{ id: string; x: number; y: number; type: string }>;
      displays: Array<{ id: string; x: number; y: number; w: number; h: number }>;
    };
  };
}
```

### 4. Validation Runner
```typescript
interface ValidationTool {
  name: "vcv_validate_panel";
  description: "Run complete panel validation against specification";
  parameters: {
    module_name: string;
    spec_file: string;  // Path to expected coordinates YAML/JSON
    tolerance_mm?: number;  // Position tolerance in mm
  };
  returns: {
    passed: boolean;
    score: number;  // Overall accuracy score
    errors: Array<{
      component: string;
      expected: { x: number; y: number };
      actual: { x: number; y: number };
      delta: { x: number; y: number };
      severity: "error" | "warning";
    }>;
    report: string;  // Formatted validation report
  };
}
```

### 5. Interactive Adjustment Helper
```typescript
interface AdjustmentHelper {
  name: "vcv_suggest_adjustments";
  description: "Suggest code changes to fix layout issues";
  parameters: {
    validation_errors: Array<any>;  // From vcv_validate_panel
    source_file: string;  // Path to DistingNT.cpp
  };
  returns: {
    suggestions: Array<{
      component: string;
      current_code: string;
      suggested_code: string;
      explanation: string;
    }>;
  };
}
```

## Implementation Blueprint

### MCP Server Structure
```
vcv-validation-mcp/
├── package.json
├── src/
│   ├── index.ts              # MCP server entry
│   ├── tools/
│   │   ├── screenshot.ts     # Screen capture using robotjs/puppeteer
│   │   ├── comparison.ts     # Image comparison using pixelmatch
│   │   ├── detection.ts      # OpenCV component detection
│   │   ├── extraction.ts     # Coordinate extraction
│   │   └── validation.ts     # Validation logic
│   ├── utils/
│   │   ├── vcv-helper.ts     # VCV Rack specific utilities
│   │   └── image-utils.ts    # Image processing helpers
│   └── types/
│       └── vcv-types.ts      # Type definitions
└── test/
    └── fixtures/             # Test images and specs
```

### Key Dependencies
```json
{
  "dependencies": {
    "@modelcontextprotocol/sdk": "latest",
    "sharp": "^0.33.0",          // Image processing
    "pixelmatch": "^5.3.0",      // Pixel comparison
    "opencv4nodejs": "^5.6.0",   // Component detection
    "robotjs": "^0.6.0",         // Screenshot capture
    "js-yaml": "^4.1.0"          // YAML parsing
  }
}
```

### Usage Example
```typescript
// In Claude Code conversation
const result = await mcp.vcv_validate_panel({
  module_name: "DistingNT",
  spec_file: "panel_coordinates.yaml",
  tolerance_mm: 0.5
});

if (!result.passed) {
  console.log(`Panel validation failed with score: ${result.score}`);
  console.log("\nErrors:");
  result.errors.forEach(err => {
    console.log(`- ${err.component}: Expected (${err.expected.x}, ${err.expected.y}), ` +
                `Got (${err.actual.x}, ${err.actual.y}), ` +
                `Delta (${err.delta.x.toFixed(1)}, ${err.delta.y.toFixed(1)})`);
  });

  // Get fix suggestions
  const fixes = await mcp.vcv_suggest_adjustments({
    validation_errors: result.errors,
    source_file: "src/DistingNT.cpp"
  });

  console.log("\nSuggested fixes:");
  fixes.suggestions.forEach(fix => {
    console.log(`\n${fix.component}:`);
    console.log(`Current: ${fix.current_code}`);
    console.log(`Suggested: ${fix.suggested_code}`);
  });
}
```

## Integration with Task PRP

Add to the vcv-panel-update-task.md validation section:

```yaml
### MCP-Powered Validation

```
CHECKPOINT mcp_validation:
  - ENSURE: VCV Rack is running with DistingNT loaded
  - RUN: mcp.vcv_screenshot({ module_name: "DistingNT" })
  - RUN: mcp.vcv_validate_panel({ 
      module_name: "DistingNT",
      spec_file: "panel_coordinates.yaml" 
    })
  - IF_FAIL: Apply suggested adjustments from mcp.vcv_suggest_adjustments()
  - ITERATE: Until validation score > 0.95
```

## Benefits

1. **Automated Visual QA**: No manual screenshot comparison needed
2. **Precise Measurements**: Pixel-accurate coordinate extraction
3. **Rapid Iteration**: Immediate feedback on layout changes
4. **Code Generation**: Suggests exact coordinate fixes
5. **CI/CD Integration**: Can run in automated pipelines
6. **Cross-Platform**: Works on all platforms VCV Rack supports

## Advanced Features (Future)

- Real-time preview with overlay showing expected positions
- A/B testing different layouts
- Accessibility validation (contrast, sizing)
- Performance impact analysis of panel complexity
- Export validation reports as PDF/HTML

This MCP would dramatically speed up the panel layout process and ensure pixel-perfect accuracy.