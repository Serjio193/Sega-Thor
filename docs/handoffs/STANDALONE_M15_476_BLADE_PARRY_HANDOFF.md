# Standalone Handoff: Milestone M15.476 — ROM Entity AI Player Elemental Blade Attack Parry & Shield Interception Engine

**ROM Range:** `0x010A74..0x010BF0` (380 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements enemy shield defense, projectile parrying, and collision interception against Player Ali's Elemental Sword Wave attacks:

1. **Pre-filter Validations (`0x010A74..0x010AA2`):**
   - Inspects fixed projectile slot `$FF1B60` (address `a0`):
     * Entity type must be `#$34` (Elemental Blade Wave).
     * Animation/attack state `+$2A(a0)` must be `2` or `4` (active damaging slash phase).
   - Facing direction comparison:
     * Enemy facing `+$16(a6)` must be directly opposite to blade direction `+$16(a0)`:
       `d1 = +$16(a6); bchg.b #1, d1; cmp.w +$16(a0), d1`.
     * Attack from the rear bypasses shield and cannot be parried.

2. **3D Vertical Height Overlap Check (`0x010AA6..0x010AC8`):**
   - Projects 3D height intervals:
     * Blade: `z_blade = +$10(a0) + +$14(a0)`, top = `z_blade + +$4A(a0)`.
     * Enemy: `z_enemy = +$10(a6) + +$14(a6)`, top = `z_enemy + +$4A(a6)`.
   - Requires bounding interval intersection:
     `z_enemy + height_enemy >= z_blade` and `z_enemy <= z_blade + height_blade`.
   - If intervals do not overlap: returns Carry clear (`andi.b #$FE, ccr; rts`).

3. **4-Way Directional Shield Reach Interception (`0x010ACC..0x010BF0`):**
   - Uses relative jump table at `0x010ADA`:
     * **Facing 0 (Up):**
       - Lateral X overlap: `[x_enemy - rx_enemy, x_enemy + rx_enemy]` intersects `[x_blade - rx_blade, x_blade + rx_blade]`.
       - Forward Y reach: `y_enemy - shield_reach <= y_blade + ry_blade`.
     * **Facing 1 (Right):**
       - Lateral Y overlap: `[y_enemy - ry_enemy, y_enemy + ry_enemy]` intersects `[y_blade - ry_blade, y_blade + ry_blade]`.
       - Forward X reach: `x_enemy + shield_reach >= x_blade - rx_blade`.
     * **Facing 2 (Down):**
       - Lateral X overlap.
       - Forward Y reach: `y_enemy + shield_reach >= y_blade - ry_blade`.
     * **Facing 3 (Left):**
       - Lateral Y overlap.
       - Forward X reach: `x_enemy - shield_reach <= x_blade + rx_blade`.
   - On intersection: returns Carry set (`ori.b #1, ccr; rts`, blade parried/blocked).
   - On miss: returns Carry clear (`andi.b #$FE, ccr; rts`).

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `PROJECTILE_SLOT_BASE` | `$FF1B60` | Fixed projectile entity slot (Type $34 blade, Type $3A arrow) |
| `SLOT_TYPE_OFFSET` | `+$00` | Entity type word (Type $34 = Elemental Blade Wave) |
| `SLOT_STATE_OFFSET` | `+$2A` | Entity active state word (2 or 4 = active damaging phase) |
| `SLOT_FACING_OFFSET` | `+$16` | Entity current facing direction (0=Up, 1=Right, 2=Down, 3=Left) |
| `SLOT_RAD_X_OFFSET` | `+$42` | Lateral collision half-width rx |
| `SLOT_RAD_Y_OFFSET` | `+$44` | Lateral collision half-depth ry |
| `SLOT_HEIGHT_OFFSET` | `+$4A` | Vertical height dimension |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::ai {

struct ProjectileSlotState {
    uint16_t type{0};
    uint16_t state_2a{0};
    uint16_t facing{0};
    int16_t x{0};
    int16_t y{0};
    int16_t rx{0};
    int16_t ry{0};
    int16_t pos_z{0};
    int16_t elev_z{0};
    int16_t height{0};
};

struct EnemyShieldState {
    uint16_t facing{0};
    int16_t x{0};
    int16_t y{0};
    int16_t rx{0};
    int16_t ry{0};
    int16_t pos_z{0};
    int16_t elev_z{0};
    int16_t height{0};
};

bool check_elemental_blade_parry(
    const ProjectileSlotState& projectile,
    const EnemyShieldState& enemy,
    int16_t shield_reach);

} // namespace thor::ai
```
