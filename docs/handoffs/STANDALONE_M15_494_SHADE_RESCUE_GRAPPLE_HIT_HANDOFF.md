# STANDALONE HANDOFF: Milestone M15.494 — ROM Shade Spirit Buzzer, Dark Hole Fall-Rescue Drain & Teleport Recovery, and Shadow Grapple Extension & Hit Latch

## 1. Subsystem Overview
- **ROM Address Range:** `0x01319E..0x01341A` (636 bytes, 147 instructions, zero embedded data tables).
- **Core Responsibilities:**
  1. `0x01319E..0x0131AE` (`AI_Shade_InsufficientSpBuzzer`, 16 bytes, 3 instructions): Plays SFX `#$00A00253` via `0x060004` and branches to `0x0136A0`.
  2. `0x0131AE..0x01325E` (`AI_Shade_State0C_DarkHoleRescueAndDrain`, 176 bytes, 39 instructions):
     - Clears Shade `+$72/+$76(a6)`.
     - While `$FF1985 != 1` and Ali SP `+$8E(a5) != 0`: decrements Shade drain timer `+$8E(a6)` by 4 (clamped `>= 0`) and drives Ali blink bit 1 on `+$36(a5)` from `$FF1971` bit 0 (clear on bit 0 = 1, set on bit 0 = 0).
     - When `$FF1985 == 1` or `+$8E(a5) == 0`: clears `+$36` bit 1 and `+$38` bits 2 & 6 on Ali, restores `+$06(a5) = $FF0D7E`, sets Ali State `0`, Anim `0`, Facing `2`, `+$82 = $0A`, snaps Ali `(+$08, +$0C)` to Shade with `+$10(a5) = 0`, invokes `0x009D00` on Ali, calls `0x01378A`, transitions Shade to State `$0E`, clears `$FF164F`, sets `$FF1896 = 1`, calls fade helper `0x002E1E`, and sets `$FF17BA = $FF`.
  3. `0x01325E..0x0132AA` (`AI_Shade_State0E_FinishDarkHoleRecovery`, 76 bytes, 13 instructions): Clears `$FF1985`, clears `+$38(a5)` bit 3 and `$FF17B8` bit 2, resets Shade State `0`, zeroes `$FF199E` and `$FF19A1`, sets `$FF19A0 = $FF`, calls `0x002D84`, clears `$FF1896`, and sets `$FF164F = $FF` if `$FF16F1` bit 1 is set.
  4. `0x0132AA..0x01341A` (`AI_Shade_State06_08_GrappleExtendAndHitLatch`, 368 bytes, 92 instructions):
     - **State `$06` (`0x0132AA..0x0133D0`):** Zeroes Ali `+$72/+$76(a5)`. Tests `(+$40 | +$41)` bit 6 (wall contact -> `0x0133E6` retract) and bit 7 (entity contact -> `0x01332A`). Otherwise calls item probe `0x013608`; on Carry clear and `+$14(a0) == +$14(a6)`, stores `+$AA(a6) = a0`, calls `0x013550`, sets `+$2A = 4` and State `$08`. Otherwise decrements `+$02` (-> State `$08, +$2A = $12` on 0) and accelerates `(0x80, 0x80)` via `0x00E2D4`, accumulating `+$4E/+$52` and integrating into `+$72/+$76`.
     - **Entity Contact (`0x01332A..0x0133CC`):** Loads `a0 = +$68(a6)`. If `+$37(a0)` bit 7 is set (grabbable) and `+$14(a0) == +$14(a6)`, stores `+$AA(a6) = a0` and calls `0x0134DE`, entering State `$08` (`+$2A = $12`). Else if `+$38(a0)` bit 3, `+$82(a0)`, and `+$83(a0)` are all 0, writes `$FF184F = $5A`, `$FF1850 = a0`, `+$7E = a6`, `+$82 = 0`, `+$83 = $0A`, `+$9D = 0`, `+$84 = +$16(a6)`, knockback `(0x500, 0x4C0)` via `0x00E2D4` -> `+$4E/+$52(a0)`, `+$56(a0) = $28000`, `+$86(a0) = 1 + ($FF0DB8 >> 1)` (with big-endian high-byte bit 7 i.e. `0x8000` set via `bset.b #7, $86(a0)` when `+$10(a0) != 0`), plays SFX `#$700345`, and transitions to State `$08` (`+$2A = $12`).
     - **State `$08` (`0x0133D0..0x01341A`):** Calls `0x0134DE`, zeroes Ali `+$72/+$76(a5)`, and on `+$37` bit 1 enters `0x0133E6` (State `$0A`, `+$2A = 6`, negates `+$4E/+$52`, sets `+$02 = 18 - +$02`, sets `+$38` bit 2).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_shade_rescue_grapple_hit_m15_494.json`
- Extractor: `tools/extract_rom_shade_rescue_grapple_hit.py`
- Test: `tests/rom_shade_rescue_grapple_hit_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::AI {

constexpr uint16_t kShadeRescueDrainStep       = 4;
constexpr uint8_t  kShadeRescueAliHitstop      = 0x0A;
constexpr uint16_t kShadeGrappleAccelStep      = 0x0080;
constexpr uint16_t kShadeGrappleKnockbackVx    = 0x0500;
constexpr uint16_t kShadeGrappleKnockbackVy    = 0x04C0;
constexpr int32_t  kShadeGrappleKnockbackVz    = 0x00028000;
constexpr uint8_t  kShadeGrappleEnemyHitstop   = 0x0A;
constexpr uint8_t  kShadeGrappleHudTimer       = 0x5A;
constexpr uint16_t kShadeAirborneWordFlag      = 0x8000;
constexpr uint16_t kShadeRetractBaseFrames     = 18;  // 0x12

inline uint16_t DrainShadeRescueTimer(uint16_t timer_8e) noexcept {
    if (timer_8e == 0) {
        return 0;
    }
    return (timer_8e > kShadeRescueDrainStep)
        ? static_cast<uint16_t>(timer_8e - kShadeRescueDrainStep)
        : 0U;
}

inline uint16_t ComputeShadeGrappleDamageWord(uint8_t shade_level_db8, bool target_airborne) noexcept {
    uint16_t dmg = static_cast<uint16_t>(1U + (static_cast<uint16_t>(shade_level_db8) >> 1));
    if (target_airborne) {
        dmg |= kShadeAirborneWordFlag;  // Big-endian bset.b #7, $86(a0) sets bit 15
    }
    return dmg;
}

inline uint16_t ComputeShadeRetractTimer(uint16_t remaining_extend_frames) noexcept {
    return static_cast<uint16_t>(kShadeRetractBaseFrames - remaining_extend_frames);
}

} // namespace Thor::AI
```
