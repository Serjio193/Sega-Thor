# STANDALONE HANDOFF: Milestone M15.491 — ROM Efreet Idle/Wander/Follow AI, Vulnerability Scanner, Dismissal & Hover Bobbing Subsystem

## 1. Subsystem Overview
- **ROM Address Range:** `0x012988..0x012C4E` (710 bytes total: 702 bytes executable M68K code across 200 instructions at `0x012988..0x012C46` + 8-byte embedded 4-word bobbing table at `0x012C46..0x012C4E`).
- **Core Responsibilities:**
  1. `0x012988..0x012BA8` (`AI_Efreet_IdleWanderFollowAndVulnScan`, 544 bytes, 154 insns):
     - **Shared Entry (`0x012988`):** Dispatched by `0x0125AE` for States `$00`, `$02`, and `$10`:
       - Checks global dismissal flag `$FF19A7 != 0` (`-> 0x012BA8`).
       - Dispatches `$FF19A2` player commands: `1 -> 0x012608` (Flame Breath), `2 -> 0x0125CA` (Dash Charge), `3 -> 0x01263C` (Fireball Meltdown).
       - Note: `0x012978` (insufficient-SP buzzer) branches directly to `0x0129B0`, bypassing dismissal & command dispatch.
       - If State `+$04 == $10` (`0x0129B0`), branches to `0x012950` (active State `$10` attack scan).
     - **4-Way Proximity / Vulnerability Scanner (`0x0129BA..0x012A28`):**
       - Saves current facing `+$16(a6)` to `+$B2(a6)` and probes Pool 2 enemies (`$FF1CD8`, 17 slots) via `0x00BAB8` with external 36-byte table `$0586E8..$05870C` in direction order `[f, (f + 1) & 3, (f - 1) & 3, f ^ 2]`:
         - Facing `0` (Up): `z=[-4..10], x=[-24..23], y=[-40..-9]`
         - Facing `1` (Right): `z=[-4..10], x=[8..47], y=[-16..15]`
         - Facing `2` (Down): `z=[-4..10], x=[-24..23], y=[8..39]`
         - Facing `3` (Left): `0x00BAB8` mirrors Facing `1` X -> `x=[-47..-8], y=[-16..15]`
       - On the first overlapping enemy (`Carry` set), branches to `0x012672` (State `$10`, `+$2A = $0A`, keeping matched facing in `+$16`). If all 4 directions miss, restores `+$16 = +$B2`.
     - **Hover Hooks (`0x012A28..0x012A2C`):** Calls `0x012BFE` (`TrackTargetElevation`) and `0x012C32` (`IntegrateHoverBobbing`), then routes State `$02` (`+$04 == 2`) to `0x012AE6`.
     - **State `$00` Idle & 30-Frame Wander Oscillation (`0x012A3A..0x012AE4`):**
       - When `+$9E(a6) == 0`: calls `0x00E398` (returns `d0` facing toward Ali, `d3 = |dx|`, `d4 = |dy|`, `d5 = |dx|+|dy|`) and computes **Chebyshev** distance `d4 = max(|dx|, |dy|)` (`0x012A46..0x012A4A`):
         - `d4 <= $40` (`<= 64` px): calls `0x00E268`; if `d0 >= $7FFF`, calls `0x00E268` again and rotates facing by `(d0 & 2) - 1` (`+/- 1` mod 4), else keeps current facing.
         - `$40 < d4 <= $80` (`65..128` px): sets facing `+$16 = d0` (toward Ali).
         - `d4 > $80` (`> 128` px): jumps to `0x01268C` (State `$02`, `+$2A = 0`) immediately.
       - For `d4 <= $80`: arms `+$9E = $1E` (30 frames) and computes cardinal wander velocity `(0x100, 0x100)` via `0x00E2D4` into `+$4E(a6)` / `+$52(a6)` (returning before velocity integration on frame 0).
       - While `+$9E(a6) != 0` (`0x012AA0..0x012AE4`): updates each non-zero `+$4E` / `+$52` via `0x012AC6` (`+$2000` magnitude ramp while `+$9E > $0F`, `-$2000` magnitude decay while `+$9E <= $0F`), adds `+$4E` to `+$72(a6)` and `+$52` to `+$76(a6)`, and decrements `+$9E`.
     - **State `$02` Diagonal Behind-Ali Homing Follow (`0x012AE6..0x012BA8`):**
       - Applies isometric Y compensation `ty += +$14(a5) - +$14(a6)` when both `+$14 >= 0`, clears `+$2A = 0`, and offsets target behind Ali by `($28, $20)` = `(40, 32)` px per `+$16(a5)` (`0:(-32,+40)`, `1:(-40,-32)`, `2:(+32,-40)`, `3:(+40,+32)`).
       - Calls `0x00E45E` and decrements 4-frame facing debounce `+$AC(a6)` if non-zero:
         - `d4 == 0`: returns to State `$00`, clears `+$9E`, zeroes `+$72/+$76`, copies `+$16(a5)` to `+$16(a6)`.
         - `1 <= d4 <= 4`: writes 1x `(vx, vy)` to `+$72/+$76`.
         - `d4 > 4`: writes 2x (`asl.l #1`) `(vx, vy)` to `+$72/+$76`.
       - Updates `+$16 = d3` only when `d3 != +$16` and `+$AC == 0`, arming `+$AC = 4`.
  2. `0x012BA8..0x012C4E` (`AI_Efreet_DismissalAndHoverBobbing`, 166 bytes: 158B code / 46 insns + 8B table):
     - **Dismissal Starter (`0x012BA8..0x012BF0`):** Sets `$FF19A7 = $FF`, State `$06`, `+$2A = 6`, facing `2`, `+$38 |= 8`, `+$36 &= ~2`, `+$A2 = $FF`, plays SFX `#$A0039E` via `0x00E7FC`, dereferences `+$A6(a6)` and clears `+$00(a0) = 0` if `+$00(a0) == $FFFE` (releasing reserved `$FF1C1C` Flame Breath slot), and clears `+$06 = 0`.
     - **State `$06` Despawn (`0x012BF0..0x012BFE`):** Clears `+$00(a6) = 0` once `+$37(a6)` bit 1 is set.
     - **Hover Target Elevation Tracker (`0x012BFE..0x012C32`):** Targets `ali_surface_z + 16 ($10)` using `self_surface_z = +$14(a6)` (or `ali_surface_z` if `< 0`). Increments `+$10.w` by `+1` if `delta < 0`; decrements `+$10.w` by `-1` only if `delta > $0C` (12 px downward hysteresis deadband) and `+$10.w > 0`. Preserves low fractional word `+$12(a6)`.
     - **Hover Bobbing Integrator & Embedded 4-Word Table (`0x012C32..0x012C4E`):** Reads 4-byte-stride animation offset `+$30(a6)` (`0, 4, 8, 12`), computes `d0 >>= 1` (`0, 2, 4, 6`), loads signed 16-bit word from `0x012C46..0x012C4E` (**actual ROM words: `0x7000, 0x3000, 0x9000, 0xD000`** = `+28672, +12288, -28672, -12288`), sign-extends and doubles (`+0xE000, +0x6000, -0xE000, -0x6000` fixed 16.16 = `+/-0.875, +/-0.375` px/frame), and adds into 32-bit `+$10(a6)`. (Verified against Efreet Action 0 script at `0x04088C`, which loops 4 keyframes at offsets `0, 4, 8, 12` for 5 frames each, yielding zero net Z drift and `+6.25` px peak.)

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_efreet_follow_dismissal_m15_491.json`
- Extractor: `tools/extract_rom_efreet_follow_dismissal.py`
- Test: `tests/rom_efreet_follow_dismissal_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kEfreetVulnTriggerState             = 0x0010;
inline constexpr uint16_t kEfreetVulnTriggerAction            = 0x000A;
inline constexpr uint16_t kEfreetWanderNearChebyshevMaxPx     = 0x0040; // 64 px
inline constexpr uint16_t kEfreetWanderFollowChebyshevMaxPx   = 0x0080; // 128 px
inline constexpr uint16_t kEfreetWanderPrngTurnThreshold      = 0x7FFF;
inline constexpr uint8_t  kEfreetWanderDurationFrames         = 0x1E;   // 30 frames
inline constexpr uint8_t  kEfreetWanderAccelThreshold         = 0x0F;   // > 15 accelerates
inline constexpr int32_t  kEfreetWanderVelOscStep             = 0x00002000;
inline constexpr int16_t  kEfreetFollowOffsetPrimaryPx        = 0x0028; // 40 px
inline constexpr int16_t  kEfreetFollowOffsetSecondaryPx      = 0x0020; // 32 px
inline constexpr uint16_t kEfreetFollowFastD4Threshold        = 0x0004;
inline constexpr uint8_t  kEfreetFollowFacingDebounceFrames   = 4;
inline constexpr uint32_t kEfreetDismissalSfx                 = 0x00A0039Eu;
inline constexpr uint16_t kEfreetFlameSlotReservedSentinel    = 0xFFFE;
inline constexpr int16_t  kEfreetHoverTargetOffsetPx          = 0x0010; // 16 px
inline constexpr int16_t  kEfreetHoverDownwardHysteresisPx    = 0x000C; // 12 px

// Exact ROM table at 0x012C46..0x012C4E (doubled before adding to 16.16 Z).
inline constexpr std::array<int16_t, 4> kEfreetHoverBobbingWords = {
    static_cast<int16_t>(0x7000), // +28672 -> +0x0000E000 (+0.875 px/frame)
    static_cast<int16_t>(0x3000), // +12288 -> +0x00006000 (+0.375 px/frame)
    static_cast<int16_t>(0x9000), // -28672 -> -0x0000E000 (-0.875 px/frame)
    static_cast<int16_t>(0xD000), // -12288 -> -0x00006000 (-0.375 px/frame)
};

struct EfreetFollowState {
    uint16_t type_00{0x001A};
    uint16_t state_04{0};
    uint16_t field_06{0};
    int16_t  x_08{0};
    int16_t  y_0c{0};
    int32_t  z_10_fixed{0};
    int16_t  surface_z_14{0};
    uint16_t facing_16{0};
    uint16_t action_2a{0};
    uint16_t anim_offset_30{0};
    uint8_t  flags_36{0};
    uint8_t  anim_flags_37{0};
    uint8_t  flags_38{0};
    int32_t  vel_x_4e{0};
    int32_t  vel_y_52{0};
    int32_t  vel_x_72{0};
    int32_t  vel_y_76{0};
    uint8_t  wander_timer_9e{0};
    uint8_t  sub_entity_a2{0};
    uint8_t  facing_debounce_ac{0};
    uint16_t saved_facing_b2{0};
};

inline void TrackEfreetTargetElevation(
    EfreetFollowState& efreet,
    int16_t ali_surface_z) noexcept {
    const int16_t self_surf = (efreet.surface_z_14 >= 0) ? efreet.surface_z_14 : ali_surface_z;
    const int16_t target_total = static_cast<int16_t>(ali_surface_z + kEfreetHoverTargetOffsetPx);
    int16_t self_z = static_cast<int16_t>(efreet.z_10_fixed >> 16);
    const int16_t delta = static_cast<int16_t>(self_surf + self_z - target_total);
    if (delta == 0) {
        return;
    }
    if (delta < 0) {
        self_z = static_cast<int16_t>(self_z + 1);
    } else if (delta > kEfreetHoverDownwardHysteresisPx && self_z > 0) {
        self_z = static_cast<int16_t>(self_z - 1);
    } else {
        return;
    }
    efreet.z_10_fixed = (static_cast<int32_t>(self_z) << 16) | (efreet.z_10_fixed & 0xFFFF);
}

inline void IntegrateEfreetHoverBobbing(EfreetFollowState& efreet) noexcept {
    const size_t idx = static_cast<size_t>((efreet.anim_offset_30 >> 2) & 0x03u);
    const int32_t delta = static_cast<int32_t>(kEfreetHoverBobbingWords[idx]) * 2;
    efreet.z_10_fixed += delta;
}

inline int32_t StepEfreetWanderAxisVelocity(int32_t vel, uint8_t timer_9e) noexcept {
    if (vel == 0) {
        return 0;
    }
    const int32_t step = (timer_9e > kEfreetWanderAccelThreshold)
        ? kEfreetWanderVelOscStep
        : -kEfreetWanderVelOscStep;
    return (vel > 0) ? (vel + step) : (vel - step);
}

inline void ComputeEfreetBehindAliTarget(
    uint16_t ali_facing,
    int16_t ali_surface_z,
    int16_t self_surface_z,
    int16_t& target_x,
    int16_t& target_y) noexcept {
    if (self_surface_z >= 0 && ali_surface_z >= 0) {
        target_y = static_cast<int16_t>(target_y + (ali_surface_z - self_surface_z));
    }
    switch (ali_facing & 0xFFu) {
        case 0: target_x -= kEfreetFollowOffsetSecondaryPx; target_y += kEfreetFollowOffsetPrimaryPx; break;
        case 1: target_x -= kEfreetFollowOffsetPrimaryPx;   target_y -= kEfreetFollowOffsetSecondaryPx; break;
        case 2: target_x += kEfreetFollowOffsetSecondaryPx; target_y -= kEfreetFollowOffsetPrimaryPx; break;
        default: target_x += kEfreetFollowOffsetPrimaryPx;  target_y += kEfreetFollowOffsetSecondaryPx; break;
    }
}

inline void StartEfreetDismissal(
    EfreetFollowState& efreet,
    uint8_t& dismiss_flag_ff19a7,
    uint16_t* reserved_flame_slot_type) noexcept {
    dismiss_flag_ff19a7 = 0xFF;
    efreet.state_04 = 0x06;
    efreet.action_2a = 0x06;
    efreet.facing_16 = 2;
    efreet.flags_38 |= 0x08u;
    efreet.flags_36 &= static_cast<uint8_t>(~0x02u);
    efreet.sub_entity_a2 = 0xFF;
    if (reserved_flame_slot_type != nullptr &&
        *reserved_flame_slot_type == kEfreetFlameSlotReservedSentinel) {
        *reserved_flame_slot_type = 0;
    }
    efreet.field_06 = 0;
}

inline bool StepEfreetState06Despawn(EfreetFollowState& efreet) noexcept {
    if ((efreet.anim_flags_37 & 0x02u) != 0) {
        efreet.type_00 = 0;
        return true;
    }
    return false;
}

} // namespace Thor::AI
```
