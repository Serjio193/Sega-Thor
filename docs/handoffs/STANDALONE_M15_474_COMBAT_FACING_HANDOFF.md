# Standalone Handoff: Milestone M15.474 — ROM Entity AI Ali Facing Tracker, Elevation Gate & 4-Way Directional Melee Attack Hitbox Collision Engine

**ROM Range:** `0x01074A..0x010916` (460 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements Player Ali facing tracking, elevation-based proximity filtering, and directional melee attack bounding-box collision detection for enemy combat AI:

1. **`0x01074A..0x010752` (`DirectionAngleTable`):**
   - 4-word cardinal angle table:
     * `0x0040`: Up (90 degrees).
     * `0x0000`: Right (0 degrees).
     * `0x00C0`: Down (270 degrees).
     * `0x0080`: Left (180 degrees).

2. **`0x010752..0x0107AA` (`AI_FaceTowardsAliAndGetDistance`):**
   - Reads Player Ali coordinates `(ali_x, ali_y)` from `$FF19F0` / `$FF19F4`.
   - Computes delta vector `(dx, dy) = (ali_x - ent_x, ali_y - ent_y)`.
   - Compares absolute distances `abs(dx)` vs `abs(dy)`:
     * If `abs(dx) > abs(dy)`: dominant horizontal axis:
       - `dx > 0` -> Facing `1` (Right), distance = `abs(dx)`.
       - `dx <= 0` -> Facing `3` (Left), distance = `abs(dx)`.
     * If `abs(dy) >= abs(dx)`: dominant vertical axis:
       - `dy > 0` -> Facing `2` (Down), distance = `abs(dy)`.
       - `dy <= 0` -> Facing `0` (Up), distance = `abs(dy)`.
   - Stores facing into entity slot `+$16(a6)`.
   - Returns primary distance in `d5.w`.

3. **`0x0107AA..0x010916` (`AI_CheckMeleeAttackBoxHit`):**
   - Elevation gate: compares `abs(ali_z - ent_z)` against `24px` (`0x18`).
     * If `diff >= 24px`: returns Carry clear (miss).
   - 4-Way facing dispatch table at `0x0107D0`:
     * Takes attack box dimensions: forward reach `d2`, left lateral span `d0`, right lateral span `d1`.
     * Combines with Player Ali's hitbox dimensions `rx = $FF1A2A`, `ry = $FF1A2C`.
     * Evaluates directional bounding box intersection:
       - **Facing 0 (Up):** `ali_y <= ent_y`, `ent_y - reach <= ali_y + ry`, `ent_x - span_left <= ali_x + rx`, `ali_x - rx <= ent_x + span_right`.
       - **Facing 1 (Right):** `ali_x >= ent_x`, `ali_x - rx <= ent_x + reach`, `ent_y - span_left <= ali_y + ry`, `ali_y - ry <= ent_y + span_right`.
       - **Facing 2 (Down):** `ali_y >= ent_y`, `ali_y - ry <= ent_y + reach`, `ent_x - span_right <= ali_x + rx`, `ali_x - rx <= ent_x + span_left`.
       - **Facing 3 (Left):** `ali_x <= ent_x`, `ali_x + rx >= ent_x - reach`, `ent_y - span_right <= ali_y + ry`, `ali_y - ry <= ent_y + span_left`.
     * If intersection holds: returns Carry set (`ori.b #1, ccr; rts`, hit confirmed).
     * If outside box: returns Carry clear (`andi.b #$FE, ccr; rts`, miss).

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `PLAYER_ALI_COORDS` | `$FF19F0` | Player Ali coordinates (X at +$00, Y at +$04, Z at +$0C) |
| `PLAYER_ALI_HITBOX` | `$FF1A2A` | Player Ali collision radius (rx at +$00, ry at +$02) |
| `SLOT_FACING_OFFSET` | `+$16` | Entity current facing direction (0=Up, 1=Right, 2=Down, 3=Left) |
| `SLOT_POS_X_OFFSET` | `+$08` | Entity world X coordinate (16-bit) |
| `SLOT_POS_Y_OFFSET` | `+$0C` | Entity world Y coordinate (16-bit) |
| `SLOT_POS_Z_OFFSET` | `+$14` | Entity elevation Z coordinate (16-bit) |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::ai {

enum class Direction : uint16_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct AttackHitbox {
    int16_t reach_forward{0};
    int16_t span_left{0};
    int16_t span_right{0};
};

struct AliState {
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
    int16_t rx{12};
    int16_t ry{12};
};

struct FacingDistanceResult {
    Direction facing{Direction::Right};
    int16_t primary_distance{0};
};

FacingDistanceResult calculate_facing_towards_ali(
    int16_t ent_x, int16_t ent_y,
    const AliState& ali);

bool check_melee_attack_hit(
    int16_t ent_x, int16_t ent_y, int16_t ent_z,
    Direction facing,
    const AttackHitbox& hitbox,
    const AliState& ali);

} // namespace thor::ai
```
