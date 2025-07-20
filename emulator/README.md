# Disting NT Emulator

A simple macOS emulator for Expert Sleepers Disting NT plugins, enabling rapid plugin development without hardware.

## Features

- **Plugin Loading**: Load native macOS .dylib plugins compiled with the disting NT API
- **96kHz Audio Processing**: Real-time audio processing at 96kHz through PortAudio
- **256x64 Display**: Accurate simulation of the disting NT OLED display
- **Hardware Controls**: Full simulation of 3 pots, 4 buttons, and 2 encoders (all pressable)
- **Hot Reload**: Automatic plugin reloading when the binary changes
- **ImGui Interface**: Clean, responsive user interface

## Requirements

- macOS 10.15+
- CMake 3.16+
- Clang with C++17 support
- PortAudio
- GLFW
- OpenGL

## Installation

### Dependencies

Install dependencies using Homebrew:

```bash
brew install portaudio glfw cmake
```

### Building

```bash
cd emulator
mkdir build && cd build
cmake ..
make -j4
```

### Building Test Plugin

```bash
cd test_plugins/simple_gain
make
```

## Usage

### Running the Emulator

```bash
./DistingNTEmulator [plugin_path.dylib]
```

### Plugin Development Workflow

1. Develop your plugin using the disting NT API
2. Compile as a native macOS .dylib for testing:
   ```bash
   clang++ -std=c++17 -fPIC -shared -I/path/to/distingnt/include -o plugin.dylib plugin.cpp
   ```
3. Load in emulator for rapid testing
4. Cross-compile for Cortex-M7 when ready for hardware

### Interface

- **Main Window**: Plugin loading, audio control, status
- **Hardware Controls**: Simulate the physical disting NT interface
  - 3 potentiometers (top row, all pressable)
  - 4 buttons (2 left, 2 right of encoders)
  - 2 encoders (center, both pressable)
- **Display**: Real-time view of plugin's 256x64 display output

## API Implementation

The emulator implements the core disting NT API functions:

- Plugin lifecycle management
- Parameter control
- Drawing primitives (text, shapes)
- MIDI I/O (stub implementation)
- Audio processing at 96kHz in 4-sample blocks

## Testing

A simple gain plugin is included for validation:

```bash
# Build test plugin
cd test_plugins/simple_gain && make

# Run emulator with test plugin
../../build/DistingNTEmulator simple_gain.dylib
```

## Success Criteria

- ✅ Disting NT Plugins can be recompiled and run in the emulator
- ✅ The emulator can be connected to MIDI and Audio inputs and outputs
- ✅ The emulator displays the output of the plugin's Draw function
- ✅ The emulator simulates the I/O and Physical knob and buttons interface
- ✅ The emulator hot reloads a plugin that has changed on disk
- ⏳ Plugin validation for hardware compatibility (TODO)

## Architecture

```
emulator/
├── src/
│   ├── core/           # Core emulator functionality
│   ├── hardware/       # Hardware simulation
│   ├── ui/            # User interface
│   └── utils/         # Utilities
├── include/distingnt/ # disting NT API headers
└── test_plugins/      # Example plugins
```

## Known Limitations

- Basic font rendering (needs proper bitmap fonts)
- Simplified MIDI implementation
- No advanced hardware timing simulation
- Memory constraints not fully simulated

## Contributing

This is a minimal viable emulator focused on enabling plugin development. Contributions welcome for:

- Better font rendering
- Complete MIDI implementation
- Hardware timing accuracy
- Additional test plugins

## License

MIT License - see disting NT API for plugin licensing terms.