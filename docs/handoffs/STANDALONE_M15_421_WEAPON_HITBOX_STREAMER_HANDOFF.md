# Standalone Handoff M15.421: ROM Weapon Attack Frame 3D Hitbox Streamer & Dual-Pool Scanner Engine

- **Milestone:** M15.421
- **ROM Source Range:** `0x00BAAC..0x00BC8E` (482 bytes)
- **SHA256:** `fd4ba425536aa8ff8fc1b5396c3869ae0243ee5b1f4c32cc367ef7dd13934312`

---

## 1. Subsystem Responsibility

The weapon attack frame hitbox streamer streams 3D bounding boxes from weapon frame tables and tests them against active entities:
1. **Hitbox Stream Format**:
   - Each entry in stream `a1` is 12 bytes (6 signed 16-bit words):
     `[z_min, z_max, x_min, y_min, x_max, y_max]`.
   - Indexed by frame offset `d0 * 12`.
2. **Left-Facing Horizontal Mirroring**:
   - When facing left (`+0x16(a6) == 3`), flips horizontal span:
     `x_min_mirrored = -x_max`, `x_max_mirrored = -x_min`.
3. **3D World Space Projection**:
   - `Z`: adds `elevation_z (+0x10) + ground_height (+0x14)`
   - `X`: adds `world_x (+0x08)`
   - `Y`: adds `world_y (+0x0C) + ground_height (+0x14)`
4. **Dual-Pool Entry Points**:
   - `0x00BAAC`: scans all Group 2 slots starting from player (`$FF19E8`, 21 slots)
   - `0x00BAB8`: scans enemy-only slots (`$FF1CD8`, 17 slots)
5. **Target Filters & Collision Response**:
   - Skips inactive (`type <= 0`), self (`target == self`), invulnerable (`+0x38 & 0x08 != 0`), and protected (`+0x3A & 0x01 != 0`).
   - On 3D AABB overlap: sets Carry flag (`ori.b #1, ccr`) and returns target entity pointer in `a0`.
   - If no overlap: clears Carry flag (`andi.b #$FE, ccr`).

---

## 2. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>
#include <optional>

namespace thor::combat {

struct WeaponHitbox12B {
    int16_t z_min{0};
    int16_t z_max{0};
    int16_t x_min{0};
    int16_t y_min{0};
    int16_t x_max{0};
    int16_t y_max{0};

    WeaponHitbox12B Mirror() const noexcept {
        return {z_min, z_max, static_cast<int16_t>(-x_max), y_min, static_cast<int16_t>(-x_min), y_max};
    }

    WeaponHitbox12B ProjectWorld(int16_t wx, int16_t wy, int16_t wz, int16_t gh) const noexcept {
        int16_t z_base = wz + gh;
        int16_t y_base = wy + gh;
        return {
            static_cast<int16_t>(z_min + z_base),
            static_cast<int16_t>(z_max + z_base),
            static_cast<int16_t>(x_min + wx),
            static_cast<int16_t>(y_min + y_base),
            static_cast<int16_t>(x_max + wx),
            static_cast<int16_t>(y_max + y_base)
        };
    }
};

} // namespace thor::combat
```
