# STANDALONE HANDOFF: Milestone M15.497 — ROM Bow Spirit (0x1E) Master Tick, 9-Entry Jump Table, Command Starters & States $0E/$0A/$0C

## 1. Subsystem Overview
- **ROM Address Range:** `0x01387C..0x013B36` (698 bytes: 156 instructions across `0x01387C..0x0138E6` and `0x0138F8..0x013B36` + 18-byte 9-entry relative jump table at `0x0138E6..0x0138F8`).
- **Core Responsibilities:**
  1. `0x01387C..0x0138F8` (`AI_Bow_MasterTickAndStateDispatcher`, 124 bytes: 28 insns + 18-byte jump table):
     - Sets `a5 = $FF19E8`; jumps straight to dispatcher `0x0138DA` if `+$A2(a6) != 0`.
     - Calls `0x00E682` (stun early-return guard) and `0x011A4C` (`Spirit_PollButtonACommandInput`).
     - Ticks 30-frame (`0x1E`) SP drain timer `+$8E(a6)` to deduct `1` SP from `+$8E(a5)` (dismissing via `0x013F22` when SP is `0` before or after decrement); toggles `+$36(a6)` bit 1 when `SP <= 1` and clears bit 1 when `SP > 1`.
     - Unconditionally clears `+$86(a6).w`, `+$83(a6).b`, and `+$82(a6).b` (Bow ignores incoming damage/hitstun).
     - Dispatches `+$04(a6)` (`$00..$10`) via the 9-entry signed word self-relative jump table at `0x0138E6..0x0138F8`.
  2. `0x0138F8..0x013924` (`AI_Bow_State0E_FinishAndPlaySfx`, 44 bytes, 9 insns):
     - Waits for `+$37(a6)` bit 1, then sets `+$18 = $04F8`, State `$02`, `+$2A = $14`, sets bit 0 of `+$36(a6)`, and plays SFX `#$80037E` (`0x00E7FC`).
  3. `0x013924..0x013996` (`AI_Bow_TryStartCommand1_LungeBite`, 114 bytes, 28 insns; `$FF19A2 == 1`):
     - Calls `0x014494` for nearest enemy/target; rejects to `0x013A3A` on Carry set or `d0 > $D8` (`216` px).
     - Consumes `4` SP via `0x011B28` (else `0x013A3A`), projects 3D launch vector (scale `$80`) toward target `(x, y, z + (+$4A >> 2))` via `0x00F10C` into `+$4E/+$52/+$56`, computes facing `+$16` via `0x00E338`, sets State `$08`, `+$2A = $22`, `+$02 = $0C` (12), and plays SFX `#$700221` (`0x00E7FC`).
  4. `0x013996..0x013A18` (`AI_Bow_TryStartCommand2_UndergroundDive`, 130 bytes, 29 insns; `$FF19A2 == 2`):
     - Rejects to `0x013A3A` unless Ali `+$10(a5) == 0`, `+$39(a5)` bit 1 clear, terrain probe `0x009C40` bit 7 clear at Ali, and `9` SP via `0x011B28`.
     - Calls `0x013FDC`, enters State `$04`, `+$2A = 0`, computes 2D homing velocity toward `$FF1C1C` `+$4E/+$52` via `0x00E45E` into `+$4E/+$52`, sets `+$38` bit 2, `+$AC = $2710`, `+$02 = $1E`, latches Ali `(x, y)` into `+$AE/+$B2`, plays SFX `#$800258`.
  5. `0x013A18..0x013A3A` (`AI_Bow_TryStartCommand3_PlantBloom`, 34 bytes, 8 insns; `$FF19A2 == 3`):
     - Consumes `28` (`$1C`) SP via `0x011B28` (else `0x013A3A`), enters State `$0A`, `+$2A = $10`, clears `+$16`, sets `+$38` bit 2.
  6. `0x013A3A..0x013A5C` (`AI_Bow_CommandRejectReset`, 34 bytes, 6 insns):
     - Sets `+$2A = 6`, State `$00`, `+$9E = 4`, plays rejection buzzer `#$90037D` via `$060004`, branches to State 0 follow loop `0x013CDC`.
  7. `0x013A5C..0x013AE8` (`AI_Bow_State0A_BloomWindup`, 140 bytes, 32 insns):
     - Plays SFX `#$800226` when `+$30 == 8` and `+$2E == 0`; while `+$30 < $0C` increments `+$10` and calls `0x014200` (`d7 = 7`).
     - On `+$37` bit 1: enters State `$0C`, `+$2A = $26`, sets `+$36` bit 1, sets `+$02 = ($FF0DB9 << 5) + $78`, sets `$FF17B8` bit 0, clears `+$10`, snaps `(x, y)` to `$FF1C1C` `+$4E/+$52`, calls `0x009D00`, clears `$FF1C1C` `+$00`, plays SFX `#$800380`.
  8. `0x013AE8..0x013B36` (`AI_Bow_State0C_BloomActiveAndCooldown`, 78 bytes, 16 insns):
     - Locks `$FF199E = $FF00 | $FF1971` and keeps `+$36` bit 1 set while `+$8E(a5) != 0` and `--+$02(a6) != 0`; when SP hits 0 or timer hits 0, clears `+$36` bit 1, clears `$FF199E` and `$FF17B8` bit 0, enters State `$02`, `+$2A = $14`, plays SFX `#$80037E`.

### Bow 9-Entry Relative Jump Table (`0x0138E6..0x0138F8`)
| State | Slot Addr | Disp | Target | Role |
|---|---|---|---|---|
| `$00` | `0x0138E6` | `+0x03CE` | `0x013CB4` | Idle / command & follow |
| `$02` | `0x0138E8` | `+0x0686` | `0x013F6E` | Post-action / summon settle |
| `$04` | `0x0138EA` | `+0x0308` | `0x013BF2` | Underground dive / emerge |
| `$06` | `0x0138EC` | `+0x0390` | `0x013C7C` | Sub-state `$06` |
| `$08` | `0x0138EE` | `+0x0248` | `0x013B36` | Lunge bite attack |
| `$0A` | `0x0138F0` | `+0x016C` | `0x013A5C` | `AI_Bow_State0A_BloomWindup` |
| `$0C` | `0x0138F2` | `+0x01F6` | `0x013AE8` | `AI_Bow_State0C_BloomActiveAndCooldown` |
| `$0E` | `0x0138F4` | `+0x0004` | `0x0138F8` | `AI_Bow_State0E_FinishAndPlaySfx` |
| `$10` | `0x0138F6` | `+0x0664` | `0x013F5A` | Dismissal descent |

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_bow_dispatch_commands_m15_497.json`
- Extractor: `tools/extract_rom_bow_dispatch_commands.py`
- Test: `tests/rom_bow_dispatch_commands_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kBowEntityType              = 0x001Eu;
inline constexpr uint16_t kBowSpDrainIntervalFrames   = 0x001Eu;     // 30 frames
inline constexpr uint16_t kBowBiteMaxDistancePx       = 0x00D8u;     // 216 px
inline constexpr int16_t  kBowBiteSpCost              = 4;
inline constexpr int16_t  kBowDiveSpCost              = 9;
inline constexpr int16_t  kBowBloomSpCost             = 28;          // 0x1C
inline constexpr uint16_t kBowBloomBaseDurationFrames = 0x0078u;     // 120 frames
inline constexpr uint16_t kBowState0EPriority         = 0x04F8u;
inline constexpr uint32_t kBowRejectSfxId             = 0x0090037Du;
inline constexpr uint32_t kBowFinishSfxId             = 0x0080037Eu;
inline constexpr uint32_t kBowBiteSfxId               = 0x00700221u;
inline constexpr uint32_t kBowDiveSfxId               = 0x00800258u;
inline constexpr uint32_t kBowWindupSfxId             = 0x00800226u;
inline constexpr uint32_t kBowBloomSfxId              = 0x00800380u;

struct BowEntityState {
    uint16_t timer_02{0};
    uint16_t state_04{0};
    int16_t  x_08{0};
    int16_t  y_0c{0};
    int32_t  z_10_fixed{0};
    uint16_t facing_16{0};
    uint16_t priority_18{0};
    uint16_t action_2a{0};
    uint16_t anim_frame_30{0};
    uint8_t  flags_36{0};
    uint8_t  anim_flags_37{0};
    uint8_t  flags_38{0};
    int32_t  vel_x_4e{0};
    int32_t  vel_y_52{0};
    int32_t  vel_z_56{0};
    uint8_t  hit_flag_82{0};
    uint8_t  hitstun_83{0};
    uint16_t damage_86{0};
    uint16_t drain_timer_8e{kBowSpDrainIntervalFrames};
    uint8_t  sub_mode_9e{0};
    uint8_t  sub_entity_a2{0};
    uint16_t field_ac{0};
    int32_t  latch_x_ae{0};
    int32_t  latch_y_b2{0};
};

inline bool StepBowMasterTickSpAndClearDamage(
    BowEntityState& bow,
    int16_t& player_sp) noexcept {
    if (bow.sub_entity_a2 != 0) {
        return true;
    }
    if (bow.drain_timer_8e != 0) {
        --bow.drain_timer_8e;
    } else {
        bow.drain_timer_8e = kBowSpDrainIntervalFrames;
        if (player_sp == 0 || --player_sp == 0) {
            return false;  // Dismiss via 0x013F22
        }
    }
    if (player_sp <= 1) {
        bow.flags_36 ^= 0x02u;
    } else {
        bow.flags_36 &= static_cast<uint8_t>(~0x02u);
    }
    bow.damage_86 = 0;
    bow.hitstun_83 = 0;
    bow.hit_flag_82 = 0;
    return true;
}

inline uint16_t ComputeBowBloomDuration(uint8_t bow_rank_ff0db9) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(bow_rank_ff0db9) << 5) + kBowBloomBaseDurationFrames);
}

inline bool StepBowBloomActiveState(
    BowEntityState& bow,
    int16_t player_sp,
    uint8_t frame_ff1971,
    uint16_t& hold_counter_ff199e,
    uint8_t& global_ff17b8) noexcept {
    hold_counter_ff199e = static_cast<uint16_t>(0xFF00u | frame_ff1971);
    bow.flags_36 |= 0x02u;
    if (player_sp != 0 && --bow.timer_02 != 0) {
        return true;
    }
    bow.flags_36 &= static_cast<uint8_t>(~0x02u);
    hold_counter_ff199e = 0;
    global_ff17b8 &= static_cast<uint8_t>(~0x01u);
    bow.state_04 = 0x02;
    bow.action_2a = 0x14;
    return false;
}

} // namespace Thor::AI
```
