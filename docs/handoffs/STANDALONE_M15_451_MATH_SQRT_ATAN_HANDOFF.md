# STANDALONE HANDOFF: Milestone M15.451 — ROM Newton-Raphson Integer Square Root & 256-Step Circular Arctan Engine

**ROM Range:** `0x00E6BA..0x00E78E` (212 bytes, 68 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_math_sqrt_atan_m15_451.json`  
**Reference Extractor:** `tools/extract_rom_math_sqrt_atan.py`  
**Reference Unit Test:** `tests/rom_math_sqrt_atan_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers the mathematical primitives used throughout Beyond Oasis for distance, proximity, circular angle resolution, and projectile trajectories:
1. Fast integer square root via Newton-Raphson division refinement (`0x00E6BA..0x00E6F0`, 48 ROM callers!).
2. 256-step circular arctan angle resolver (`0x00E6F0..0x00E78E`, 42 ROM callers!) with four-quadrant sign reflection and a 9-entry tangent threshold table at ROM `0x05D906`.

### Subroutines in this Range:
1. `0x00E6BA..0x00E6F0` (`integer_sqrt_divu_newton_raphson` — 48 callers!):
   - Computes integer square root `floor(sqrt(d0))` for any 32-bit unsigned integer `d0`.
   - Initial guess:
     - If `d0 < 0x400` (1024): initial guess `d2 = d0`.
     - Else: initial guess clamped to `d2 = 0x400`.
   - Newton-Raphson refinement:
     - Loop: `d1 = d2; d2 = (d0 / d1 + d1) / 2` (using M68K `divu.w`).
     - Terminates when `d2 >= d1`, returning converged root `d0 = d1`.

2. `0x00E6F0..0x00E706` (`trig_atan2_target_coords` — 14 callers):
   - Relative target angle: `dx = target_x (d1) - entity_x ($8(a6))`, `dy = entity_y ($C(a6)) - target_y (d2)`.
   - Dispatches to four-quadrant resolver `0x00E722`.

3. `0x00E706..0x00E768` (`trig_atan2_player_ali` — 28 callers):
   - Relative Player Ali angle: `dx = player_x ($FF19E8) - entity_x ($8(a6))`, `dy = entity_y ($8(a6)) - player_y ($FF19E8)`.
   - Four-quadrant mapping:
     - `dx >= 0, dy >= 0` (Q1): `angle = octant(dx, dy)`.
     - `dx >= 0, dy < 0` (Q4): `angle = -octant(dx, -dy)`.
     - `dx < 0, dy >= 0` (Q2): `angle = -octant(-dx, dy) + 0x80`.
     - `dx < 0, dy < 0` (Q3): `angle = octant(-dx, -dy) + 0x80`.

4. `0x00E768..0x00E78E` (`trig_atan_first_octant_table_search`):
   - If `dx == 0`: returns `0x40` (64 = 90 degrees).
   - Computes fixed-point slope: `slope = (dy << 8) / dx` via `divu.w`.
   - Compares against 9-entry tangent table at `0x05D906`:
     - `{25, 78, 137, 210, 312, 479, 844, 2599, 65535}`.
   - For each table entry where `slope > entry`, increments angle by `+8`.
   - Returns 256-step angle in `[0..64]` for first octant.

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::math {

constexpr uint16_t isqrt(uint32_t val) noexcept {
    if (val == 0) return 0;
    uint32_t d2 = (val < 0x400U) ? val : 0x400U;

    while (true) {
        uint32_t d1 = d2;
        d2 = (val / d1 + d1) / 2U;
        if (d2 >= d1) {
            return static_cast<uint16_t>(d1);
        }
    }
}

inline constexpr uint16_t TANGENT_OCTANT_TABLE[9] = {
    25, 78, 137, 210, 312, 479, 844, 2599, 65535
};

constexpr uint8_t atan2_256(int16_t dx, int16_t dy) noexcept {
    auto first_octant = [](int16_t x, int16_t y) noexcept -> uint8_t {
        if (x == 0) return 64U; // 90 degrees
        uint32_t slope = (static_cast<uint32_t>(y) << 8) / static_cast<uint32_t>(x);
        uint8_t angle = 0;
        for (uint16_t threshold : TANGENT_OCTANT_TABLE) {
            if (slope <= threshold) break;
            angle += 8U;
        }
        return angle;
    };

    if (dx >= 0) {
        if (dy >= 0) {
            return first_octant(dx, dy);
        } else {
            return static_cast<uint8_t>(-first_octant(dx, static_cast<int16_t>(-dy)));
        }
    } else {
        int16_t neg_x = static_cast<int16_t>(-dx);
        if (dy >= 0) {
            return static_cast<uint8_t>(-first_octant(neg_x, dy) + 0x80U);
        } else {
            return static_cast<uint8_t>(first_octant(neg_x, static_cast<int16_t>(-dy)) + 0x80U);
        }
    }
}

} // namespace oasis::math
```

---

## 3. Acceptance Criteria

1. **Integer Square Root Parity:** 100% agreement with M68K Newton-Raphson division convergence.
2. **Tangent Table Exactness:** Matches ROM `0x05D906` constants `[25, 78, 137, 210, 312, 479, 844, 2599, 65535]`.
3. **256-Step Circular Angle:** Exact sector assignment (East=0, South=64, West=128, North=192).
4. All C++ source and test files must strictly stay `<= 500 lines`.
