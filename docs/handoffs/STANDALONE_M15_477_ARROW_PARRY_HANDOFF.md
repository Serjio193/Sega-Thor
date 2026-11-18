# Standalone Handoff: Milestone M15.477 — ROM Entity AI Player Bow Arrow Attack Parry & Shield Interception Engine

**ROM Range:** `0x010BF0..0x010CFE` (270 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements enemy shield defense, projectile blocking, and collision interception against Player Ali's Bow Arrow attacks:

1. **Pre-filter Validations (`0x010BF0..0x010C06`):**
   - Inspects fixed projectile slot `$FF1B60` (address `a0`):
     * Entity type must be `#$3A` (Player Ali Bow Arrow).
     * Elevation coordinate `+$14(a0)` must exactly equal enemy elevation `+$14(a6)`.
     * If type or elevation mismatch: returns Carry clear (`andi.b #$FE, ccr; rts`, arrow cannot be blocked).

2. **4-Way Directional Shield Reach Interception (`0x010C0A..0x010CFE`):**
   - Uses relative jump table at `0x010C18`:
     * **Facing 0 (Up):**
       - Forward Y check: `arrow_y <= enemy_y` and `enemy_y - shield_reach <= arrow_y`.
       - Lateral X check: `enemy_x - span_lat <= arrow_x <= enemy_x + span_lat`.
     * **Facing 1 (Right):**
       - Forward X check: `arrow_x >= enemy_x` and `arrow_x <= enemy_x + shield_reach`.
       - Lateral Y check: `enemy_y - span_lat <= arrow_y <= enemy_y + span_lat`.
     * **Facing 2 (Down):**
       - Forward Y check: `arrow_y >= enemy_y` and `arrow_y <= enemy_y + shield_reach`.
       - Lateral X check: `enemy_x - span_lat <= arrow_x <= enemy_x + span_lat`.
     * **Facing 3 (Left):**
       - Forward X check: `arrow_x <= enemy_x` and `arrow_x >= enemy_x - shield_reach`.
       - Lateral Y check: `enemy_y - span_lat <= arrow_y <= enemy_y + span_lat`.
   - On intersection: returns Carry set (`ori.b #1, ccr; rts`, arrow blocked/parried by shield).
   - On miss: returns Carry clear (`andi.b #$FE, ccr; rts`).

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `PROJECTILE_SLOT_BASE` | `$FF1B60` | Fixed projectile entity slot (Type $3A arrow) |
| `SLOT_TYPE_OFFSET` | `+$00` | Entity type word (Type $3A = Player Ali Bow Arrow) |
| `SLOT_FACING_OFFSET` | `+$16` | Entity current facing direction (0=Up, 1=Right, 2=Down, 3=Left) |
| `SLOT_POS_X_OFFSET` | `+$08` | Entity world X coordinate (16-bit) |
| `SLOT_POS_Y_OFFSET` | `+$0C` | Entity world Y coordinate (16-bit) |
| `SLOT_POS_Z_OFFSET` | `+$14` | Entity elevation Z coordinate (16-bit) |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::ai {

struct ArrowProjectileState {
    uint16_t type{0};
    int16_t x{0};
    int16_t y{0};
    int16_t pos_z{0};
};

struct ArrowShieldTarget {
    uint16_t facing{0};
    int16_t x{0};
    int16_t y{0};
    int16_t pos_z{0};
};

bool check_bow_arrow_parry(
    const ArrowProjectileState& arrow,
    const ArrowShieldTarget& enemy,
    int16_t lateral_half_span,
    int16_t forward_reach);

} // namespace thor::ai
```
