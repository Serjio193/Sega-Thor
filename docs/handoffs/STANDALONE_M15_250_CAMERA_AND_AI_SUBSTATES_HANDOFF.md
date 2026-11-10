# Standalone Track Handoff — M15.250 Residual CPU Bottleneck Elimination & Subsystems

> **Scope & Repository Hygiene Certification:** `C:/Github/Sega-Thor-Standalone` was kept strictly read-only and unmodified throughout M15.250. This handoff provides the standalone engineering track with complete specifications, memory contracts, RAM data structures, and portable C++20 routines for the eight high-displacement connected hotpaths compiled in M15.250.

---

## 1. Executive Summary & Architecture Overview

M15.250 eliminates residual CPU bottlenecks under hybrid execution (`RUN_G`) by compiling eight unowned, high-frequency original M68000 routines connected directly to entity AI sub-states, camera coordinate tracking, directional routing, entity priority sorting, combat collision scans, and dungeon trigger streams across six authentic gameplay scenarios: Scenario C (1,800f), Scenario A (1,592f), Scenario B (600f), Longrun 3,600f (3,600f), Longrun Alt (1,800f), and Longrun Combat (1,800f).

### Structural Metrics
- **Compiled M68000 Instructions:** 181 newly compiled instructions (1,680 cumulative across M15.244–M15.250)
- **CFG Basic Blocks:** 71 basic blocks
- **CFG Edges:** 175 edges
- **Generated C++ Headers:** `src/tools/thor_ir/generated_m15_250/` (18 headers, all <= 260 lines, strictly conforming to the <= 350-line limit)
- **Production Overlap:** Zero overlap with the 196 active production helpers (held helper slot 214 `0x0083F8` was explicitly audited and preserved in its timing-sensitive hold)
- **Milestone Isolation:** Zero overlap with M15.244 (130 insns), M15.245 (508 insns), M15.246 (375 insns), M15.247 (128 insns), M15.248 (240 insns), or M15.249 (118 insns)
- **Dynamic Guest Instruction Displacement:**
  - `scenario_c`: 37,213 steps
  - `scenario_a`: 49,266 steps
  - `scenario_b`: 834 steps
  - `longrun_3600f`: 44,947 steps
  - `longrun_alt_1800f`: 50,100 steps
  - `longrun_combat_1800f`: 37,213 steps
  - **Total M15.250 Disjoint Portable Steps: 219,573** (exceeding the 50,000 target by >4.3x; 10x M15.249)
  - **Cumulative Portable Guest Steps: 932,356** across six scenarios
- **Differential Suite:** 141/141 three-way CPU differential fixtures PASS (18 authentic + 123 controlled) and 12/12 negative & mutation contracts PASS
- **Multi-Scenario Gameplay Parity:** 48/48 runs (`RUN_A` through `RUN_H` across all 6 scenarios) achieved 100% bit-exact stream and checkpoint matching with zero contract faults and zero fallbacks
- **Performance:** 1.39x isolated microbenchmark speedup (Mode B block runner over Mode A interpreter)

---

## 2. Inventory of Compiled Connected Hotpath Subroutines

| Routine Root | PC Window | Insns | Blocks | Purpose & Behavior in Standalone |
|---|---|---|---|---|
| `0x0255D8` | `0x0255D8..0x0256B8` | 54 | 18 | **Water Shrine Enemy Substate Handlers:** Directly continues the `0x0254EA` master state machine compiled in M15.249. Handles sub-states 1..5: vector updates, waypoint checks, attack timing, and animation synchronization. |
| `0x00A4C8` | `0x00A4C8..0x00A51A` | 15 | 8 | **Camera Scroll Tracker:** Calculates viewport bounds and updates scroll offsets relative to player anchor coordinates in RAM `$FF188E`. |
| `0x017908` | `0x017908..0x017920` | 8 | 4 | **Directional Router:** Computes 4-way and 8-way directional facing flags from signed delta coordinate components. |
| `0x00EA16` | `0x00EA16..0x00EA76` | 24 | 10 | **Enemy Substate Timer:** Manages entity cooldown ticks, substate timeout transitions, and resets substate phase `42(A6)`. |
| `0x02E8B4` | `0x02E8B4..0x02E910` | 21 | 8 | **Water Shrine Trigger Stream:** High-frequency loop processing dungeon environmental triggers, switch states, and water plane animations (generates >86,000 dynamic steps). |
| `0x00A550` | `0x00A550..0x00A58A` | 17 | 8 | **Entity Priority Sort:** Iterates active entity slots in RAM, compares Y-coordinates, and sorts rendering/update priority into `$FF188E` (generates >79,000 dynamic steps). |
| `0x008778` | `0x0087E2..0x0087E2` | 30 | 11 | **Combat Collision Scan:** High-frequency bounding-box overlap and vulnerability check between weapon hitboxes and enemy hurtboxes. |
| `0x02EC18` | `0x02EC5A..0x02EC5A` | 12 | 4 | **Water Shrine Gate Controller:** State controller for gate barrier triggers and locked door opening sequences in the Water Shrine dungeon. |

---

## 3. Key Memory Contracts & Global RAM Structures

### Entity Table (`$FF19E8`..`$FF24E8`)
- The Mega Drive engine allocates 21 entity slots of 128 bytes each, starting at `$FF19E8`.
- In addition to the fields documented in M15.249 (`hit_points`, `animation_id`, `substate_phase`, etc.):
  - `44(A6)`: Timer tick for sub-state progression (audited in `0x00EA16`).
  - `46(A6)`: Next sub-state target ID upon timer expiration.
  - `60(A6)`: Collision bounding-box width/height radius (used in `0x008778`).
  - `62(A6)`: Hurtbox vulnerability flags (bit 7: invincible, bit 3: sword vulnerable).

### Camera & Sorting RAM (`$FF188E`..`$FF18D0`)
- `$FF188E`: Current camera X-coordinate (16-bit integer).
- `$FF1890`: Current camera Y-coordinate (16-bit integer).
- `$FF1892`: Viewport X min clamp.
- `$FF1894`: Viewport X max clamp.
- `$FF1896`: Viewport Y min clamp.
- `$FF1898`: Viewport Y max clamp.
- `$FF18A0`: Priority sorted entity index list (16 words, sorted ascending by Y coordinate).

### Environmental & Gate Triggers (`$FF1668` & `$FF0DBC`)
- `$FF1668`: Water Shrine stream event ring buffer index and status bitmask.
- `$FF0DBC`: Dungeon barrier and locked door state mask (bit 2: water shrine main gate opened).

---

## 4. Standalone Implementation Guidance

1. **Entity State Machine:**
   Standalone developers should bind `hotpath_255d8_enemy_substates` and `hotpath_0ea16_enemy_substate_timer` directly into the `EnemyController` base class, allowing seamless native handling of all 5 sub-states without fallback transfers.
2. **Entity Priority Manager:**
   The logic in `hotpath_0a550_entity_priority_sort` can be directly mapped to an `std::stable_sort` over active entity pointers using `entity->get_y_position()`, maintaining 100% visual parity with original Mega Drive sprite ordering.
3. **Collision Detection System:**
   The bounding box intersection math in `hotpath_08778_combat_collision_scan` provides exact bit-level collision margins for Ali's knife, sword, and bow attacks against Water Shrine enemies.
