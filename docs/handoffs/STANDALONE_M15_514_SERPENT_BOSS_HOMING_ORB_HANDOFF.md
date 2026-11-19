# STANDALONE HANDOFF: Milestone M15.514 — ROM Composite Serpent Boss State `$12` Homing Orb Projectile, Ali Grab Pinning, Homing Hit Callback & Smooth Turn-Rate Steering (`0x0160E2..0x016294`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x0160E2..0x016294` (434 bytes total: 124 M68K instructions across 5 contiguous routines).
- **Core Responsibilities:**
  1. `0x0160E2..0x016168` (`AI_SerpentBoss_HomingOrb_Tick`, 134 bytes, 38 insns):
     - State `$12` Homing Orb/Fireball Projectile tick loop.
     - Fadeout mode (`+$2A == 0x38`): continues steering via `0x0161FA` until animation complete flag `+$37:1` is set, then despawns orb (`+$0 = 0`).
     - Active flight mode: checks cancellation conditions (incoming damage `+$86 != 0`, lifetime counter `+$A1 == 0`, animation finished `+$37:1`, or terrain/wall collision bit 6 in `+$40 | +$41`). On cancellation, clears velocities and sets fadeout anim `0x38` with invulnerability bit 3 in `+$38`.
     - Collision check: after age counter `+$30 >= 12` frames, performs AABB collision test vs Ali via `0x00B852` with bounding box `[-4..+4, -4..+4]`. On contact, invokes Hit Callback `0x0161A2`. Otherwise continues steering via `0x0161FA`.
  2. `0x016168..0x016184` (`AI_SerpentBoss_SubStepAnchorIntegrator`, 28 bytes, 8 insns):
     - Sub-Step Anchor Integrator: decrements step timer `+$2`.
     - If timer expires (`timer == 0`), returns immediately with CCR Zero flag set.
     - If timer active (`timer > 0`), accumulates delta vectors `+$A6` into 32-bit position `+$72` and `+$AA` into `+$76`, explicitly clearing the CCR Zero flag (`andi.b #0xFB, ccr`).
  3. `0x016184..0x0161A2` (`AI_SerpentBoss_AliGrabPinning`, 30 bytes, 6 insns):
     - Ali Grab Pinning helper: copies boss 32-bit XYZ coordinates to Ali entity `$FF19E8` with a vertical offset of `ali_y = boss_y - 8`, pinning Ali inside the serpent's mouth/jaw.
  4. `0x0161A2..0x0161FA` (`AI_SerpentBoss_HomingOrb_HitCallback`, 88 bytes, 17 insns):
     - Checks Ali invulnerability bit 3 in `+$38`.
     - If Ali is vulnerable: sets attacker pointer `ali.+$7E = a6`, hitstun timer `ali.+$83 = 8`, clears `ali.+$9D = 0`, sets reaction mode `ali.+$84 = orb.+$7C` (matching orb facing), plays SFX `$800216` via `0x00E7FC`, transfers orb velocity to Ali knockback `ali.+$4E/+$52 = orb.+$4E/+$52`.
     - Damage: if grounded (`ali.+$10 == 0`), inflicts 2 damage (`ali.+$86 = 2`) with `ali.+$56 = 0`; if airborne (`ali.+$10 != 0`), inflicts `0x8002` damage with upward impulse `ali.+$56 = +0x00010000` (+1.0 px).
     - Zeroes orb velocities and transitions to fadeout anim `0x38`.
  5. `0x0161FA..0x016294` (`AI_SerpentBoss_SmoothHomingTurnRateSteering`, 154 bytes, 55 insns):
     - Speed check: if speed `+$56 == 0x00030000`, skips turn-rate adjustment.
     - Otherwise computes angle to Ali via `0x00E706`, evaluates clockwise vs counter-clockwise delta, clamps turn-rate to `<= 8` units/frame (out of 256 circle).
     - Quantizes angle into 4-way cardinal facing `+$7C`:
       - `[0x00..0x1F]`: facing 1 (Right)
       - `[0x20..0x5F]`: facing 0 (Down)
       - `[0x60..0x9F]`: facing 3 (Left)
       - `[0xA0..0xDF]`: facing 2 (Up)
       - `[0xE0..0xFF]`: facing 1 (Right)
     - Vector stepping: reads sine and cosine from ROM table `$5D706` scaled by `speed >> 7`.
     - Updates position: `pos_x += cos * scale`, `pos_y += -sin * scale`.
     - Stores velocities: `vel_x = cos * scale`, `vel_y = -sin * scale`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_serpent_boss_homing_orb_m15_514.json`
- Extractor Tool: `tools/extract_rom_serpent_boss_homing_orb.py`
- Test Suite: `tests/rom_serpent_boss_homing_orb_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>

namespace Thor::BossSerpent {

// --- Projectile Constants ---
inline constexpr uint16_t kOrbFadeoutAnimId           = 0x0038u;
inline constexpr uint32_t kOrbHitSfxId                = 0x00800216u;
inline constexpr uint16_t kOrbWarmupAgeThreshold      = 12u;
inline constexpr int8_t   kOrbAABBMin                 = -4;
inline constexpr int8_t   kOrbAABBMax                 = 4;
inline constexpr uint8_t  kMaxTurnRatePerFrame        = 8u;
inline constexpr uint32_t kSkipTurnSpeedThreshold     = 0x00030000u;
inline constexpr uint8_t  kAliHitstunDurationFrames   = 8u;
inline constexpr uint16_t kGroundedOrbDamage          = 2u;
inline constexpr uint16_t kAirborneOrbDamage          = 0x8002u;
inline constexpr int32_t  kAirborneOrbZImpulse        = 0x00010000; // +1.0 px/frame

// --- Cardinal Facing Enum ---
enum class Facing4Way : uint16_t {
    Down  = 0,
    Right = 1,
    Up    = 2,
    Left  = 3,
};

// --- Quantize Angle (0..255) to 4-Way Facing ---
inline constexpr Facing4Way QuantizeAngleToFacing(uint8_t angle) noexcept {
    if (angle < 0x20u) return Facing4Way::Right;
    if (angle < 0x60u) return Facing4Way::Down;
    if (angle < 0xA0u) return Facing4Way::Left;
    if (angle < 0xE0u) return Facing4Way::Up;
    return Facing4Way::Right;
}

// --- Smooth Turn-Rate Calculation ---
inline constexpr uint8_t SmoothTurnTowards(uint8_t current_angle, uint8_t target_angle, uint32_t speed) noexcept {
    if (speed == kSkipTurnSpeedThreshold) {
        return current_angle;
    }
    const uint8_t cw_diff  = static_cast<uint8_t>(current_angle - target_angle);
    const uint8_t ccw_diff = static_cast<uint8_t>(target_angle - current_angle);

    if (cw_diff < ccw_diff) {
        const uint8_t turn = (cw_diff <= kMaxTurnRatePerFrame) ? cw_diff : kMaxTurnRatePerFrame;
        return static_cast<uint8_t>(current_angle - turn);
    } else {
        const uint8_t turn = (ccw_diff <= kMaxTurnRatePerFrame) ? ccw_diff : kMaxTurnRatePerFrame;
        return static_cast<uint8_t>(current_angle + turn);
    }
}

// --- Ali Grab Pinning ---
inline constexpr void PinAliToBossJaw(int32_t boss_x, int32_t boss_y, int32_t boss_z,
                                     int32_t& ali_x, int32_t& ali_y, int32_t& ali_z) noexcept {
    ali_x = boss_x;
    ali_y = boss_y - 8;
    ali_z = boss_z;
}

// --- Sub-Step Anchor Integration ---
struct SubStepResult {
    bool timer_expired{false};
    bool zero_flag_cleared{false};
};

inline constexpr SubStepResult StepSubStepAnchor(uint16_t& timer, int32_t delta_x, int32_t delta_y,
                                                int32_t& pos_x, int32_t& pos_y) noexcept {
    if (timer <= 1u) {
        timer = 0;
        return SubStepResult{ .timer_expired = true, .zero_flag_cleared = false };
    }
    --timer;
    pos_x += delta_x;
    pos_y += delta_y;
    return SubStepResult{ .timer_expired = false, .zero_flag_cleared = true };
}

} // namespace Thor::BossSerpent
```
