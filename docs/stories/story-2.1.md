# Story 2.1: Linux Build Validation

**Epic:** 2 - Multiplatform VCV Plugin Release
**Story ID:** Epic-2.1
Status: Ready for Development
**Estimated Effort:** 3-4 hours

---

## User Story

As a Linux user,
I want to download and use DistingNT plugin,
So that I can develop algorithms on my desktop regardless of OS choice.

---

## Technical Requirements

- Compile successfully on Ubuntu 20.04 LTS with gcc/clang
- Build glibc-compatible .so library
- Run all 16 unit tests on Linux
- Integration test with VCV Rack Linux build
- Performance validation (audio latency, CPU usage)

---

## Acceptance Criteria

1. Linux build succeeds with zero compiler warnings
2. All 16 unit tests pass on Linux platform
3. Plugin loads successfully in VCV Rack Linux build
4. Audio latency < 1ms measured on Linux
5. CPU profile shows < 2% overhead at idle
6. Linux-specific MIDI configuration verified (JACK/ALSA/PulseAudio)
7. Plugin registry accepts .so artifact

---

## Prerequisites

- Story 1.1 complete
- Story 1.2 complete

---

## Validation Gates

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
