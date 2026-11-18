# STANDALONE HANDOFF: Milestone M15.449 — ROM Player Ali Targeting, Normalized Aim Vector & Environmental Velocity Engine

**ROM Range:** `0x00E398..0x00E502` (362 bytes, 120 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_player_targeting_m15_449.json`  
**Reference Extractor:** `tools/extract_rom_player_targeting.py`  
**Reference Unit Test:** `tests/rom_player_targeting_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers Player Ali relative targeting, aim vector normalization using fixed-point M68K `divs.w`, and directional projectile launch physics.

### Subroutines in this Range:
1. `0x00E398..0x00E3E8` (`target_player_ali_quadrant_angle`):
   - Computes delta to Player Ali: `dx = player_x ($FF19F0) - entity_x ($8(a6))`, `dy = player_y ($FF19F4) - entity_y ($C(a6))`.
   - Computes absolute deltas: `abs_x = |dx|`, `abs_y = |dy|`.
   - Calculates Manhattan distance: `dist = abs_x + abs_y` in `d5`.
   - Compares `abs_y` and `abs_x`:
     - If `abs_y > abs_x`:
       - `dy < 0` -> direction `d0 = 0` (Up)
       - `dy >= 0` -> direction `d0 = 2` (Down)
     - Else:
       - `dx < 0` -> direction `d0 = 3` (Left)
       - `dx >= 0` -> direction `d0 = 1` (Right)
   - Returns direction code in `d0` (`0`=Up, `1`=Right, `2`=Down, `3`=Left), and Manhattan distance in `d5`.

2. `0x00E3E8..0x00E45E` (`target_player_ali_and_load_motion_table`):
   - Calculates dominant cardinal direction to Ali into `d0` (1=Right, 3=Left, 2=Down, 0=Up).
   - Stores facing direction: `+$16(a6) = d0`.
   - Scales direction offset: `table_offset = d0 << 3` (8 bytes per entry).
   - Loads velocity vector from `(a0, table_offset)` into entity velocity accumulators:
     - `+$4E(a6) = *(int32_t*)(a0 + table_offset)`
     - `+$52(a6) = *(int32_t*)(a0 + table_offset + 4)`
   - Computes dominant axis distance `d5`:
     - Clamps to minimum 16 pixels: if `d5 < 16`, `d5 = 16`.
     - Assigns to entity motion countdown timer: `+$02(a6) = d5`.

3. `0x00E45E..0x00E4CE` (`normalize_target_aim_vector_divs`):
   - Inputs: target coordinates `(target_x, target_y)`.
   - Delta from entity: `dx = target_x - entity_x`, `dy = target_y - entity_y`.
   - Dominant axis half-span:
     - `d4 = |dx|`, `d5 = |dy|`.
     - `d3 = 1` (X dominant); if `d4 <= d5`: `d4 = d5`, `d3 = 0` (Y dominant).
     - `half_span = d4 >> 1`.
   - If `half_span == 0`: returns zero velocity `(0, 0)` and retains existing facing `+$16(a6)`.
   - Fixed-point division:
     - `q_x = ((int32_t)dx << 8) / half_span` (via M68K `divs.w`).
     - `q_y = ((int32_t)dy << 8) / half_span` (via M68K `divs.w`).
     - Velocity components shifted `<< 8` to form standard 16.16 fixed-point velocities:
       - `vel_x = q_x << 8`, `vel_y = q_y << 8`.
   - Quadrant refinement into `d3`:
     - If `d3 == 1` (X dominant): if `vel_x < 0`, `d3 = 3` (Left) else `1` (Right).
     - If `d3 == 0` (Y dominant): if `vel_y >= 0`, `d3 = 2` (Down) else `0` (Up).

4. `0x00E4D0..0x00E502` (`launch_directional_projectile_physics`):
   - Evaluates suppression mask: `if (d7 & 0xC000) != 0` -> returns immediately.
   - Loads base projection speeds:
     - `speed_x = 0x0300` (768 subpixels / frame).
     - `speed_y = 0x02E0` (736 subpixels / frame; ~0.733x isometric ratio).
   - Reads projectile direction from `+$84(a6)`.
   - Calls `0x00E2D4` (`project_velocity_8way` from M15.448).
   - Stores resulting velocities into `+$4E(a6)` and `+$52(a6)`.
   - Initializes upward launch velocity: `+$56(a6) = 0x00060000` (+6.0 fixed-point).

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::targeting {

struct TargetResult {
    uint8_t direction{0}; // 0: Up, 1: Right, 2: Down, 3: Left
    uint16_t distance{0};
};

struct NormalizedAimVector {
    int32_t vel_x{0};     // 16.16 fixed point
    int32_t vel_y{0};     // 16.16 fixed point
    uint8_t quadrant{0};  // 0: Up, 1: Right, 2: Down, 3: Left
};

constexpr TargetResult target_player_quadrant(int16_t player_x, int16_t player_y,
                                             int16_t entity_x, int16_t entity_y) noexcept {
    int16_t dx = player_x - entity_x;
    int16_t dy = player_y - entity_y;
    uint16_t abs_x = static_cast<uint16_t>((dx < 0) ? -dx : dx);
    uint16_t abs_y = static_cast<uint16_t>((dy < 0) ? -dy : dy);
    uint16_t manhattan = abs_x + abs_y;

    uint8_t dir = 0;
    if (abs_y > abs_x) {
        dir = (dy < 0) ? 0U : 2U; // Up : Down
    } else {
        dir = (dx < 0) ? 3U : 1U; // Left : Right
    }
    return {dir, manhattan};
}

constexpr NormalizedAimVector normalize_aim_vector(int16_t target_x, int16_t target_y,
                                                  int16_t entity_x, int16_t entity_y,
                                                  uint8_t current_facing) noexcept {
    int16_t dx = target_x - entity_x;
    int16_t dy = target_y - entity_y;
    int16_t abs_x = (dx < 0) ? -dx : dx;
    int16_t abs_y = (dy < 0) ? -dy : dy;

    uint8_t dominant_axis = 1; // X dominant
    int16_t max_span = abs_x;
    if (abs_x <= abs_y) {
        max_span = abs_y;
        dominant_axis = 0; // Y dominant
    }

    int16_t half_span = max_span >> 1;
    if (half_span == 0) {
        return {0, 0, current_facing};
    }

    int32_t q_x = (static_cast<int32_t>(dx) << 8) / half_span;
    int32_t q_y = (static_cast<int32_t>(dy) << 8) / half_span;

    int32_t vel_x = q_x << 8;
    int32_t vel_y = q_y << 8;

    if (vel_x == 0 && vel_y == 0) {
        return {0, 0, current_facing};
    }

    uint8_t quad = dominant_axis;
    if (dominant_axis == 1) {
        if (vel_x < 0) quad += 2; // 3: Left
    } else {
        if (vel_y >= 0) quad += 2; // 2: Down
    }

    return {vel_x, vel_y, quad};
}

} // namespace oasis::targeting
```

---

## 3. Acceptance Criteria

1. **Quadrant & Manhattan Distance Parity:** Identical quadrant resolution and Manhattan distance calculation with ROM `0x00E398`.
2. **Motion Table Offset & Distance Clamping:** Exact `d0 * 8` table offset and `max(dist, 16)` clamp.
3. **Aim Vector Normalization:** Exact 16.16 velocity outputs matching `divs.w` fixed-point behavior.
4. **Projectile Launch Parameters:** Speed constants `0x0300, 0x02E0`, mask check `d7 & 0xC000`, and upward launch velocity `0x00060000`.
5. All C++ source and test files must strictly stay `<= 500 lines`.
