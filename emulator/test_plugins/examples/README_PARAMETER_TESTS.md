# Parameter Extraction Test Suite

This directory contains comprehensive tests for extracting and validating plugin parameters, including their values, units, scaling factors, enum strings, and parameter page organization.

## Test Programs

### 1. `test_parameters` - Basic Parameter Analysis
Extracts and displays all parameter metadata for plugins:
- Parameter names, ranges, defaults
- Unit types (Audio Input/Output, CV, Percent, dB, Hz, etc.)
- Scaling factors (None, x10, x100, x1000)
- Enum string values
- Parameter page organization

**Usage:**
```bash
./test_parameters                  # Test all plugins
./test_parameters <plugin_name>    # Test specific plugin
```

### 2. `test_parameter_values` - Parameter Value Testing
Tests parameter values with proper scaling and formatting:
- Displays values with correct units and scaling applied
- Tests `parameterChanged()` callback functionality
- Validates enum string resolution
- Tests parameter ranges and boundary values

**Usage:**
```bash
./test_parameter_values                  # Test all plugins
./test_parameter_values <plugin_name>    # Test specific plugin
```

### 3. `scaling_demo.dylib` - Scaling Demonstration Plugin
Custom plugin demonstrating all parameter types and scaling factors:
- Basic values (None scaling)
- Percentages
- Voltage with x10 scaling (0.0-10.0V)
- Frequency with x100 scaling (10.00-20.00 Hz)
- Time with x1000 scaling (1.000-5.000 ms)
- Decibel values with x100 scaling (-6.00 to +6.00 dB)
- Musical units (MIDI notes, cents, semitones)
- Time units (ms, seconds, BPM)
- Enum parameter with string values

## Parameter Analysis Results

### Gain Plugin (`gain.dylib`)
- **GUID**: Exga
- **Parameters**: 4 total
  - Input (Audio Input): 1-28, default 1
  - Output (Audio Output): 1-28, default 13  
  - Output mode (Output Mode): 0-1, default 0
  - Gain (Percent): 0-100%, default 50%
- **Pages**: 2 pages - "Gain" page with gain parameter, "Routing" page with I/O parameters

### Monosynth Plugin (`monosynth.dylib`)
- **GUID**: Exms
- **Parameters**: 4 total
  - Output (Audio Output): 1-28, default 13
  - Output mode (Output Mode): 0-1, default 0
  - MIDI channel: 1-16, default 1
  - Waveform (Enum): Square/Sawtooth/Setup, default Square
- **Pages**: 2 pages with parameter organization
- **Features**: Enum parameter with string values

### Fourteen Plugin (`fourteen.dylib`)
- **GUID**: Th14 (14-bit CC to CV)
- **Parameters**: 6 total
  - CV Output: 1-28, default 13
  - CV Output mode: 0-1, default 0
  - CC: 0-127, default 1
  - MIDI Ch: 0-16, default 0
  - Bipolar: 0-1, default 0
  - Smoothing (Percent): 0-100%, default 0%
- **Pages**: 3 pages - "Setup", "CV", "Routing"

### Scaling Demo Plugin (`scaling_demo.dylib`)
- **GUID**: Scal
- **Parameters**: 14 total demonstrating all unit types and scaling factors
- **Features**: 
  - All scaling factors (None, x10, x100, x1000)
  - All major unit types (V, Hz, ms, dB, MIDI notes, etc.)
  - Enum parameter with multiple string options
  - Complex parameter page organization (6 pages)

## Key Discoveries

### Parameter Structure Access
All plugins successfully provide access to:
- `algorithm->parameters` - Array of parameter definitions
- `algorithm->parameterPages` - Parameter page organization
- `algorithm->v` - Current parameter values array

### Scaling Factor Implementation
The scaling factors work as divisors:
- `kNT_scaling10`: Divide raw value by 10 (e.g., 50 → 5.0V)
- `kNT_scaling100`: Divide raw value by 100 (e.g., 4400 → 44.00Hz)
- `kNT_scaling1000`: Divide raw value by 1000 (e.g., 2500 → 2.500ms)

### Enum String Handling
Enum parameters require:
- `unit = kNT_unitEnum`
- `enumStrings` pointing to null-terminated string array
- Raw parameter values as indices into the string array

### Parameter Page Organization
Parameter pages group related parameters:
- Each page has a name and list of parameter indices
- Pages enable organized navigation in UI
- Multiple organizational schemes possible (by function, by type, etc.)

### Unit Type Coverage
Successfully tested all major unit types:
- Audio/CV routing: `kNT_unitAudioInput`, `kNT_unitAudioOutput`, `kNT_unitCvInput`, `kNT_unitCvOutput`
- Musical: `kNT_unitMIDINote`, `kNT_unitSemitones`, `kNT_unitCents`, `kNT_unitBPM`
- Time: `kNT_unitMs`, `kNT_unitSeconds`, `kNT_unitFrames`
- Level: `kNT_unitDb`, `kNT_unitDb_minInf`, `kNT_unitPercent`
- Electrical: `kNT_unitVolts`, `kNT_unitMillivolts`
- Frequency: `kNT_unitHz`
- Special: `kNT_unitEnum`, `kNT_unitNone`, `kNT_unitOutputMode`

## Testing Status

✅ **All parameter extraction functionality verified:**
- Parameter metadata extraction
- Parameter value formatting with scaling
- Enum string resolution
- Parameter page navigation
- Parameter change callback testing
- Unit type display and formatting
- GUID display as four-character ASCII

✅ **No API drift detected** in any tested plugins

✅ **No crashes or memory corruption** during parameter access

The parameter system is fully functional and ready for integration with the VCV Rack module.