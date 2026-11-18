# STANDALONE HANDOFF: Milestone M15.448 — ROM Core PRNG Subsystem, 8-Way Directional Velocity Projector & Cardinal Angle Resolver

**ROM Range:** `0x00E25C..0x00E398` (316 bytes, 122 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_prng_vector_engine_m15_448.json`  
**Reference Extractor:** `tools/extract_rom_prng_vector_engine.py`  
**Reference Unit Test:** `tests/rom_prng_vector_engine_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers the ubiquitous pseudo-random number generator and directional projection engine in Beyond Oasis / The Story of Thor.
With over **206 ROM callers** referencing `0x00E268` and **104 callers** referencing `0x00E2D4`, this is one of the highest-fan-in mathematical core modules in the game.

### Subroutines in this Range:
1. `0x00E25C..0x00E268` (`entity_lifecycle_timer_countdown`):
   - Decrements word at `+$02(a6)`.
   - If decremented value reaches 0, clears byte at `+$00(a6)` (despawning/deactivating entity slot).
2. `0x00E268..0x00E2A2` (`prng_step_16bit_lfsr` — 206 callers!):
   - Reads 32-bit PRNG state from `$FF1928`.
   - Executes 4 sequential Galois/Fibonacci XOR-shift and rotate rounds:
     - `d0 = seed`, `d1 = (d0 >> 1) ^ (d0 >> 2) ^ ...`
     - Shifts, masks, and updates 32-bit state at `$FF1928`.
     - Returns pseudo-random 16-bit unsigned integer in `d0.w`.
3. `0x00E2A2..0x00E2BA` (`prng_range_word` — 63 callers!):
   - Input: `d0.w = N` (range limit).
   - Calls `prng_step_16bit_lfsr` to obtain random `rand16`.
   - Performs fixed-point integer scaling: `(rand16 * N) >> 16`.
   - Returns uniformly distributed random value in range `[0..N-1]`.
4. `0x00E2BA..0x00E2D4` (`prng_range_byte`):
   - Input: `d0 = N`. Masks `d0 &= 0xFF`.
   - Scales random word by `(N & 0xFF) >> 16`.
   - Returns random integer in range `[0..(N & 0xFF) - 1]`.
5. `0x00E2D4..0x00E338` (`vector_directional_projection_8way` — 104 callers!):
   - Inputs:
     - `d1.w`: horizontal speed / magnitude.
     - `d2.w`: vertical speed / magnitude.
     - `d3.w`: 8-way direction index (`0..7`):
       - `0`: Up (`dy = -d2`, `dx = 0`)
       - `1`: Up-Right (`dx = +d1 * 3/4`, `dy = -d2 * 3/4`)
       - `2`: Right (`dx = +d1`, `dy = 0`)
       - `3`: Down-Right (`dx = +d1 * 3/4`, `dy = +d2 * 3/4`)
       - `4`: Down (`dx = 0`, `dy = +d2`)
       - `5`: Down-Left (`dx = -d1 * 3/4`, `dy = +d2 * 3/4`)
       - `6`: Left (`dx = -d1`, `dy = 0`)
       - `7`: Up-Left (`dx = -d1 * 3/4`, `dy = -d2 * 3/4`)
   - Diagonal scaling in ROM uses exact 3/4 fixed-point isometric ratio:
     - `diag = ((val >> 1) + (val >> 2))` (e.g., `0.5 + 0.25 = 0.75`).
   - Outputs: `d1.w` = signed X velocity, `d2.w` = signed Y velocity.
6. `0x00E338..0x00E36E` (`vector_to_cardinal_direction_4way` — 16 callers):
   - Inputs: `d1.w = dx`, `d2.w = dy`.
   - Computes absolute magnitudes `|dx|` and `|dy|`.
   - If `|dx| > |dy|`:
     - If `dx >= 0` -> direction `d3 = 2` (Right).
     - Else -> direction `d3 = 6` (Left).
   - Else:
     - If `dy >= 0` -> direction `d3 = 4` (Down).
     - Else -> direction `d3 = 0` (Up).
7. `0x00E36E..0x00E398` (`entity_ground_water_surface_probe` — 30 callers):
   - Reads entity world position `(x, y)` from `+$04(a6), +$08(a6)`.
   - Calls terrain tile attribute lookup (`0x009C40` / `0x009C7C`).
   - Checks if tile attribute byte has high bit set (`attr & 0x80`, indicating water / liquid surface).
   - If water: sets bit 3 of entity status byte `+$36(a6)`.

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::math {

struct PrngState {
    uint32_t seed{0x1A2B3C4D};

    constexpr uint16_t step() noexcept {
        // 4 rounds of Galois XOR-shift matching 0x00E268:
        for (int round = 0; round < 4; ++round) {
            uint32_t bit = ((seed >> 0) ^ (seed >> 2) ^ (seed >> 3) ^ (seed >> 5)) & 1U;
            seed = (seed >> 1) | (bit << 31);
        }
        return static_cast<uint16_t>(seed & 0xFFFFU);
    }

    constexpr uint16_t range_word(uint16_t limit) noexcept {
        if (limit == 0) return 0;
        uint32_t r = step();
        return static_cast<uint16_t>((r * static_cast<uint32_t>(limit)) >> 16);
    }

    constexpr uint8_t range_byte(uint8_t limit) noexcept {
        if (limit == 0) return 0;
        return static_cast<uint8_t>(range_word(limit));
    }
};

struct Velocity2D {
    int16_t dx{0};
    int16_t dy{0};
};

constexpr Velocity2D project_velocity_8way(int16_t speed_x, int16_t speed_y, uint8_t direction) noexcept {
    constexpr auto scale_3_4 = [](int16_t val) noexcept -> int16_t {
        return static_cast<int16_t>((val >> 1) + (val >> 2));
    };

    switch (direction & 7U) {
        case 0: return {0, static_cast<int16_t>(-speed_y)};
        case 1: return {scale_3_4(speed_x), static_cast<int16_t>(-scale_3_4(speed_y))};
        case 2: return {speed_x, 0};
        case 3: return {scale_3_4(speed_x), scale_3_4(speed_y)};
        case 4: return {0, speed_y};
        case 5: return {static_cast<int16_t>(-scale_3_4(speed_x)), scale_3_4(speed_y)};
        case 6: return {static_cast<int16_t>(-speed_x), 0};
        case 7: return {static_cast<int16_t>(-scale_3_4(speed_x)), static_cast<int16_t>(-scale_3_4(speed_y))};
        default: return {0, 0};
    }
}

constexpr uint8_t resolve_cardinal_direction_4way(int16_t dx, int16_t dy) noexcept {
    int32_t abs_x = (dx < 0) ? -dx : dx;
    int32_t abs_y = (dy < 0) ? -dy : dy;

    if (abs_x > abs_y) {
        return (dx >= 0) ? 2U : 6U; // Right : Left
    } else {
        return (dy >= 0) ? 4U : 0U; // Down : Up
    }
}

} // namespace oasis::math
```

---

## 3. Acceptance Criteria

1. **Deterministic PRNG parity:** 100% agreement with M68K LFSR sequence.
2. **Integer scaling:** `range_word` and `range_byte` produce values strictly within `[0..limit-1]`.
3. **8-Way Projection:** Exact 3/4 isometric scaling on diagonal directions `1, 3, 5, 7`.
4. **Cardinal direction resolver:** Correct resolution across all 4 quadrants and equal-magnitude axes.
5. All C++ source/test files must strictly stay `<= 500 lines`.
