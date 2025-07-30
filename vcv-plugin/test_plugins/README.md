# Test Plugins Directory

This directory contains all test plugins and utilities for the DistingNT VCV Plugin development.

## Building

To build all test plugins and executables:
```bash
make all
```

To build only plugins:
```bash
make plugins
```

To build only test executables:
```bash
make tests
```

To see available targets:
```bash
make list
```

## Test Plugins (.dylib files)

These are DistingNT plugins that can be loaded into the VCV plugin:

- **test_customui_plugin.dylib** - Custom UI testing plugin
- **drawtest_plugin.dylib** - Display drawing tests
- **fonttest_plugin.dylib** - Font rendering tests
- **fourteen.dylib** - Fourteen algorithm test
- **gain.dylib** - Simple gain algorithm test
- **scaling_demo.dylib** - UI scaling demonstration
- **simple_gain.dylib** - Basic gain plugin
- **test_plugin.dylib** - General test plugin

## Test Executables

These are standalone test programs:

- **test_api_comprehensive** - Comprehensive API testing
- **test_crash_repro** - Crash reproduction testing
- **test_distingnt** - Core DistingNT functionality tests
- **test_extract_params** - Parameter extraction testing
- **test_final_verification** - Final integration verification
- **test_fourteen_debug** - Fourteen algorithm debugging
- **test_fourteen_fixed** - Fixed version of fourteen tests
- **test_fourteen_vcv_style** - VCV-style fourteen implementation
- **test_integration** - Integration testing
- **test_structure_mismatch** - Structure mismatch debugging
- **test_symbol_resolution** - Symbol resolution testing

## Source Files

- **test_customui_loader.cpp** - Custom UI loader implementation
- **test_old_api_structure.cpp** - Legacy API structure tests
- **test_routing_sync.sh** - Routing synchronization script

## Usage

1. Build the desired test plugins/executables using the Makefile
2. For plugin tests (.dylib), load them through the VCV plugin's "Load Plugin" menu
3. For executable tests, run them directly from the command line
4. Check console output and VCV Rack logs for test results

## Development Notes

- All plugins use the DistingNT API defined in `../emulator/include/`
- Test executables can be used for unit testing without VCV Rack
- Plugin tests require the full VCV Rack environment
- Debug symbols are available in `.dSYM` directories when built with debug flags