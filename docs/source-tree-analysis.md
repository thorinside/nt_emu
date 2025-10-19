# Source Tree Analysis - NT_EMU Project

Generated: 2025-10-19

## Repository Structure

This is a **multi-part repository** containing a VCV Rack audio plugin paired with an NT hardware emulator and PRP framework infrastructure.

## Complete Directory Tree

```
nt_emu/
├── vcv-plugin/                  # Part 1: VCV Rack Audio Plugin (C++)
│   ├── src/                     # Modular plugin source code
│   │   ├── NtEmu.cpp           # Main module entry (2089 lines)
│   │   ├── plugin/             # Plugin management subsystem
│   │   │   ├── PluginManager.hpp/cpp    # Dynamic plugin loading (465 lines)
│   │   │   └── PluginExecutor.hpp/cpp   # Safe execution with error handling
│   │   ├── parameter/          # Parameter routing and management
│   │   │   └── ParameterSystem.hpp/cpp  # 450 lines
│   │   ├── menu/               # Menu navigation state machine
│   │   │   └── MenuSystem.hpp/cpp       # 380 lines
│   │   ├── midi/               # MIDI I/O processing
│   │   │   └── MidiProcessor.hpp/cpp    # 220 lines
│   │   ├── display/            # Display rendering system
│   │   │   ├── DisplayRenderer.hpp/cpp  # 490 lines
│   │   │   └── IDisplayDataProvider.hpp # Interface pattern
│   │   ├── api/                # NT API wrapper layer
│   │   │   └── NTApiWrapper.hpp/cpp
│   │   ├── dsp/                # DSP and audio routing
│   │   │   └── BusSystem.hpp
│   │   ├── widgets/            # UI components
│   │   │   ├── PressableEncoder.hpp/cpp
│   │   │   └── PressablePot.hpp/cpp
│   │   └── algorithms/         # DSP algorithm implementations
│   │       └── Algorithm.hpp
│   ├── Rack-SDK/               # VCV Rack 2 SDK (included)
│   ├── res/                    # Resources (SVG panels, etc.)
│   ├── test_plugins/           # Test plugin infrastructure
│   ├── docs/                   # Plugin-specific documentation
│   ├── scripts/                # Build and utility scripts
│   ├── Makefile                # VCV standard build system
│   └── plugin.json             # Plugin metadata (v2.0.0)
│
├── emulator/                    # Part 2: NT Hardware Emulator Core
│   ├── include/                # Public headers
│   │   └── nt_api/            # NT hardware API definitions
│   ├── src/                    # Emulator implementation
│   │   └── core/              # Core emulation logic
│   ├── test_plugins/           # Emulator test plugins
│   │   └── examples/          # Example plugin implementations
│   ├── third_party/            # External dependencies
│   │   ├── imgui/             # Dear ImGui for UI
│   │   ├── json/              # nlohmann/json for serialization
│   │   ├── spdlog/            # Logging library
│   │   └── rtmidi/            # MIDI support
│   └── README.md               # Emulator documentation
│
├── PRPs/                        # Product Requirement Prompts Framework
│   ├── scripts/                # PRP execution automation
│   │   └── prp_runner.py      # Main PRP runner
│   ├── templates/              # PRP structure templates
│   ├── ai_docs/               # Curated AI context docs
│   └── *.md                   # 24+ active PRPs
│
├── bmad/                        # Business Model & Analysis Documents
│   ├── core/                   # Core BMAD framework
│   │   ├── tasks/             # Task definitions (workflow.xml, etc.)
│   │   └── workflows/         # Core workflows
│   └── bmm/                    # BMM module
│       ├── config.yaml        # User configuration
│       ├── agents/            # Agent definitions
│       └── workflows/         # Analysis workflows
│           ├── 1-analysis/    # Analysis phase workflows
│           ├── 2-plan-workflows/
│           ├── 3-solutioning/
│           └── 4-implementation/
│
├── .claude/                     # Claude Code Integration
│   ├── commands/               # 28+ Claude commands
│   │   ├── PRPs/              # PRP-related commands
│   │   ├── development/       # Development utilities
│   │   ├── code-quality/     # Review and refactoring
│   │   ├── rapid-development/ # Experimental parallel PRPs
│   │   └── git-operations/   # Git workflow commands
│   └── agents/                 # Agent orchestration
│
├── TASK_PRP/                   # Standalone PRP examples
├── fonts/                      # Shared font resources
├── docs/                       # Project documentation
│   └── pixel_format.md       # Technical specifications
│
├── CLAUDE.md                   # Claude Code instructions (project)
├── CLAUDE.local.md            # Local environment config
├── README.md                   # Main project README
├── Makefile                    # Root build orchestration
├── plugin.json                 # VCV plugin manifest
└── pyproject.toml             # Python project config
```

## Critical Directories

### VCV Plugin Core (`vcv-plugin/src/`)
**Purpose**: Modular VCV Rack audio plugin implementation
- **Entry Point**: `NtEmu.cpp` - Main module initialization and coordination
- **Architecture**: Clean modular design with dependency injection
- **Key Systems**: Plugin loading, parameter routing, MIDI processing, display rendering
- **Integration**: Communicates with emulator core via NT API

### Emulator Engine (`emulator/`)
**Purpose**: Hardware emulation of NT (Disting-XT) device
- **Core Logic**: `src/core/` - Emulation implementation
- **API Layer**: `include/nt_api/` - Public interface definitions
- **Plugin System**: Supports loadable algorithm plugins
- **Safety**: Exception-safe plugin execution environment

### PRP Framework (`PRPs/`)
**Purpose**: AI-assisted development methodology implementation
- **Automation**: `scripts/prp_runner.py` for executing PRPs
- **Templates**: Structured formats for requirements and implementation
- **Context**: Curated documentation for AI agent context injection

### BMAD Workflows (`bmad/`)
**Purpose**: Business analysis and project management framework
- **Analysis**: Market research, requirements gathering workflows
- **Planning**: Project planning and architecture design
- **Implementation**: Development story management
- **Status Tracking**: Workflow progress monitoring

### Claude Commands (`.claude/commands/`)
**Purpose**: Pre-configured AI assistant commands
- **Categories**: PRPs, development, code quality, git operations
- **Integration**: Seamless with Claude Code environment
- **Automation**: Reduces repetitive tasks

## Integration Points

### Between vcv-plugin and emulator
- **Interface**: NT API headers in `emulator/include/nt_api/`
- **Communication**: Direct C++ API calls
- **Plugin Loading**: Dynamic .dylib loading for algorithms
- **Shared Resources**: Font system, parameter definitions

### Between PRPs and codebase
- **Documentation**: PRPs reference source structure
- **Validation**: Build commands and test suites
- **Context**: CLAUDE.md files provide AI guidance

## Build Artifacts

### VCV Plugin Build
- **Output**: `dist/DistingNT/` (plugin distribution)
- **Library**: `plugin.dylib` (macOS)
- **Test Plugins**: `test_customui_plugin.dylib`, etc.

### Installation Paths
- **VCV Rack Plugins**: `~/.Rack2/plugins/DistingNT/`
- **Documentation**: `docs/` (generated by workflows)

## Development Entry Points

### For VCV Plugin Development
- Start: `vcv-plugin/src/NtEmu.cpp`
- Build: `cd vcv-plugin && make clean && make`
- Install: `make install`

### For Emulator Development
- Start: `emulator/src/core/`
- API: `emulator/include/nt_api/`

### For PRP/AI Development
- Start: `PRPs/README.md`
- Execute: `uv run PRPs/scripts/prp_runner.py`
- Commands: `.claude/commands/`

## Testing Infrastructure

### Unit Tests
- JSON Bridge: 16 tests in `vcv-plugin/tests/`
- Test Plugins: `test_plugins/` in both vcv-plugin and emulator

### Integration Testing
- Plugin Loading: `test_customui_plugin.dylib`
- Display Testing: `drawtest_plugin.dylib`
- Font Testing: `fonttest_plugin.dylib`

## Configuration Files

### Build Configuration
- `vcv-plugin/Makefile` - VCV Rack standard build
- `pyproject.toml` - Python project setup

### Plugin Configuration
- `plugin.json` - VCV plugin metadata
- `bmad/bmm/config.yaml` - BMAD user configuration

### AI Assistant Configuration
- `CLAUDE.md` - Project-wide AI instructions
- `CLAUDE.local.md` - Local environment settings
- `.claude/settings.local.json` - Tool permissions