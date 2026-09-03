# Development Worklog

Chronological record of meaningful project actions. New entries go at the top under the latest date.

Each work session/task should state:
- objective;
- files changed;
- evidence/reasoning;
- tests/build performed;
- result;
- unresolved questions;
- exact next step.

## 2026-09-03 — Governance/documentation pass
**Objective:** Prevent project drift and make work reproducible for human and AI contributors.

**Actions:**
- added mandatory `AGENTS.md` workflow;
- documented project vision and non-goals;
- documented architecture/layer boundaries;
- added canonical file map;
- added ordered development roadmap;
- added coding/reverse-engineering rules;
- added decision log and reverse-engineering ledger;
- established 500-line hard limit for each source/document file;
- required documentation updates for every meaningful change.

**Result:** Project direction is now explicitly constrained. Story of Thor 2 and remaster features remain deferred. The first reverse-engineering target remains graphics decompression routine `0x00003820` after ROM revision identification.

**Tests/build:** Documentation-only changes; existing code build status is unchanged by these files.

**Unresolved:** Exact supported reference ROM revision/hash still needs to be established.

**Next step:** Complete M1 documentation integration in README, then begin M2 ROM revision identification.

## 2026-09-03 — Initial C++ bootstrap
**Objective:** Create a minimal native C++ project suitable for incremental Beyond Oasis translation.

**Actions:**
- created C++20/CMake build;
- added ROM loader/header parsing;
- added minimal 24-bit memory bus/work RAM scaffold;
- added minimal VDP/VRAM scaffold;
- recorded known addresses (`0xC00000`, `0xC00004`, `0xFF0000`, `0x00003820`);
- translated initial tile-copy helper behavior;
- added smoke test.

**Result:** Repository has a compilable architectural starting point without ROM/assets.

**Next step at the time:** translate graphics decompression routine. Governance work was inserted before that to stabilize development process.

## Entry template
```text
## YYYY-MM-DD — Short task name
Objective:

Actions:

Files changed:

Evidence:

Tests/build:

Result:

Unresolved:

Next step:
```
