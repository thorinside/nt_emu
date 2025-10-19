# Story 1.2: Consolidate Technical Decisions Documentation

**Epic:** 1 - Finish DistingNT Feature Emulation
**Story ID:** Epic-1.2 (formerly Story 5.6)
Status: Ready for Development
**Estimated Effort:** 2.5 hours

---

## User Story

As a new developer joining the team,
I want to understand all major architectural decisions and their rationale,
So that I can make informed decisions when extending the system without reading 25 PRPs.

---

## Current Issue

- Architectural decisions scattered across CLAUDE.md, 25 PRPs, and architecture documentation
- New developer onboarding time ~1 day (reading, analysis, pairing)
- No centralized reference for future decision-making

---

## Deliverable

**Document:** `technical-decisions.md` (1000-1500 words)

### Content Sections

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

### Additional Sections

- **Future Decision Template**: How to propose and evaluate new architectural decisions
- **Decision Review Process**: Criteria for accepting/rejecting architectural changes
- **Anti-Patterns We Avoided**: Common mistakes and why we rejected them

---

## Acceptance Criteria

1. Document created at `docs/technical-decisions.md`
2. All 5+ major decisions documented with clear structure
3. Each decision includes: rationale, trade-offs, code references, validation approach
4. Code examples included where helpful (e.g., architectural patterns used)
5. Future decision template provided for extending document
6. Verified readable by developer unfamiliar with project
7. Developer feedback: ≥1 person successfully onboarded using doc in < 1 hour

---

## Prerequisites

None (pure documentation)

---

## Validation Gates

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

**AI Session Compatibility:** ✅ Fits within 2-4 hour session (documentation only)
