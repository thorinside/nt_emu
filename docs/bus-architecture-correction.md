# Bus Architecture Correction

**Date:** 2025-10-19
**Issue:** Documentation incorrectly specified fixed signal-type assignments for buses
**Status:** CORRECTED

---

## The Problem

Previous documentation incorrectly claimed:
```
Buses 0-3:   Audio inputs
Buses 4-11:  CV modulation
Buses 12-19: Reserved
Buses 20-23: Audio outputs
Buses 24-27: CV outputs
```

**This was FALSE.** The Disting NT hardware specification does not assign signal types to specific buses.

---

## The Truth: NT Hardware Specification

**28 Buses Total** (0-indexed in code as buses 0-27, hardware numbered 1-28):

### Bus Categories by Function
- **Buses 0-11** (NT 1-12): Hardware inputs (mapped from VCV Rack input jacks)
- **Buses 12-19** (NT 13-20): Hardware outputs (mapped to VCV Rack output jacks)
- **Buses 20-27** (NT 21-28): Auxiliary buses for inter-algorithm communication

### Critical Understanding
**All buses carry identical voltage signals (-12V to +12V)**
- Any bus can carry audio, CV, or modulation signals
- Signal type is determined by **algorithm usage**, not bus number
- Buses are voltage carriers, not signal-type specific
- Algorithms decide how to use each bus

---

## Code Verification

### BusSystem.hpp Implementation

```cpp
// Route 12 inputs to buses 0-11 (0-based indexing)
for (int i = 0; i < 12; i++) {
    setBus(i, currentSample, voltage);  // Buses 0-11: inputs
}

// Route buses 12-19 to 8 outputs
for (int i = 0; i < 8; i++) {
    float busValue = getBus(12 + i, currentSample);  // Buses 12-19: outputs
}

// Buses 12-27 are used for internal algorithm processing
// They will be populated by the algorithms themselves
```

**Buses 20-27** are not explicitly routed by the emulator - they're available for algorithms to use internally or to pass signals between algorithms.

---

## NT API Specification

From `/emulator/include/distingnt/api.h:393-398`:

```c
/*
 * Primary audio rendering call.
 * busFrames gives access to all 28 busses.
 * numFramesBy4 is the number of frames to render, divided by 4.
 * The busses are arranged one after the other.
 * i.e. numFrames of bus 0, then numFrames of bus 1, etc.
 */
void (*step)(_NT_algorithm *self, float *busFrames, int numFramesBy4);
```

**Key Point:** The API provides "access to all 28 busses" with **no mention of signal-type assignments**.

---

## Corrected Documentation

### Files Updated (2025-10-19)
1. ✅ `/docs/solution-architecture.md` - Audio Path section
2. ✅ `/docs/solution-architecture.md` - Key Architecture Decisions table
3. ✅ `/docs/architecture-decisions.md` - ADR-004: 28-Bus Audio Architecture
4. ✅ `/docs/COMPREHENSIVE-PROJECT-STATUS.md` - System 4: Audio Routing
5. ✅ `/docs/epics.md` - Story 1.2 technical decisions

### Corrected Audio Path Flow

```
VCV Audio Inputs (±5V)
    ↓
[Level Scaling to ±1.0]
    ↓
BusSystem (28-bus flexible routing)
    ├─ Buses 0-11:  NT hardware inputs (mapped from VCV inputs)
    ├─ Buses 12-19: NT hardware outputs (mapped to VCV outputs)
    └─ Buses 20-27: Auxiliary buses for inter-algorithm communication

    Note: All buses carry identical voltage signals (-12V to +12V)
    Any bus can carry audio, CV, or modulation - algorithms decide usage
    ↓
NT Plugin (Process Block - 4 samples @ variable rate: 44.1-96kHz)
    ↓
[Level Scaling to ±5V]
    ↓
VCV Audio Outputs
```

---

## Why This Matters

### For Algorithm Developers
- ✅ **Freedom:** Use any bus for any signal type your algorithm needs
- ✅ **Flexibility:** Route signals creatively between processing stages
- ✅ **Inter-algorithm:** Use auxiliary buses (20-27) to pass signals between algorithms

### For Documentation Accuracy
- ✅ **Truth:** Documentation now matches NT hardware specification
- ✅ **Clarity:** Developers understand they control signal routing
- ✅ **Correctness:** No false constraints on bus usage

### For System Architecture
- ✅ **Simplicity:** No need to enforce signal-type restrictions
- ✅ **Performance:** Algorithms optimize their own bus usage
- ✅ **Compatibility:** Perfect match with NT hardware behavior

---

## Lessons Learned

**AI agents sometimes invent structure where none exists.**

The previous documentation created false bus assignments that seemed logical but were not grounded in the actual NT hardware specification. This highlighted the importance of:

1. **Verifying claims against source material** (NT API documentation)
2. **Checking code implementation** (BusSystem.hpp)
3. **Questioning inherited assumptions** (bus allocations appeared in multiple docs)
4. **Trusting domain experts** (Neal's correction about flexible bus usage)

---

## References

- NT API Specification: `/emulator/include/distingnt/api.h`
- Code Implementation: `/vcv-plugin/src/dsp/BusSystem.hpp`
- Architecture Decisions: `/docs/architecture-decisions.md` (ADR-004)
- Solution Architecture: `/docs/solution-architecture.md`

---

**Status:** All documentation corrected as of 2025-10-19
**Validator:** Winston (Architect Agent)
**Verified By:** Neal Sanche (Domain Expert)
