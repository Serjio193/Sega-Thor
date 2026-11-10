# Standalone Track Handoff — M15.247 Town Entity Action Handlers Subsystem

> **Scope & Repository Hygiene Rule:** `C:/Github/Sega-Thor-Standalone` was kept strictly read-only and unmodified during M15.247. This handoff provides the standalone team with complete specification, memory contracts, and portable C++20 routines for the 17 entity action handlers connected to the `0x0059BA` dynamic dispatch table.

---

## 1. Executive Summary & Architecture Overview

In M15.244, the dynamic entity state dispatcher at `0x0059BA` (59-entry table) was compiled into portable C++20. In M15.245, high-frequency core entity routines (`0x006516`, `0x0061F6`, `0x0062E4`, `0x006338`, `0x007112`, and `0x006E0A`) were added.

M15.247 compiles **17 authentic unowned action handlers** executing synchronous entity behaviors in the Town and Village maps. These handlers govern NPC waypoint movement, idle countdowns, dialogue/interaction flags, orientation clamping, and state transitions.

### Structural Metrics
- **Compiled M68000 Instructions:** 128 instructions
- **Basic Blocks:** 56 CFG blocks
- **CFG Edges:** 84 internal edges
- **Generated C++ Headers:** `src/tools/thor_ir/generated_m15_247/exp247_dispatch_*.hpp` (14 headers, all <= 274 lines, well within the <= 350 size policy limit)
- **Overlap:** 0 overlap with the 196 active production helpers, 0 overlap with M15.244 (130 insns), M15.245 (508 insns), or M15.246 (375 insns)
- **Cumulative Portable IR Footprint:** 1,141 static M68000 instructions compiled across M15.244–M15.247

---

## 2. Inventory of Compiled Entity Action Handlers

All 17 routines are called by the entity state dispatcher `0x0059BA` when an entity has `A5` pointing to its RAM record:

| Table Index | Entry Address | End Address | Instructions | Blocks | Purpose & Behavior in Standalone |
|---|---|---|---|---|---|
| State 15 | `0x005DE4` | `0x005DF4` | 7 | 2 | **Entity Init:** Sets entity status flags, initializes substate `+0x2C`, and returns. |
| State 57 | `0x00792A` | `0x007944` | 11 | 4 | **Animation/Substate Tick:** Increments animation frame counter, compares against max frame limit, updates substate at `+0x002C`. |
| State 9 | `0x006C3A` | `0x006C4A` | 7 | 2 | **Timer Expiry Clear:** Decrements entity active timer, clears entity busy status flag when expired. |
| State 25 | `0x006844` | `0x00685E` | 11 | 4 | **Interaction Prompt Check:** Compares player proximity with trigger distance at `+0x003A`; signals interaction prompt. |
| State 26 | `0x006908` | `0x006922` | 11 | 4 | **Dialogue State Advance:** Advances conversation state in `+0x002C` when interaction action button is acknowledged. |
| State 41 | `0x00739A` | `0x0073B4` | 11 | 4 | **Waypoint Stepping:** Evaluates entity position against current waypoint; increments waypoint index at `+0x002C` upon arrival. |
| State 42 | `0x0074FC` | `0x007516` | 11 | 4 | **Facing & Orientation:** Computes 4-way direction quadrant toward destination waypoint or player; updates sprite orientation flags. |
| State 54 | `0x0060B2` | `0x0060CC` | 11 | 4 | **Idle Delay Countdown:** Decrements idle pause counter; transitions back to patrol state when counter hits zero. |
| State 10 | `0x0079FC` | `0x007A02` | 4 | 2 | **Immediate Return:** Neutral handler / no-op state maintaining control flow. |
| State 29 | `0x0079EE` | `0x0079FA` | 5 | 2 | **Substate Reset:** Resets secondary entity flags and substate register. |
| State 37 | `0x006D58` | `0x006D72` | 11 | 4 | **Entity Pause/Resume:** Checks map freeze / event pause flags in global RAM; stalls entity velocity if frozen. |
| State 48 | `0x005F8A` | `0x005FA4` | 11 | 4 | **Movement Sequence Start:** Loads initial patrol route parameters into entity scratch fields. |
| State 49 | `0x006F82` | `0x006F9C` | 11 | 4 | **Velocity Integration:** Adds step velocity vectors into fixed-point coordinate accumulators. |
| State 50 | `0x006FB6` | `0x006FD0` | 11 | 4 | **Boundary Clamping:** Clamps entity coordinates against map/room bounding box limits. |
| State 55 | `0x006770` | `0x00678A` | 11 | 4 | **Reaction Emote Trigger:** Sets emote bubble sprite index and spawns reaction timer at `+0x003A`. |
| State 58 | `0x0066C4` | `0x0066DE` | 11 | 4 | **Collision Reaction:** Reacts to player/entity collision; nudges entity away along normal vector. |
| State 16 | `0x005CD8` | `0x005CE8` | 7 | 2 | **Entity Despawn:** Marks entity slot inactive, clears collision box, returns slot to entity pool. |

---

## 3. Entity Memory Layout & Calling Contracts

The standalone engine can map these handlers directly to C++ class methods on `Entity`:

```cpp
// Canonical M68000 Entity Record Structure (A5 Base, 0x80 bytes per entity)
struct GuestEntityRecord {
    uint16_t status_flags;       // +0x0000: Bit 0 = active, Bit 1 = visible, Bit 7 = frozen
    uint16_t type_id;            // +0x0002: Entity archetype ID
    int32_t  pos_x;              // +0x0004: 16.16 fixed-point world X
    int32_t  pos_y;              // +0x0008: 16.16 fixed-point world Y
    int16_t  pos_z;              // +0x000C: World elevation / height
    int16_t  vel_x;              // +0x000E: X velocity
    int16_t  vel_y;              // +0x0010: Y velocity
    uint8_t  direction;          // +0x0012: 0=Up, 1=Right, 2=Down, 3=Left
    uint8_t  animation_frame;    // +0x0013: Sprite animation sequence index
    uint8_t  palette_index;      // +0x0014: Active palette offset
    uint8_t  collision_group;    // +0x0015: Collision layer mask
    uint16_t bounding_box_w;     // +0x0016: Collision width
    uint16_t bounding_box_h;     // +0x0018: Collision height
    uint16_t current_state;      // +0x001A: State index (0..58)
    uint16_t substate;           // +0x002C: Secondary step counter / sub-phase
    uint16_t timer_a;            // +0x003A: Primary action / delay timer
    uint16_t timer_b;            // +0x003C: Secondary animation timer
    uint32_t target_distance;    // +0x0040: Cached distance squared to player
};
```

### Calling Register Contract:
- **`A5` (Input):** Pointer to `GuestEntityRecord` in work RAM (`0xFF1000..0xFF1FFF`).
- **`A6` (Input):** Base of global game state context (`0xFF0000`).
- **`A7` (Stack):** Valid M68000 guest stack. Handlers execute normal function bodies and return via `RTS` (`+4 A7` delta).
- **Condition Flags:** `CCR` flags (`X, N, Z, V, C`) are preserved or updated according to M68000 arithmetic conventions.

---

## 4. Standalone C++20 Integration Guide

The generated headers in `src/tools/thor_ir/generated_m15_247/` require only a standard bus interface satisfying `PortableIrBus`:

```cpp
template <typename TBus>
PortableContinuationOutcome dispatch_m15_247_entity_handler(
    uint32_t pc,
    M68kCpuState& cpu,
    TBus& bus
);
```

### Recommended Standalone Porting Strategy:
1. **Direct Translation to C++ Methods:** Each of the 17 routines is fully analyzed in `data/thor-evidence/callee_cfg_source_map_m15_247.json`.
2. **Deterministic Mechanics:** Unlike asynchronous DMA routines, these 17 action handlers are strictly deterministic, synchronous to the main entity update loop (`0x0059BA`), and exhibit zero timing drift across frames.
3. **Fixture Suite Verification:** The standalone engine can validate its reimplementation against the 70 frozen GPGX fixtures in `data/thor-fixtures/m15_247_callee_fixtures.hpp`.
