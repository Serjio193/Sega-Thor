# Standalone Handoff: Milestone M15.475 — ROM Entity AI Isometric Projected Melee Hitbox & Reverse-Facing Collision Detection Engine

**ROM Range:** `0x010916..0x010A74` (350 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements screen-space isometric projected bounding box collision detection and reverse-facing (rear attack/defend) evaluation for enemy AI:

1. **4 Specialized Entrypoints (`0x010916..0x010934`):**
   - `0x010916` (`CheckHit_ReverseFacing_NoElevGate`):
     * Inverts facing bit 1: `bchg.b #1, d2` (e.g. Up `0` <-> Down `2`, Right `1` <-> Left `3`).
     * Skips elevation difference gate, directly evaluates isometric hitbox.
   - `0x010920` (`CheckHit_NormalFacing_NoElevGate`):
     * Uses current facing, directly evaluates isometric hitbox without elevation check.
   - `0x010926` (`CheckHit_ReverseFacing_WithElevGate`):
     * Inverts facing bit 1.
     * Evaluates elevation gate: `abs(ali_z - ent_z) < 24px` (`0x18`).
     * If elevation diff `>= 24px`: returns Carry clear (miss).
     * If within threshold: evaluates isometric hitbox.
   - `0x010930` (`CheckHit_NormalFacing_WithElevGate`):
     * Evaluates elevation gate (`< 24px`), then evaluates isometric hitbox.

2. **Isometric Screen Depth Projection (`y + z`):**
   - Unlike pure world coordinate tests, this routine projects 3D height into screen Y coordinates:
     * `ali_iso_y = ali_y + ali_z` (`$FF19F4 + $FF19FC`).
     * `ent_iso_y = ent_y + ent_z` (`+$0C(a6) + +$14(a6)`).

3. **4-Way Facing Collision Evaluator (`0x01095E..0x010A74`):**
   - Uses relative jump table at `0x010956`:
     * **Facing 0 (Up):**
       `ali_iso_y <= ent_iso_y` and `ent_iso_y - reach_fwd <= ali_iso_y`.
       `ent_x - span_lat <= ali_x <= ent_x + span_lat`.
     * **Facing 1 (Right):**
       `ent_x <= ali_x <= ent_x + reach_fwd`.
       `ent_iso_y - span_lat <= ali_iso_y <= ent_iso_y + span_lat`.
     * **Facing 2 (Down):**
       `ent_iso_y <= ali_iso_y <= ent_iso_y + reach_fwd`.
       `ent_x - span_lat <= ali_x <= ent_x + span_lat`.
     * **Facing 3 (Left):**
       `ent_x - reach_fwd <= ali_x <= ent_x`.
       `ent_iso_y - span_lat <= ali_iso_y <= ent_iso_y + span_lat`.
   - Returns Carry set (`ori.b #1, ccr; rts`) on hit.
   - Returns Carry clear (`andi.b #$FE, ccr; rts`) on miss.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `PLAYER_ALI_COORDS` | `$FF19F0` | Player Ali world coordinates (X at +$00, Y at +$04, Z at +$0C) |
| `SLOT_FACING_OFFSET` | `+$16` | Entity current facing direction (0=Up, 1=Right, 2=Down, 3=Left) |
| `SLOT_POS_X_OFFSET` | `+$08` | Entity world X coordinate (16-bit) |
| `SLOT_POS_Y_OFFSET` | `+$0C` | Entity world Y coordinate (16-bit) |
| `SLOT_POS_Z_OFFSET` | `+$14` | Entity elevation Z coordinate (16-bit) |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::ai {

enum class FacingDirection : uint16_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct IsometricHitboxParams {
    int16_t forward_reach{0};
    int16_t lateral_half_span{0};
    bool reverse_facing{false};
    bool enable_elevation_gate{true};
};

bool check_isometric_hitbox(
    int16_t ent_x, int16_t ent_y, int16_t ent_z,
    FacingDirection facing,
    const IsometricHitboxParams& params,
    int16_t ali_x, int16_t ali_y, int16_t ali_z);

} // namespace thor::ai
```
