# Standalone Handoff M15.420: ROM Entity 3D AABB Combat Overlap & Multi-Pool Collision Iterator Engine

- **Milestone:** M15.420
- **ROM Source Range:** `0x00B79A..0x00BAAC` (786 bytes)
- **SHA256:** `44c804a30d0386394e13b5e7f573de8be1cbb17e39f064e89aac1f8be05f67c0`

---

## 1. Subsystem Architecture

The combat overlap engine evaluates directional attack hitboxes and multi-pool entity collisions in full 3D space:
1. **Directional Combat Hitbox (`0x00B79A..0x00B854`)**:
   - Takes entity radii `+0x42(a6)` (X) and `+0x44(a6)` (Y).
   - Biases the bounding box forward based on 4-way facing direction `+0x16(a6)`:
     * `0 (Down)`: `y_min = 0`, span `[-rad_x..+rad_x, 0..+rad_y]`
     * `1 (Left)`: `x_max = 0`, span `[-rad_x..0, -rad_y..+rad_y]`
     * `2 (Up)`: `y_max = 0`, span `[-rad_x..+rad_x, -rad_y..0]`
     * `3 (Right)`: `x_min = 0`, span `[0..+rad_x, -rad_y..+rad_y]`
   - Overlap Target Filtering:
     * Disregards intangible/invulnerable targets (`+0x38(a0) & 0x0C != 0`)
     * Disregards protected status (`+0x3A(a0) & 0x01 != 0`)
     * Disregards targets in hit cooldown (`+0x82(a0) != 0`)
     * Prevents self-damage (`+0x7E(a6) == a0`)
     * Registers hit on target: `+0x82(a0) += 1`, records source pointer in `+0x7E(a0)`
2. **Group 2 3D AABB Overlap Scanner (`0x00B856..0x00B920`)**:
   - Scans 21 slots of 188 bytes at `$FF19E8` (`d7 = 20`).
   - Uses coroutine-style context save buffer `$FF194A..$FF196A` on hit.
   - Clears Carry flag on hit; returns to caller.
   - Resumes next entity via `find_next` at `0x00B912`.
3. **Group 2 3D Interval Scanner (`0x00B922..0x00B9DC`)**:
   - Evaluates vertical Z overlap using explicit interval `[d5, d6]` (`[z_low, z_high]`).
4. **Group 4 Breakables/Pickups Scanner (`0x00B9EC..0x00BAAC`)**:
   - Scans 8 slots of 90 bytes at `$FF2ABC` for weapon impacts against breakable pots, chests, and pickups.

---

## 2. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>
#include <optional>

namespace thor::combat {

struct Aabb3D {
    int16_t x_min{0};
    int16_t x_max{0};
    int16_t y_min{0};
    int16_t y_max{0};
    int16_t z_min{0};
    int16_t z_max{0};

    bool Overlaps(const Aabb3D& o) const noexcept {
        return (x_max >= o.x_min) && (x_min <= o.x_max) &&
               (y_max >= o.y_min) && (y_min <= o.y_max) &&
               (z_max >= o.z_min) && (z_min <= o.z_max);
    }
};

class CombatOverlapEngine {
public:
    static Aabb3D ComputeDirectionalBox(int16_t rad_x, int16_t rad_y, uint8_t facing) noexcept {
        Aabb3D box{-rad_x, rad_x, -rad_y, rad_y, 0, 32};
        switch (facing & 3) {
            case 0: box.y_min = 0; break; // Down
            case 1: box.x_max = 0; break; // Left
            case 2: box.y_max = 0; break; // Up
            case 3: box.x_min = 0; break; // Right
        }
        return box;
    }

    static bool IsVulnerable(uint8_t flags_38, uint8_t flags_3a, uint8_t cooldown_82) noexcept {
        return ((flags_38 & 0x0C) == 0) &&
               ((flags_3a & 0x01) == 0) &&
               (cooldown_82 == 0);
    }
};

} // namespace thor::combat
```
