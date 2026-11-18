# STANDALONE HANDOFF: Milestone M15.488 — ROM Dytto Spirit Idle Follow, Auto-Rescue Heal Trigger, Circular Orbit & Dismissal Physics Subsystem

## 1. Subsystem Overview
- **ROM Address Range:** `0x0120D4..0x0123E2` (782 bytes, 219 instructions).
- **Core Responsibilities:**
  1. `0x0120D4..0x0121AA` (`AI_Dytto_ApproachStatesAndBuzzer`, 214 bytes, 56 insns):
     - **State `$02` (`0x0120D4`):** Calls `0x0123B4` and `0x01215E` for half-speed (`asr.l #1`) homing until `+$37(a6)` bit 1 is set, then transitions to State `$04` (`+$2A = 8`) and enters `0x012150`.
     - **State `$06` (`0x012104`):** Calls `0x0123B4` and `0x01215E` for half-speed homing until `+$37(a6)` bit 1 is set, then transitions to State `$00` and enters `0x0121AA`.
     - **State `$1A` (`0x01212E`):** Waits for `+$37(a6)` bit 1; clears `+$2A(a6)` if non-zero, otherwise enters State `$00` (`0x0121AA`).
     - **State `$04` (`0x012150`) & Homing (`0x01215E`):** Calls `0x0123B4` and `0x00E398`; when Manhattan distance `d5 <= 0x20` (32 px), transitions to State `$06` (`+$2A = 0x16`); otherwise `0x01215E` computes 2D homing velocity toward `(+$08(a5), +$0C(a5) + z_diff)` via `0x00E45E` (doubled into `+$72/+$76`, facing `+$16 = d3`), where `z_diff = +$14(a5) - +$14(a6)` when both `+$14 >= 0`.
     - **Insufficient-SP Buzzer (`0x01219A`):** Plays `#$00A00253` via `$060004` and jumps into `State00` post-command flow (`0x0121D2`).
  2. `0x0121AA..0x012314` (`AI_Dytto_State00_IdleCommandAndFollowLoop`, 362 bytes, 110 insns):
     - Guards dismissal flag `$FF19A7 != 0` (-> `0x01231C`).
     - Dispatches `$FF19A2`: `1 -> 0x011D8A`, `2 -> 0x011BCC`, `3 -> 0x011FA6`.
     - Evaluates auto-rescue heal (`0x0121D2`): decrements cooldown `+$AA(a6)`; when `0`, requires `$FF0DB6 >= 3`, `+$8A(a5) < (+$88(a5) >> 3)`, and 20 SP via `0x011B28`, then sets `+$AA = 0x1E` (30) and enters `0x011BE0` (State `$18`, `+$2A = $0E`, silent cast).
     - Tracks elevation (`0x0123B4`), compensates `ty += ali_z14 - self_z14`, and runs either:
       - **Circular orbit (`0x01222A..0x012288`):** After 40-frame delay `+$A1` expires, decrements 120-frame timer `+$A0`, advances `+$9E` by `+6` (`+$9F` bit 1 clear) or `-6` (`+$9F` bit 1 set), reads `sin`/`cos` high bytes from ROM `$05D706`, and offsets target by `(cos >> 2, (sin >> 3) + ((sin >> 3) >> 1))`.
       - **18px (`0x12`) 4-way facing follow (`0x01228A..0x012312`):** Offsets behind Ali by `(±18, ±18)` per `+$16(a5)` (`0:(-18,+18)`, `1:(-18,-18)`, `2:(+18,-18)`, `3:(+18,+18)`). Calls `0x00E45E`: `d4 == 0` clears velocity, copies Ali's facing, clears `+$2A`, and arms `+$A0 = 0x78`, `+$A1 = 0x28`, `+$9F = prng` (`0x00E268`) if `+$A1 == 0`; `1 <= d4 <= 2` halves velocity and copies Ali's facing; `d4 > 2` writes full velocity, facing `d3`, and `+$2A = 2`.
  3. `0x012314..0x0123AE` (`AI_Dytto_DismissalAndState1C_1E`, 154 bytes, 33 insns):
     - `0x012314`: guards `+$04 == 0` (else branches to `0x011B8A`).
     - `0x01231C`: plays SFX `#$A0039E` (`0x00E7FC`), sets facing `2`, State `$1C`, anim `$26` (`0x008EF8`), clears 32-bit `+$56(a6)`.
     - State `$1C` (`0x012342`): clears `+$36` bit 1; once `+$30 >= 0x14` (20), integrates gravity `-0xC000` on `+$56` and `+$10` until `+$10 < 0` -> clamps `+$10 = 0`, sets State `$1E`, `+$2A = $24`, sprite `#$00141374`, priority `$07A8`, `+$36` bit 0 cleared & bit 3 set, facing `0`.
     - State `$1E` (`0x012394`): clears `+$36` bit 1, sets bit 3, despawns `+$00 = 0` when `+$37` bit 1 is set.
  4. `0x0123AE..0x0123E2` (`AI_Dytto_TrackTargetElevation`, 52 bytes, 20 insns):
     - `0x0123AE` (`d4 = 8`) / `0x0123B4` (`d4 = 48`): adjusts `+$10(a6)` by `+/- 1` clamped `>= 0` toward `+$14(a5) + d4` using `+$14(a6)` (or `+$14(a5)` if `+$14(a6) < 0`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_dytto_follow_dismissal_m15_488.json`
- Extractor: `tools/extract_rom_dytto_follow_dismissal.py`
- Test: `tests/rom_dytto_follow_dismissal_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kDyttoApproachThresholdPx      = 0x0020;      // 32 px
inline constexpr uint32_t kDyttoInsufficientSpBuzzerSfx  = 0x00A00253u;
inline constexpr uint32_t kDyttoDismissalSfx             = 0x00A0039Eu;
inline constexpr uint8_t  kDyttoAutoRescueCooldownFrames = 0x1E;        // 30 frames
inline constexpr uint8_t  kDyttoAutoRescueMinJewelRank   = 3;
inline constexpr int16_t  kDyttoAutoRescueSpCost         = 20;
inline constexpr uint8_t  kDyttoOrbitDurationFrames      = 0x78;        // 120 frames
inline constexpr uint8_t  kDyttoOrbitDelayFrames         = 0x28;        // 40 frames
inline constexpr int8_t   kDyttoOrbitAngleStep           = 6;
inline constexpr int16_t  kDyttoFollowOffsetPx           = 0x0012;      // 18 px
inline constexpr uint16_t kDyttoDismissalFallDelayFrames = 0x0014;      // 20 frames
inline constexpr int32_t  kDyttoDismissalGravityFixed    = 0x0000C000;
inline constexpr int16_t  kDyttoHoverOffsetLow           = 8;
inline constexpr int16_t  kDyttoHoverOffsetDefault       = 48;

struct DyttoFollowState {
    uint16_t type_00{0x0018};
    uint16_t state_04{0};
    int16_t  x_08{0};
    int16_t  y_0c{0};
    int32_t  z_10_fixed{0};
    int16_t  surface_z_14{0};
    uint16_t facing_16{0};
    uint16_t priority_18{0};
    uint32_t sprite_table_1a{0};
    uint16_t action_2a{0};
    uint16_t anim_frame_30{0};
    uint8_t  flags_36{0};
    uint8_t  anim_flags_37{0};
    int32_t  vel_z_56{0};
    int32_t  vel_x_72{0};
    int32_t  vel_y_76{0};
    uint8_t  orbit_angle_9e{0};
    uint8_t  orbit_dir_flags_9f{0};
    uint8_t  orbit_timer_a0{0};
    uint8_t  orbit_delay_a1{0};
    uint8_t  auto_heal_cd_aa{0};
};

inline void TrackDyttoTargetElevation(
    DyttoFollowState& dytto,
    int16_t ali_surface_z,
    int16_t hover_offset_d4) noexcept {
    int16_t self_surf = (dytto.surface_z_14 >= 0) ? dytto.surface_z_14 : ali_surface_z;
    int16_t target_total = static_cast<int16_t>(ali_surface_z + hover_offset_d4);
    int16_t self_z = static_cast<int16_t>(dytto.z_10_fixed >> 16);
    int16_t delta = static_cast<int16_t>(self_surf + self_z - target_total);
    if (delta == 0) {
        return;
    }
    if (delta < 0) {
        self_z = static_cast<int16_t>(self_z + 1);
    } else {
        if (self_z <= 0) {
            return;
        }
        self_z = static_cast<int16_t>(self_z - 1);
    }
    dytto.z_10_fixed = (static_cast<int32_t>(self_z) << 16) | (dytto.z_10_fixed & 0xFFFF);
}

inline bool TryTriggerAutoRescueHeal(
    DyttoFollowState& dytto,
    uint8_t jewel_rank_ff0db6,
    uint16_t ali_max_hp,
    uint16_t ali_hp,
    int16_t& ali_sp) noexcept {
    if (dytto.auto_heal_cd_aa != 0) {
        --dytto.auto_heal_cd_aa;
        return false;
    }
    if (jewel_rank_ff0db6 < kDyttoAutoRescueMinJewelRank ||
        ali_hp >= static_cast<uint16_t>(ali_max_hp >> 3)) {
        return false;
    }
    const int16_t rem_sp = static_cast<int16_t>(ali_sp - kDyttoAutoRescueSpCost);
    if (rem_sp < 1) {
        return false;
    }
    ali_sp = rem_sp;
    dytto.auto_heal_cd_aa = kDyttoAutoRescueCooldownFrames;
    dytto.facing_16 = 0;
    dytto.state_04 = 0x18;
    dytto.action_2a = 0x0E;
    return true;
}

inline void ComputeBehindAliFollowOffset(
    uint16_t ali_facing,
    int16_t& target_x,
    int16_t& target_y) noexcept {
    switch (ali_facing & 0xFFu) {
        case 0: target_x -= kDyttoFollowOffsetPx; target_y += kDyttoFollowOffsetPx; break;
        case 1: target_x -= kDyttoFollowOffsetPx; target_y -= kDyttoFollowOffsetPx; break;
        case 2: target_x += kDyttoFollowOffsetPx; target_y -= kDyttoFollowOffsetPx; break;
        default: target_x += kDyttoFollowOffsetPx; target_y += kDyttoFollowOffsetPx; break;
    }
}

inline void ApplyIsometricOrbitOffset(
    DyttoFollowState& dytto,
    int8_t sin_byte,
    int8_t cos_byte,
    int16_t& target_x,
    int16_t& target_y) noexcept {
    const int8_t delta = (dytto.orbit_dir_flags_9f & 0x02u) ? -kDyttoOrbitAngleStep : kDyttoOrbitAngleStep;
    dytto.orbit_angle_9e = static_cast<uint8_t>(dytto.orbit_angle_9e + delta);
    const int16_t sy = static_cast<int16_t>(sin_byte >> 3);
    const int16_t dy_iso = static_cast<int16_t>(sy + (sy >> 1));
    const int16_t dx_iso = static_cast<int16_t>(cos_byte >> 2);
    target_x = static_cast<int16_t>(target_x + dx_iso);
    target_y = static_cast<int16_t>(target_y + dy_iso);
}

inline bool StepDyttoDismissalFall(DyttoFollowState& dytto) noexcept {
    dytto.flags_36 &= static_cast<uint8_t>(~0x02u);
    if (dytto.anim_frame_30 < kDyttoDismissalFallDelayFrames) {
        return false;
    }
    dytto.vel_z_56 -= kDyttoDismissalGravityFixed;
    dytto.z_10_fixed += dytto.vel_z_56;
    if (dytto.z_10_fixed >= 0) {
        return false;
    }
    dytto.z_10_fixed = 0;
    dytto.state_04 = 0x1E;
    dytto.action_2a = 0x24;
    dytto.sprite_table_1a = 0x00141374u;
    dytto.priority_18 = 0x07A8;
    dytto.flags_36 = static_cast<uint8_t>((dytto.flags_36 & ~0x01u) | 0x08u);
    dytto.facing_16 = 0;
    return true;
}

inline bool StepDyttoDismissalDespawn(DyttoFollowState& dytto) noexcept {
    dytto.flags_36 = static_cast<uint8_t>((dytto.flags_36 & ~0x02u) | 0x08u);
    if ((dytto.anim_flags_37 & 0x02u) != 0) {
        dytto.type_00 = 0;
        return true;
    }
    return false;
}

} // namespace Thor::AI
```
