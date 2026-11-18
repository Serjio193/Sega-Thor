# STANDALONE HANDOFF: Milestone M15.479 — ROM Entity AI Viewport Culling, Parent Targeting, Cutscene Locks & 3D Ballistic Gravity Arc Physics Subsystem

## 1. Subsystem Overview
- **ROM Address Range:** `0x010EEA..0x01108C` (418 bytes, 117 instructions).
- **Core Responsibilities:**
  1. `0x010EEA` (`AI_CheckEntityInViewportWithMargin`): Tests whether an entity's 2D world position lies within the active camera plane's viewport extended symmetrically by margin parameter `d7`. Automatically selects Plane A (`$FF1716`) or Plane B (`$FF173E`) based on bit 7 of `$FF16F0`.
  2. `0x010F48` (`AI_FaceTowardsParentEntity`): Compares delta-X vs delta-Y to parent entity (`a5`), selecting the dominant axis and updating self facing `+$16(a6)` (`0=Up, 1=Right, 2=Down, 3=Left`). When deltas are equal, Y-axis takes precedence.
  3. `0x010F84` (`AI_JumpToSpiritSoundEffect`): Targets active Spirit entity slot (`$FF1AA4`) and dispatches sound effect routine `0x00E710`.
  4. `0x010F94` (`AI_CheckCutscenePlayerLockFlags`): Inspects engine-wide locks: player lock (`$FF19EC`), cutscene lock (`$FF1982`), fade lock (`$FF1985`), and menu lock (`$FF1987`). Returns Carry set (unlocked) or Carry clear (locked).
  5. `0x010FC8` (`Physics_BallisticGravityArcStep_Simple`): Applies constant downward gravity (`-0xC000` / frame in 16.16 fixed-point) to vertical velocity `+$56(a6)` and steps elevation `+$10(a6)`. If elevation falls below zero, it clamps to ground (`z = 0`), zeroes vertical velocity, sets ground-landed status bit (`+$39 |= 4`), resets recovery timer `+$9A = 15`, and returns `d0 = 1`. If still airborne, conditionally integrates horizontal velocity `+$4E/+$52` into `+$72/+$76` unless collision bits `0xC0` are set in `+$40/+$41`.
  6. `0x01101E` (`Physics_BallisticGravityArcStep_CollisionChecked`): Ballistic gravity step that also checks terrain and obstacle flags (`+$6F` bit 1) and invokes surface contact dispatcher `0x00BCD8`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_entity_ai_culling_gravity_m15_479.json`
- Extractor: `tools/extract_rom_entity_ai_culling_gravity.py`
- Test: `tests/rom_entity_ai_culling_gravity_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <cmath>

namespace Thor::AI {

enum class Direction : uint16_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct ViewportCullingParams {
    int16_t camera_x;
    int16_t camera_y;
    int16_t entity_x;
    int16_t entity_y;
    int16_t margin;
};

inline bool IsEntityCulled(const ViewportCullingParams& p) noexcept {
    int32_t min_x = static_cast<int32_t>(p.camera_x) - p.margin;
    int32_t min_y = static_cast<int32_t>(p.camera_y) - p.margin;
    int32_t max_x = static_cast<int32_t>(p.camera_x) + 320 + p.margin;
    int32_t max_y = static_cast<int32_t>(p.camera_y) + 224 + p.margin;

    return (p.entity_x < min_x || p.entity_y < min_y ||
            p.entity_x > max_x || p.entity_y > max_y);
}

inline Direction CalculateFacingTowardsParent(int16_t self_x, int16_t self_y,
                                             int16_t parent_x, int16_t parent_y) noexcept {
    int16_t dx = parent_x - self_x;
    int16_t dy = parent_y - self_y;
    int16_t abs_dx = std::abs(dx);
    int16_t abs_dy = std::abs(dy);

    if (abs_dx <= abs_dy) {
        return (dy >= 0) ? Direction::Down : Direction::Up;
    } else {
        return (dx >= 0) ? Direction::Right : Direction::Left;
    }
}

inline bool AreEngineLocksActive(uint16_t player_lock, uint8_t cutscene_lock,
                                uint8_t fade_lock, uint8_t menu_lock) noexcept {
    return (player_lock != 0 || cutscene_lock != 0 || fade_lock != 0 || menu_lock != 0);
}

struct BallisticEntityState {
    int32_t z{0};           // 16.16 fixed point (+$10)
    int32_t vel_z{0};       // 16.16 fixed point (+$56)
    int32_t pos_x{0};       // 16.16 fixed point (+$72)
    int32_t pos_y{0};       // 16.16 fixed point (+$76)
    int32_t vel_x{0};       // 16.16 fixed point (+$4E)
    int32_t vel_y{0};       // 16.16 fixed point (+$52)
    uint8_t col_40{0};
    uint8_t col_41{0};
    uint16_t val_2e{0};
    uint8_t status_39{0};
    uint8_t timer_9a{0};
};

inline bool StepBallisticGravityArc(BallisticEntityState& ent) noexcept {
    ent.vel_z -= 0xC000;
    ent.z += ent.vel_z;

    if (ent.z < 0) {
        ent.z = 0;
        ent.vel_z = 0;
        ent.val_2e = 0;
        ent.status_39 |= (1 << 2);
        ent.timer_9a = 15;
        return true; // Landed
    }

    if (((ent.col_40 | ent.col_41) & 0xC0) == 0) {
        ent.pos_x += ent.vel_x;
        ent.pos_y += ent.vel_y;
    }
    return false; // Airborne
}

} // namespace Thor::AI
```
