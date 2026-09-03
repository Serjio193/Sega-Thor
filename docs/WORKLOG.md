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

## 2026-09-03 — M2 canonical ROM identity implementation
**Objective:** Pin down the binary reference used for reverse engineering and make ROM identification explicit/reproducible.

**Actions:**
- accepted ADR-0005: USA retail `Beyond Oasis` is the canonical engineering reference; final C++ game model remains region-independent;
- added `RomIdentity`, Mega Drive header parsing, Sega checksum validation, CRC32, SHA-1, and SHA-256;
- added classification states `SUPPORTED`, `KNOWN_UNSUPPORTED`, `MODIFIED`, `UNKNOWN`;
- configured USA `SUPPORTED` matching to require size `3145728`, CRC32 `C4728225`, and SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- recorded Europe CRC32 `1110B0DB` as a known unsupported secondary revision;
- added CLI identity/fingerprint reporting;
- added synthetic hash/header/checksum tests;
- added GitHub Actions CI for CMake build + CTest;
- updated reverse-engineering ledger, file map, project state, and task state.

**Files changed:** `docs/DECISIONS.md`, `PROJECT_STATE.md`, `TASK.md`, `src/core/rom_identity.hpp`, `src/core/rom_identity.cpp`, `src/main.cpp`, `tests/rom_identity_test.cpp`, `CMakeLists.txt`, `.github/workflows/ci.yml`, `docs/REVERSE_ENGINEERING.md`, `docs/FILE_MAP.md`, `docs/WORKLOG.md`.

**Evidence:**
- current MAME metadata marks the USA ROM as `good`, size `3145728`, CRC32 `c4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- GameHacking.org independently reports USA size 3M and CRC32 `C4728225`;
- public `smd_beyondoasis` work targets the USA good dump.

**Tests/build:**
- local reconstructed bootstrap compiled successfully with the first ROM identity implementation;
- smoke and ROM identity tests passed (`2/2`);
- standard vectors verified CRC32 `123456789` and SHA-256 `abc` before SHA-1 strengthening;
- SHA-1 standard-vector test was then added to repository;
- actual branch-level GitHub Actions result still needs to be observed before M2 is marked DONE.

**Result:** Canonical reference policy and implementation are in place. M2 is functionally implemented but remains ACTIVE until current branch CI is observed green.

**Unresolved:** SHA-256 of the confirmed clean USA ROM is not yet independently pinned; it is diagnostic and not required for current exact matching because confirmed SHA-1 is available.

**Exact next step:** Observe branch CI. If green, mark M2 DONE and begin M3 by establishing exact routine boundaries and call contract for USA address `0x00003820`.

## 2026-09-03 — Governance enforcement completed
**Objective:** Make the project resistant to scope drift and ensure rules are enforceable, not merely descriptive.

**Actions:**
- added `docs/TASK_TEMPLATE.md` for task/AI handoff;
- added `tests/check_file_limits.cmake`;
- registered the line-limit check with CTest;
- updated README to make governance documents mandatory entry points;
- updated canonical file map;
- marked M1 governance milestone complete and M2 active.

**Files changed:** `README.md`, `CMakeLists.txt`, `docs/TASK_TEMPLATE.md`, `tests/check_file_limits.cmake`, `docs/FILE_MAP.md`, `docs/ROADMAP.md`, `docs/WORKLOG.md`.

**Evidence/reasoning:** The owner requested persistent rules, architecture/file map, <=500 lines per file, documented development steps, idea/roadmap descriptions, and safeguards against AI development drift.

**Tests/build:** Added `project_file_line_limit` CTest. It checks C/C++/Markdown/CMake project files and fails if any checked file exceeds 500 lines. No runtime behavior changed in this documentation/governance pass.

**Result:** M1 is complete. Project direction, scope, file-size policy, documentation obligations, and AI handoff process are now explicit. The next active milestone is M2 ROM revision identification.

**Unresolved:** Exact canonical Beyond Oasis ROM revision/hash remains to be identified.

**Exact next step:** Implement and document ROM revision identification without starting `0x3820` translation until the reference revision is pinned down.

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
