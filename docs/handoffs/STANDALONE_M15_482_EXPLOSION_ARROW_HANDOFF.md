# STANDALONE HANDOFF: Milestone M15.482 — ROM Entity AI Environmental Explosion Waves, Screen Shake & Arrow Proximity AABB Interceptor

## 1. Subsystem Overview
- **ROM Address Range:** `0x0114AE..0x0115C8` (282 bytes, 78 instructions).
- **Core Responsibilities:**
  1. `0x0114AE` (`AI_SpawnEnvironmentalExplosionWave`): Calculates centered explosion coordinates across dual camera planes: `base_x = 0x100 - plane_b_x + plane_a_x`, `base_y = 0x80 - plane_b_y + plane_a_y`. Applies random scatter `0x00E268`, spawns particle explosion wave `0x00E90E` with radius `0x14`, and computes screen shake counter `$FF17C4 = (frame & 1) + 2` using frame counter `$FF1971`.
  2. `0x011508` (`AI_SpawnEntityGroundPoundImpactWave`): Centers explosion wave on entity position `+$8/+$C(a6)` with elevation `d3 = 0x20`, applies quarter-scale scatter `(0x00E268 >> 2)` across (x, y, z), spawns particle wave `0x00E90E`, and sets screen shake counter `$FF17C4 = (frame & 1) + 2`.
  3. `0x011554` (`AI_CheckBowArrowProximityHitbox`): Intercepts bow arrow attacks from projectile slot `$FF1B60`. Verifies type `$3A` (bow arrow), active attack `+$2A != 0`, and identical elevation `+$14(a0) == +$14(a6)`. Evaluates 2D AABB bounding box collision within radius `d0`: `abs(arrow_x - self_x) <= d0` and `abs(arrow_y - self_y) <= d0`. Returns Carry set on hit (inside proximity), or Carry clear on miss.
  4. `0x0115B6` (`AI_JumpToProjectileTrigVelocity`): Loads projectile coordinates `$FF1B68/$FF1B6C` and jumps to trigonometric velocity resolver `0x00E3A4`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_entity_ai_explosion_arrow_m15_482.json`
- Extractor: `tools/extract_rom_entity_ai_explosion_arrow.py`
- Test: `tests/rom_entity_ai_explosion_arrow_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <cmath>

namespace Thor::AI {

inline uint16_t ComputeScreenShakeTimer(uint8_t frame_counter) noexcept {
    return static_cast<uint16_t>((frame_counter & 1) + 2);
}

struct DualPlaneCoordinates {
    int16_t plane_a_x;
    int16_t plane_a_y;
    int16_t plane_b_x;
    int16_t plane_b_y;
};

struct WorldPoint2D {
    int16_t x;
    int16_t y;
};

inline WorldPoint2D ComputeEnvironmentalExplosionOrigin(const DualPlaneCoordinates& planes) noexcept {
    int16_t base_x = 0x100 - planes.plane_b_x + planes.plane_a_x;
    int16_t base_y = 0x80 - planes.plane_b_y + planes.plane_a_y;
    return { base_x, base_y };
}

struct ArrowProjectileState {
    uint16_t type{0};      // Must be 0x3A (Bow Arrow)
    uint16_t state_2a{0};   // Non-zero if active attack
    int16_t z_14{0};       // Elevation
    int16_t x{0};
    int16_t y{0};
};

inline bool CheckBowArrowProximityAABB(const ArrowProjectileState& arrow,
                                     int16_t entity_x, int16_t entity_y, int16_t entity_z,
                                     int16_t radius) noexcept {
    if (arrow.type != 0x3A || arrow.state_2a == 0 || arrow.z_14 != entity_z) {
        return false;
    }

    if (arrow.y < entity_y - radius || arrow.y > entity_y + radius) {
        return false;
    }

    if (arrow.x < entity_x - radius || arrow.x > entity_x + radius) {
        return false;
    }

    return true;
}

} // namespace Thor::AI
```
