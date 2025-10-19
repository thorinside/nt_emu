# NT Emulator (nt_emu)

A VCV Rack module that emulates the Expert Sleepers Disting NT hardware, enabling desktop development and testing of NT algorithm plugins without requiring the physical device.

## What is This?

The NT Emulator provides a faithful software implementation of the Disting NT hardware API inside VCV Rack. This allows developers to:

- Write and debug NT algorithm plugins on their desktop
- Test plugins in a full modular synthesis environment
- Iterate rapidly without hardware deployment
- Use standard C++ debugging tools

For musicians, this module brings the power of custom Disting NT algorithms into the VCV Rack ecosystem.

## Quick Start

### Prerequisites

- **VCV Rack 2.0+** - Download from [vcvrack.com](https://vcvrack.com/)
- **VCV Rack SDK 2.0** - Required for building
- **C++ Compiler** - Xcode (macOS), GCC/Clang (Linux), or MSVC (Windows)
- **GNU Make** - Build system

### Building the Module

```bash
# Clone the repository
git clone https://github.com/thorinside/nt_emu.git
cd nt_emu/vcv-plugin

# Build the module
make clean && make

# Install to your VCV Rack plugins directory
make install
```

The `make install` command copies the built plugin to:
- **macOS**: `~/Library/Application Support/Rack2/plugins/DistingNT/`
- **Linux**: `~/.Rack2/plugins/DistingNT/`
- **Windows**: `%USERPROFILE%/Documents/Rack2/plugins/DistingNT/`

### Running in VCV Rack

1. **Launch VCV Rack**:
   ```bash
   # macOS
   /Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack

   # Or with debug output
   /Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d
   ```

2. **Add the module**:
   - Right-click in VCV Rack
   - Search for "Disting NT"
   - Select "NT Emulator"

3. **Load a plugin**:
   - Click the display or use the encoder
   - Navigate to "Load Plugin"
   - Select your `.dylib` (macOS), `.so` (Linux), or `.dll` (Windows) file

### Building Test Plugins

```bash
cd vcv-plugin

# Build example plugins
make test-plugins

# This creates:
# - test_customui_plugin.dylib
# - drawtest_plugin.dylib
# - fonttest_plugin.dylib
```

Load these test plugins through the NT Emulator module menu to verify your setup.

## Module Interface

### Controls

- **Rotary Encoder** - Navigate menus and adjust parameters
- **4 Potentiometers (Z, X, Y, I)** - Parameter controls with CV inputs
- **4 Buttons** - Function buttons with CV triggers

### Inputs/Outputs

- **4 Audio Inputs** - Stereo pairs (L1/R1, L2/R2)
- **4 Audio Outputs** - Stereo pairs (L1/R1, L2/R2)
- **8 CV Inputs** - For modulating pots and buttons
- **MIDI In/Out** - Full MIDI support

### Display

- **128x64 OLED Emulation** - Real-time rendering of plugin graphics
- Shows menu system, parameter values, and custom plugin UIs

## Module Features

### Plugin Management
- **Hot Loading** - Load/unload plugins without restarting VCV Rack
- **Exception Safety** - Plugin crashes don't crash VCV Rack
- **Multiple Plugins** - Switch between different algorithms on the fly

### Parameter System
- **16x16 Routing Matrix** - Flexible parameter to control mapping
- **CV Modulation** - All parameters can be CV-controlled
- **Preset Management** - Save/load plugin states via VCV Rack

### MIDI Integration
- **MIDI Input** - Route MIDI from any device to your plugin
- **MIDI Output** - Plugins can generate MIDI
- **Activity Indicators** - Visual feedback for MIDI traffic

### Audio Processing
- **4-Bus System** - Matches Disting NT hardware topology
- **Real-time Processing** - Low-latency audio with VCV Rack integration
- **Sample-accurate** - Precise timing for all operations

## Developing NT Plugins

### Plugin Structure

NT plugins are shared libraries that implement the NT API:

```cpp
#include "nt_api.h"

extern "C" {
    // Required plugin interface
    void init(_NT_algorithm* algorithm) { /* ... */ }
    void step(_NT_algorithm* algorithm, float* buses, int numFramesBy4) { /* ... */ }
    void customUi(_NT_algorithm* algorithm, const _NT_uiData& uiData) { /* ... */ }
    bool draw(_NT_algorithm* algorithm) { /* ... */ }
}
```

### Building Your Plugin

```bash
# Example build command (macOS)
clang++ -std=c++11 -shared -o my_plugin.dylib my_plugin.cpp \
    -I../emulator/include

# Linux
g++ -std=c++11 -shared -fPIC -o my_plugin.so my_plugin.cpp \
    -I../emulator/include

# Windows
cl /LD /std:c++11 my_plugin.cpp /I..\emulator\include
```

### API Reference

The NT API provides:

- **Parameter Management** - Define and control plugin parameters
- **Audio Processing** - Access to 4-bus audio system
- **Display System** - Graphics drawing primitives (pixels, text, shapes)
- **MIDI Handling** - Send and receive MIDI messages
- **Control Input** - Read encoder, buttons, and pots
- **Menu Integration** - Custom menu items and settings

See `emulator/include/nt_api.h` for the complete API documentation.

### Example Plugin

Check out `vcv-plugin/test_plugins/test_customui_plugin.cpp` for a complete working example demonstrating:

- Parameter definition and mapping
- Audio I/O
- Custom UI drawing
- Button and encoder handling
- Menu system integration

## Debugging Plugins

### Using LLDB (macOS/Linux)

```bash
# Start VCV Rack under debugger
lldb /Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack

# Set breakpoints in your plugin
(lldb) breakpoint set --file my_plugin.cpp --line 42

# Run
(lldb) run -d
```

### Debug Output

The emulator provides extensive logging:

```bash
# Enable debug output
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

# Shows:
# - Plugin load/unload events
# - Parameter changes
# - MIDI activity
# - Error conditions
```

### Common Issues

**Plugin won't load:**
- Check that it exports the required C functions (`init`, `step`, etc.)
- Verify C++11 compatibility
- Ensure no unresolved symbols

**Audio glitches:**
- Verify `step()` function processes correct number of samples
- Check for buffer overruns
- Ensure thread-safety in audio callback

**Display not updating:**
- `draw()` must return `true` when content changes
- Check that graphics calls are within bounds (0-127, 0-63)

## Project Structure

```
nt_emu/
├── vcv-plugin/              # VCV Rack module source
│   ├── src/                 # Module implementation
│   │   ├── NtEmu.cpp        # Main module
│   │   ├── plugin/          # Plugin management system
│   │   ├── parameter/       # Parameter routing system
│   │   ├── menu/            # Menu navigation system
│   │   ├── midi/            # MIDI processing
│   │   ├── display/         # Display rendering
│   │   └── dsp/             # Audio processing
│   ├── test_plugins/        # Example plugins
│   └── Makefile            # Build system
├── emulator/               # NT API emulation core
│   ├── include/            # API headers
│   └── src/                # Core implementation
└── docs/                   # Documentation
```

## Testing

### Unit Tests

```bash
cd vcv-plugin
make test

# Runs JSON serialization tests
# Output: 16/16 tests passing
```

### Integration Tests

```bash
# Build and install
make clean && make && make install

# Launch VCV Rack
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

# Load a test plugin and verify:
# - Audio passes through
# - Display updates
# - Parameters respond to controls
# - MIDI functions correctly
```

## Architecture

The module uses a **modular component architecture**:

- **Plugin Manager** (465 lines) - Safe plugin loading with exception handling
- **Parameter System** (450 lines) - Flexible 16x16 routing matrix
- **Menu System** (380 lines) - State machine for navigation
- **MIDI Processor** (220 lines) - MIDI I/O with activity tracking
- **Display Renderer** (490 lines) - OLED graphics emulation

Each component is independently testable and uses dependency injection for clean separation of concerns.

## Documentation

- **[Project Overview](docs/project-overview.md)** - High-level project structure
- **[VCV Module Architecture](docs/architecture-vcv-plugin.md)** - Module design details
- **[Emulator Architecture](docs/architecture-emulator.md)** - API emulation layer
- **[Development Guide](docs/development-guide.md)** - Setup and workflow
- **[Source Tree Analysis](docs/source-tree-analysis.md)** - Complete codebase map

## Requirements

### Runtime
- VCV Rack 2.0 or later
- macOS 10.13+, Linux (Ubuntu 20.04+), or Windows 10+

### Development
- C++11 compatible compiler
- VCV Rack SDK 2.0
- GNU Make or compatible build system
- Python 3.12+ (for PRP tooling, optional)

## Contributing

This project demonstrates AI-assisted development using the PRP (Product Requirement Prompt) methodology. See the `docs/` directory for extensive documentation about the development process.

### Development Workflow

1. Review existing architecture documentation
2. Create or modify PRPs in `docs/stories/`
3. Use Claude Code commands from `.claude/commands/`
4. Follow the modular architecture patterns
5. Ensure tests pass before committing

## License

MIT License - See LICENSE file for details

## Author

Neal Sanche (No Such Device)
- Website: [nosuch.dev](https://nosuch.dev)
- Email: thorinside@gmail.com
- GitHub: [@thorinside](https://github.com/thorinside)

## Acknowledgments

- Expert Sleepers for the Disting NT hardware and API
- VCV Rack community for the excellent SDK
- PRP methodology pioneers for AI-assisted development patterns

## Support

- **Issues**: [GitHub Issues](https://github.com/thorinside/nt_emu/issues)
- **Discussions**: Use GitHub Discussions for questions
- **Documentation**: Check `docs/` for detailed guides

---

**Ready to start developing?** Build the module, load a test plugin, and start experimenting. The NT API gives you complete control over audio, MIDI, and graphics in the VCV Rack environment.
