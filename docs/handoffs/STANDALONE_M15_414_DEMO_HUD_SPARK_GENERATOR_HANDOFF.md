# Standalone Handoff: M15.414 ROM Demo Mode Overlay SAT Compositor & Camera Particle Spark Generator

## Summary
- **Source Address Range:** `0x00A7E6..0x00A9A0` (442 bytes total)
- **Target Area:** Demo Overlay & Camera Particle Sprite Rendering (`demo_overlay.cpp` / `particle_sparks.hpp`)
- **Evidence File:** `data/thor-evidence/rom_demo_hud_spark_generator_m15_414.json`
- **Unit Test File:** `tests/rom_demo_hud_spark_generator_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x00A7E6`: Demo Mode Overlay SAT Compositor
- **RAM Inputs / State:**
  - `$FF1656.w`: Demo overlay status register. Bit 7 set suppresses overlay.
  - Bit 6 set: 3-sprite mode (`X = [0xF0, 0x110, 0x130]`, `Y = 0xE4`). Queues DMA from ROM `0x000AA1FD` to VRAM `$F360` (size `0xC0` words) if `$FF17C0.b != 1`.
  - Bit 6 clear: 2-sprite mode (`X = [0x10C, 0x124]`, `Y = 0xE4`). Queues DMA from ROM `0x000AA1AD` to VRAM `$F360` (size `0x50` words) if `$FF17C0.b != 0xFF`.
  - SAT buffer pointer `$FF13CC + $FF188C.w` and current sprite index `$FF188A.w`. Max sprite count capped at 80 (`0x50`).

### 2. `0x00A8F0`: Camera-Relative Particle Spark SAT Generator
- **Registers:**
  - `a6`: Particle / spark entity pointer (`$6(a6)` = lifetime timer, `$4(a6)` = world X, `$2(a6)` = world Y).
  - `a1`: Camera scroll plane register (`$FF1716` Layer 1 vs `$FF173E` Layer 2, selected by bit 7 of `$FF16F0`).
- **M68K Logic:**
  - Decrements lifetime: `subq.w #1, $6(a6)`. If `<= 0`, clears slot `clr.w (a6)` and returns.
  - If current hardware sprite count `$FF188A >= 0x50`, returns without emitting.
  - Computes screen coordinates:
    `screen_y = world_y - cam_y + 0x78` (120)
    `screen_x = world_x - cam_x + 0x78` (120)
  - Screen bounds clipping:
    `adj_y = screen_y - 0x70` (112); if `adj_y > 0xF0` (240), dropped.
    `adj_x = screen_x - 0x70` (112); if `adj_x > 0x150` (336), dropped.
  - If in bounds: emits sprite to SAT, advances sprite count by 1 and SAT byte offset by 8.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::rendering {

struct SparkParticleState {
    int16_t type{0};
    int16_t world_x{0};
    int16_t world_y{0};
    int16_t lifetime{0};
};

inline bool process_camera_spark(
    SparkParticleState& spark,
    int16_t cam_x,
    int16_t cam_y,
    uint16_t current_sprites,
    auto&& sat_emitter) {
    if (--spark.lifetime <= 0) {
        spark.type = 0;
        return false;
    }

    if (current_sprites >= 80) {
        return false;
    }

    const int16_t screen_y = (spark.world_y - cam_y) + 120;
    const int16_t screen_x = (spark.world_x - cam_x) + 120;

    const int16_t adj_y = screen_y - 112;
    if (adj_y < 0 || adj_y > 240) {
        return false;
    }

    const int16_t adj_x = screen_x - 112;
    if (adj_x < 0 || adj_x > 336) {
        return false;
    }

    sat_emitter(screen_x, screen_y);
    return true;
}

} // namespace oasis::rendering
```
