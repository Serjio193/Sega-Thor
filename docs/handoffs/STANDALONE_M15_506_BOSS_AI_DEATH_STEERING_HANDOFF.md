# STANDALONE HANDOFF: Milestone M15.506 — ROM Boss AI State `$1A` Death Cutscene Sequence, Explosion Shake & Smooth Angle Steering Helper (`0x014F60..0x015136`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x014F60..0x015136` (470 bytes, 121 M68K instructions; contiguous executable code handling boss death sequence milestones, screen shake, explosion particle emission, angular turn rate limiting, and velocity integration).
- **Core Responsibilities:**
  1. `0x014F60..0x01503C` (`BossAI_State1A_DeathCutsceneSequence`, 220 bytes, 57 insns):
     - Increments frame counter `+$2(a6) += 1` each tick.
     - Frame 180 (`$B4`):
       - Clears screen update flag `sf.b $FF164F`.
       - Sets palette param `$FF1896 = 6` and invokes palette effect `0x002E78`.
       - Jumps to explosion particle / screen shake helper `0x01503C`.
     - Frames 181..239 (`< $F0`):
       - Branches directly to explosion particle / screen shake helper `0x01503C`.
     - Frame 300 (`$12C`):
       - Clears 17 Pool 2 entity slots starting at `$FF1D94` (stride `0xBC` via `dbra #16`).
       - Enqueues Nemesis DMA stream from `$167C54` into VRAM tile `$8F00` via `0x00D9A4`.
       - Spawns reward item Entity Type `0x56` (Spirit Item / Chest) at slot `$FF2D8C` via `0x008D06`:
         - Tile attribute `$18(a6) = 0x0478`
         - Position `(X=0x0100 / 256, Y=0x0098 / 152)`
         - Initial vertical velocity `$52(a6) = 0x00010000`
         - Gravity / acceleration `$56(a6) = 0x000331DC`
     - Frame 360 (`$168`):
       - Clears cutscene/camera lock `sf.b $FF1983`.
       - Sets `$FF1896 = 4` and invokes palette restore `0x002DB6`.
       - Triggers room event progression `0x00DAC2` with parameter `d0 = 0x011F`.
       - Sets boss defeated bit `bset.b #1, $FF1D0E`.
       - Sets environmental progression flag `bset.b #2, $FF0DC3`.
     - Frame 420 (`$1A4`):
       - Restores screen flag `st.b $FF164F`.
       - Despawns boss slot `clr.w $FF1CD8`.
     - Other frames: falls through to `rts` at `0x014F8C`.
  2. `0x01503C..0x0150A0` (`BossAI_DeathExplosionShake`, 100 bytes, 25 insns):
     - Computes explosion particle position relative to Plane A (`$FF1716`) and Plane B (`$FF173E`) scrolls:
       - `X = 0x0100 - plane_b_x + plane_a_x + (prng >> 1)`
       - `Y = 0x00E8 - plane_b_y + plane_a_y + (prng >> 1) + 0x00E0`
     - Spawns explosion particle Type 1 via `0x00E90E`.
     - Oscillates screen shake at `$FF17C4`: `($FF1971 & 1) + 2` (2 on even frames, 3 on odd frames).
  3. `0x0150A0..0x015136` (`BossAI_SmoothAngleSteering`, 150 bytes, 39 insns):
     - Computes target angle towards target via `0x00E6F0`.
     - Clamps angular turn rate: maximum 8 units (`0..255` circle) toward target angle into `+$AE(a6)`.
     - Computes X and Y velocity components via sin/cos table `$5D706`:
       - Base speed scale: `d6 = +$B0(a6) >> 7`.
       - Sine lookup: `sin = (int8_t)table[angle * 2]`.
       - Cosine lookup: `cos = (int8_t)table[((angle + 0x40) & 0xFF) * 2]`.
       - Stores velocities: `+$4E = d6 * cos`, `+$52 = -(d6 * sin)`.
       - Integrates positions: `+$72 += +$4E`, `+$76 += +$52`.
     - Updates boss facing `+$16(a6)` relative to Ali (`$FF19E8`):
       - `ali_x < 0x00A0` (160) $\to$ facing `3` (left)
       - `ali_x >= 0x0160` (352) $\to$ facing `1` (right)
       - `0x00A0 <= ali_x < 0x0160` $\to$ facing `2` (down / center)

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_boss_ai_death_steering_m15_506.json`
- Extractor Tool: `tools/extract_rom_boss_ai_death_steering.py`
- Test Suite: `tests/rom_boss_ai_death_steering_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Thor::BossAI {

inline constexpr uint8_t  kMaxTurnRateStep          = 8u;
inline constexpr uint16_t kDeathStage1Frame         = 180u;
inline constexpr uint16_t kDeathShakeEndFrame       = 240u;
inline constexpr uint16_t kDeathItemSpawnFrame      = 300u;
inline constexpr uint16_t kDeathEventReleaseFrame   = 360u;
inline constexpr uint16_t kDeathDespawnFrame        = 420u;

inline constexpr uint16_t kRewardItemEntityType     = 0x0056u;
inline constexpr uint16_t kRewardItemTileAttr       = 0x0478u;
inline constexpr int16_t  kRewardItemInitialX       = 256;
inline constexpr int16_t  kRewardItemInitialY       = 152;
inline constexpr int32_t  kRewardItemInitialZVel    = 0x00010000;
inline constexpr int32_t  kRewardItemGravity        = 0x000331DC;

inline constexpr uint16_t kAliFacingThresholdLeft   = 160u;
inline constexpr uint16_t kAliFacingThresholdRight  = 352u;

inline uint8_t StepSmoothTurnAngle(uint8_t current_angle, uint8_t target_angle) noexcept {
    const uint8_t forward_dist = static_cast<uint8_t>(target_angle - current_angle);
    const uint8_t backward_dist = static_cast<uint8_t>(current_angle - target_angle);

    if (forward_dist < backward_dist) {
        const uint8_t step = std::min(forward_dist, kMaxTurnRateStep);
        return static_cast<uint8_t>(current_angle + step);
    } else {
        const uint8_t step = std::min(backward_dist, kMaxTurnRateStep);
        return static_cast<uint8_t>(current_angle - step);
    }
}

inline uint16_t DetermineBossFacingFromAliX(int16_t ali_x) noexcept {
    if (ali_x < static_cast<int16_t>(kAliFacingThresholdLeft)) {
        return 3u; // left
    }
    if (ali_x >= static_cast<int16_t>(kAliFacingThresholdRight)) {
        return 1u; // right
    }
    return 2u; // center / down
}

inline uint16_t ComputeScreenShake(uint8_t frame_counter) noexcept {
    return static_cast<uint16_t>((frame_counter & 1u) + 2u);
}

} // namespace Thor::BossAI
```
