# NT_EMU Project Overview

## Project Identity

**Name**: NT Emulator (nt_emu)
**Type**: Multi-part Hardware Emulation & Development Framework
**Author**: Neal Sanche (No Such Device)
**License**: MIT
**Version**: 2.0.0

## Executive Summary

NT_EMU is a comprehensive development environment for creating and testing Disting NT algorithm plugins on desktop computers. It provides a VCV Rack module that faithfully emulates the Disting NT hardware API, allowing developers to write, test, and debug NT plugins without requiring the physical device. The project also includes an extensive PRP (Product Requirement Prompt) framework demonstrating AI-assisted development methodologies.

## Project Structure

This is a **multi-part repository** with two main components:

1. **VCV Rack NT Emulator Module** - A VCV Rack synthesizer module that emulates the Disting NT hardware
2. **Disting NT API Emulator** - Core emulation engine implementing the complete NT C++ API

## Technology Stack Summary

| Component | Primary Technology | Purpose |
|-----------|-------------------|---------|
| **VCV Module** | C++11, VCV Rack SDK 2.0 | Desktop synthesizer integration |
| **Emulator Core** | C++, Hardware API emulation | NT API implementation |
| **Build System** | GNU Make | Cross-platform compilation |
| **Documentation** | Markdown, PRP Framework | AI-assisted development docs |
| **Automation** | Python 3.12+ | PRP execution and tooling |

## Architecture Classification

### Design Pattern: **Modular Component Architecture**

The project employs a clean modular design with:
- **Dependency Injection** for component coordination
- **Interface-based Design** for extensibility
- **Observer Pattern** for state management
- **Safe Execution Environment** for plugin isolation

### Repository Type: **Hybrid Development Framework**

Combines:
- Production VCV Rack audio module
- Hardware API emulation layer
- AI-assisted development methodology (PRPs)
- Comprehensive documentation system

## Key Features

### For Musicians/Users
- Load and run Disting NT algorithm plugins in VCV Rack
- Full parameter control with CV modulation
- OLED display emulation
- MIDI input/output support
- 12 audio inputs + 8 audio outputs

### For Developers
- Complete Disting NT C++ API implementation
- Desktop debugging capabilities
- Hot-reload plugin support
- Comprehensive test infrastructure
- Exception-safe plugin execution

### For AI-Assisted Development
- 24+ Product Requirement Prompts (PRPs)
- 28+ Claude Code commands
- Structured development workflows
- Documentation generation system

## Quick Start Guide

### Building the VCV Module

```bash
cd vcv-plugin
make clean && make
make install
```

### Running in VCV Rack

```bash
# Launch VCV Rack with debug output
/Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d
```

### Testing Plugins

```bash
# Build test plugins
make test-plugins

# Run unit tests
make test
```

## Development Status

### Current State
- ✅ **Build Status**: Compiles successfully
- ✅ **Architecture**: Recently refactored to modular design
- ✅ **Tests**: 16/16 JSON bridge tests passing
- ✅ **Code Reduction**: Main module reduced 30% (2961 → 2089 lines)

### Recent Achievements
- Successful modular refactoring completed
- Clean interface-based architecture implemented
- Automated comment removal tool created
- Comprehensive documentation generated

## Project Components

### Core Modules
1. **Plugin Manager** - Dynamic plugin loading (465 lines)
2. **Parameter System** - Parameter routing matrix (450 lines)
3. **Menu System** - Navigation state machine (380 lines)
4. **MIDI Processor** - MIDI I/O handling (220 lines)
5. **Display Renderer** - OLED emulation (490 lines)

### Supporting Infrastructure
- **PRP Framework** - AI-assisted development methodology
- **BMAD Workflows** - Business analysis and planning tools
- **Claude Commands** - Pre-configured AI assistant commands
- **Test Plugins** - Validation and example implementations

## Documentation Map

### Architecture Documentation
- [VCV Module Architecture](./architecture-vcv-plugin.md) - Module design and implementation
- [Emulator Architecture](./architecture-emulator.md) - Core API emulation details
- [Source Tree Analysis](./source-tree-analysis.md) - Complete directory structure

### Development Guides
- [Development Guide](./development-guide.md) - Setup and build instructions
- [Integration Architecture](./integration-architecture.md) _(To be generated)_

### Existing Documentation
- [Main README](../README.md) - Project introduction
- [VCV Plugin README](../vcv-plugin/README.md) - Plugin-specific details
- [PRP Documentation](../PRPs/README.md) - PRP framework guide

## Repository Statistics

- **Total Files**: 440+ source files (excluding third-party)
- **Languages**: C++ (primary), Python, Markdown
- **Code Size**: ~5000 lines of custom code
- **Documentation**: 70+ documentation files
- **Build Time**: < 30 seconds full rebuild

## Links and Resources

### Source Code
- **GitHub**: [https://github.com/thorinside/nt_emu](https://github.com/thorinside/nt_emu)
- **Author Website**: [https://nosuch.dev/nt-emu](https://nosuch.dev/nt-emu)

### Documentation
- **Plugin Manual**: [https://nosuch.dev/nt-emu](https://nosuch.dev/nt-emu)
- **VCV Rack SDK**: [VCV Developer Documentation](https://vcvrack.com/manual/PluginDevelopmentTutorial)

### Support
- **Email**: thorinside@gmail.com
- **Issues**: GitHub issue tracker

## Next Steps

For developers new to the project:
1. Review the [Development Guide](./development-guide.md)
2. Explore the [Source Tree Analysis](./source-tree-analysis.md)
3. Study the architecture documents for your area of interest
4. Check the PRPs in `/PRPs/` for development patterns
5. Use Claude Code commands for AI-assisted development