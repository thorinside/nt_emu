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

# Fetch the VCV Rack SDK for your platform
./fetch-sdk.sh

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

- **2 Rotary Encoders (L, R)** - Navigate menus and adjust parameters (continuous rotation)
- **3 Potentiometers (L, C, R)** - Parameter controls with CV inputs (0.0-1.0)
- **4 Buttons** - Function buttons with CV triggers
- **Button Presses** - Encoders and pots are also pressable

### Inputs/Outputs

- **12 Inputs** - Full NT hardware complement (can carry audio or CV)
- **8 Outputs** - Full NT hardware complement (can carry audio or CV)
- **MIDI In/Out** - Full bidirectional MIDI support
- **28-Bus Internal Routing** - Flexible signal routing between inputs, outputs, and algorithms

### Display

- **256x64 Pixel OLED Emulation** - Real-time rendering of plugin graphics
- 128x64 byte framebuffer with 4-bit grayscale (2 pixels per byte)
- Shows menu system, parameter values, and custom plugin UIs

## Module Features

### Plugin Management
- **Single Algorithm** - One algorithm plugin runs at a time
- **Hot Loading** - Load/unload plugins without restarting VCV Rack
- **Exception Safety** - Plugin crashes don't crash VCV Rack
- **Plugin Switching** - Load different algorithm plugins via menu navigation

### Parameter System
- **Algorithm Parameters** - Each algorithm defines its own parameter count
- **3 Physical Pots** - Control algorithm parameters via menu navigation
- **CV Modulation** - All pots can be CV-controlled
- **Preset Management** - Save/load plugin states via VCV Rack

### MIDI Integration
- **MIDI Input** - Route MIDI from any device to your plugin
- **MIDI Output** - Plugins can generate MIDI
- **Activity Indicators** - Visual feedback for MIDI traffic

### Audio Processing
- **28-Bus System** - Matches Disting NT hardware topology (12 inputs, 8 outputs, 8 auxiliary)
- **Flexible Routing** - Any bus can carry audio, CV, or modulation signals
- **Real-time Processing** - Low-latency audio with VCV Rack integration
- **4-Sample Block Processing** - Hardware-accurate processing at variable sample rates (44.1-96kHz)
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

- **Parameter Management** - Define and control algorithm parameters (variable count per algorithm)
- **Audio Processing** - Access to 28-bus audio system with flexible routing
- **Display System** - 256x64 pixel graphics drawing primitives (pixels, text, shapes)
- **MIDI Handling** - Send and receive MIDI messages
- **Control Input** - Read 2 encoders, 3 pots, and 9 buttons (4 discrete + 3 pot-press + 2 encoder-press)
- **Menu Integration** - Custom menu items and settings

See `emulator/include/distingnt/api.h` for the complete API documentation.

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
- Check that graphics calls are within bounds (x: 0-255, y: 0-63)

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

### Architecture & Specifications
- **[Solution Architecture](docs/solution-architecture.md)** - Complete system architecture
- **[Hardware Controls Spec](docs/HARDWARE-CONTROLS-SPEC.md)** - All physical controls reference
- **[Bus Architecture](docs/bus-architecture-correction.md)** - 28-bus routing explained
- **[Architecture Decisions](docs/architecture-decisions.md)** - Key technical decisions

### Development Guides
- **[Project Overview](docs/project-overview.md)** - High-level project structure
- **[Development Guide](docs/development-guide.md)** - Setup and workflow
- **[PRD](docs/prd.md)** - Product requirements
- **[Epics](docs/epics.md)** - Implementation roadmap

### Reference
- **[Documentation Corrections](docs/DOCUMENTATION-CORRECTIONS-2025-10-19.md)** - Verified specifications
- **[VCV Module Architecture](docs/architecture-vcv-plugin.md)** - Module design details
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
