# Story 2.2: Windows Build Validation

**Epic:** 2 - Multiplatform VCV Plugin Release
**Story ID:** Epic-2.2
Status: Ready for Development
**Estimated Effort:** 3-4 hours

---

## User Story

As a Windows user,
I want to download and use DistingNT plugin,
So that I can participate in the community algorithm ecosystem regardless of OS.

---

## Technical Requirements

- Compile successfully on Windows 10/11 with MSVC 2019+
- Build Windows-compatible DLL library
- Implement Windows-specific audio I/O (WASAPI, WinMM)
- Run all 16 unit tests on Windows
- Integration test with VCV Rack Windows build
- Performance validation (audio latency, CPU usage)

---

## Acceptance Criteria

1. Windows build succeeds with zero compiler warnings
2. All 16 unit tests pass on Windows
3. Plugin loads successfully in VCV Rack Windows build
4. Audio latency < 1ms measured on Windows
5. CPU profile shows < 2% overhead at idle
6. Windows audio device selection verified (WinMM, WASAPI, ASIO optional)
7. Plugin registry accepts DLL artifact

---

## Prerequisites

- Story 1.1 complete
- Story 1.2 complete

---

## Validation Gates

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
