# Development Worklog

Chronological record of meaningful project actions. New entries go at the top under the latest date.

Each work session/task should state objective, files changed, evidence, tests, result, unresolved questions, and exact next step.

## 2026-09-03 — M3 static contract for decompressor `0x3820`
**Objective:** Establish routine boundaries and calling contract before any C++ translation.

**Actions:**
- added an M68K disassembly probe workflow using the verified uploaded USA ROM;
- found 52 direct absolute `JSR 0x3820` call sites and no direct absolute JMPs;
- disassembled the complete candidate routine and representative callers;
- established candidate range `0x3820..0x3B3C`, with unrelated next-function prologue at `0x3B3E`;
- identified two decompression paths selected by `source[2]`;
- confirmed A0 as compressed source and A1 as destination using multiple callers;
- confirmed A1 returns as the end-of-output pointer because caller `0x3A7FE` computes `A1 - initial_A1` after return;
- documented literal, repeated-byte and backreference behavior without assigning an unsupported external algorithm name.

**Evidence:**
- callers at `0xC394`, `0xD3C8`, `0x3A7FE`, and `0x3E820` load ROM addresses into A0 and RAM buffers into A1;
- routine writes through `A1@+`, reads through `A0@+`, and performs backreferences from previous output;
- returns occur at `0x38CE` and `0x3A24`; refill/helper branches extend to `0x3B3A`; unrelated prologue begins at `0x3B3E`;
- routine contains no JSR/BSR and is self-contained apart from memory access.

**Tests/build:** M3 probe workflows completed successfully against the exact reference ROM fingerprint. Static evidence is now reproducible in GitHub Actions.

**Result:** Major static contract is established. C++ translation remains intentionally blocked by project rules until an independent dynamic reference trace is captured.

**Unresolved:** A0 returned-advanced behavior is HIGH but not yet independently observed; exact output fingerprints for representative streams are not yet captured.

**Exact next step:** Execute the original routine bytes in an isolated M68K harness against representative compressed streams and record source-consumed length, output length, and SHA-256 without committing raw extracted assets.

## 2026-09-03 — M2 canonical ROM identity verified — DONE
**Objective:** Pin down the exact binary reference used for reverse engineering.

**Actions:**
- accepted ADR-0005: USA retail `Beyond Oasis` is the canonical engineering reference while final C++ remains region-independent;
- implemented Mega Drive header parsing, Sega checksum, CRC32, SHA-1, SHA-256 and ROM classification;
- added synthetic tests, CLI reporting, general CI and dedicated reference-ROM verification;
- fixed an actual CI-detected C++ most-vexing-parse error in `rom.cpp`;
- verified the user-uploaded `Beyond Oasis (USA).md` inside GitHub Actions.

**Confirmed fingerprint:**
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`;
- detector status `SUPPORTED`.

**Tests/build:** General CI passed after the loader fix; reference-ROM workflow passed and asserted `SUPPORTED`.

**Result:** M2 completed with executable evidence; M3 activated.

**Exact next step:** Establish and verify the `0x3820` decompressor contract.

## 2026-09-03 — Governance enforcement completed
**Objective:** Make the project resistant to scope drift and ensure rules are enforceable, not merely descriptive.

**Actions:**
- added `AGENTS.md`, architecture/vision/file-map/roadmap/rules/decision/research/worklog documents;
- added task/session handoff files;
- added `tests/check_file_limits.cmake` and CTest enforcement of <=500 lines per file.

**Result:** Project direction, scope, file-size policy, documentation obligations, and AI handoff process are explicit and enforceable.

## 2026-09-03 — Initial C++ bootstrap
**Objective:** Create a minimal native C++ project suitable for incremental Beyond Oasis translation.

**Actions:**
- created C++20/CMake build;
- added ROM loader/header parsing;
- added minimal memory bus and VDP/VRAM scaffolding;
- recorded known hardware/routine addresses;
- translated initial tile-copy helper behavior;
- added smoke test.

**Result:** Repository has a compilable architectural starting point without requiring ROM bytes in source/tests.

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
Exact next step:
```
