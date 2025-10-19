# Development Guide - NT_EMU Project

## Prerequisites

### System Requirements
- **Operating System**: macOS (primary), Linux, Windows
- **C++ Compiler**: C++11 compatible (clang, gcc)
- **Python**: 3.12+ (for PRP framework)
- **VCV Rack**: Version 2.x (for testing)
- **Build Tools**: make, git

### Development Environment Setup

1. **Clone the repository**:
   ```bash
   git clone https://github.com/thorinside/nt_emu.git
   cd nt_emu
   ```

2. **VCV Rack SDK**: Already included in `vcv-plugin/Rack-SDK/`

3. **Python environment** (for PRP framework):
   ```bash
   # Install uv if not present
   pip install uv

   # Install Python dependencies
   uv sync
   ```

## Building the Project

### VCV Plugin Build

The main build process for the VCV Rack plugin:

```bash
# Navigate to plugin directory
cd vcv-plugin

# Clean build
make clean

# Build the plugin
make

# Build and install to VCV Rack
make install
```

### Test Plugins Build

Build test plugins for development:

```bash
# Build all test plugins
make test-plugins

# Or build individually
make test_customui_plugin.dylib
make drawtest_plugin.dylib
make fonttest_plugin.dylib
```

### Installation

The plugin installs to the VCV Rack user directory:
- **macOS**: `~/Library/Application Support/Rack2/plugins/DistingNT/`
- **Linux**: `~/.Rack2/plugins/DistingNT/`
- **Windows**: `%APPDATA%/Rack2/plugins/DistingNT/`

## Running the Plugin

1. **Launch VCV Rack**:
   ```bash
   # macOS
   /Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d

   # With development mode for debugging
   /Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d
   ```

2. **Load the module**:
   - Right-click in Rack
   - Navigate to "No Such Device" > "NT Emulator"

## Testing

### Unit Tests

Run the JSON bridge test suite:

```bash
cd vcv-plugin
make test
```

Expected output: `16/16 tests pass`

### Integration Testing

Test plugin loading and functionality:

```bash
# Copy test plugin to Rack plugins folder
cp test_customui_plugin.dylib ~/.Rack2/plugins/DistingNT/

# Launch Rack and verify plugin loads
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d
```

## Development Workflow

### Code Organization

The project follows a modular architecture:

1. **Main Module** (`NtEmu.cpp`): Coordinates all subsystems
2. **Subsystems**: Each in its own directory under `src/`
   - `plugin/` - Plugin management
   - `parameter/` - Parameter routing
   - `menu/` - Menu navigation
   - `midi/` - MIDI processing
   - `display/` - Display rendering

### Making Changes

1. **Identify the subsystem**: Locate the relevant module directory
2. **Edit header/implementation**: Modify `.hpp` and `.cpp` files
3. **Rebuild**: Run `make` to compile changes
4. **Test**: Load in VCV Rack with `-d` flag for debugging

### Adding New Features

1. **Create new subsystem** (if needed):
   ```bash
   mkdir src/newsystem
   touch src/newsystem/NewSystem.hpp
   touch src/newsystem/NewSystem.cpp
   ```

2. **Implement interface**: Follow existing patterns (e.g., `IDisplayDataProvider`)

3. **Wire into main module**: Add to `NtEmu.cpp`

4. **Build and test**: Makefile automatically includes new subdirectories

## Using the PRP Framework

### Running PRPs

Execute Product Requirement Prompts:

```bash
# Interactive mode
uv run PRPs/scripts/prp_runner.py --prp [prp-name] --interactive

# Headless mode
uv run PRPs/scripts/prp_runner.py --prp [prp-name] --output-format json
```

### Claude Code Commands

Access development commands in Claude Code:
- `/prime-core` - Load project context
- `/review-staged-unstaged` - Review git changes
- `/prp-base-create` - Create new PRP
- `/prp-base-execute` - Execute existing PRP

## Common Development Tasks

### Clean Rebuild

```bash
cd vcv-plugin
make clean && make && make install
```

### Debug Output

Enable debug logging in the plugin:
```cpp
// In your code
#include <rack.hpp>
DEBUG("Parameter value: %f", value);
```

View logs:
```bash
# macOS
tail -f ~/Documents/Rack2/log.txt
```

### Static Analysis

Run static analysis on the codebase:
```bash
./scripts/run-static-analysis.sh
```

### Comment Block Removal

Clean up commented code blocks:
```bash
# Use the automated tool
python scripts/clean_comments.py src/
```

## Troubleshooting

### Build Failures

1. **Missing SDK**: Ensure `Rack-SDK` is present in `vcv-plugin/`
2. **Compiler errors**: Check C++11 compatibility
3. **Linking issues**: Verify all source files are in Makefile

### Plugin Not Loading

1. **Check installation path**: Verify plugin.dylib is in correct Rack2 folder
2. **Architecture mismatch**: Ensure plugin matches Rack architecture (x64/ARM)
3. **Dependencies**: Check all required libraries are linked

### Development Tips

1. **Use incremental builds**: Only `make` after changes (no clean)
2. **Enable debug mode**: Launch Rack with `-d` flag
3. **Monitor logs**: Keep `log.txt` open during development
4. **Test early**: Load plugin frequently to catch issues

## Contributing

### Code Style

- **C++ Standard**: C++11 (VCV Rack requirement)
- **Naming**:
  - Classes: `PascalCase`
  - Methods: `camelCase`
  - Members: `m_memberName`
- **Headers**: Include guards with `#pragma once`
- **Documentation**: Use Doxygen-style comments

### Pull Request Process

1. Create feature branch from `main`
2. Make changes following code style
3. Ensure `make clean && make` succeeds
4. Test in VCV Rack
5. Submit PR with description

## Resources

### Documentation
- [VCV Rack SDK Documentation](https://vcvrack.com/manual/PluginDevelopmentTutorial)
- [Project README](../README.md)
- [CLAUDE.md](../CLAUDE.md) - AI assistant instructions

### Support
- **GitHub Issues**: [https://github.com/thorinside/nt_emu/issues](https://github.com/thorinside/nt_emu/issues)
- **Author**: Neal Sanche (thorinside@gmail.com)

## Next Steps

1. Review the [Architecture Documentation](./architecture-vcv-plugin.md)
2. Explore the [Source Tree Analysis](./source-tree-analysis.md)
3. Check existing [PRPs](../PRPs/) for development patterns
4. Use Claude Code commands for AI-assisted development