# Development Worklog

Chronological record of meaningful project actions. New entries go at the top.

Each task records objective, actions, evidence, tests, result, unresolved questions and exact next step.

## 2026-09-03 — M8 state-2/state-4 branch closure
**Objective:** Validate the stop and turning branches reached through the player state dispatcher before modelling any presentation state.

**Evidence:**
- state `2` enters at `0x62E4`; no direction branches to `0x62CC`, which clears `+0x4E/+0x52`, writes `+0x2A=0` and state `+0x04=0`; cleanup of `+0x72/+0x76` belongs to shared movement;
- state `4` enters at `0x6516`, maps the current direction through `0x83D4`, and uses `+0x17` plus the normalized input to select an axis;
- the valid state-4 path accumulates deltas in `+0x72/+0x76`; the no-input path compares `FF197E` against `6` and writes state `+0x04=0x000C` after the threshold.

**Actions:** Added ROM byte-oracle assertions for the state-2 stop block and state-4 turning/timeout branches. Recorded only raw offsets and observed transitions; presentation meanings of `+0x2A`, `+0x26` and related callbacks remain unknown.

**Result:** State-2 stopping and state-4 directional accumulation are closed at the branch level. The native slice now models those raw state transitions and leaves presentation callbacks outside the API.

**Exact next step:** verify the native state driver against the shared movement consumer and isolate the remaining state-4 slowdown context around `0x64C4`.

## 2026-09-03 — M8 shared movement consumer integration
**Objective:** Verify that the native state driver hands accumulated deltas to the same terrain-gated consumer and preserve the footprint mask needed by `0x64C4`.

**Actions:** Added an integration test covering state-4 axis selection through `try_move`, terrain aggregation and accumulated-delta cleanup. Preserved the aggregate OR mask as raw entity `+0x6F` and added a ROM oracle for the `0x64C4` branch structure.

**Result:** State-driver output reaches the shared movement consumer and the native player records the confirmed footprint mask. The remaining external flags at `FF1985`, `FF1984` and `FF16F1` are isolated as slowdown context, not presentation state.

**Exact next step:** determine the lifecycle and writers of the three `0x64C4` global flags before adding a slowdown context object.

## 2026-09-03 — M8 velocity-adjust context isolation
**Objective:** Determine the writers and ownership boundary of the external flags consumed by `0x64C4`.

**Evidence:**
- `FF1985` has direct writes in event/control paths at `0x56F2`, `0x57C6` and `0x7E50`, while player update and `0x64C4` only read it;
- `FF1984` is cleared/set by the active-entity scans at `0x2D220..0x2D23A` and `0x2F250..0x2F286`, with the main entity selected through `FF19E8`;
- bit 4 of `FF16F1` is read at `0x64D8`, but no direct bit-4 write appears among the scanned references.

**Result:** The three conditions are external timing/context inputs, not player presentation fields. A future context object can expose them without assigning gameplay names that the ROM does not establish.

**Exact next step:** add a small `VelocityAdjustContext` and differential unit tests for the three confirmed `0x64C4` branch outcomes.

## 2026-09-03 — M8 velocity-adjust context implementation
**Objective:** Add the confirmed `0x64C4` branch outcomes without assigning semantics to external RAM flags.

**Actions:** Added an optional raw `VelocityAdjustContext` carrying `FF1985`, `FF1984`, bit 4 of `FF16F1` and footprint `+0x6F`. Added unit coverage for bypass, half-both and Y-only-half outcomes, and connected the context to state-4 retained-axis accumulation.

**Result:** The native driver reproduces the isolated `0x64C4` arithmetic when a caller supplies external context; the default path remains unchanged when context is unavailable.

**Exact next step:** verify frame-boundary ordering between state-4 context sampling, footprint update and shared movement consumption.

## 2026-09-03 — M8 frame-boundary ordering
**Objective:** Verify that state-4 velocity context is sampled before the shared movement consumer updates the footprint mask.

**Actions:** Added a two-frame test: the first state-4 update uses the old `footprint_any_bits`, then `try_move` records the new aggregate mask; the second update consumes that new mask.

**Result:** The native order matches the recovered loop: player update reads prior-frame context, shared movement commits/cleans accumulated deltas, and the footprint OR mask becomes available to the next frame.

**Exact next step:** finish M8 acceptance review and keep the ROM reference workflow local-only.

## 2026-09-03 — Windows test runtime fix
**Objective:** Make the documented CTest command work from a clean PowerShell session.

**Actions:** Added a Windows GNU/Clang CMake test environment that supplies the active compiler's runtime DLL directory to CTest, and documented the behavior in the README.

**Result:** Debug and Release CTest runs pass 11/11 without manually editing `PATH`; the previous `libwinpthread-1.dll` dialog is no longer produced by the test workflow.

**Exact next step:** finish M8 acceptance review and keep the ROM reference workflow local-only.

## 2026-09-03 — M8 player update dispatch closure
**Objective:** Close the indirect player update path and preserve its confirmed state transition in the native slice.

**Evidence:**
- main loop `0x8B22` calls player update `0x557A` before shared movement `0x8E90`;
- `0x557A` selects `FF19E8` and `0x5670` dispatches through table `0x59BA`;
- dispatch entries route state `0` to `0x61F6`, state `2` to `0x62E4`, and state `4` to `0x6516`;
- `0x61F6` writes direction `+0x16`, intent deltas `+0x4E/+0x52`, accumulated deltas `+0x72/+0x76`, and state `+0x04=2`.

**Actions:** Added the confirmed update/state addresses to the player API, modelled the state-2 transition, and extended the local ROM oracle to validate the dispatch entries.

**Result:** The earlier absence of direct calls to `0x61F6` is explained by the indirect `0x59BA` dispatcher. Animation callback/presentation meaning remains intentionally outside the native movement slice.

**Exact next step:** validate the state-2/state-4 stop and turning branches against additional ROM traces before modelling any presentation state.

## 2026-09-03 — M8 player movement evidence and native slice
**Objective:** Translate the confirmed player input-to-movement path into a portable deterministic model.

**Actions before implementation:**
- installed CMake 4.4.3 and LLVM-MinGW locally because the workspace had no C++ toolchain;
- traced player initialization at `0x13D6..0x142E`, which selects entity slot `0xFF19E8` and type `2`;
- confirmed controller normalization at `0x2992` into `0xFF165C..0xFF1661`;
- decoded the direction dispatch table at `0x85E2`: low nibble of `0xFF165E` maps to cardinal/diagonal fixed-point vectors;
- confirmed the shared movement cluster starts at `0x8F12`, samples the entity footprint through `0x9BF2`, and gates terrain changes through `0x938E`.

**Understanding confidence:** 93% for input mapping, player slot and movement-gate contract; unknown animation/entity callback semantics remain outside this slice.

**Acceptance criteria for this task:**
- portable player state and deterministic direction mapping;
- default speeds and diagonal vectors match the confirmed `0x85E2` table;
- free and blocked movement tests reuse the M7 footprint and terrain-gate APIs;
- local Debug and Release builds/tests pass;
- evidence, file map and milestone state are updated.

**Exact next step:** implement and test the isolated player movement slice, then run both build configurations.

## 2026-09-03 — Repository merge and ROM hygiene
**Objective:** Bring the native implementation into `main` without tracking the user-supplied commercial ROM.

**Actions:**
- merged `origin/oasis-cpp-bootstrap` into `main`;
- removed the ROM archive from the Git tree while retaining it locally for reference checks;
- added the ROM archive and generated build/inspection artifacts to `.gitignore`;
- removed GitHub workflows that attempted to retrieve the ROM from the repository.

**Result:** `main` contains the native source, tests and documentation; the reference ROM remains local-only.

**Unresolved:** ROM-backed probes must be run locally or through a separately provisioned private CI mechanism.

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
