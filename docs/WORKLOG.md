# Development Worklog

Chronological record of meaningful project actions. New entries go at the top.

Each task records objective, actions, evidence, tests, result, unresolved questions and exact next step.

## 2026-09-03 — M7 world/map/collision research started
**Objective:** Identify raw room/map/collision data before creating native structures.

**Actions:**
- activated M7 after verified completion of deterministic runtime/input;
- inspected the public `hansbonini/smd_beyondoasis` reverse-engineering repository for map-related evidence;
- confirmed its `tilemap/` directory contains fixed UI/screen tilemaps (`game_over`, `name_screen`, `save_screen`), not documented world-room formats;
- searched broader public material; available map images/walkthroughs are useful visual references but do not establish ROM data layout.

**Evidence:** Existing public translation work gives useful address/text/graphics hooks but does not yet provide a proven world-map structure suitable for native implementation.

**Result:** No semantic room structure has been invented. M7 remains ACTIVE.

**Unresolved:** Exact room table, layer dimensions, collision representation and loader routines remain unknown.

**Exact next step:** Probe the canonical USA binary for a verified room-loading path and pointer tables, then document raw fields before adding `src/game/world/` code.

## 2026-09-03 — M6 deterministic runtime/input — DONE
**Objective:** Establish portable one-frame stepping and controller snapshots before gameplay translation.

**Actions:**
- added `src/core/runtime.hpp/.cpp`;
- defined portable Genesis-style button masks and two controller ports;
- added explicit integer frame index and `FrameContext`;
- added `RuntimeLoop::step()` with one update per explicit call;
- added replay-style deterministic tests;
- detected and removed a duplicate experimental `game/runtime` API so one source of truth remains.

**Evidence/tests:**
- identical input sequences produce identical recorded traces;
- changing one frame of input changes the trace;
- no wall-clock, SDL or OS input API is used by core runtime;
- GitHub Actions build/test completed successfully after duplicate removal.

**Result:** M6 completed; M7 activated.

**Exact next step:** Establish room/map/collision raw formats from original evidence.

## 2026-09-03 — M5 narrow VDP model — DONE
**Objective:** Replace the placeholder video scaffold with only the Mega Drive state needed for reconstruction.

**Actions:**
- modeled 64 KiB VRAM, 128-byte CRAM and 80-byte VSRAM;
- added bounded byte-span and big-endian word access;
- decoded standard tile pattern-name fields: index, palette, priority, H/V flip;
- added thin plane-cell and raw four-word sprite attribute representations;
- documented explicitly unsupported emulator semantics in `docs/VDP_MODEL.md`;
- added/registered VDP tests.

**Evidence/tests:** Bounds, word storage, tile attributes and sprite fields are covered by synthetic tests; current CI build/test is green.

**Result:** M5 completed without adding a full VDP emulator or renderer dependency.

**Exact next step:** Deterministic runtime/input skeleton.

## 2026-09-03 — M4 local asset inspector — DONE
**Objective:** Inspect graphics from a user-owned ROM using the verified native decompressor.

**Actions:**
- added Genesis 4bpp tile decoding and CRAM-to-RGB conversion;
- added `oasis_inspect` local ROM tool;
- added PGM index output and optional palette-based PPM output;
- ignored generated outputs through `.gitignore`;
- added ROM-backed inspector verification.

**Evidence/tests:**
- reference source `0x16943C` consumes `1217` compressed bytes and produces `3072` bytes;
- inspector interprets this as `96` complete tiles and a default `128x48` sheet;
- CI verifies PGM header and exact payload size;
- five CTests passed at M4 closure.

**Result:** M4 completed; no extracted commercial assets are committed.

**Exact next step:** Narrow VDP state model.

## 2026-09-03 — M3 native graphics decompressor — DONE
**Objective:** Translate original 68000 decompression routine `0x3820` into verified native C++.

**Actions:**
- found 52 direct absolute `JSR 0x3820` callers and no direct absolute JMPs;
- established routine range `[0x3820, 0x3B3E)`;
- confirmed A0 source and A1 destination/end-pointer roles;
- identified two formats selected by `source[2]`;
- executed the original routine bytes in an isolated M68K/QEMU harness to obtain reference behavior;
- translated both paths to C++;
- detected and corrected two errors in an earlier implementation (format-A extension handling and format-B repeated-byte path).

**Reference evidence:**
- format A `0x16943C`: consumed `1217`, output `3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`;
- format B `0x1894EA`: consumed `112`, output `128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

**Tests/build:** Native C++ matches original source-consumed count, output length and output SHA-256 for both formats; ordinary CI and reference workflow pass.

**Result:** M3 completed with differential evidence.

**Exact next step:** Build local asset inspection tooling.

## 2026-09-03 — M2 canonical ROM identity verified — DONE
**Objective:** Pin down the exact binary reference used for reverse engineering.

**Actions:**
- accepted USA retail `Beyond Oasis` as address/disassembly reference while keeping final C++ region-independent;
- implemented header/checksum/CRC32/SHA-1/SHA-256 identification and ROM classification;
- added synthetic tests and GitHub reference-ROM verification;
- fixed a CI-detected C++ construction error in the ROM loader.

**Confirmed fingerprint:**
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`;
- detector status `SUPPORTED`.

**Result:** M2 completed with executable evidence.

## 2026-09-03 — Governance enforcement — DONE
**Objective:** Make project direction recoverable and resistant to scope drift.

**Actions:**
- added mandatory AI/contributor rules, architecture/vision/file-map/roadmap/decision/research/work logs;
- added `PROJECT_STATE.md`, `TASK.md` and task template;
- enforced <=500 lines per source/project-document file using CTest.

**Result:** Project direction, completion criteria and documentation obligations are explicit.

## 2026-09-03 — Initial C++ bootstrap — DONE
**Objective:** Create a minimal native C++ starting point.

**Actions:** C++20/CMake project, ROM loader, memory/VDP scaffold, known symbol table, tile-copy compatibility helpers and smoke test.

**Result:** Compilable native bootstrap established.

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
