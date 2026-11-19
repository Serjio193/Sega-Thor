# STANDALONE HANDOFF: Milestone M15.496 — ROM Shade Spirit Idle Command Dispatcher, Hitstun Recovery, Ascend Dismissal, Approach Homing & 2-Slot Ali Pose History Ring Buffer

## 1. Subsystem Overview
- **ROM Address Range:** `0x01366A..0x01387C` (530 bytes, 135 M68K instructions; contiguous executable code immediately preceding Bow Spirit master tick at `0x01387C`).
- **Core Responsibilities:**
  1. `0x01366A..0x0136AC` (`AI_Shade_State00_IdleCommandAndHistoryFollow`, 66 bytes, 15 insns):
     - Propagates Ali's low-SP blink bit (`+$36(a5)` bit 1 -> `+$36(a6)` bit 1 when set).
     - Checks dismissal flag `$FF19A7` (`!= 0` -> `0x0136D4`).
     - Dispatches command code `$FF19A2`: `1 -> 0x01301E`, `2 -> 0x01319E`, `3 -> 0x013134`.
     - When no command is active, asserts `$FF1989 = 0xFF` and calls `0x0137F0` (`StepPoseHistoryRing`).
  2. `0x0136AC..0x0136CC` (`AI_Shade_State04_HitstunBlinkRecovery`, 32 bytes, 7 insns):
     - Decrements `+$83(a6)` hitstun counter. While non-zero, sets blink bit `+$36(a6)` bit 6 and returns.
     - On reaching `0`, clears `+$36(a6)` bit 6 and `+$39(a6)` bit 3 and exits via `0x01378A` into State `0` doppelganger mimic mode.
  3. `0x0136CC..0x013716` (`AI_Shade_DismissalAndState10_AscendDespawn`, 74 bytes, 19 insns):
     - `0x0136CC` (`SPDrainDismissalGuard`): Guards `+$04(a6) == 0`; if non-zero diverts to `0x012F6A`.
     - `0x0136D4` (`StartDismissal`): Plays SFX `#$00A0039E` (`0x00E7FC`), switches to Shade body descriptor `$03F4A6` via `0x013764`, clears `+$16(a6) = 0`, enters State `$10`, and sets anim `$14` via `0x008EF8`.
     - `0x0136F8` (`State10_AscendDespawn`): Clears `+$36(a6)` bit 1, increments `+$10(a6)` by `+8` per frame while `<= $0100` (`256`), and clears `+$00(a6) = 0` (despawn) once `+$10(a6) > 256`.
  4. `0x013716..0x013764` (`AI_Shade_State02_SummonApproachHoming`, 78 bytes, 23 insns):
     - Promotes `+$2A(a6) = $14` once `+$37(a6)` bit 1 is set.
     - Calls `0x0123B8` (`d4 = 1`) for `+1` Z-elevation tracking.
     - Computes Z-compensated Ali target Y (`ali_y + (ali_ground_z - shade_ground_z)` when both `ground_z >= 0`) and invokes `0x00E45E` (`homing_velocity_helper`).
     - If returned half-Chebyshev distance `d4 <= 7` (`max(|dx|, |dy|) <= 15`), branches to `0x01378A`; otherwise writes `4x` (`asl.l #2`) velocity into `+$72(a6)/+$76(a6)`.
  5. `0x013764..0x01387C` (`AI_Shade_DescriptorSwitchAndPoseHistoryRing`, 280 bytes, 71 insns):
     - `0x013764` (`SwitchToShadeBodyDescriptor`): Copies 3 longs from ROM `$03F4A6` (`0x00142B6A, 0x0010F100, 0x000409A8`) into `+$1A/+$1E/+$26(a6)`, sets `+$38 = $0E`, `+$2C = $FFFF`, `+$34 = $FFFF`.
     - `0x01378A` (`EnterDoppelgangerMimicMode`): Resets State `0`, loads ROM `$03F306` (`0x0013CDFC, 0x000C0000, 0x0003FF7A`) into `+$1A/+$1E/+$26(a6)`, sets `+$38 = $0D`, clears `+$06/+$39/+$82/+$83/+$86`, invalidates `+$34 = $FFFF`, primes both 12-byte ring slots `$FF19A8` and `$FF19B4` via `0x01386E`, and falls through into `0x0137F0`.
     - `0x0137F0` (`StepPoseHistoryRing`): Clears `+$72/+$76`, copies 6 words `(x, y, z, ground_z, facing, anim_id)` from `$FF19B4` into Shade (`+$08/+$0C/+$10/+$14/+$16/+$32`), locks `+$2E = $03DE`, clears `+$39/+$6E/+$6F`, shifts 6 words `$FF19A8 -> $FF19B4`, and captures Ali's live pose `(x + int16(+$72), y + int16(+$76), z, ground_z, facing, anim_id)` into `$FF19A8` via `0x01386E` (producing a steady 2-frame trailing doppelganger lag).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_shade_follow_history_m15_496.json`
- Extractor: `tools/extract_rom_shade_follow_history.py`
- Test: `tests/rom_shade_follow_history_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint32_t kShadeDismissalSfxId          = 0x00A0039Eu;
inline constexpr uint16_t kShadeDismissalAnimId         = 0x0014u;
inline constexpr int16_t  kShadeAscendStepZ             = 8;
inline constexpr int16_t  kShadeAscendDespawnThreshold  = 0x0100;  // despawn when z > 256
inline constexpr uint16_t kShadeApproachAnimAction      = 0x0014u;
inline constexpr int16_t  kShadeApproachElevOffset      = 1;
inline constexpr uint16_t kShadeApproachArrivalD4       = 7;
inline constexpr uint16_t kShadeAnimTimerLock           = 0x03DEu;
inline constexpr uint8_t  kShadeBodyFlags38             = 0x0Eu;
inline constexpr uint8_t  kShadeDoppelgangerFlags38     = 0x0Du;

struct ShadeDescriptorPointers {
    uint32_t sat_data_1a;
    uint32_t frame_table_1e;
    uint32_t anim_seq_26;
};

inline constexpr ShadeDescriptorPointers kShadeBodyDescriptor{
    0x00142B6Au, 0x0010F100u, 0x000409A8u
};
inline constexpr ShadeDescriptorPointers kAliDoppelgangerDescriptor{
    0x0013CDFCu, 0x000C0000u, 0x0003FF7Au
};

struct ShadePoseSnapshot {
    int16_t  x{0};
    int16_t  y{0};
    int16_t  z{0};
    int16_t  ground_z{0};
    uint16_t facing{0};
    uint16_t sprite_frame{0};
};

struct ShadePoseHistoryRing {
    ShadePoseSnapshot slot0_current{};  // $00FF19A8 (12 bytes)
    ShadePoseSnapshot slot1_delayed{};  // $00FF19B4 (12 bytes)
};

inline ShadePoseSnapshot CaptureAliPose(
    int16_t x, int16_t y, int32_t delta_x_72, int32_t delta_y_76,
    int16_t z, int16_t ground_z, uint16_t facing, uint16_t sprite_frame) noexcept {
    return ShadePoseSnapshot{
        static_cast<int16_t>(x + static_cast<int16_t>(delta_x_72 >> 16)),
        static_cast<int16_t>(y + static_cast<int16_t>(delta_y_76 >> 16)),
        z,
        ground_z,
        facing,
        sprite_frame,
    };
}

inline ShadePoseSnapshot StepPoseHistoryRing(
    ShadePoseHistoryRing& ring,
    const ShadePoseSnapshot& next_ali_pose) noexcept {
    ShadePoseSnapshot output = ring.slot1_delayed;
    ring.slot1_delayed = ring.slot0_current;
    ring.slot0_current = next_ali_pose;
    return output;
}

inline bool StepShadeAscendDismissal(int16_t& z_10, uint16_t& type_00, uint8_t& flags_36) noexcept {
    flags_36 &= static_cast<uint8_t>(~0x02u);
    if (z_10 > kShadeAscendDespawnThreshold) {
        type_00 = 0;
        return true;  // despawned
    }
    z_10 = static_cast<int16_t>(z_10 + kShadeAscendStepZ);
    return false;
}

} // namespace Thor::AI
```
