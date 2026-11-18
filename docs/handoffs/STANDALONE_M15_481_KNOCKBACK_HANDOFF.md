# STANDALONE HANDOFF: Milestone M15.481 — ROM Entity AI Mass Confusion, Palette Transfer, Ali Range Tracker & Impact Knockback Dispatcher

## 1. Subsystem Overview
- **ROM Address Range:** `0x011276..0x0114AE` (568 bytes, 156 instructions).
- **Core Responsibilities:**
  1. `0x011276` (`AI_ApplyFearConfusionToAllEnemiesPool2`): Iterates over the 17 slots of Pool 2 (`$FF1CD8`), skips inactive slots (`type <= 0`), skips self (`a6 == a5`), skips entities with status `$38` bit 3, status `$3A` bit 0, or non-zero action state `+$82`. Inverts facing direction bit 1 (`facing ^ 2`), stores inverted facing to `+$84`, invokes animation setup `0x00E4D0`, and locks confusion timer `+$86 = 0xCFFF`.
  2. `0x0112CC` (`AI_FindSlotWithTypePool2`): Scans the 17 slots of Pool 2 looking for an active slot matching entity type `d0.w`. Returns `d0 = 0` if found, or `d0 = -1` (`$FF`) if missing.
  3. `0x0112F6` (`AI_ProbeEntityAllocationEviction`): Calls slot probe routine `0x00DA72` and inverts the Carry flag convention: Carry clear becomes Carry set (failure), Carry set becomes Carry clear (success).
  4. `0x011310` (`Gfx_TransferCRAMPaletteLine_Lines`): Copies 16 palette words (32 bytes) from `(a1)+` into CRAM mirror buffers at `$FF136C` or `$FF138C` and asserts bit 0 of CRAM transfer request flag `$FF164D`.
  5. `0x011378` (`AI_ComputeAliDistanceAndDominantFacing`): Computes delta `(d1, d2)` between entity `(+$8, +$C)` and Player Ali (`$FF19F0/$FF19F4`), compares absolute magnitudes `|dx|` vs `|dy|`, and returns 4-way facing (`0=Up, 1=Right, 2=Down, 3=Left`) in `d0` and dominant distance in `d5`.
  6. `0x0113CC` (`AI_TriggerTargetImpactKnockback`): Verifies target collision via `0x00B926`, verifies immunity bits (`$38` bit 3, `$3A` bit 0, `+$82 != 0`), links attacker pointer `a6` into victim's `+$7E(a0)`, unpacks impact parameter from stream, sets impact state `+$82 = 3` and `+$83 = 6`, zeroes timer `+$9D = 0`, sets upward vertical launch velocity `+$56 = 0x00020000`, calculates directional knockback speed (`0x410 / 0x400`, doubled if `param & 0xF000 == 0xC000`), projects 2D velocity via `0x00E2D4` into `+$4E/+$52(a0)`, triggers sound `$80022C` via `0x00E7FC`, and returns Carry set.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_entity_ai_knockback_m15_481.json`
- Extractor: `tools/extract_rom_entity_ai_knockback.py`
- Test: `tests/rom_entity_ai_knockback_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <cmath>

namespace Thor::AI {

struct KnockbackVictim {
    uint16_t status_38{0};
    uint16_t status_3a{0};
    uint8_t state_82{0};
    uint8_t substate_83{0};
    uint8_t timer_9d{0};
    uint32_t attacker_id{0};
    uint16_t target_param_86{0};
    int32_t elevation_10{0};
    int32_t vel_z_56{0};
    int32_t speed_x{0};
    int32_t speed_y{0};
};

inline bool ApplyImpactKnockback(KnockbackVictim& target, uint32_t attacker_id, uint16_t param_d0) noexcept {
    if ((target.status_38 & (1 << 3)) != 0 ||
        (target.status_3a & (1 << 0)) != 0 ||
        target.state_82 != 0) {
        return false;
    }

    target.attacker_id = attacker_id;
    target.target_param_86 = param_d0;
    target.state_82 = 3;
    target.timer_9d = 0;

    uint16_t hi_nibble = param_d0 & 0xF000;
    bool double_speed = false;
    if (hi_nibble != 0) {
        target.elevation_10 += 1;
        if (hi_nibble == 0xC000) {
            double_speed = true;
        }
    }

    if (target.elevation_10 > 0) {
        target.target_param_86 |= 0x8000;
    }

    target.substate_83 = 6;
    target.vel_z_56 = 0x00020000;

    int32_t speed_x = 0x410;
    int32_t speed_y = 0x400;
    if (double_speed) {
        speed_x *= 2;
        speed_y *= 2;
    }

    target.speed_x = speed_x;
    target.speed_y = speed_y;
    return true;
}

struct DominantFacingResult {
    uint16_t facing; // 0=Up, 1=Right, 2=Down, 3=Left
    int16_t distance;
};

inline DominantFacingResult ComputeAliDistanceAndDominantFacing(int16_t self_x, int16_t self_y,
                                                               int16_t ali_x, int16_t ali_y) noexcept {
    int16_t dx = ali_x - self_x;
    int16_t dy = ali_y - self_y;
    int16_t abs_dx = std::abs(dx);
    int16_t abs_dy = std::abs(dy);

    if (abs_dx > abs_dy) {
        return { static_cast<uint16_t>(dx >= 0 ? 1 : 3), abs_dx };
    } else {
        return { static_cast<uint16_t>(dy >= 0 ? 2 : 0), abs_dy };
    }
}

} // namespace Thor::AI
```
