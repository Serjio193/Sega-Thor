# STANDALONE HANDOFF: M15.447 — ROM Spark Effect Dispatcher, Projectile Gravity Fall, Lifetime Blink & Despawn Subsystem

- **Source Routine:** `projectile_spark_physics_lifecycle_subsystem` (`0x00E0BA..0x00E25C`, 418 bytes).
- **Domain:** Projectile Entity Pool / Spark Effects / Fixed-Point Gravity / Visibility Blinking / Despawn.
- **Evidence Reference:** `data/thor-evidence/rom_projectile_physics_m15_447.json`.
- **Test Reference:** `tests/rom_projectile_physics_test.py`.

---

## 1. Architectural Role & Context

In Beyond Oasis / The Story of Thor, active combat effects and thrown/shot items (such as arrows, bombs, fireballs, energy sparks, and drop pickups) operate across two dedicated pools:
1. **Spark Effect Pool (`$FF2D8C`, 6 slots, stride 90 bytes / `$5A`):**
   - Dispatched by `0x00E0BA` (`dispatch_spark_effects`).
   - Sweeps 6 slots; for active slots (`$0(a6) != 0`) with non-null callback pointers at `+$56(a6)`, executes the callback `jsr (a0)`.
2. **Projectile Entity Pool (`$FF2ABC`, 8 slots, stride 90 bytes / `$5A`):**
   - Dispatched by `0x00E0F4` (`dispatch_projectile_entities`).
   - Decrements global timer `$FF1856.l` if non-zero.
   - Evaluates active slots (`$0(a6) > 0`) and jumps via PC-relative word jump table `0x00E140` indexed by state offset `+$04(a6)`.

### Core Projectile States
- **State 0 (`0x00E150`, Gravity Fall & Landing):**
  - Subtracts fixed-point gravity constant `#$C000` from velocity accumulator at `+$56(a6)`.
  - Integrates velocity into elevation `+$10(a6)`.
  - When landing on ground (`elevation <= 0`):
    - Clamps elevation to 0.
    - If spawned by an actor entity (`a6 < $FF2ABC`): unlatches parent actor bit 1 on `+$36(a4)` (`bclr.b #1, $36(a4)`), synchronizes Y position to parent, and frees the projectile slot (`clr.w $0(a6)`).
- **State 2 (`0x00E192`, Gravity Fall with Lifetime Blink & Despawn):**
  - Applies identical gravity fall physics.
  - Increments 16-bit lifetime frame counter `+$22(a6)`.
  - **Blink Window (240 to 300 frames, or 4 to 5 seconds):** Toggles visibility flag bit 1 on `+$36(a6)` on alternating frames (`bset` on odd frames, `bclr` on even frames).
  - **Despawn Threshold (> 300 frames):** Clears slot status `clr.w $0(a6)` to despawn.

---

## 2. Register Conventions & Memory Offsets

| Offset | Size | Type | Field Semantic |
|---|---|---|---|
| `+$00` | Word | `int16_t` | Slot active status / type (`> 0` indicates active) |
| `+$04` | Word | `int16_t` | Projectile state offset (indexes relative jump table `0x00E140`) |
| `+$0C` | Word | `int16_t` | Entity Y coordinate |
| `+$10` | Long | `int32_t` | Elevation Z coordinate (32-bit fixed point) |
| `+$22` | Word | `uint16_t` | Lifetime frame counter |
| `+$36` | Byte | `uint8_t` | Visibility & latch bitflags (bit 1: blink/visible latch) |
| `+$56` | Long | `int32_t` | 32-bit vertical velocity / spark callback pointer |
| `+$A6` | Long | `uint32_t` | Pointer to parent entity |

---

## 3. C++20 Cleanroom Interface Design

```cpp
#pragma once

#include <cstdint>
#include <span>
#include <optional>

namespace thor::projectile {

constexpr int32_t kGravityStep = 0x0000C000;
constexpr uint16_t kBlinkStartFrame = 240; // 0xF0
constexpr uint16_t kBlinkEndFrame = 300;   // 0x12C

struct ProjectileSlot {
    int16_t type{0};
    int16_t state{0};
    int16_t pos_y{0};
    int32_t elevation{0};
    uint16_t lifetime{0};
    uint8_t flags_36{0};
    int32_t velocity_z{0};
    size_t parent_index{0};
    bool has_parent{false};
};

struct SparkSlot {
    int16_t type{0};
    int32_t callback_ptr{0};
};

inline void update_projectile_gravity_landing(
    ProjectileSlot& proj,
    std::span<ProjectileSlot> parent_pool
) {
    if (proj.elevation != 0) {
        proj.velocity_z -= kGravityStep;
        proj.elevation += proj.velocity_z;
        if (proj.elevation < 0) {
            proj.elevation = 0;
        }
    }

    if (proj.has_parent && proj.elevation == 0) {
        if (proj.parent_index < parent_pool.size()) {
            auto& parent = parent_pool[proj.parent_index];
            parent.flags_36 &= ~0x02; // Unlatch bit 1
            parent.pos_y = proj.pos_y;
        }
        proj.type = 0; // Free slot
    }
}

inline void update_projectile_gravity_blink_despawn(
    ProjectileSlot& proj,
    std::span<ProjectileSlot> parent_pool
) {
    if (proj.elevation != 0) {
        proj.velocity_z -= kGravityStep;
        proj.elevation += proj.velocity_z;
        if (proj.elevation < 0) {
            proj.elevation = 0;
        }
    }

    if (proj.has_parent && proj.elevation == 0) {
        if (proj.parent_index < parent_pool.size()) {
            auto& parent = parent_pool[proj.parent_index];
            parent.flags_36 &= ~0x02;
            parent.pos_y = proj.pos_y;
        }
        proj.type = 0;
        return;
    }

    proj.lifetime++;
    if (proj.lifetime >= kBlinkStartFrame && proj.lifetime <= kBlinkEndFrame) {
        if (proj.lifetime & 1) {
            proj.flags_36 |= 0x02;
        } else {
            proj.flags_36 &= ~0x02;
        }
    } else if (proj.lifetime > kBlinkEndFrame) {
        proj.type = 0; // Despawn
    }
}

} // namespace thor::projectile
```
