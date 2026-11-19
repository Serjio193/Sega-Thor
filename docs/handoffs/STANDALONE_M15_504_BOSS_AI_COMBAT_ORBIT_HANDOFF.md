# STANDALONE HANDOFF: Milestone M15.504 — ROM Boss AI Combat States `$10`/`$06`/`$08`/`$0A`/`$16`/`$18` & Orbit Target Helpers (`0x014A72..0x014D1A`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x014A72..0x014D1A` (680 bytes, 173 M68K instructions; contiguous executable code implementing boss combat attack states, hit thrust, hit callback, and 3-tier Chebyshev orbit target kinematics).
- **Core Responsibilities:**
  1. `0x014A72..0x014ABC` (`BossAI_State10_CombatAttack`, 74 bytes, 18 insns):
     - Decrements timer `+$2(a6)`; when 0, calls High Orbit helper `0x014C1C`.
     - Steers towards target `(+$A6, +$AA)` via `0x0150A0`.
     - When `+$30 == 0x18` and `+$2E == 0`, spawns projectile via `0x015214` and plays SFX `#$0080034F` via `0x00E7FC`.
     - On animation completion (`+$37:1`), sets substate `+$2A = 8` and transitions to `0x014958`.
  2. `0x014ABC..0x014AEA` (`BossAI_State06_OrbitApproach`, 46 bytes, 12 insns):
     - Decrements timer `+$2(a6)`; when 0, calls Low Orbit helper `0x014CA0`.
     - Steers towards target via `0x0150A0`.
     - On animation completion (`+$37:1`), sets `+$2A = 0` and transitions to State `$08` (`+$04 = 8`).
  3. `0x014AEA..0x014B50` (`BossAI_State08_OrbitCombatAttack`, 102 bytes, 25 insns):
     - Decrements timer `+$2(a6)`; when 0, calls Low Orbit helper `0x014CA0`.
     - When counter `+$32 == 12` and `$FF1971 % 3 == 0`, invokes attack helper `0x015136`.
     - If quotient `($FF1971 / 3) & 3 == 0`, plays SFX `#$0080034F`.
     - On animation completion (`+$37:1`), sets `+$2A = 4` and transitions to State `$0A` (`+$04 = $0A`).
  4. `0x014B50..0x014B7C` (`BossAI_State0A_Retreat`, 44 bytes, 11 insns):
     - Decrements timer `+$2(a6)`; when 0, calls Low Orbit helper `0x014CA0`.
     - Steers towards target via `0x0150A0`.
     - On animation completion (`+$37:1`), sets `+$2A = 8` and transitions to `0x014958`.
  5. `0x014B7C..0x014BB8` (`BossAI_State16_ChaseAliHitbox`, 60 bytes, 15 insns):
     - Steers towards player Ali (`$FF19E8`) via `0x0150A0`.
     - Tests boss attack hitbox table `$58CE6` against Ali via `0x00BAAC`.
     - If hit lands (`carry` set) or timer `+$2(a6)` expires: transitions to State `$18` (`+$04 = $18`, `+$2A = $1A`, speed `+$B0 = $00010000`).
  6. `0x014BB8..0x014C1C` (`BossAI_State18_HitThrustAndCallback`, 100 bytes, 25 insns):
     - State `$18` (`0x014BB8..0x014C04`, 76 bytes): Steers towards Ali, registers hit callback `0x014C04` in `$FF1940`, and calls `0x00BCE2`.
     - On animation completion (`+$37:1`): resets via `0x014958`, sets `+$2A = 8`, speed `+$B0 = $00030000`, timer `+$2 = 20`, and flips angle `+$AF += $80` (180 degrees).
     - Hit Callback (`0x014C04..0x014C1C`, 24 bytes): calls `0x00BEBA` (target hurt response); if boss facing `+$16 != 2`, applies knockback `+$52(a0) = $00020000` to Ali.
  7. `0x014C1C..0x014CA0` (`BossAI_HighOrbitTargetHelper`, 132 bytes, 33 insns):
     - Target offsets relative to Ali (`$FF19E8`):
       - facing `0`: `(ali_x + 0x60, ali_y - 0x30)`
       - facing `1`: `(ali_x - 0x60, ali_y - 0x30)`
       - facing `2`: `(ali_x, ali_y - 0x68)`
     - Clamps target Y: `d2 = max(d2, 0x00C8)`.
     - Computes Chebyshev distance $L_\infty = \max(|dx|, |dy|)$ and sets speed tier:
       - `dist > 24`: speed `$00030000` (3.0 px/frame)
       - `17 <= dist <= 24`: speed `$00020000` (2.0 px/frame)
       - `dist <= 16`: speed `$00018000` (1.5 px/frame)
     - Reloads timer `+$2 = 4`.
  8. `0x014CA0..0x014D1A` (`BossAI_LowOrbitTargetHelper`, 122 bytes, 31 insns):
     - Target offsets relative to Ali:
       - facing `0`: `(ali_x + 0x48, ali_y - 0x30)`
       - facing `1`: `(ali_x - 0x48, ali_y - 0x30)`
       - facing `2`: `(ali_x, ali_y - 0x38)`
     - Computes Chebyshev distance $L_\infty = \max(|dx|, |dy|)$ and sets speed tier:
       - `dist > 24`: speed `$00028000` (2.5 px/frame)
       - `17 <= dist <= 24`: speed `$00020000` (2.0 px/frame)
       - `dist <= 16`: speed `$00018000` (1.5 px/frame)
     - Reloads timer `+$2 = 4`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_boss_ai_combat_orbit_m15_504.json`
- Extractor Tool: `tools/extract_rom_boss_ai_combat_orbit.py`
- Test Suite: `tests/rom_boss_ai_combat_orbit_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace Thor::BossAI {

inline constexpr uint32_t kBossProjectileSfxId       = 0x0080034Fu;
inline constexpr int32_t  kAliKnockbackVelocity      = 0x00020000;
inline constexpr int16_t  kHighOrbitYClampMin        = 0x00C8;
inline constexpr uint16_t kOrbitTimerReloadFrames    = 4u;
inline constexpr uint16_t kState18PostHitTimer       = 20u;

struct OrbitTargetResult {
    int16_t target_x{0};
    int16_t target_y{0};
    int32_t speed_b0{0};
    uint16_t timer_reload{kOrbitTimerReloadFrames};
};

inline OrbitTargetResult ComputeHighOrbitTarget(
    int16_t ali_x, int16_t ali_y, uint16_t facing_16,
    int16_t boss_x, int16_t boss_y) noexcept {
    int16_t tx = ali_x;
    int16_t ty = ali_y;

    if (facing_16 == 1) {
        tx = static_cast<int16_t>(tx - 0x60);
        ty = static_cast<int16_t>(ty - 0x30);
    } else if (facing_16 == 2) {
        ty = static_cast<int16_t>(ty - 0x68);
    } else {
        tx = static_cast<int16_t>(tx + 0x60);
        ty = static_cast<int16_t>(ty - 0x30);
    }

    if (ty < kHighOrbitYClampMin) {
        ty = kHighOrbitYClampMin;
    }

    const int16_t dx = std::abs(static_cast<int>(tx - boss_x));
    const int16_t dy = std::abs(static_cast<int>(ty - boss_y));
    const int16_t dist = std::max(dx, dy);

    int32_t speed = 0x00030000;
    if (dist <= 16) {
        speed = 0x00018000;
    } else if (dist <= 24) {
        speed = 0x00020000;
    }

    return OrbitTargetResult{tx, ty, speed, kOrbitTimerReloadFrames};
}

inline OrbitTargetResult ComputeLowOrbitTarget(
    int16_t ali_x, int16_t ali_y, uint16_t facing_16,
    int16_t boss_x, int16_t boss_y) noexcept {
    int16_t tx = ali_x;
    int16_t ty = ali_y;

    if (facing_16 == 1) {
        tx = static_cast<int16_t>(tx - 0x48);
        ty = static_cast<int16_t>(ty - 0x30);
    } else if (facing_16 == 2) {
        ty = static_cast<int16_t>(ty - 0x38);
    } else {
        tx = static_cast<int16_t>(tx + 0x48);
        ty = static_cast<int16_t>(ty - 0x30);
    }

    const int16_t dx = std::abs(static_cast<int>(tx - boss_x));
    const int16_t dy = std::abs(static_cast<int>(ty - boss_y));
    const int16_t dist = std::max(dx, dy);

    int32_t speed = 0x00028000;
    if (dist <= 16) {
        speed = 0x00018000;
    } else if (dist <= 24) {
        speed = 0x00020000;
    }

    return OrbitTargetResult{tx, ty, speed, kOrbitTimerReloadFrames};
}

} // namespace Thor::BossAI
```
