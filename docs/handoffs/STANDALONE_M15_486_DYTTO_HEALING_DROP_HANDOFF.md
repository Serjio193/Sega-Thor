# STANDALONE HANDOFF: Milestone M15.486 — ROM Dytto Spirit 16-State Dispatcher, Healing Cast Sub-Entity Spawner & Jewel-Rank HP Recovery Subsystem

## 1. Subsystem Overview
- **ROM Address Range:** `0x011BA0..0x011D8A` (490 bytes: 127 instructions across `0x011BA0..0x011BAC` and `0x011BCC..0x011D8A` + 32-byte relative jump table at `0x011BAC..0x011BCC`).
- **Core Responsibilities:**
  1. `0x011BA0..0x011BCC` (`AI_Dytto_StateDispatcher`, 44 bytes: 4 insns + 16-entry `int16` jump table):
     - Reads state word `+$04(a6)` (`$00..$1E` in steps of 2), loads table slot address `a1 = 0x011BAC + d0`, adds signed 16-bit displacement `*(int16_t*)a1` (self-relative to the slot's own address), and jumps to `(a1)`.
  2. `0x011BCC..0x011BF2` (`AI_Dytto_TryStartHealingCast`, 38 bytes, 9 insns):
     - Attempts to deduct 20 (`0x14`) SP via `0x011B28` (`Spirit_TryConsumePlayerSP`). On Carry set (`remaining < 1`), branches to error buzzer `0x01219A` (`#$00A00253`).
     - On success: plays SFX `#$800258` via `0x00E7FC`, clears `+$16(a6)`, sets State `$18` (`+$04(a6)`), and sets action/anim `+$2A(a6) = $0E`.
  3. `0x011BF2..0x011C7A` (`AI_Dytto_State18_HealingCastAndSpawnOrb`, 136 bytes, 29 insns):
     - When `+$30(a6) == 0x28` (frame 40): allocates a Pool 1 slot (`0x00DA1A`), initializes sub-entity Type `$18` via `0x008D06`, sets sprite table `#$00141374`, priority `(parent_18 & 0x8000) | 0x07A8`, `+$36 = 8`, `+$38 = $0D`, `+$A2 = $FF`, State `$12` (`+$2A = $20`), copies `(x, y, z)` from parent with `+$10 += $20` (32 px), and clears `+$16`.
     - On other frames: checks `+$37(a6)` bit 1; if set, resets `+$04(a6) = 0` and jumps to State 0 handler `0x0121AA`.
  4. `0x011C7A..0x011D7C` (`AI_Dytto_State12_HealingOrbHomingAndHpRestore`, 258 bytes, 81 insns):
     - Steps `+$10(a6)` toward `+$10(a5) + $20` clamped to `[-3..+3]` while recording pre-step unsigned elevation delta `d6 = abs(target_z - z_before_step)`.
     - Calls `0x00E45E` toward Ali `(+$08(a5), +$0C(a5) + 6)` and doubles returned fixed-point velocities into `+$72(a6)` / `+$76(a6)`.
     - If `d6 > 4` or `d4 > 2`: on even frames (`($FF1971 & 1) == 0`), allocates a Pool 1 slot (`0x00DA1A`), copies the 188-byte entity (`0x00E66E`), sets State `$14` (`+$2A = $1C`), and applies PRNG jitter `(rng & 7) - 3` (`[-3..+4]`) via `0x00E268` to X and Y.
     - Upon arrival (`d6 <= 4` and `d4 <= 2`): despawns orb (`+$00(a6) = 0`), computes heal amount from `max_hp = +$88(a5)` and jewel rank `$FF0DB6` (`tier = (($FF0DB6 + 1) & 0xFF) >> 2`), clamps `current_hp + heal` to `max_hp`, stores to `+$8A(a5)` unless `$FF1987 != 0`, and plays `#$D00355` via `$060004`.
  5. `0x011D7C..0x011D8A` (`AI_Dytto_State14_HealingSparkleDespawn`, 14 bytes, 4 insns):
     - Clears `+$00(a6) = 0` when `+$37(a6)` bit 1 is set.

### 16-Entry Relative Jump Table (`0x011BAC..0x011BCC`)
| State | Slot Addr | Disp | Target | Role |
|---|---|---|---|---|
| `$00` | `0x011BAC` | `+0x05FE` | `0x0121AA` | Idle / command input |
| `$02` | `0x011BAE` | `+0x0526` | `0x0120D4` | State `$02` |
| `$04` | `0x011BB0` | `+0x05A0` | `0x012150` | State `$04` |
| `$06` | `0x011BB2` | `+0x0552` | `0x012104` | State `$06` |
| `$08` | `0x011BB4` | `+0x0442` | `0x011FF6` | State `$08` |
| `$0A` | `0x011BB6` | `+0x0474` | `0x01202A` | State `$0A` |
| `$0C` | `0x011BB8` | `+0x04CE` | `0x012086` | State `$0C` |
| `$0E` | `0x011BBA` | `+0x01FC` | `0x011DB6` | State `$0E` |
| `$10` | `0x011BBC` | `+0x0316` | `0x011ED2` | State `$10` |
| `$12` | `0x011BBE` | `+0x00BC` | `0x011C7A` | `AI_Dytto_State12_HealingOrbHomingAndHpRestore` |
| `$14` | `0x011BC0` | `+0x01BC` | `0x011D7C` | `AI_Dytto_State14_HealingSparkleDespawn` |
| `$16` | `0x011BC2` | `+0x03D6` | `0x011F98` | State `$16` |
| `$18` | `0x011BC4` | `+0x002E` | `0x011BF2` | `AI_Dytto_State18_HealingCastAndSpawnOrb` |
| `$1A` | `0x011BC6` | `+0x0568` | `0x01212E` | State `$1A` |
| `$1C` | `0x011BC8` | `+0x077A` | `0x012342` | State `$1C` |
| `$1E` | `0x011BCA` | `+0x07CA` | `0x012394` | State `$1E` |

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_dytto_healing_drop_m15_486.json`
- Extractor: `tools/extract_rom_dytto_healing_drop.py`
- Test: `tests/rom_dytto_healing_drop_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kDyttoHealingCastSpCost    = 0x0014;      // 20 SP
inline constexpr uint32_t kDyttoCastSfxId            = 0x00800258u;
inline constexpr uint32_t kDyttoHealSfxId            = 0x00D00355u;
inline constexpr uint16_t kDyttoOrbSpawnAnimFrame    = 0x0028;      // 40
inline constexpr uint32_t kDyttoOrbSpriteTablePtr    = 0x00141374u;
inline constexpr uint16_t kDyttoOrbPriorityBase      = 0x07A8;
inline constexpr int16_t  kDyttoOrbZOffset           = 0x0020;      // 32 px
inline constexpr int16_t  kDyttoOrbZStepClamp        = 3;
inline constexpr uint16_t kDyttoOrbArrivalDzMax      = 4;
inline constexpr uint16_t kDyttoOrbArrivalD4Max      = 2;

enum class DyttoStateId : uint16_t {
    Idle            = 0x00,
    HealingOrb      = 0x12,
    HealingSparkle  = 0x14,
    HealingCast     = 0x18,
};

struct DyttoEntityRecord {
    uint16_t type_00{0};
    uint16_t state_04{0};
    int16_t  x_08{0};
    int16_t  y_0c{0};
    int16_t  z_10{0};
    uint16_t facing_16{0};
    uint16_t priority_18{0};
    uint32_t sprite_table_1a{0};
    uint16_t action_2a{0};
    uint16_t anim_frame_30{0};
    uint8_t  flags_36{0};
    uint8_t  anim_flags_37{0};
    uint8_t  flags_38{0};
    int32_t  vel_x_72{0};
    int32_t  vel_y_76{0};
    uint16_t max_hp_88{0};
    uint16_t hp_8a{0};
    int16_t  sp_8e{0};
    uint8_t  sub_entity_a2{0};
};

inline uint16_t ComputeDyttoJewelRankHealAmount(uint16_t max_hp, uint8_t jewel_rank_ff0db6) noexcept {
    const uint8_t tier = static_cast<uint8_t>(static_cast<uint8_t>(jewel_rank_ff0db6 + 1u) >> 2);
    switch (tier) {
        case 4:  return max_hp;
        case 3: {
            const uint16_t half = static_cast<uint16_t>(max_hp >> 1);
            return static_cast<uint16_t>(half + (half >> 1));
        }
        case 2:  return static_cast<uint16_t>(max_hp >> 1);
        case 1:  return static_cast<uint16_t>(max_hp >> 2);
        default: return static_cast<uint16_t>(max_hp >> 3);
    }
}

inline void ApplyHealingOrbRestoration(
    DyttoEntityRecord& orb,
    DyttoEntityRecord& player,
    uint8_t jewel_rank_ff0db6,
    bool suppress_hp_write_ff1987) noexcept {
    orb.type_00 = 0;
    const uint16_t max_hp = player.max_hp_88;
    const uint16_t heal = ComputeDyttoJewelRankHealAmount(max_hp, jewel_rank_ff0db6);
    const uint16_t clamped_hp = static_cast<uint16_t>(
        std::min<uint32_t>(max_hp, static_cast<uint32_t>(player.hp_8a) + heal));
    if (!suppress_hp_write_ff1987) {
        player.hp_8a = clamped_hp;
    }
}

inline uint16_t StepHealingOrbElevation(DyttoEntityRecord& orb, int16_t player_z) noexcept {
    const int16_t target_z = static_cast<int16_t>(player_z + kDyttoOrbZOffset);
    const int16_t delta_z = static_cast<int16_t>(target_z - orb.z_10);
    const int16_t step = std::clamp<int16_t>(delta_z, -kDyttoOrbZStepClamp, kDyttoOrbZStepClamp);
    orb.z_10 = static_cast<int16_t>(orb.z_10 + step);
    return static_cast<uint16_t>(delta_z < 0 ? -delta_z : delta_z);
}

inline void InitSpawnedHealingOrb(
    DyttoEntityRecord& orb,
    const DyttoEntityRecord& parent) noexcept {
    orb = DyttoEntityRecord{};
    orb.type_00 = 0x0018;
    orb.sprite_table_1a = kDyttoOrbSpriteTablePtr;
    orb.priority_18 = static_cast<uint16_t>((parent.priority_18 & 0x8000u) | kDyttoOrbPriorityBase);
    orb.flags_36 = 0x08;
    orb.flags_38 = 0x0D;
    orb.sub_entity_a2 = 0xFF;
    orb.state_04 = static_cast<uint16_t>(DyttoStateId::HealingOrb);
    orb.action_2a = 0x0020;
    orb.x_08 = parent.x_08;
    orb.y_0c = parent.y_0c;
    orb.z_10 = static_cast<int16_t>(parent.z_10 + kDyttoOrbZOffset);
    orb.facing_16 = 0;
}

inline void InitSpawnedHealingSparkle(
    DyttoEntityRecord& sparkle,
    const DyttoEntityRecord& orb,
    uint16_t rng_x,
    uint16_t rng_y) noexcept {
    sparkle = orb;
    sparkle.state_04 = static_cast<uint16_t>(DyttoStateId::HealingSparkle);
    sparkle.action_2a = 0x001C;
    sparkle.x_08 = static_cast<int16_t>(sparkle.x_08 + static_cast<int16_t>(rng_x & 7u) - 3);
    sparkle.y_0c = static_cast<int16_t>(sparkle.y_0c + static_cast<int16_t>(rng_y & 7u) - 3);
}

inline bool StepHealingSparkleDespawn(DyttoEntityRecord& sparkle) noexcept {
    if ((sparkle.anim_flags_37 & 0x02u) != 0) {
        sparkle.type_00 = 0;
        return true;
    }
    return false;
}

} // namespace Thor::AI
```
