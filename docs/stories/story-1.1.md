# Story 1.1: Real-Time Parameter Synchronization

Status: Ready for Development

## Story

As a parameter-tweaking musician,
I want parameter changes to apply to audio routing instantly,
so that I experience zero lag between menu adjustment and audio effect.

## Acceptance Criteria

1. Parameter routing updates apply within 1 sample block of menu encoder change (< 0.02ms @ 96kHz)
2. Display feedback reflects parameter change instantly (already works)
3. Audio output applies new routing within 1 sample block
4. All 16 unit tests still pass
5. Zero performance regression (CPU profiling shows no change)
6. Integration test with VCV Rack confirms behavior
7. Code review approves implementation

## Tasks / Subtasks

- [ ] Modify MenuSystem::updateParameterValue() to apply routing immediately (AC: #1, #3)
  - [ ] Add parameterSystem->applyRoutingChanges(idx) call
  - [ ] Test parameter change latency measurement
- [ ] Run validation gates (AC: #4, #5, #6)
  - [ ] Build succeeds: make clean && make
  - [ ] Tests pass: make test (16/16)
  - [ ] Integration test with VCV Rack
  - [ ] CPU profiling shows no regression
- [ ] Code review (AC: #7)
  - [ ] Submit for review
  - [ ] Address feedback

## Dev Notes

### Current Issue
- ~1ms delay observed between parameter menu adjustment and audio routing application
- Parameter visibly changes on display, but audio routing applies after menu exit
- Impact: Low (imperceptible in musical contexts), but UX improvement opportunity high

### Technical Context
- Parameter system uses matrix-based routing architecture ✅
- Menu system currently batches parameter updates until menu exit
- Audio processing path reads from routing matrix (no latency issues)
- No thread contention expected (parameter system uses lock-free reads)

### Technical Implementation Options

**Option 1:** Direct routing map updates on parameter change (simplest, ~1 hour)
**Option 2:** Direct parameter reads in audio path during processing (elegant, ~2 hours)
**Option 3:** Hybrid approach - update maps immediately, cache reads (recommended, ~1.5 hours)

### Recommended Approach: Option 3 (hybrid)

```cpp
// Modify MenuSystem::updateParameterValue()
MenuSystem::updateParameterValue(int idx, float val) {
    parameters[idx] = val;
    parameterSystem->applyRoutingChanges(idx);  // Apply immediately
}
```

### Project Structure Notes

- **MenuSystem:** `vcv-plugin/src/menu/MenuSystem.hpp/cpp` (380 lines)
- **ParameterSystem:** `vcv-plugin/src/parameter/ParameterSystem.hpp/cpp` (450 lines)
- **Integration:** Modular architecture with dependency injection
- **Naming:** Follows existing camelCase conventions

### References

- [Source: docs/solution-architecture.md#Component Architecture]
- [Source: docs/epics/epic-1.md]
- [Source: vcv-plugin/src/menu/MenuSystem.hpp]
- [Source: vcv-plugin/src/parameter/ParameterSystem.hpp]

### Validation Gates

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

### Prerequisites
None (builds on existing architecture)

### Estimated Effort
1.5 hours (fits within 2-4 hour AI session including testing/validation)

## Dev Agent Record

### Context Reference

<!-- Path to story context XML will be added by story-context workflow -->

### Agent Model Used

<!-- To be filled during implementation -->

### Debug Log References

<!-- To be added during implementation -->

### Completion Notes List

<!-- To be added during implementation -->

### File List

<!-- Modified files to be listed here -->
