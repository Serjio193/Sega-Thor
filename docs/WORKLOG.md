# Development Worklog

Chronological record of meaningful project actions. New entries go at the top.

Each task records objective, actions, evidence, tests, result, unresolved questions and exact next step.

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

**Result:** A stable scene-resource loader and a strong candidate screen-descriptor shape are established. `0x5CE96` is deliberately not called a room table; its exact graphics/data role remains to be classified. M7 remains ACTIVE.

**Unresolved:** Need direct pointer/dispatcher evidence connecting 26-byte descriptors to developer screen identities, then collision data/query evidence.

**Exact next step:** Locate table(s) of pointers to candidate descriptor starts and map at least one descriptor to a named developer screen before implementing native world structures.

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

**Evidence/tests:**
- identical input sequences produce identical recorded traces;
- changing one frame of input changes the trace;
- no wall-clock, SDL or OS input API is used by core runtime;
- GitHub Actions build/test completed successfully after duplicate removal.

**Result:** M6 completed; M7 activated.

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

## 2026-09-03 — M3 native graphics decompressor — DONE
**Objective:** Translate original 68000 decompression routine `0x3820` into verified native C++.

**Actions:**
- found 52 direct absolute `JSR 0x3820` callers and no direct absolute JMPs;
- established routine range `[0x3820, 0x3B3E)`;
- confirmed A0 source and A1 destination/end-pointer roles;
- identified two formats selected by `source[2]`;
- executed the original routine bytes in an isolated M68K/QEMU harness to obtain reference behavior;
- translated both paths to C++;
- corrected format-A extension handling and format-B repeated-byte behavior using oracle evidence.

**Reference evidence:**
- format A `0x16943C`: consumed `1217`, output `3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`;
- format B `0x1894EA`: consumed `112`, output `128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

**Result:** Native C++ matches both original traces; M3 completed.

## 2026-09-03 — M2 canonical ROM identity verified — DONE
**Objective:** Pin down the exact binary reference used for reverse engineering.

**Confirmed fingerprint:**
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`;
- detector status `SUPPORTED`.

**Result:** M2 completed with executable evidence.

## 2026-09-03 — Governance enforcement — DONE
**Objective:** Make project direction recoverable and resistant to scope drift.

**Actions:** mandatory AI workflow, architecture/vision/file-map/roadmap/decision/research/work logs, `PROJECT_STATE.md`, `TASK.md`, and <=500-line CTest enforcement.

**Result:** Project direction and completion criteria are explicit.

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
