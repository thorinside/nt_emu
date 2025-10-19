# nt_emu - Epic Breakdown

**Author:** Neal
**Date:** 2025-10-19
**Project Level:** 3 (Complex System)
**Target Scale:** Community plugin ecosystem with 5000+ active users

---

## Overview

This document provides detailed epic and story breakdown for nt_emu, expanding on the high-level epic list in the [PRD](./PRD.md).

Each epic includes:
- Expanded goal and value proposition
- Complete story breakdown with user stories
- Acceptance criteria for each story
- Story sequencing and dependencies

**Epic Sequencing Principles:**

- Epic 1: Complete remaining feature implementations and document technical decisions (foundation for ecosystem)
- Epic 2: Release production plugin across all platforms with comprehensive documentation (enable community growth)
- Epic 3: Grow algorithm ecosystem with ported algorithms and developer community (realize vision)
- Stories within each epic are vertically sliced, testable, and sequentially ordered
- No forward dependencies - each story builds only on previous work

---

## Epic 1: Finish DistingNT Feature Emulation

**Duration:** 1 week
**Priority:** CRITICAL (blocking Epic 2)
**Business Value:** Achieve 100% feature parity, establish technical foundation for ecosystem
**Owner:** Neal

### Epic Goal

Complete all remaining DistingNT hardware feature implementations (92% → 100%) and consolidate architectural decisions to enable ecosystem growth. This epic captures the final technical refinements before production release.

**Note:** This is a brownfield project at 92% completion. Epic 1 completes final features and documentation rather than establishing initial infrastructure foundation, as core architecture is already in place and validated.

### Epic Stories

---

**Story 1.1: Real-Time Parameter Synchronization**

**Story ID:** Epic-1.1 (formerly Story 2.6)

**User Story:**
As a parameter-tweaking musician,
I want parameter changes to apply to audio routing instantly,
So that I experience zero lag between menu adjustment and audio effect.

**Current Issue:**
- ~1ms delay observed between parameter menu adjustment and audio routing application
- Parameter visibly changes on display, but audio routing applies after menu exit
- Impact: Low (imperceptible in musical contexts), but UX improvement opportunity high

**Technical Context:**
- Parameter system uses matrix-based routing architecture ✅
- Menu system currently batches parameter updates until menu exit
- Audio processing path reads from routing matrix (no latency issues)
- No thread contention expected (parameter system uses lock-free reads)

**Acceptance Criteria:**
1. Parameter routing updates apply within 1 sample block of menu encoder change (< 0.02ms @ 96kHz)
2. Display feedback reflects parameter change instantly (already works)
3. Audio output applies new routing within 1 sample block
4. All 16 unit tests still pass
5. Zero performance regression (CPU profiling shows no change)
6. Integration test with VCV Rack confirms behavior
7. Code review approves implementation

**Technical Implementation Options** (choose one):
- Option 1: Direct routing map updates on parameter change (simplest, ~1 hour)
- Option 2: Direct parameter reads in audio path during processing (elegant, ~2 hours)
- Option 3: Hybrid approach - update maps immediately, cache reads (recommended, ~1.5 hours)

**Recommended Approach:** Option 3 (hybrid)
```cpp
// Modify MenuSystem::updateParameterValue()
MenuSystem::updateParameterValue(int idx, float val) {
    parameters[idx] = val;
    parameterSystem->applyRoutingChanges(idx);  // Apply immediately
}
```

**Estimated Effort:** 1.5 hours
**Estimated Effort for 2-4 hour AI session:** ✅ Fits (plus testing/validation)

**Prerequisites:** None (builds on existing architecture)

**Validation Gates:**
```bash
# Build succeeds
make clean && make

# Tests pass
make test                    # Confirm 16/16 pass

# Integration test
- Launch VCV Rack with DistingNT plugin
- Select algorithm with audio routing parameters
- Adjust parameter via menu encoder
- Verify audio routing applies within 1 sample block
- Measure latency with timing instrumentation

# Performance check
- Profile audio processing before/after changes
- Verify CPU usage unchanged (< 0.1% variance acceptable)
```

---

**Story 1.2: Consolidate Technical Decisions Documentation**

**Story ID:** Epic-1.2 (formerly Story 5.6)

**User Story:**
As a new developer joining the team,
I want to understand all major architectural decisions and their rationale,
So that I can make informed decisions when extending the system without reading 25 PRPs.

**Current Issue:**
- Architectural decisions scattered across CLAUDE.md, 25 PRPs, and architecture documentation
- New developer onboarding time ~1 day (reading, analysis, pairing)
- No centralized reference for future decision-making

**Deliverable:** `technical-decisions.md` (1000-1500 words)

**Content Sections:**

1. **Modular Component Architecture**
   - Decision: Dependency injection with 5 core modular systems
   - Rationale: Scalability, parallel development, testability, code reduction
   - Trade-offs: Slight architectural complexity vs rapid development velocity
   - Code references: PluginManager.hpp, ParameterSystem.hpp, MenuSystem.hpp, MidiProcessor.hpp, DisplayRenderer.hpp
   - Validation: Successfully reduced NtEmu.cpp from 2961 to 2089 lines (30% reduction)

2. **C++11 Compliance**
   - Decision: No C++14/17/20 features in audio path
   - Rationale: VCV Rack standard compatibility, real-time safety predictability
   - Trade-offs: No modern convenience features vs guaranteed runtime consistency
   - Code references: Build configuration, vcv-plugin/src/
   - Validation: Successful cross-platform builds, zero language feature issues

3. **28-Bus Audio Architecture**
   - Decision: Fixed hardware-matched bus allocation (0-3 inputs, 4-11 CV, 20-23 outputs, 24-27 output CV)
   - Rationale: 100% compatibility with hardware, predictable SIMD alignment, no allocation in audio thread
   - Trade-offs: No runtime bus reconfiguration vs guaranteed performance and correctness
   - Code references: BusSystem.hpp, audio routing system
   - Validation: Supports all routing scenarios, zero audio artifacts

4. **Three-Mode Parameter Menu**
   - Decision: State machine: OFF → PAGE_SELECT → PARAM_SELECT → VALUE_EDIT
   - Rationale: Familiar to hardware users, progressive disclosure, intuitive navigation
   - Trade-offs: More complex state management vs UX familiarity for target users
   - Code references: MenuSystem.hpp, state machine implementation
   - Validation: Matches hardware behavior exactly, users report intuitive control

5. **Exception-Safe Plugin Execution**
   - Decision: Try-catch wrapper around all plugin API calls
   - Rationale: System stability, ecosystem protection against malformed plugins
   - Trade-offs: Slight performance overhead (~0.1%) vs robust system reliability
   - Code references: PluginExecutor.hpp, exception handling patterns
   - Validation: 16/16 tests pass, zero crashes in 1000+ plugin executions

6. **Display Renderer Design**
   - Decision: Dirty region tracking with 30-60 FPS adaptive refresh
   - Rationale: Efficient rendering (minimal overdraw), consistent UI responsiveness
   - Trade-offs: Added complexity vs near-zero CPU cost for display updates
   - Code references: DisplayRenderer.hpp, frame buffer management
   - Validation: Consistent 30-60 FPS, smooth visual updates

**Additional Sections:**

- **Future Decision Template**: How to propose and evaluate new architectural decisions
- **Decision Review Process**: Criteria for accepting/rejecting architectural changes
- **Anti-Patterns We Avoided**: Common mistakes and why we rejected them

**Acceptance Criteria:**
1. Document created at `docs/technical-decisions.md`
2. All 5+ major decisions documented with clear structure
3. Each decision includes: rationale, trade-offs, code references, validation approach
4. Code examples included where helpful (e.g., architectural patterns used)
5. Future decision template provided for extending document
6. Verified readable by developer unfamiliar with project
7. Developer feedback: ≥1 person successfully onboarded using doc in < 1 hour

**Estimated Effort:** 2.5 hours (research consolidation, writing, review)
**Estimated Effort for 2-4 hour AI session:** ✅ Fits (documentation only)

**Prerequisites:** None (pure documentation)

**Validation Gates:**
```bash
# Document exists and validates
ls -la docs/technical-decisions.md

# Content completeness check
- 5+ major decisions documented ✅
- Each includes: decision, rationale, trade-offs, references ✅
- Code examples provided ✅
- Template included for future decisions ✅

# Onboarding effectiveness test
- Have unfamiliar developer review document
- Time to understand architecture: < 1 hour ✅
- Can identify where new features would be implemented ✅
- Provides feedback for improvement
```

---

## Epic 2: Multiplatform VCV Plugin Release

**Duration:** 3 weeks
**Priority:** CRITICAL (enables ecosystem)
**Business Value:** Production-ready plugin available to 50K+ VCV Rack users
**Owner:** Neal

### Epic Goal

Transform DistingNT emulator from development project into production-ready plugin available across macOS, Linux, Windows with comprehensive documentation and community infrastructure.

### Epic Stories

---

**Story 2.1: Linux Build Validation**

**Story ID:** Epic-2.1

**User Story:**
As a Linux user,
I want to download and use DistingNT plugin,
So that I can develop algorithms on my desktop regardless of OS choice.

**Technical Requirements:**
- Compile successfully on Ubuntu 20.04 LTS with gcc/clang
- Build glibc-compatible .so library
- Run all 16 unit tests on Linux
- Integration test with VCV Rack Linux build
- Performance validation (audio latency, CPU usage)

**Acceptance Criteria:**
1. Linux build succeeds with zero compiler warnings
2. All 16 unit tests pass on Linux platform
3. Plugin loads successfully in VCV Rack Linux build
4. Audio latency < 1ms measured on Linux
5. CPU profile shows < 2% overhead at idle
6. Linux-specific MIDI configuration verified (JACK/ALSA/PulseAudio)
7. Plugin registry accepts .so artifact

**Estimated Effort:** 3-4 hours
**Prerequisites:** Story 1.1, 1.2 complete

**Validation Gates:**
```bash
# Linux build succeeds
make clean && make

# Tests pass
make test                    # 16/16 tests pass

# Integration test
vcvrack DistingNT_test.vcv   # Plugin loads, plays audio

# Performance validated
- Audio latency: < 1ms
- CPU usage: < 2% at idle
```

---

**Story 2.2: Windows Build Validation**

**Story ID:** Epic-2.2

**User Story:**
As a Windows user,
I want to download and use DistingNT plugin,
So that I can participate in the community algorithm ecosystem regardless of OS.

**Technical Requirements:**
- Compile successfully on Windows 10/11 with MSVC 2019+
- Build Windows-compatible DLL library
- Implement Windows-specific audio I/O (WASAPI, WinMM)
- Run all 16 unit tests on Windows
- Integration test with VCV Rack Windows build
- Performance validation (audio latency, CPU usage)

**Acceptance Criteria:**
1. Windows build succeeds with zero compiler warnings
2. All 16 unit tests pass on Windows
3. Plugin loads successfully in VCV Rack Windows build
4. Audio latency < 1ms measured on Windows
5. CPU profile shows < 2% overhead at idle
6. Windows audio device selection verified (WinMM, WASAPI, ASIO optional)
7. Plugin registry accepts DLL artifact

**Estimated Effort:** 3-4 hours
**Prerequisites:** Story 1.1, 1.2 complete

**Validation Gates:**
```bash
# Windows build succeeds
msbuild DistingNT.sln /p:Configuration=Release

# Tests pass
test_json_bridge.exe        # 16/16 tests pass

# Integration test
vcvrack.exe DistingNT_test.vcv  # Plugin loads, plays audio

# Performance validated
- Audio latency: < 1ms
- CPU usage: < 2% at idle
```

---

**Story 2.3: CI/CD Pipeline Configuration**

**Story ID:** Epic-2.3

**User Story:**
As a maintainer,
I want automated build and test execution on every commit,
So that cross-platform compatibility is verified before release.

**Technical Requirements:**
- GitHub Actions workflow for macOS builds
- GitHub Actions workflow for Linux builds
- GitHub Actions workflow for Windows builds
- Automated binary artifact upload to GitHub releases
- Automated test execution on each commit
- Build time tracking and optimization

**Acceptance Criteria:**
1. CI/CD pipeline defined in `.github/workflows/`
2. All three platforms (macOS, Linux, Windows) build in CI
3. All 16 unit tests run automatically on each platform
4. Build artifacts uploaded to GitHub releases
5. Build status badge displayed in README
6. Pipeline run time < 15 minutes per commit
7. Failed builds notify maintainer automatically

**Estimated Effort:** 2-3 hours
**Prerequisites:** Stories 2.1, 2.2 complete

---

**Story 2.4: Audio Determinism Validation**

**Story ID:** Epic-2.4

**User Story:**
As a user sharing presets across platforms,
I want identical audio output regardless of OS,
So that my music sounds the same on macOS, Linux, and Windows.

**Technical Requirements:**
- Record reference audio output on macOS
- Reproduce on Linux and verify bit-identical output
- Reproduce on Windows and verify bit-identical output
- Document any platform-specific variations
- Create automated audio validation test suite

**Acceptance Criteria:**
1. Audio output bit-identical across all platforms (within 10-6 floating point precision)
2. FFT analysis shows < 0.1dB variance across platforms
3. Phase alignment verified (phase difference < 0.1 degrees)
4. Performance metrics identical (< 0.5% variance)
5. Audio validation test suite automated
6. All tests pass in CI/CD pipeline

**Estimated Effort:** 2-3 hours
**Prerequisites:** Stories 2.1, 2.2, 2.3 complete

---

**Story 2.5: Plugin Registry Submission**

**Story ID:** Epic-2.5

**User Story:**
As a VCV Rack user,
I want to find and install DistingNT from the official Plugin Browser,
So that I can use it like any other VCV plugin without manual configuration.

**Technical Requirements:**
- Create plugin.json metadata file with proper versioning
- Define download URLs for all three platforms
- Generate SHA256 hashes for binary verification
- Validate against VCV plugin schema
- Submit plugin to official registry
- Verify one-click install functionality

**Deliverable:** `plugin.json`
```json
{
  "slug": "DistingNT",
  "name": "Disting NT Emulator",
  "description": "100% compatible Disting NT desktop emulation with community algorithm support",
  "version": "2.0.0",
  "license": "GPL-3.0",
  "author": "Neal Sanche",
  "homepage": "[URL]",
  "manual": "[URL]",
  "source": "[GitHub URL]",
  "downloads": {
    "darwin": {"url": "[macOS dylib]", "sha256": "[hash]"},
    "linux": {"url": "[Linux .so]", "sha256": "[hash]"},
    "win": {"url": "[Windows DLL]", "sha256": "[hash]"}
  }
}
```

**Acceptance Criteria:**
1. plugin.json validates against VCV plugin schema
2. All download URLs are accessible
3. SHA256 hashes verify binary integrity
4. Plugin appears in official VCV Plugin Browser
5. One-click install works for end users
6. Plugin auto-updates mechanism functional
7. Download statistics tracked successfully

**Estimated Effort:** 2-3 hours
**Prerequisites:** Stories 2.1-2.4 complete

---

**Story 2.6: User Guide & Documentation**

**Story ID:** Epic-2.6

**User Story:**
As a new user,
I want clear installation and usage instructions,
So that I can get the plugin working and understand its capabilities without support requests.

**Deliverables:**

1. **Installation Guide** (per platform)
   - macOS: One-click from Plugin Browser, manual installation (drag-drop)
   - Linux: Package management, manual .so installation
   - Windows: One-click from Plugin Browser, manual DLL installation

2. **User Guide** (2000 words)
   - Feature overview with screenshots
   - Quick start (first 5 minutes)
   - Algorithm selection and parameters
   - Preset management
   - Troubleshooting FAQ (top 10 questions)

3. **Algorithm Development Primer** (1500 words)
   - What is a Disting NT algorithm
   - Algorithm structure overview
   - API basics (without deep technical dive)
   - Getting started developing
   - Where to find resources

**Acceptance Criteria:**
1. Installation guide covers all platforms
2. Installation works for new users (no developer knowledge required)
3. User guide comprehensive and clear
4. FAQ addresses top 10 support questions
5. Algorithm primer accessible to interested developers
6. Documentation reviewed by unfamiliar user
7. Time to first use < 5 minutes

**Estimated Effort:** 3-4 hours
**Prerequisites:** Stories 2.1-2.5 complete

---

**Story 2.7: Community Infrastructure Setup**

**Story ID:** Epic-2.7

**User Story:**
As a community member,
I want to report issues, request features, and connect with other users,
So that I can contribute to the project and get help when needed.

**Technical Requirements:**
- GitHub Issues template for bug reports and features
- GitHub Discussions for general questions and algorithm showcase
- Discord server for real-time community chat
- Algorithm submission form
- Contribution guidelines document

**Deliverables:**

1. **GitHub Issues Templates**
   - Bug report template (reproduction steps, expected/actual behavior)
   - Feature request template (use case, benefit)
   - Enhancement template (detailed specification)

2. **Contribution Guidelines** (2000 words)
   - Code of conduct
   - Algorithm submission process
   - Code review criteria
   - Pull request requirements
   - Development setup instructions

3. **Discord Server**
   - #announcements channel (releases, major updates)
   - #general channel (off-topic discussion)
   - #algorithms channel (algorithm showcase and discussion)
   - #development channel (developer questions and help)
   - #support channel (user help and troubleshooting)

**Acceptance Criteria:**
1. GitHub Issues template clear and comprehensive
2. GitHub Discussions categories established and documented
3. Discord server created with 5+ channels
4. Contribution guidelines published and accessible
5. First community algorithm submission received
6. Support questions answered within 24 hours

**Estimated Effort:** 2-3 hours
**Prerequisites:** Stories 2.5-2.6 complete

---

**Story 2.8: Performance & Stability Validation**

**Story ID:** Epic-2.8

**User Story:**
As a user relying on DistingNT in live performance,
I want the plugin to be rock-solid stable with zero crashes,
So that I can use it confidently in production contexts.

**Technical Requirements:**
- Memory profiling over 8+ hour session (detect leaks)
- CPU profiling under sustained load
- Stress testing (1000+ plugin load/unload cycles)
- Edge case handling (null inputs, extreme parameters)
- Cross-platform consistency validation

**Test Suite:**
```
Stability Test Suite:
├── Memory Leak Detection
│   └── 8 hour run with monitoring (target: < 10MB growth)
├── CPU Profile
│   └── 4-8 MIDI algorithms simultaneously (target: < 5% variance)
├── Plugin Load/Unload Cycle
│   └── 1000x plugin instantiation (target: zero crashes)
├── Audio Latency
│   └── Measure across all sample rates 44.1k-96k (target: < 1ms)
└── Platform Consistency
    └── FFT comparison of audio output (target: < 0.1dB variance)
```

**Acceptance Criteria:**
1. Zero memory growth over 8 hour session (< 10MB growth acceptable)
2. CPU usage stable (variance < 5%)
3. Audio latency consistently < 1ms
4. Zero crashes in 1000+ load/unload cycles
5. Platform audio variance < 0.1dB
6. No edge cases crash the plugin
7. Performance consistent across all three platforms

**Estimated Effort:** 3-4 hours (profiling, analysis, documentation)
**Prerequisites:** Stories 2.1-2.7 complete

---

## Epic 3: Algorithm Ecosystem Expansion (Planned)

**Duration:** 4 weeks (future phase)
**Priority:** HIGH
**Business Value:** Vibrant community, 50+ algorithms, 5000+ active users
**Owner:** Neal + Community

### Epic Goal

Grow sustainable algorithm ecosystem with ported existing algorithms and engaged developer community. This epic implements the deferred algorithm ecosystem features from PRD (FR012-FR014, FR016).

### Planned Stories (outline only)

- Story 3.1: Algorithm discovery and versioning system (FR012, FR013)
- Story 3.2: One-click algorithm installation from plugin menu (FR014)
- Story 3.3: Create example algorithms for common use cases (FR016)
- Story 3.4: Port 20+ existing Disting NT algorithms
- Story 3.5: Implement algorithm rating and review system
- Story 3.6: Create algorithm developer workshop materials
- Story 3.7: Establish algorithm publication workflow (FR017 enhancement)
- Story 3.8: Grow community to 5000+ active users
- Story 3.9: Implement algorithm marketplace

---

## Story Guidelines Reference

**Story Format:**

```
**Story [EPIC.N]: [Story Title]**

**Story ID:** Epic-X.Y

**User Story:**
As a [user type],
I want [goal/desire],
So that [benefit/value].

**Acceptance Criteria:**
1. [Specific testable criterion]
2. [Another specific criterion]
3. [etc.]

**Prerequisites:** [Dependencies on previous stories]

**Estimated Effort:** [X-Y hours]

**Validation Gates:** [Executable tests to verify completion]
```

**Story Requirements:**

- **Vertical slices** - Complete, testable functionality delivery
- **Sequential ordering** - Logical progression within epic
- **No forward dependencies** - Only depend on previous work
- **AI-agent sized** - Completable in 2-4 hour focused session
- **Value-focused** - Each story delivers measurable end-user or technical value

---

**For implementation:** Use the `create-story` workflow to generate individual story implementation plans from this epic breakdown.

**Epic Owner:** Neal
**Status:** Ready for implementation (Epic 1 now, Epic 2 after Epic 1, Epic 3 after Epic 2)
**Last Updated:** 2025-10-19
