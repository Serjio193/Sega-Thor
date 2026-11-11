# STANDALONE M15.256 HOTSPOT HANDOFF: HOST CPU PROFILING, MUSASHI ATTRIBUTION, AND PORTABLE COVERAGE EXPANSION

**Document Version:** 1.0.0  
**Date:** 2026-11-11  
**Milestone:** M15.256  
**Track:** HYBRID_ROM_TO_CPP  
**Target Audience:** Autonomous Agents, Standalone Implementers, System Architects  

---

## 1. Executive Summary

Milestone M15.256 conducted host CPU execution time profiling across all 6 gameplay scenarios (11,192 frames) to establish the true distribution of host execution time.

### Key Discoveries:
1. **Musashi CPU Interpreter Accounts for 74.42% of Host Execution Time:**
   - Interpreted M68000 instruction emulation is the primary consumer of host CPU cycles.
   - Native production helpers consume 16.17%.
   - Runner overhead (frame presentation, VDP sync) consumes 7.08%.
   - Audio (Z80 / YM2612 emulation) consumes 1.33%.
   - Memory bus callbacks (<0.01%) and scheduler/interrupt checks (<0.01%) are computationally negligible.
2. **Idle Wait Loops Represent 50.1% of Guest Instructions:**
   - 56,083,610 guest steps are spent spin-waiting on `$FF1654` for VINT synchronization (`0x008ACA..0x008AD0` and `0x008B72..0x008B7A`).
   - These wait loops cannot be skipped without breaking guest frame pacing and VDP beam synchronization.
3. **Profile-Guided Coverage Expansion:**
   - 5 high-ROI, clean leaf routines (`0x010F48`, `0x0112CC`, `0x0302F4`, `0x006B12`, `0x0141CA`) were compiled to portable C++20 (73 instructions, 34 basic blocks).
   - All 146 pinned reference fixtures passed bit-exact across Mode A and Mode B under ASan/UBSan.
   - All 72 qualification sessions across 12 configurations (`RUN_A`..`RUN_L`) and 6 scenarios produced 100% bit-exact stream matching (576/576 stream files, 31/31 checkpoints).
4. **Performance Measurement Verdict:**
   - In 10-trial measured OS process wall-clock benchmarks, compiling 73 leaf instructions yielded timings within the host OS $\pm 1.5\%$ noise window (median 15.3132s vs baseline 15.2886s).
   - Moving the integrated performance needle requires replacing unowned, heavy computational loops (such as `0x00C8AA` tilemap prep and `0x00D406` graphics line engine) once their multi-routine call graphs are compiled together.

---

## 2. Host CPU Execution Breakdown (11,192 Frames)

| Workload Category | Host Wall-Clock Time (%) | Description / Origin |
|:---|:---:|:---|
| **Musashi M68000 Interpreter** | **74.42%** | Uncompiled ROM guest instructions executed via `m68k_execute` |
| **Production Native Helpers** | **16.17%** | 196 active production native helpers executed natively |
| **Runner Overhead** | **7.08%** | Libretro frontend, frame presentation, texture update |
| **Audio Emulation (Z80/YM2612)** | **1.33%** | Pinned GPGX sound chip update loops |
| **Portable Hotpaths (IR)** | **0.90%** | Bounded portable C++20 routines (`RUN_L`) |
| **VDP Rendering & DMA** | **0.10%** | Scanline rendering and DMA copy operations |
| **Virtual Bus Callbacks** | **<0.01%** | `read_m` / `write_m` pointer indirections |
| **Scheduler & Interrupt Checks** | **<0.01%** | Cycle counter decrements and VINT polling |
| *Host Profiler Overhead* | *27.33%* | Sampling overhead (110.24s baseline -> 140.37s instrumented) |

---

## 3. Profile-Guided Compiled Routines

The 5 candidate routines selected and compiled in M15.256 are:

| Routine Label | ROM Entry | ROM Exit | Instructions | Basic Blocks | Primary Responsibility |
|:---|:---:|:---:|:---:|:---:|:---|
| `hotpath_10f48_entity_bounds_calc` | `0x010F48` | `0x010F84` | 25 | 11 | Calculates entity bounding box extents from offset tables |
| `hotpath_112cc_entity_slot_lookup` | `0x0112CC` | `0x0112EE` | 10 | 5 | Scans entity table slots in Work RAM (`$FF2000`) |
| `hotpath_302f4_entity_flag_sync` | `0x0302F4` | `0x030324` | 11 | 6 | Synchronizes entity state and animation flags |
| `hotpath_06b12_velocity_damp_math` | `0x006B12` | `0x006B2A` | 9 | 5 | Applies velocity deceleration damping vector math |
| `hotpath_141ca_enemy_coord_clamp` | `0x0141CA` | `0x014200` | 18 | 7 | Clamps enemy position within room boundary extents |

### Implementation Artifacts:
- **Thor-IR Specification:** `src/tools/thor_ir/m15_256_callee_ir.json`
- **PC Source Map:** `src/tools/thor_ir/m15_256_source_map.json`
- **C++20 Generated Headers:** `src/tools/thor_ir/generated_m15_256/` (11 partitioned headers $\le 260$ lines)
- **Hybrid Dispatch Bridge:** `src/tools/thor_ir/exp256_dispatch_hybrid_adapter.hpp` (143 lines)
- **Differential Fixtures:** `data/thor-fixtures/m15_256_callee_fixtures.hpp` (146 fixtures)
- **Test Suite:** `tests/thor_ir_m15_256_diff_test.cpp` (146 lines)

---

## 4. Heavy Hotspot Landscape for M15.257+

To eliminate significant portions of the 74.42% Musashi execution time, upcoming milestones must target the top unowned computational routines:

1. **`0x00C8AA` (Tilemap Update Loop):**
   - **Dynamic Guest Instructions:** ~2,572,160 steps across qualification scenarios.
   - **Nature:** Heavy memory copying and tile attribute indexing loop.
   - **Prerequisites:** Requires verifying Work RAM tile attribute tables and handling plane register indexing.
2. **`0x00D406` (Graphics Line Interpolation):**
   - **Dynamic Guest Instructions:** ~2,572,160 steps across qualification scenarios.
   - **Nature:** Coordinate calculation and Bresenham line stepping for graphics rendering.
   - **Prerequisites:** Pure mathematical leaf; zero hardware register dependencies.
3. **`0x00A9A0` (Object Position & Physics Coordinator):**
   - **Dynamic Guest Instructions:** ~1,850,000 steps.
   - **Nature:** Calls subroutines `0x008F22`, `0x0023CC`, `0x002512`.
   - **Prerequisites:** Multi-routine cluster fusion.

---

## 5. Verification Matrix Summary

| Test / Gate | Target | Result | Status |
|:---|:---:|:---:|:---:|
| **Canonical ROM SHA256** | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | Exact match | **PASS** |
| **Pinned GPGX SHA256** | `ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134` | Exact match | **PASS** |
| **Production Helpers** | 196 active helpers (`17396fda...`) | 196 active helpers | **PASS** |
| **Production Source Hash** | `28b6617d71065897eb5c2584e70f5b3c6e34ad92244e41f587cae4b2ec0c4ce7` | Exact match (0 diffs) | **PASS** |
| **Reference Fixtures** | 146 fixtures (Mode A vs Mode B vs Musashi) | 146/146 bit-exact | **PASS** |
| **Mutation Contracts** | 12 mutation safety contracts under ASan/UBSan | 12/12 passing | **PASS** |
| **12-Config Replay** | 72 sessions, 576 streams across 6 scenarios | 576/576 bit-exact | **PASS** |
| **Checkpoint Parity** | 31 serialized checkpoints across 6 scenarios | 31/31 bit-exact | **PASS** |
| **Receipt Verification** | `ReceiptVerifier` on `combined-clusters-qualification-m15-256-20261111.json` | `status=VERIFIED, valid=True` | **PASS** |
| **Source File Limit** | AGENTS.md Rule 3: All governed files $\le 500$ lines | 2564/2564 files $\le 500$ lines | **PASS** |
| **Standalone Repo** | `C:/Github/Sega-Thor-Standalone` | Strictly read-only & untouched | **PASS** |
