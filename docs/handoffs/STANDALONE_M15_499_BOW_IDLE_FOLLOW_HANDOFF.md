# STANDALONE HANDOFF: Milestone M15.499 — ROM Bow Spirit (0x1E) State $00 Idle, Command Dispatch, Vulnerability/Wander & Leashed Follow AI

## 1. Subsystem Overview
- **ROM Address Range:** `0x013CB4..0x013F22` (622 contiguous bytes of executable M68K code, 163 instructions, zero embedded data tables, ending cleanly at `0x013F20` `rts`).
- **Core Responsibilities:**
  1. `0x013CB4..0x013DAA` (`AI_Bow_State00_DispatchAndSubstateRouter`, 246 bytes, 61 insns):
     - **State `$00` Entry (`0x013CB4`):** Dispatched via jump table `0x0138E6`. Checks global dismissal `$FF19A7 != 0` (`-> 0x013F2A`) and dispatches `$FF19A2` player commands: `1 -> 0x013924` (Lunge Bite), `2 -> 0x013996` (Underground Dive), `3 -> 0x013A18` (Plant Bloom).
     - **Command Reject Continuation (`0x013CDC`):** Entered from `0x013A3A` (`+$2A = 6`, State `0`, `+$9E = 4`). Fast-paths `+$9E == 3` directly to `0x013E26`. Otherwise calls `0x013FDC` (updates `+$16` facing toward stem anchor `$FF1C1C` `+$4E/+$52`) and `0x0143C8` (auto-pounce scan over 17 Pool 2 slots within `<= $36` px of `$FF1C1C`, arming `+$2A = $22`, `+$02 = $0C`, `+$9E = 3`).
     - **Substate Router (`0x013CEE`):** Routes `+$9E`: `0 -> 0x013D0C`, `1 -> 0x013DE0`, `2 -> 0x013DF2`, `3 -> 0x013E26`, `4 -> 0x013EBC`.
     - **Idle PRNG Selector (`0x013D0C..0x013DA8`):** Clears `+$38` bit 2, updates stem chain via `0x014242` (`d7 = 6`), and rolls `0x00E268` byte `d0.b`:
       - `<= $C0`: starts PRNG hop (`0x013DAA`)
       - `$C1..$C8`: `+$2A = 2`, `+$9E = 4`, SFX `#$70025C`
       - `$C9..$D0`: `+$2A = 4`, `+$9E = 4`, SFX `#$700221`
       - `$D1..$D8`: `+$2A = $0A`, `+$9E = 4` (silent)
       - `$D9..$E0`: `+$2A = $0E`, `+$9E = 4`, SFX `#$700234`
       - `$E1..$FF`: rolls `0x00E268` for `+$02 = (d0 & $0F) + $0C` (`12..27` frames) and enters Substate `1`.
  2. `0x013DAA..0x013E26` (`AI_Bow_Substates_01_02_HopAndRest`, 124 bytes, 37 insns):
     - **Hop Launch (`0x013DAA`):** `+$56 = (u16(prng) << 2) + $30000`, `+$4E = s16(prng) << 1`, `+$52 = s16(prng) << 1`, `+$9E = 2`.
     - **Substate `1` Rest (`0x013DE0`):** Calls `0x014242` (`d7 = 6`), decrements `+$02`, clears `+$9E = 0` on `0`.
     - **Substate `2` Hop/Settle (`0x013DF2`):** Integrates `+$4E/+$52 -> +$72/+$76`, subtracts gravity `$C000` from `+$56 -> +$10` (clearing `+$10 = 0` and `+$9E = 0` when `< 0`), and calls `0x014200` (`d7 = 6` px/seg leash).
  3. `0x013E26..0x013EBC` (`AI_Bow_Substate03_AutoPounceAttack`, 150 bytes, 40 insns):
     - Decrements `+$02` (`-> 0x013E96` on `0`).
     - If `+$2A != 8`, probes Pool 2 via `0x00BBA8` with hitbox table `$05887C` and sets `+$2A = 8` on hit (`Carry` set).
     - When `+$2A == 8`, writes `+$90 = (($FF0DB9 + 1) >> 2) * 2` (2x chomp bonus), calls `0x00BCA4` (`d0 = 0`), clears `+$90`, and exits to `0x013E96` on `+$37` bit 1.
     - Otherwise integrates `+$4E/+$52 -> +$72/+$76` and `+$56 -> +$10` (clamped `>= 0`, no gravity subtraction), and calls `0x014200` (`d7 = 8` px/seg leash).
     - **Exit (`0x013E96`):** Sets `+$2A = 0`, `+$9E = 2`, zeroes `+$4E/+$52/+$56`, clears `+$38` bit 2, and calls `0x014200` (`d7 = 6`).
  4. `0x013EBC..0x013F22` (`AI_Bow_Substate04_EmoteAndSettle`, 102 bytes, 25 insns):
     - Plays SFX `#$700235` via `0x00E7FC` when `+$32 == $3F` and `+$2E == 0`.
     - Sets `+$9E = 2` and `+$2A = 0` on `+$37` bit 1.
     - Integrates `+$4E/+$52 -> +$72/+$76` and gravity `-$C000` on `+$56 -> +$10` (clearing `+$10`, `+$4E`, and `+$52` when `< 0`), then calls `0x014200` (`d7 = 6`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_bow_idle_follow_m15_499.json`
- Extractor: `tools/extract_rom_bow_idle_follow.py`
- Test: `tests/rom_bow_idle_follow_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kBowIdleLeashMarginPx        = 6;
inline constexpr uint16_t kBowPounceLeashMarginPx      = 8;
inline constexpr uint8_t  kBowPrngHopMaxByte           = 0xC0;
inline constexpr uint16_t kBowRestBaseFrames           = 0x000C;      // 12
inline constexpr uint16_t kBowRestRandMask             = 0x000F;
inline constexpr int32_t  kBowHopVzBaseFixed           = 0x00030000;
inline constexpr int32_t  kBowIdleGravityStep          = 0x0000C000;
inline constexpr uint16_t kBowPounceBiteAnimId         = 0x0008;
inline constexpr uint16_t kBowSubstate04SoundSpriteId  = 0x003F;
inline constexpr uint32_t kBowEmoteSfx70025C           = 0x0070025Cu;
inline constexpr uint32_t kBowEmoteSfx700221           = 0x00700221u;
inline constexpr uint32_t kBowEmoteSfx700234           = 0x00700234u;
inline constexpr uint32_t kBowEmoteSfx700235           = 0x00700235u;

struct BowIdleState {
    int16_t  timer_02{0};
    uint16_t state_04{0};
    int32_t  z_10_fixed{0};
    uint16_t action_2a{0};
    uint16_t anim_delay_2e{0};
    uint16_t sprite_frame_32{0};
    uint8_t  anim_flags_37{0};
    uint8_t  flags_38{0};
    int32_t  vel_x_4e{0};
    int32_t  vel_y_52{0};
    int32_t  vel_z_56{0};
    int32_t  vel_x_72{0};
    int32_t  vel_y_76{0};
    uint16_t attack_bonus_90{0};
    uint8_t  substate_9e{0};
};

inline uint16_t ComputeBowAutoPounceBonusDamage(uint8_t bow_rank_ff0db9) noexcept {
    return static_cast<uint16_t>(((static_cast<uint16_t>(bow_rank_ff0db9) + 1u) >> 2) * 2u);
}

inline void StepBowSubstate02HopPhysics(BowIdleState& bow) noexcept {
    bow.vel_x_72 += bow.vel_x_4e;
    bow.vel_y_76 += bow.vel_y_52;
    bow.vel_z_56 -= kBowIdleGravityStep;
    bow.z_10_fixed += bow.vel_z_56;
    if (bow.z_10_fixed < 0) {
        bow.z_10_fixed = 0;
        bow.substate_9e = 0;
    }
}

inline void StepBowSubstate04EmoteSettle(BowIdleState& bow) noexcept {
    if ((bow.anim_flags_37 & 0x02u) != 0u) {
        bow.substate_9e = 2;
        bow.action_2a = 0;
    }
    bow.vel_x_72 += bow.vel_x_4e;
    bow.vel_y_76 += bow.vel_y_52;
    bow.vel_z_56 -= kBowIdleGravityStep;
    bow.z_10_fixed += bow.vel_z_56;
    if (bow.z_10_fixed < 0) {
        bow.z_10_fixed = 0;
        bow.vel_x_4e = 0;
        bow.vel_y_52 = 0;
    }
}

} // namespace Thor::AI
```
