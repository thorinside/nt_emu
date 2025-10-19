# Story 2.5: Plugin Registry Submission

**Epic:** 2 - Multiplatform VCV Plugin Release
**Story ID:** Epic-2.5
Status: Ready for Development
**Estimated Effort:** 2-3 hours

---

## User Story

As a VCV Rack user,
I want to find and install DistingNT from the official Plugin Browser,
So that I can use it like any other VCV plugin without manual configuration.

---

## Technical Requirements

- Create plugin.json metadata file with proper versioning
- Define download URLs for all three platforms
- Generate SHA256 hashes for binary verification
- Validate against VCV plugin schema
- Submit plugin to official registry
- Verify one-click install functionality

---

## Deliverable

`plugin.json`:

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

---

## Acceptance Criteria

1. plugin.json validates against VCV plugin schema
2. All download URLs are accessible
3. SHA256 hashes verify binary integrity
4. Plugin appears in official VCV Plugin Browser
5. One-click install works for end users
6. Plugin auto-updates mechanism functional
7. Download statistics tracked successfully

---

## Prerequisites

- Stories 2.1-2.4 complete
