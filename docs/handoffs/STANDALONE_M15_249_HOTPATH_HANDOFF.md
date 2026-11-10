# Standalone Track Handoff — M15.249 High-Displacement Connected Hotpath Subsystem

> **Scope & Repository Hygiene Certification:** `C:/Github/Sega-Thor-Standalone` was kept strictly read-only and unmodified throughout M15.249. This handoff provides the standalone engineering track with complete specifications, memory contracts, and portable C++20 routines for the high-displacement unowned connected hotpath routines compiled in M15.249.

---

## 1. Executive Summary & Architecture Overview

M15.249 expands the portable recompilation pipeline into high-displacement, unowned connected execution paths across six authentic gameplay scenarios: Scenario C (1,800f), Scenario A (1,592f), Scenario B (600f), Longrun 3,600f (3,600f), Longrun Alt (1,800f), and Longrun Combat (1,800f).

Seven unowned high-frequency subroutines were reverse-engineered and translated into portable Thor IR and auto-partitioned C++20 headers under `src/tools/thor_ir/generated_m15_249/`.

### Structural Metrics
- **Compiled M68000 Instructions:** 118 instructions
- **Basic Blocks:** 45 CFG basic blocks
- **CFG Edges:** 71 edges
- **Generated C++ Headers:** `src/tools/thor_ir/generated_m15_249/` (12 headers, all <= 260 lines, strictly conforming to the <= 350-line limit)
- **Production Overlap:** Zero overlap with the 196 active production helpers
- **Milestone Isolation:** Zero overlap with M15.244 (130 insns), M15.245 (508 insns), M15.246 (375 insns), M15.247 (128 insns), or M15.248 (240 insns)
- **Cumulative Footprint:** 1,499 cumulative compiled original M68000 instructions across M15.244–M15.249
- **CFG Edge Coverage:** Internal edge fixture coverage increased from 123/151 (81.5%) to 133/151 (88.1%), with 150/151 (99.3%) combined coverage

### DMA & Hardware-Polling Boundary Forensics
Comprehensive audit of DMA status register `$FFFF164D` and queue status register `$FFFF1658`:
- Polling loops at `0x002DB6` and `0x002E78` check bit 0 of `$FF164D` (VDP DMA busy) without cycle-exact hardware bus stall emulation, introducing 1-scanline timing jitter.
- Spin loops at `0x002FDA` and `0x00302E` poll `$FF1658` (DMA FIFO queue ready).
- These routines remain safely quarantined in hybrid execution.
- In contrast, the M15.249 target routines (`0x0254EA`, `0x00EA04`, `0x016D5C`, `0x0170E0`, `0x0171C4`, `0x00E0B8`, `0x00F96A`) have zero hardware polling, zero timing hazards, and zero production helper collisions.

---

## 2. Inventory of Compiled Connected Hotpath Subroutines

| Routine Root | Window | Insns | Blocks | Purpose & Behavior in Standalone |
|---|---|---|---|---|
| `0x0254EA` | `0x0254EA..0x0255D8` | 49 | 16 | **Water Shrine Combat Enemy Controller:** Master state machine for combat enemy AI in the Water Shrine dungeon. Dispatches 5 sub-states (init, lookup table navigation, attack phase, animation trigger, retreat). Displaces > 500,000 dynamic steps across Scenario C and Longrun journals. |
| `0x00EA04` | `0x00EA04..0x00EA16` | 5 | 3 | **Enemy Substate Check:** Validates entity substate index `42(A6)` against threshold bounds and dispatches or returns via RTS. |
| `0x016D5C` | `0x016D5C..0x016D76` | 7 | 2 | **Enemy Cooldown Init:** Initializes entity combat cooldown timers `136(A6)` and `138(A6)` from parameter `160(A6)`. |
| `0x0170E0` | `0x0170E0..0x01710A` | 11 | 4 | **Enemy Target Lock:** Sets entity state `102(A6) = 1`, copies target angle `160(A6)` to lifetime `2(A6)`, and binds animation timer `16(A6)`. |
| `0x0171C4` | `0x0171C4..0x0171F8` | 13 | 4 | **Enemy Target Track:** Multi-parameter tracking routine binding angle, target entity reference, and speed parameters. |
| `0x00E0B8` | `0x00E0B8..0x00E0BA` | 1 | 1 | **Dummy RTS Handler:** Clean null handler in the master enemy dispatch table at `0x00DF80`. |
| `0x00F96A` | `0x00F96A..0x00F9C6` | 32 | 15 | **Movement State Logic:** High-frequency character and entity movement parameter parser evaluating command byte streams. |

---

## 3. Master Enemy Dispatch Architecture (`0x00DF80`)

The M15.249 investigation revealed the complete layout of the original Mega Drive engine's master enemy controller dispatch table located at ROM address `0x00DF80`:

```text
ROM:0x00DF10  ADD.w   D6, D6
ROM:0x00DF12  MOVEA.l 0x00DF28(PC, D6.l), A0  ; Load controller from table at 0x00DF80
ROM:0x00DF16  MOVEM.l D1/A6, -(SP)
ROM:0x00DF1A  JSR     (A0)                    ; Call enemy controller
ROM:0x00DF1C  MOVEM.l (SP)+, D1/A6
```

Table entries:
- `Table[0] (0x00DF80)`: `0x0254EA` (Water Shrine enemy — compiled in M15.249)
- `Table[1] (0x00DF84)`: `0x00E25C` (Entity despawn timer — claimed by production helper `m68k_00e25c_...`)
- `Table[4] (0x00DF90)`: `0x00EA04` (Enemy substate check — compiled in M15.249)
- `Table[6] (0x00DF98)`: `0x01AC90` (Production helper `m68k_01ac90_...`)
- `Table[8] (0x00DFA0)`: `0x017D12` (Production helper `m68k_017d12_...`)
- `Table[9] (0x00DFA4)`: `0x00E0B8` (Dummy RTS — compiled in M15.249)
- `Table[15] (0x00DFBC)`: `0x016D5C` (Enemy cooldown init — compiled in M15.249)
- `Table[18] (0x00DFC8)`: `0x0170E0` (Enemy target lock — compiled in M15.249)
- `Table[19] (0x00DFCC)`: `0x0171C4` (Enemy target track — compiled in M15.249)

Standalone developers can now implement a native polymorphic `EnemyController` base class replacing this jump table cleanly.

---

## 4. Entity RAM Field Contract for Standalone Implementations

| Offset | Size | Name | Purpose |
|---|---|---|---|
| `0(A6)` | Word | `entity_type` | Entity classification index (0 = inactive/despawned) |
| `2(A6)` | Word | `lifetime_counter` | Countdown timer for despawn |
| `4(A6)` | Word | `animation_id` | Current active sprite animation frame sequence |
| `16(A6)` | Word | `anim_delay` | Timer ticks between animation frame steps |
| `42(A6)` | Word | `substate_phase` | Combat state machine major phase |
| `54(A6)` | Byte | `status_flags_lo` | Entity attribute bitmask (bit 1: visible, bit 0: active) |
| `55(A6)` | Byte | `status_flags_hi` | Entity attribute bitmask |
| `56(A6)` | Byte | `combat_flags` | Bit 5: aggressive, bits 2..3: attack cooldown mask |
| `74(A6)` | Word | `hit_points` | Entity current health |
| `102(A6)` | Byte | `state_index` | AI substate index (0..5) |
| `136(A6)` | Word | `cooldown_timer` | Attack cooldown countdown |
| `138(A6)` | Word | `cooldown_reset` | Reload value for attack cooldown |
| `140(A6)` | Word | `target_angle` | Orientation angle toward target entity |
| `144(A6)` | Word | `velocity_x` | 16-bit signed horizontal displacement |
| `150(A6)` | Word | `direction_id` | 0..3 directional facing index |
| `160(A6)` | Word | `velocity_y` | 16-bit signed vertical displacement |
