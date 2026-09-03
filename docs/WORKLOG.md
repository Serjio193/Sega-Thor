# Development Worklog

Chronological record of meaningful project actions. New entries go at the top.

Each task records objective, actions, evidence, tests, result, unresolved questions and exact next step.

## 2026-09-03 — M7 world/map/collision foundations — DONE
**Objective:** Establish a verified room/screen loading path and tested collision/world-grid primitives before player translation.

**Actions:**
- resolved screen dispatcher `0xC8F0` through group table `0xC92C` to 26-byte descriptors and added ROM-backed native descriptor loading;
- confirmed auxiliary byte-grid descriptors at `0xFF1766` / `0xFF1770` and 8-pixel world-to-cell addressing;
- identified `0x9C40` as a single-cell grid query;
- identified `0x9BF2` as OR/AND footprint aggregation across covered cells and translated it to `ByteGridView`;
- recovered terrain tables `0x96E8` / `0x96F8` and their class mapping role;
- identified `0x938E` as the directional terrain movement gate: carry set blocks, carry clear allows;
- added portable terrain-gate semantics and synthetic tests;
- reduced completed M7 research workflow to manual dispatch only.

**Evidence/tests:**
- confirmed descriptor mappings include `0x0009 -> 0x02CF82`, `0x000C -> 0x02D3E8`, `0x0704 -> 0x032144`, `0x0705 -> 0x03285C`;
- movement loop calls `0x938E` before committing movement and branches on carry to the stop path;
- byte-grid, footprint and terrain-gate synthetic tests pass;
- final `build-test`, reference-ROM workflow and M7 probe all completed successfully.

**Result:** M7 acceptance criteria are satisfied. M8 player-system work is activated.

**Exact next step:** identify the player entity/update entry point and trace controller input into the movement cluster and confirmed terrain gate.

## 2026-09-03 — M7 indexed resources and screen descriptor pattern
**Objective:** Move from generic world-data searching to a reproducible original loading path without inventing room semantics.

**Actions:**
- added ROM-backed `m7-world-probe` workflow;
- identified indexed compressed-resource table at `0x05CE96` with 108 entries including a zero entry and 107 dense ROM pointers;
- established reader routine `0xD3B2`: incoming `D0` is multiplied by four, used to select a pointer, and the selected stream is decompressed to `0xFF2FA8`;
- found seven direct `D3B2` calls using immediate resource IDs `3`, `4`, `35`, `36`, and `87`;
- located the ROM developer screen-name inventory beginning at `0x05DB4D`;
- found repeated 26-byte blocks immediately before several setup sequences calling `0xD406`;
- observed that `D406` reads a structured input through `A1` through at least offset `+24`;
- launched a focused probe for pointer tables referencing candidate 26-byte descriptor starts.

**Evidence:**
- `0xD3B2` performs `A0=0x5CE96`, `D0<<=2`, pointer lookup, `JSR 0x3820`;
- caller pairs `3/4` and `35/36` pass `D1=0x4000/0x5000`, followed by original VDP/DMA activity, indicating these selected resources are likely graphics/data banks rather than the room record itself;
- index 87 is separately loaded by another setup path;
- developer strings include `VILLAGE`, `ECAPITAL`, `HARBOR`, numbered dungeon screens, boss labels, and `14-01 KING`;
- candidate descriptor spans such as `0x2CF82..0x2CF9B` and `0x2D3E8..0x2D401` are exactly 26 bytes, matching `D406` accesses through offset `+24`.

**Result:** A stable scene-resource loader and a strong candidate screen-descriptor shape were established for later closure evidence.

## 2026-09-03 — M7 world/map/collision research started
**Objective:** Identify raw room/map/collision data before creating native structures.

**Actions:**
- activated M7 after verified completion of deterministic runtime/input;
- inspected public reverse-engineering work for map-related evidence;
- confirmed its documented tilemaps are fixed UI/screen tilemaps rather than proven world-room formats.

**Result:** No semantic room structure was invented; binary-first probing was selected.

## 2026-09-03 — M6 deterministic runtime/input — DONE
**Objective:** Establish portable one-frame stepping and controller snapshots before gameplay translation.

**Actions:**
- added `src/core/runtime.hpp/.cpp`;
- defined portable Genesis-style button masks and two controller ports;
- added explicit integer frame index and `FrameContext`;
- added `RuntimeLoop::step()` with one update per explicit call;
- added replay-style deterministic tests;
- detected and removed a duplicate experimental `game/runtime` API so one source of truth remains.

**Evidence/tests:** identical input sequences produce identical recorded traces; changing one frame changes the trace; no wall-clock, SDL or OS input API is used; CI passed.

**Result:** M6 completed; M7 activated.

## 2026-09-03 — M5 narrow VDP model — DONE
**Objective:** Replace the placeholder video scaffold with only the Mega Drive state needed for reconstruction.

**Actions:** modeled 64 KiB VRAM, 128-byte CRAM and 80-byte VSRAM; bounded byte/word access; tile pattern-name fields; minimal plane/sprite data; explicit unsupported emulator semantics; tests.

**Result:** M5 completed without full VDP emulation or renderer dependency.

## 2026-09-03 — M4 local asset inspection tool — DONE
**Objective:** Inspect graphics from a user-owned ROM using the verified native decompressor.

**Actions:** Genesis 4bpp tile decoding, CRAM-to-RGB conversion, `oasis_inspect`, PGM/PPM output, ignored generated outputs, ROM-backed inspector verification.

**Result:** M4 completed; no extracted commercial assets are intentionally part of the native code model.

## 2026-09-03 — M3 native graphics decompressor — DONE
**Objective:** Translate original 68000 decompression routine `0x3820` into verified native C++.

**Evidence:** format A `0x16943C`: `1217 -> 3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`; format B `0x1894EA`: `112 -> 128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

**Result:** Native C++ matches original traces; M3 completed.

## 2026-09-03 — M2 canonical ROM identity verified — DONE
**Confirmed fingerprint:** size `3145728`; CRC32 `C4728225`; SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`; SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**Result:** M2 completed with executable evidence.

## 2026-09-03 — Governance enforcement — DONE
Mandatory AI workflow, architecture/vision/file-map/roadmap/decision/research/work logs, `PROJECT_STATE.md`, `TASK.md`, and <=500-line CTest enforcement established.

## 2026-09-03 — Initial C++ bootstrap — DONE
C++20/CMake project, ROM loader, memory/VDP scaffold, known symbol table, tile-copy compatibility helpers and smoke test established.

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
