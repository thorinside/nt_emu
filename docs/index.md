# NT_EMU Documentation Index

Generated: 2025-10-19 | Version: 2.0.0

## Project Overview

- **Type:** Multi-part repository with 2 components
- **Primary Language:** C++
- **Architecture:** Modular Component Architecture with Dependency Injection
- **Purpose:** Desktop emulation environment for Disting NT plugin development

## Quick Reference

### VCV Rack NT Emulator Module (vcv-plugin)
- **Type:** Desktop synthesizer module
- **Tech Stack:** C++11, VCV Rack SDK 2.0
- **Entry Point:** `src/NtEmu.cpp`
- **Build:** `make clean && make`
- **Root:** `/vcv-plugin`

### Disting NT API Emulator (emulator)
- **Type:** Hardware API emulation system
- **Tech Stack:** C++, ImGui, nlohmann/json, RtMidi
- **API Layer:** `include/nt_api/`
- **Core Logic:** `src/core/`
- **Root:** `/emulator`

## Generated Documentation

### Core Documentation
- [Project Overview](./project-overview.md) - Executive summary and quick reference
- [Source Tree Analysis](./source-tree-analysis.md) - Complete directory structure and organization

### Architecture Documentation
- [VCV Module Architecture](./architecture-vcv-plugin.md) - Detailed module design and components
- [Emulator Architecture](./architecture-emulator.md) - Core API emulation system

### Development Documentation
- [Development Guide](./development-guide.md) - Setup, building, and development workflow

### Component Documentation
- [Component Inventory - VCV Module](./component-inventory-vcv-plugin.md) _(To be generated)_
- [Component Inventory - Emulator](./component-inventory-emulator.md) _(To be generated)_

### Integration Documentation
- [Integration Architecture](./integration-architecture.md) _(To be generated)_

### API Documentation
- [API Contracts - VCV Module](./api-contracts-vcv-plugin.md) _(To be generated)_
- [API Contracts - Emulator](./api-contracts-emulator.md) _(To be generated)_

### Data Models
- [Data Models - VCV Module](./data-models-vcv-plugin.md) _(To be generated)_
- [Data Models - Emulator](./data-models-emulator.md) _(To be generated)_

### Deployment Documentation
- [Deployment Guide](./deployment-guide.md) _(To be generated)_

## Existing Project Documentation

### Primary Documentation
- [Main Project README](../README.md) - Project introduction and overview
- [VCV Plugin README](../vcv-plugin/README.md) - Plugin-specific information
- [Emulator README](../emulator/README.md) - Emulator core details

### PRP Framework Documentation
- [PRPs README](../PRPs/README.md) - Product Requirement Prompts methodology
- [Active PRPs](../PRPs/) - 24+ executable development prompts

### Development Methodology
- [CLAUDE.md](../CLAUDE.md) - AI assistant instructions for the project
- [CLAUDE.local.md](../CLAUDE.local.md) - Local environment configuration

### Specialized Documentation
- [Parameter Tests](../emulator/test_plugins/examples/README_PARAMETER_TESTS.md) - Parameter API testing
- [Makefile Augmenter](../vcv-plugin/docs/README_makefile_augmenter.md) - Build system enhancement
- [Scripts Documentation](../vcv-plugin/scripts/README.md) - Utility scripts

### BMAD Framework Documentation
- [BMM Module](../bmad/bmm/README.md) - Business Model Module
- [Workflows](../bmad/bmm/workflows/README.md) - Analysis and planning workflows
- [Workflow Status](../bmad/bmm/workflows/workflow-status/README.md) - Progress tracking

## Getting Started

### For New Developers

1. **Understand the Project**
   - Read the [Project Overview](./project-overview.md)
   - Review the [Source Tree Analysis](./source-tree-analysis.md)

2. **Set Up Development Environment**
   - Follow the [Development Guide](./development-guide.md)
   - Configure your IDE for C++11

3. **Understand the Architecture**
   - Study [VCV Module Architecture](./architecture-vcv-plugin.md) for UI and audio
   - Review [Emulator Architecture](./architecture-emulator.md) for API implementation

4. **Start Contributing**
   - Pick an area of interest
   - Review relevant PRPs for context
   - Use Claude Code commands for assistance

### For Plugin Developers

1. **Learn the NT API**
   - Review emulator API headers in `emulator/include/nt_api/`
   - Study test plugins in `test_plugins/examples/`

2. **Create Your Plugin**
   - Use existing plugins as templates
   - Implement required entry points
   - Test in emulator environment

3. **Debug and Optimize**
   - Use VCV Rack's debug mode
   - Monitor performance metrics
   - Validate with test suite

### For AI-Assisted Development

1. **Use the PRP Framework**
   - Browse available PRPs in `/PRPs/`
   - Execute PRPs with `prp_runner.py`
   - Create custom PRPs as needed

2. **Leverage Claude Commands**
   - Access via `/` prefix in Claude Code
   - Use `/prime-core` to load context
   - Apply `/review-staged-unstaged` for code review

## Key Technologies

| Category | Technology | Version | Usage |
|----------|------------|---------|-------|
| **Core** | C++ | C++11 | Plugin and emulator implementation |
| **Audio** | VCV Rack SDK | 2.0 | Modular synthesis platform |
| **UI** | Dear ImGui | Latest | Debug interface |
| **Serialization** | nlohmann/json | 3.x | State management |
| **MIDI** | RtMidi | Included | MIDI I/O |
| **Python** | Python | 3.12+ | Automation and PRPs |
| **Build** | GNU Make | Standard | Compilation |

## Status Summary

### Build Status
✅ **Successful** - `make clean && make` completes without errors

### Test Status
✅ **Passing** - 16/16 JSON bridge tests pass

### Documentation Status
✅ **Core Documentation** - Generated successfully
⚠️ **Component Documentation** - To be generated on demand
⚠️ **Integration Documentation** - Pending generation

### Code Quality
✅ **Modular Architecture** - Successfully refactored
✅ **Code Reduction** - 30% reduction in main module
✅ **Clean Interfaces** - Interface-based design implemented

## Contact & Support

- **Author**: Neal Sanche (No Such Device)
- **Email**: thorinside@gmail.com
- **GitHub**: [https://github.com/thorinside/nt_emu](https://github.com/thorinside/nt_emu)
- **Website**: [https://nosuch.dev/nt-emu](https://nosuch.dev/nt-emu)

---

*This documentation was generated using the BMAD Document Project workflow with #yolo mode quick scan.*