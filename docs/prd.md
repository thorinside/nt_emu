# nt_emu Product Requirements Document (PRD)

**Author:** Neal
**Date:** 2025-10-19
**Project Level:** 3 (Complex System)
**Target Scale:** Community plugin with 5000+ active users, 10+ ecosystem contributors

---

## Goals and Background Context

### Goals

1. **Complete DistingNT Feature Emulation**: Finish all remaining hardware feature implementations to achieve 100% API parity with physical hardware
2. **Release Production-Ready Plugin**: Package and distribute DistingNT emulator as official VCV Rack plugin across macOS, Linux, Windows
3. **Enable Algorithm Ecosystem**: Establish sustainable platform for community developers to create, share, and discover custom algorithms
4. **Demonstrate Hardware Emulation Model**: Prove that faithful C++ API emulation enables zero-modification plugin portability across platforms
5. **Build Developer Community**: Create documentation, tools, and infrastructure for algorithm developer onboarding and contribution

### Background Context

The Disting NT is an inaccessible but powerful synthesizer ($300+ cost, slow development cycles, limited ecosystem). NT_EMU solves this through complete C++ API emulation within VCV Rack, enabling unmodified plugins to run identically on desktop.

The project has successfully evolved into a modular, production-ready system:
- **92% complete**: 33 of 36 feature stories implemented
- **Builds successfully**: Modular architecture (5 core systems)
- **Test coverage**: 16/16 unit tests passing
- **Zero critical issues**: One minor UX improvement identified (1ms parameter sync delay)

This PRD defines the final push to ecosystem readiness, combining feature completion with distribution, documentation, and community infrastructure.

---

## Requirements

### Functional Requirements

**Core Plugin Functionality** (92% implemented, 8% remaining)
- FR001: Support dynamic loading of Disting NT algorithm plugins (.dylib macOS, .so Linux, .dll Windows) ✅
- FR002: Execute loaded plugins safely with exception handling ✅
- FR003: Maintain 100% API compatibility with Disting NT C++ interface ✅
- FR004: Support hot-reload of plugins without host restart ✅
- FR005: Preserve and restore plugin state via preset system ✅
- FR006: Provide 256x64 monochrome OLED emulation with full graphics API ✅
- FR007: Support 3 physical pots controlling algorithm parameters (parameter count varies by algorithm) with CV modulation ✅
- FR008: Implement hierarchical menu system for algorithm navigation ✅
- FR009: Support 12 audio inputs + 8 audio outputs with 28-bus internal routing ✅
- FR010: **NEW**: Synchronize parameter changes to audio routing in real-time (< 1 sample block latency)

**Distribution & Ecosystem**
- FR011: Enable one-click plugin installation from VCV Rack Plugin Browser
- FR012: Provide algorithm discovery mechanism (browser for community algorithms)
- FR013: Support algorithm versioning and dependency management
- FR014: Enable algorithm installation with one-click from plugin menu

**Developer Support**
- FR015: Publish comprehensive algorithm development guide
- FR016: Provide example algorithms for common use cases
- FR017: Support community algorithm submission workflow
- FR018: Document API reference for third-party developers

### Non-Functional Requirements

- NFR001: **Performance**: Audio processing < 1ms latency overhead, parameter updates < 1 sample block
- NFR002: **Reliability**: Zero crashes from invalid plugin input, 8+ hour stability without memory leaks
- NFR003: **Compatibility**: 100% API fidelity with Disting NT; audio output bit-identical across platforms
- NFR004: **Build Time**: Full rebuild < 30 seconds on modern processors
- NFR005: **Code Quality**: Modular architecture (< 2100 lines core), 16/16 unit tests passing
- NFR006: **Distribution**: Installation < 50MB, plugin loads in < 2 seconds
- NFR007: **Community**: 1000+ downloads month 1, 5000+ active users month 3, 10+ algorithm contributions quarter 1

---

## User Journeys

### Journey 1: Algorithm Developer - Development Cycle
1. Developer downloads DistingNT emulator from VCV Plugin Browser (one-click) [FR011]
2. Develops algorithm using local C++ toolchain (IDE, debugger) [FR015, FR018]
3. Tests algorithm in real-time within VCV Rack using emulator [FR003, FR016]
4. Submits algorithm to community repository [FR017]
5. Community provides feedback and ratings [FR012]
6. Algorithm appears in plugin's algorithm browser [FR012]
7. Other developers download and use algorithm [FR014]

**Success Metric**: Average algorithm development time reduced from 1 day (hardware cycle) to 2 hours (desktop cycle)

### Journey 2: End User - Plugin Discovery & Installation
1. User discovers DistingNT in VCV Plugin Browser [FR011]
2. Reads clear description: "100% compatible Disting NT emulator + community algorithm support" [FR011]
3. Clicks "Install" - plugin auto-downloads and installs [FR011]
4. Opens plugin, selects algorithm via menu [FR008]
5. Browses community algorithms within plugin [FR012]
6. One-click installs new algorithm [FR014]
7. Algorithm immediately available in algorithm menu [FR001]

**Success Metric**: Time from discovery to first use < 5 minutes; zero support requests about installation

### Journey 3: Platform User - Multiplatform Experience
1. User on macOS discovers and installs plugin [FR011]
2. Later switches to Linux - plugin already available [FR011]
3. Loads saved presets and plugin state transfers identically [FR005, NFR003]
4. Algorithms perform identically (audio output bit-identical) [NFR003]
5. All controls work identically (no platform surprises) [FR003, NFR003]
6. User shares preset with Windows user who loads it identically [FR005, NFR003]

**Success Metric**: <1% platform-specific bug reports; 90%+ cross-platform satisfaction

---

## UX Design Principles

1. **Faithful Hardware Emulation**: UI/behavior matches Disting NT exactly for user familiarity
2. **Progressive Disclosure**: Advanced features hidden until needed (algorithm menu, parameter editing)
3. **Zero-Install Complexity**: One-click install, zero configuration, works immediately
4. **Responsive Feedback**: All user actions reflected instantly in display and audio
5. **Consistent Across Platforms**: Identical experience on macOS, Linux, Windows
6. **Community-First**: Algorithms discovered and installed within plugin interface
7. **Developer Empowerment**: Clear documentation enables algorithm creation without plugin expertise

---

## User Interface Design Goals

1. **256x64 OLED Emulation**: Faithful reproduction of hardware display including fonts, contrast, refresh
2. **Rotary Encoder Metaphor**: Three-mode menu navigation matches hardware exactly
3. **Parameter Controls**: 3 physical pots (L, C, R) controlling algorithm parameters via menu navigation with smooth CV modulation
4. **Algorithm Browser**: Hierarchical menu showing available algorithms with one-encoder selection
5. **Real-Time Feedback**: Display updates instantly on all user interactions
6. **Visual Consistency**: All UI elements scale and render identically on macOS, Linux, Windows

---

## Epic List

1. **Epic 1: Finish DistingNT Feature Emulation** (1 week)
   - Real-time parameter synchronization (Story 1.1)
   - Consolidate technical decisions documentation (Story 1.2)
   - **Delivers**: 100% feature parity, technical foundation for ecosystem

2. **Epic 2: Multiplatform VCV Plugin Release** (3 weeks)
   - Cross-platform build validation (macOS, Linux, Windows)
   - Plugin registry submission and documentation
   - User/developer documentation and community infrastructure
   - Performance & stability validation
   - **Delivers**: Production-ready plugin in official registry, installation guides, community infrastructure foundation
   - **Note**: Algorithm ecosystem features (FR012-FR014, FR016) deferred to Epic 3

3. **Epic 3: Algorithm Ecosystem Expansion** (4 weeks, planned)
   - Algorithm discovery and versioning system (FR012, FR013)
   - One-click algorithm installation from plugin (FR014)
   - Example algorithms for common use cases (FR016)
   - Port 20+ existing Disting NT algorithms to community repository
   - Algorithm marketplace with ratings and reviews
   - Algorithm developer workshop and tutorials
   - **Delivers**: Thriving community, 50+ algorithms, 5000+ active users, complete algorithm ecosystem

---

## Out of Scope

- **Physical hardware support**: This is a software emulator, not USB interface to hardware
- **Disting NT variants**: Focus on standard NT model; N4+ features deferred
- **Algorithm licensing**: Community submission is open; no DRM or purchase system
- **Advanced DSP algorithms**: Focus on community porting existing algorithms, not new algorithm development
- **Marketplace payments**: Algorithm sharing is free; no commercial transaction system
- **Mobile platforms**: VCV Rack is desktop-focused; mobile support deferred indefinitely
- **Custom hardware encoders**: Integration with external hardware controllers deferred to future phase

---

**Related Documents:**
- `epics.md` - Full epic and story breakdown
- `bmm-workflow-status.md` - Current project status and workflow state
- `COMPREHENSIVE-PROJECT-STATUS.md` - Detailed implementation analysis
- `architecture-vcv-plugin.md` - Plugin architecture reference
