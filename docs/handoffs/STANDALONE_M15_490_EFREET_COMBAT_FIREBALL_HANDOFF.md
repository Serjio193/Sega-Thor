# STANDALONE HANDOFF: Milestone M15.490 — ROM Efreet Dash Charge Attack, Flame Breath State Controller & Radial Fireball Supernova Spawner

## 1. Subsystem Overview
- **ROM Address Range:** `0x012680..0x012988` (776 bytes, 206 instructions, zero embedded data tables).
- **Core Responsibilities:**
  1. `0x012680..0x0127A4` (`AI_Efreet_DashChargeStates`, 292 bytes, 81 instructions):
     - `0x012680` (**State `$04`**): Waits for `+$37` bit 1 to transition to State `$02` (`+$2A = 0`).
     - `0x01269A` (**`AI_Efreet_ReturnToIdle`**): Sets `+$04 = 0`, `+$2A = 0`, clears `+$9E = 0`.
     - `0x0126AC` (**State `$08`**): Accelerates via `0x01274C` until `+$37` bit 1 -> transitions to State `$12` (`+$2A = $1A`, `+$02 = $0A`).
     - `0x0126CE` (**State `$12`**): Computes jewel-rank bonus `(($FF0DB7 + 1) >> 2)` into `+$90(a6)`, executes `0x00BCA4` (`d0 = 0`), clears `+$90(a6)`, calls `0x01274C`, and counts down `+$02` (10 frames) -> State `$14` (`+$2A = $1C`).
     - `0x012702` (**State `$14`**): Decelerates `+$4E/+$52` by `(0xC0, 0xC0)` via `0x00E2D4`, integrates into `+$72/+$76`, and on `+$37` bit 1 returns to idle (`0, 0`) with cooldown `+$9E = $0E` (14 frames).
     - `0x01274C` (**`AI_Efreet_StepClampedDashVelocity`**): Adds `(0x100, 0x100)` via `0x00E2D4`, clamps `+$4E/+$52` to `[-0x00080000, +0x00080000]`, and integrates into `+$72/+$76`.
  2. `0x0127A4..0x012802` (`AI_Efreet_FlameBreathStates`, 94 bytes, 24 instructions):
     - `0x0127A4` (**State `$0A`**): Waits for `+$37` bit 1 -> transitions to State `$0E` (`+$2A = $18`, `+$02 = $46` = 70 frames).
     - `0x0127C0` (**State `$0E`**): Decrements `+$02`; while `+$02 >= $14` (20) and `(+$02 & 3) == 0`, spawns flame particles via `0x012DFA` and plays SFX `#$0080034F`. Calls `0x012CDC` and `0x012EA8` every active frame. When `+$02` reaches 0, clears the `$FF1C1C` slot pointed to by `+$A6(a6)`, clears `+$06(a6)`, and branches to `0x01269A`.
  3. `0x012802..0x012914` (`AI_Efreet_FireballSupernovaStates`, 274 bytes, 71 instructions):
     - `0x012802` (**State `$0C`**): Waits for `+$37` bit 1 -> State `$18` (`+$2A = $1E`).
     - `0x012818` & `0x012858` (**State `$18`**): Sets `$FF17C4 = ($FF1971 & 1) + 3`. While `+$02` does not borrow on decrement, plays `#$00B00224` and `#$00700383`. On borrow, decrements `+$AF` (entering State `$1A`, `+$2A = $20` on underflow `< 0`), reloads `+$02 = 2`, decrements angle `+$AE -= $20`, reads signed high bytes from external trig table `$05D706` (`vx = cos * $1000`, `vy = -sin * $1000`), allocates a `$FF1C1C` slot via `0x00DA1A` (clearing `+$02`, restoring `+$AE += $20` and `+$AF += 1` if full), clones Efreet via `0x00E66E`, configures State `$16` (`+$2A = $16`, `+$A2 = $FF`, `+$18 = (+$18 & $8000) | $07A8`, `+$36` bit 0 cleared, `+$37` bit 1 cleared, `+$16 = 0`), and offsets `+$08/+$0C` by `2 * vel`.
     - `0x01284C` (**State `$1A`**): Recovery state returning via `0x01269A` on `+$37` bit 1.
  4. `0x012914..0x012988` (`AI_Efreet_FireballFlightMeleeAndBuzzer`, 116 bytes, 30 instructions):
     - `0x012914..0x012950` (**State `$16`**): Resolves cardinal facing from `+$4E/+$52` via `0x00E338` into `+$16`, executes `0x00BCA4` (`d0 = 2`), clears `+$16 = 0`, despawns (`+$0 = 0`) on `+$37` bit 1, or integrates `+$4E/+$52` into `+$72/+$76`. (Verified directly from ROM: `0x012914..0x012950` does not use `0x00B922`, `0x012DAE`, `-0x20000`, `0x00E78A`, or `#$B00346`.)
     - `0x012950..0x012978` (**State `$10` Melee Scan**, jumped from `0x0129B0`): Applies `(($FF0DB7 + 1) >> 2)` into `+$90`, executes `0x00BCA4` (`d0 = 0`), clears `+$90`, and branches to `0x01269A` on `+$37` bit 1.
     - `0x012978..0x012988` (**Insufficient-SP / Slot-Full Buzzer**): Plays `#$00A00253` via `0x060004` and branches to `0x0129B0`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_efreet_combat_fireball_m15_490.json`
- Extractor: `tools/extract_rom_efreet_combat_fireball.py`
- Test: `tests/rom_efreet_combat_fireball_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <cstdint>

namespace Thor::AI {

constexpr uint16_t kEfreetDashAccelStep         = 0x0100;
constexpr uint16_t kEfreetDashDecelStep         = 0x00C0;
constexpr int32_t  kEfreetDashVelocityClamp     = 0x00080000;
constexpr uint16_t kEfreetDashActiveFrames      = 10;
constexpr uint8_t  kEfreetDashCooldownFrames    = 14;
constexpr uint16_t kEfreetFlameBreathFrames     = 70;   // 0x46
constexpr uint16_t kEfreetFlameEmitMinTimer     = 20;   // 0x14
constexpr uint8_t  kEfreetSupernovaAngleStep    = 0x20;
constexpr int32_t  kEfreetSupernovaSpeedScale   = 0x1000;
constexpr uint16_t kEfreetSupernovaSpawnDelay   = 2;
constexpr uint8_t  kEfreetFireballAttackIndex   = 2;

inline uint16_t ComputeEfreetJewelBonusDamage(uint8_t efreet_level_db7) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(efreet_level_db7) + 1U) >> 2);
}

inline void ClampDashVelocity(int32_t& vx, int32_t& vy, int32_t step_x, int32_t step_y) noexcept {
    vx = std::clamp(vx + step_x, -kEfreetDashVelocityClamp, kEfreetDashVelocityClamp);
    vy = std::clamp(vy + step_y, -kEfreetDashVelocityClamp, kEfreetDashVelocityClamp);
}

inline bool ShouldEmitFlameBreathParticle(uint16_t decremented_timer) noexcept {
    return decremented_timer >= kEfreetFlameEmitMinTimer && (decremented_timer & 3U) == 0U;
}

inline uint16_t ComputeSupernovaScreenShake(uint8_t frame_counter_1971) noexcept {
    return static_cast<uint16_t>((frame_counter_1971 & 1U) + 3U);
}

struct SupernovaVelocity {
    int32_t vx{0};
    int32_t vy{0};
};

// Cleanroom projection from signed 8-bit Q15 sine table high bytes matching $05D706.
inline SupernovaVelocity ComputeSupernovaFireballVelocity(int8_t sin_hi, int8_t cos_hi) noexcept {
    return {
        static_cast<int32_t>(cos_hi) * kEfreetSupernovaSpeedScale,
        -(static_cast<int32_t>(sin_hi) * kEfreetSupernovaSpeedScale),
    };
}

} // namespace Thor::AI
```
