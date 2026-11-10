# Standalone Track Handoff — M15.248 Profile-Guided Hotpath Subsystem

> **Scope & Repository Hygiene Certification:** `C:/Github/Sega-Thor-Standalone` was kept strictly read-only and unmodified throughout M15.248. This handoff provides the standalone engineering track with complete specifications, memory contracts, and portable C++20 routines for the high-frequency unowned hotpath routines compiled in M15.248.

---

## 1. Executive Summary & Architecture Overview

M15.248 expands the portable recompilation pipeline using profile-guided candidate selection across six authentic scenarios: Scenario C (1,800f), Scenario A (1,592f), Scenario B (600f), Longrun 3,600f (3,600f), Longrun Alt (1,800f), and the newly captured Longrun Combat (1,800f).

Fifteen unowned high-frequency subroutines were reverse-engineered and translated into portable Thor IR and partitioned C++20 headers under `src/tools/thor_ir/generated_m15_248/`.

### Structural Metrics
- **Compiled M68000 Instructions:** 240 instructions
- **Basic Blocks:** 76 CFG basic blocks
- **CFG Edges:** 126 internal edges
- **Generated C++ Headers:** `src/tools/thor_ir/generated_m15_248/` (19 headers, all <= 259 lines, strictly conforming to the <= 350-line limit)
- **Production Overlap:** Zero overlap with the 196 active production helpers
- **Milestone Isolation:** Zero overlap with M15.244 (130 insns), M15.245 (508 insns), M15.246 (375 insns), or M15.247 (128 insns)
- **Cumulative Footprint:** 1,381 cumulative compiled original M68000 instructions across M15.244–M15.248
- **CFG Edge Coverage:** 123/151 (81.457%) internal edge coverage in fixture suite, exceeding the >= 120/151 requirement; 150/151 (99.338%) combined coverage

### DMA Forensics & Execution Safety
Forensic analysis of VDP-interacting routines revealed that routines containing spin loops on hardware interrupt completion flags (specifically `0x002DB6` and `0x002E78` polling `$ff164d`) introduce scanline-level timing variance without cycle-exact DMA bus stall modeling. Consequently, hardware-polling routines remain quarantined in hybrid execution, while pure deterministic routines (coordinate transforms, collision bounding box tests, linear congruential RNG steps, trigonometric lookup, linear interpolation, tile coordinate decoding, and input buffer polling) run natively with bit-exact parity.

### Dynamic Execution Displacement
- **Scenario C (1,800f):** 2,137 disjoint new native steps
- **Scenario A (1,592f):** 2,387 disjoint new native steps
- **Scenario B (600f):** 425 disjoint new native steps
- **Longrun 3,600f (3,600f):** 2,711 disjoint new native steps
- **Longrun Alt 1,800f (1,800f):** 2,565 disjoint new native steps
- **Longrun Combat 1,800f (1,800f):** 2,137 disjoint new native steps
- **Total New Steps Across Scenarios:** 12,362 disjoint new native steps
- **Cumulative Portable Executions:** 691,019 guest instructions across all six scenarios
- **Bit-Exact Streams:** 48/48 raw streams match (8/8 per scenario across RUN_A..RUN_F)
- **Contract Integrity:** 0 original CPU fallbacks, 0 contract faults, 0 unhandled exceptions

---

## 2. Inventory of Compiled Hotpath Subroutines

| Routine Root | Window | Insns | Blocks | Purpose & Behavior in Standalone |
|---|---|---|---|---|
| `0x00FCB8` | `0x00FCB8..0x00FCCA` | 10 | 2 | **Transform Load:** Loads 16-bit spatial transformation matrices into work RAM for rendering pipelines. |
| `0x00D9A4` | `0x00D9A4..0x00D9B0` | 7 | 2 | **Math Calc:** Fixed-point geometric distance and velocity calculations. |
| `0x002D84` | `0x002D84..0x002DA8` | 13 | 4 | **Coord Prep:** Prepares entity screen-space coordinate blocks for display rendering. |
| `0x002DB6` | `0x002DB6..0x002DFE` | 36 | 6 | **VDP Reg Config:** Configures VDP registers; contains `$ff164d` polling loop (quarantined in hybrid). |
| `0x002E1E` | `0x002E1E..0x002E4E` | 25 | 4 | **Status Prep:** Prepares display status registers and plane priority masks. |
| `0x002E78` | `0x002E78..0x002EAE` | 16 | 4 | **DMA Queue Calc:** Computes DMA transfer queue bounds; contains `$ff164d` polling loop (quarantined). |
| `0x003586` | `0x003586..0x003592` | 7 | 3 | **Sprite Coord:** Evaluates sprite coordinate clipping against viewport boundaries. |
| `0x00F80E` | `0x00F80E..0x00F838` | 16 | 6 | **Collision Bounds:** Computes collision bounding box intersections and displacement vectors. |
| `0x01A184` | `0x01A184..0x01A1CE` | 27 | 9 | **RNG / State Step:** Multi-tap linear congruential pseudorandom generator and entity behavior state selection. |
| `0x00FCCE` | `0x00FCCE..0x00FCE0` | 10 | 3 | **Vector Adjust:** Adjusts spatial directional vectors based on terrain surface normals. |
| `0x001112` | `0x001112..0x00113C` | 16 | 4 | **Math Lookup:** High-speed trigonometric lookup for entity heading angles. |
| `0x00114A` | `0x00114A..0x001174` | 16 | 4 | **Math Interpolate:** Linear interpolation helper between consecutive trajectory keyframes. |
| `0x001E98` | `0x001E98..0x001ECC` | 13 | 3 | **Tile Offset Calc:** Computes 2D tilemap block byte offsets from world coordinates. |
| `0x009BC0` | `0x009BC0..0x009C0E` | 10 | 1 | **Input Buffer Poll:** High-frequency input polling helper verifying controller state bitmasks. |
| `0x001E46` | `0x001E46..0x001E96` | 18 | 4 | **Tile Coord Decode:** Decodes compressed tile indices and attribute flags into staging buffer. |

---

## 3. Standalone Engine Memory & Calling Conventions

### Calling Register Contracts
- **`0x009BC0` (Input Buffer Poll):** Executes on every input poll event; reads controller buffer at `0xFF1766`; returns via `RTS`.
- **`0x01A184` (RNG / State Step):** `A6` points to entity context; checks `0x66(A6)`; updates `0x16(A6)`; deterministic branch transitions.
- **`0x00F80E` (Collision Bounds):** `A0` points to collision bounding structure; parses collision type flags; branches cleanly on bounds overlap.
- **`0x001112` / `0x00114A` (Trig & Interpolation):** `D0` passes angle index / interpolation factor; `A0` points to sine/cosine or keyframe tables; returns via `RTS`.
- **`0x001E46` / `0x001E98` (Tilemap Coordinates):** `A0` points to tile source; `A1` points to destination buffer; uses `(An)+` post-increment addressing to copy and unpack tile coordinates.

---

## 4. Standalone C++20 Integration Recommendation

All compiled headers in `src/tools/thor_ir/generated_m15_248/` are self-contained and implement the standard `step_ir_0x...` template interface:

```cpp
template <typename Bus>
inline bool step_ir_0x009BC0(Bus& bus, std::uint32_t& out_pc) noexcept;
```

For the standalone engine:
1. **Math & Trig (`0x001112`, `0x00114A`, `0x00D9A4`):** Can be mapped directly to native C++ functions without memory bus overhead.
2. **Input Polling (`0x009BC0`):** Replaces guest input polling with native gamepad state query.
3. **Collision & Coordinate Helpers (`0x00F80E`, `0x001E46`, `0x001E98`):** Can be integrated into native collision and tilemap managers.
4. **VDP Isolation:** Hardware-polling routines (`0x002DB6`, `0x002E78`) should NOT be translated as busy loops in the standalone engine; their intended high-level state updates should be invoked directly when the VDP DMA queue commits.
