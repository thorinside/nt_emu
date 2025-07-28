# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Nature

This is a **PRP (Product Requirement Prompt) Framework** repository, not a traditional software project. The core concept: **"PRP = PRD + curated codebase intelligence + agent/runbook"** - designed to enable AI agents to ship production-ready code on the first pass.

## Core Architecture

### VCV Plugin Modular Architecture (Latest)

**DistingNT Plugin** is being refactored from a 3000-line monolith into a modular architecture (WORK IN PROGRESS):

```
vcv-plugin/src/
├── DistingNT.cpp           # Thin coordination layer (~500 lines target)
├── plugin/
│   ├── PluginManager.hpp   # Plugin loading/unloading (465 lines)
│   └── PluginExecutor.hpp  # Safe plugin execution with error handling (190 lines)
├── parameter/
│   └── ParameterSystem.hpp # Parameter management & routing matrix (450 lines)
├── menu/
│   └── MenuSystem.hpp      # State machine for menu navigation (380 lines)
├── midi/
│   └── MidiProcessor.hpp   # MIDI I/O with activity tracking (220 lines)
└── [existing components]
    ├── dsp/BusSystem.hpp   # Audio routing
    ├── EmulatorCore.hpp    # Core emulator logic
    └── display/OLEDWidget.hpp # Display handling
```

**Key Patterns Implemented:**
- **Dependency Injection**: Main module coordinates via smart pointers
- **Observer Pattern**: Components notify of state changes  
- **Safe Execution**: Comprehensive exception handling for plugin crashes
- **C++11 Compatibility**: VCV Rack standard compliance
- **Real-time Safety**: Audio thread considerations maintained

**Current Status:**
- ✅ **Modular Components**: All 5 core modules created (individual .hpp/.cpp files)
- ⚠️ **Build Status**: `make clean && make` FAILS - compilation errors in DistingNT.cpp
- ❌ **Integration**: Legacy method calls reference removed variables (pluginHandle, pluginFactory, etc.)
- 🔄 **Next Steps**: Systematic migration of ~200+ legacy references to use new modular components

### Command-Driven System

- **28+ pre-configured Claude Code commands** in `.claude/commands/`
- Commands organized by function:
  - `PRPs/` - PRP creation and execution workflows
  - `development/` - Core development utilities (prime-core, onboarding, debug)
  - `code-quality/` - Review and refactoring commands
  - `rapid-development/experimental/` - Parallel PRP creation and hackathon tools
  - `git-operations/` - Conflict resolution and smart git operations

### Template-Based Methodology

- **PRP Templates** in `PRPs/templates/` follow structured format with validation loops
- **Context-Rich Approach**: Every PRP must include comprehensive documentation, examples, and gotchas
- **Validation-First Design**: Each PRP contains executable validation gates (syntax, tests, integration)

### AI Documentation Curation

- `PRPs/ai_docs/` contains curated Claude Code documentation for context injection
- `claude_md_files/` provides framework-specific CLAUDE.md examples

## Development Commands

### PRP Execution

```bash
# Interactive mode (recommended for development)
uv run PRPs/scripts/prp_runner.py --prp [prp-name] --interactive

# Headless mode (for CI/CD)
uv run PRPs/scripts/prp_runner.py --prp [prp-name] --output-format json

# Streaming JSON (for real-time monitoring)
uv run PRPs/scripts/prp_runner.py --prp [prp-name] --output-format stream-json
```

### Key Claude Commands

- `/prp-base-create` - Generate comprehensive PRPs with research
- `/prp-base-execute` - Execute PRPs against codebase
- `/prp-planning-create` - Create planning documents with diagrams
- `/prime-core` - Prime Claude with project context
- `/review-staged-unstaged` - Review git changes using PRP methodology

## Critical Success Patterns

### The PRP Methodology

1. **Context is King**: Include ALL necessary documentation, examples, and caveats
2. **Validation Loops**: Provide executable tests/lints the AI can run and fix
3. **Information Dense**: Use keywords and patterns from the codebase
4. **Progressive Success**: Start simple, validate, then enhance

### PRP Structure Requirements

- **Goal**: Specific end state and desires
- **Why**: Business value and user impact
- **What**: User-visible behavior and technical requirements
- **All Needed Context**: Documentation URLs, code examples, gotchas, patterns
- **Implementation Blueprint**: Pseudocode with critical details and task lists
- **Validation Loop**: Executable commands for syntax, tests, integration

### Validation Gates (Must be Executable)

```bash
# CURRENT BUILD STATUS: FAILS
make clean && make
# ERROR: Multiple compilation errors in DistingNT.cpp
# - Legacy references to removed variables (pluginHandle, pluginFactory, pluginAlgorithm)
# - MIDI variable references (midiInputLight, midiOutputLight) 
# - Parameter system integration incomplete

# Individual component verification (when build is working):
# make src/plugin/PluginManager.cpp.o
# make src/parameter/ParameterSystem.cpp.o  
# make src/menu/MenuSystem.cpp.o
# make src/midi/MidiProcessor.cpp.o

# Integration test (after successful build):
# cp dist/DistingNT/plugin.dylib ~/.Rack2/plugins/DistingNT/
# /Applications/VCV\ Rack\ 2.app/Contents/MacOS/Rack -d
```

### VCV Plugin Development Notes

**Critical API Considerations:**
- NT API uses C++11 features, ensure compatibility
- Plugin execution must be exception-safe (crashes handled gracefully)
- Parameter system requires proper bounds checking and clamping
- MIDI processing needs thread-safe implementation
- Display updates should only mark dirty when changed

**Build Requirements:**
- `make clean && make` for full rebuild
- All new subdirectories automatically included via `$(wildcard src/*/*.cpp)`
- Components compile independently for modular development

## Anti-Patterns to Avoid

- L Don't create minimal context prompts - context is everything - the PRP must be comprehensive and self-contained, reference relevant documentation and examples.
- L Don't skip validation steps - they're critical for one-pass success - The better The AI is at running the validation loop, the more likely it is to succeed.
- L Don't ignore the structured PRP format - it's battle-tested
- L Don't create new patterns when existing templates work
- L Don't hardcode values that should be config
- L Don't catch all exceptions - be specific

## Working with This Framework

### When Creating new PRPs

1. **Context Process**: New PRPs must consist of context sections, Context is King!
2.

### When Executing PRPs

1. **Load PRP**: Read and understand all context and requirements
2. **ULTRATHINK**: Create comprehensive plan, break down into todos, use subagents, batch tool etc check prps/ai_docs/
3. **Execute**: Implement following the blueprint
4. **Validate**: Run each validation command, fix failures
5. **Complete**: Ensure all checklist items done

### Command Usage

- Read the .claude/commands directory
- Access via `/` prefix in Claude Code
- Commands are self-documenting with argument placeholders
- Use parallel creation commands for rapid development
- Leverage existing review and refactoring commands

## Project Structure Understanding

```
PRPs-agentic-eng/
.claude/
  commands/           # 28+ Claude Code commands
  settings.local.json # Tool permissions
PRPs/
  templates/          # PRP templates with validation
  scripts/           # PRP runner and utilities
  ai_docs/           # Curated Claude Code documentation
   *.md               # Active and example PRPs
 claude_md_files/        # Framework-specific CLAUDE.md examples
 pyproject.toml         # Python package configuration
```

Remember: This framework is about **one-pass implementation success through comprehensive context and validation**. Every PRP should contain the exact context for an AI agent to successfully implement working code in a single pass.
