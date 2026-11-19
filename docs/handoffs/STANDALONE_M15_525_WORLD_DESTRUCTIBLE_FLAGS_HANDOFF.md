# STANDALONE HANDOFF: Milestone M15.525 — ROM Interactive/Destructible Enemy/Object & Core Persistent Flag Bitfield Helpers

## 1. Subsystem Overview
- **ROM Address Range:** `0x017E28..0x018072` (586 bytes, 146 M68K instructions across 3 routines, plus 2 embedded data tables totaling 18 bytes).
- **Core Responsibilities:**
  1. `0x017E28..0x017FF2` (`AI_WorldObject_DestructibleEnemyObject`, 458 bytes, 107 instructions):
     - **First-Tick Init (`0x017E28`):** If `+$66 != 0`, clears facing `+$16 = 0` and clears `+$66 = 0`. Calls common entity status helper `0x00E682`.
     - **Damage & Defense Processing (`0x017E3E..0x017EBC`):** When `+$86 != 0`, clears `+$86` and checks bit 13 (`0x2000`): if set, sets hit flag `+$39:3` and plays SFX `#$8004D9` via `0x00E7E8`. Masks 12-bit base damage (`d0 & 0x0FFF`), subtracts defense `+$92`, clamps minimum damage to `1`, and subtracts from HP `+$8A` (clamped `>= 0`).
       - If `HP == 0`: transitions to Death State `2` (`+$4 = 2`), sets anim `2` via `0x008EF8`, sets blink timer `+$2 = 16`, sets invulnerability `+$38:3`, and plays death SFX `#$900232` via `0x00E7FC`.
       - Else (`HP > 0`): transitions to Hit/Flinch State `8` (`+$4 = 8`) and sets anim `10` (`$A`) via `0x008EF8`.
     - **Z-Axis Gravity (`0x017EBC..0x017EDA`):** When `z` (`+$10.l`) is non-zero, subtracts `$C000` from vertical velocity `+$56.l`, integrates `z += vz`, and clamps `z < 0` to `0`.
     - **Relative Jump Table (`0x017EE6`, 5 entries for states `0, 2, 4, 6, 8` via `+$4`):**
       - `0 -> 0x017F40` (State 0 — Idle Wait): checks active screen shake `$FF17C4`; if non-zero or decremented timer `+$2 <= 0`, invokes action selector `0x017EF0`.
       - `2 -> 0x017F50` (State 2 — Death Blink & Loot Drop): toggles visibility bit `+$36:1` for `+$2` (16) frames; upon expiration passes embedded loot table `0x017F6C` (`00 00 1E 01 00 00 1E 01`) to `0x00F476` and despawns (`+$0 = 0`).
       - `4 -> 0x017F74` (State 4 — Secondary Idle / React Anim): waits for animation finished bit `+$37:1 != 0`, then branches to `0x017EF0`.
       - `6 -> 0x017F80` (State 6 — Close-Range Attack & Callback `0x017FC0`): on animation frame `+$30 == 12` (`$C`) or `20` (`$14`) with sub-tick `+$2E == 0`, plays swing SFX `#$800202` via `0x00E7FC`. Registers hit callback `0x017FC0` into `$FF1940` and calls `0x00BCE2`. Callback `0x017FC0` calls `0x00BEBA`, sets target knockback mode `+$84 = 2`, and negates target Y velocity `+$52`. On `+$37:1 != 0`, branches to `0x017EF0`.
       - `8 -> 0x017FD2` (State 8 — Hit / Flinch Recovery): sets `+$36:6` while animating; when `+$37:1 != 0`, clears `+$36:6` and `+$39:3` and branches to `0x017EF0`.
     - **Action Selector (`0x017EF0..0x017F40`):** Queries distance `d5` via `0x00E398`: if `d5 < 48` (`$30`), enters State `6` (`+$4 = 6, +$2A = 6`); else if screen shake `$FF17C4 != 0` or random `0x00E268 < 0x1A00`, enters State `4` (`+$4 = 4, +$2A = 4`); else enters State `0` (`+$2 = 4, +$4 = 0, +$2A = 0`).
  2. `0x017FF2..0x018030` (`PersistentFlag_SetBit`, 62 bytes, 19 instructions):
     - Maps entity bank selector `+$A2(a0)` (`1 -> $FF0DBC`, `2 -> $FF0E1C`, `3 -> $FF0E2C`; `0` or `>= 4` -> no-op) and bit index `+$A4(a0)` (`byte_offset = +$A4 >> 3`, `bit_idx = +$A4 & 7`) to set the persistent bit in RAM (`bset.b d1, (a1, d0.w)`).
  3. `0x018030..0x018072` (`PersistentFlag_ResolveBit`, 66 bytes, 20 instructions):
     - Maps entity bank selector `+$A2(a6)` (`1 -> $FF0DBC`, `2 -> $FF0E1C`, `3 -> $FF0E2C`) and bit index `+$A4(a6)` to return `(a0 = base + (+$A4 >> 3), d0 = +$A4 & 7, C = 0)`. If `+$A2 == 0` or `>= 4`, sets Carry (`C = 1`) and returns.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_world_destructible_flags_m15_525.json`
- Extractor: `tools/extract_rom_world_destructible_flags.py`
- Unit Tests: `tests/rom_world_destructible_flags_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace Thor::World::DestructibleFlags {

enum class PersistentBank : uint8_t {
    None  = 0,
    Bank1 = 1, // 0xFF0DBC
    Bank2 = 2, // 0xFF0E1C
    Bank3 = 3, // 0xFF0E2C
};

struct PersistentFlagStorage {
    std::span<uint8_t> bank1_ff0dbc;
    std::span<uint8_t> bank2_ff0e1c;
    std::span<uint8_t> bank3_ff0e2c;
};

struct ResolvedFlagBit {
    uint8_t* byte_ptr{nullptr};
    uint8_t bit_index{0};
};

/// Emulates PersistentFlag_ResolveBit (0x018030..0x018072).
/// Returns std::nullopt when Carry would be set (bank 0 or >= 4).
inline std::optional<ResolvedFlagBit> ResolvePersistentFlagBit(
    uint8_t bank_a2,
    uint16_t bit_index_a4,
    PersistentFlagStorage& storage
) noexcept {
    std::span<uint8_t> target_bank{};
    if (bank_a2 == 1) {
        target_bank = storage.bank1_ff0dbc;
    } else if (bank_a2 == 2) {
        target_bank = storage.bank2_ff0e1c;
    } else if (bank_a2 == 3) {
        target_bank = storage.bank3_ff0e2c;
    } else {
        return std::nullopt;
    }

    const uint16_t byte_offset = bit_index_a4 >> 3;
    const uint8_t bit_idx = static_cast<uint8_t>(bit_index_a4 & 0x07);
    if (byte_offset >= target_bank.size()) {
        return std::nullopt;
    }
    return ResolvedFlagBit{&target_bank[byte_offset], bit_idx};
}

/// Emulates PersistentFlag_SetBit (0x017FF2..0x018030).
inline bool SetPersistentFlagBit(
    uint8_t bank_a2,
    uint16_t bit_index_a4,
    PersistentFlagStorage& storage
) noexcept {
    auto resolved = ResolvePersistentFlagBit(bank_a2, bit_index_a4, storage);
    if (!resolved.has_value()) {
        return false;
    }
    *(resolved->byte_ptr) |= static_cast<uint8_t>(1U << resolved->bit_index);
    return true;
}

inline constexpr std::array<uint8_t, 8> kLootDropTable017F6C = {
    0x00, 0x00, 0x1E, 0x01, 0x00, 0x00, 0x1E, 0x01
};

struct CombatTargetEntity {
    uint16_t knockback_mode_84{0};
    int32_t vy_52{0};
    bool hit_beba_applied{false};
};

/// Emulates Attack Hit Callback (0x017FC0..0x017FD2).
inline void ApplyAttackHitCallback017FC0(CombatTargetEntity& target) noexcept {
    target.hit_beba_applied = true;
    target.knockback_mode_84 = 2;
    target.vy_52 = -target.vy_52;
}

struct DestructibleEnemyObject {
    uint16_t active_0{1};
    int16_t timer_2{0};
    uint16_t state_4{0};          // 0, 2, 4, 6, 8
    int32_t z_10{0};              // 16.16 fixed point
    uint16_t facing_16{0};
    uint16_t anim_2a{0};
    uint16_t subtick_2e{0};
    uint16_t anim_frame_30{0};
    uint8_t flags_36{0};          // bit 1: hidden/blink, bit 6: flinching
    uint8_t flags_37{0};          // bit 1: anim finished
    uint8_t flags_38{0};          // bit 3: invulnerable
    uint8_t flags_39{0};          // bit 3: special hit status
    int32_t vz_56{0};
    uint8_t init_flag_66{1};
    uint16_t damage_86{0};
    int16_t hp_8a{10};
    int16_t defense_92{0};
    uint32_t last_sfx{0};
    bool loot_spawned{false};
};

/// Emulates action selector 0x017EF0..0x017F40.
inline void SelectNextAction017EF0(
    DestructibleEnemyObject& obj,
    uint16_t target_distance_d5,
    uint16_t screen_shake_ff17c4,
    uint16_t prng_d0
) noexcept {
    if (target_distance_d5 < 0x0030) {
        obj.state_4 = 6;
        obj.anim_2a = 6;
        return;
    }
    if (screen_shake_ff17c4 != 0 || prng_d0 < 0x1A00) {
        obj.state_4 = 4;
        obj.anim_2a = 4;
        return;
    }
    obj.timer_2 = 4;
    obj.state_4 = 0;
    obj.anim_2a = 0;
}

/// Emulates AI_WorldObject_DestructibleEnemyObject (0x017E28..0x017FF2).
inline void StepDestructibleEnemyObject(
    DestructibleEnemyObject& obj,
    uint16_t target_distance_d5,
    uint16_t screen_shake_ff17c4,
    uint16_t prng_d0,
    CombatTargetEntity* hit_target = nullptr
) noexcept {
    if (obj.init_flag_66 != 0) {
        obj.facing_16 = 0;
        obj.init_flag_66 = 0;
    }

    if (obj.damage_86 != 0) {
        const uint16_t dmg = obj.damage_86;
        obj.damage_86 = 0;

        if ((dmg & (1U << 13)) != 0) {
            obj.flags_39 |= 0x08;
            obj.last_sfx = 0x8004D9;
        }

        int16_t net_dmg = static_cast<int16_t>(dmg & 0x0FFF) - obj.defense_92;
        if (net_dmg <= 0) {
            net_dmg = 1;
        }

        int16_t next_hp = obj.hp_8a - net_dmg;
        if (next_hp < 0) {
            next_hp = 0;
        }
        obj.hp_8a = next_hp;

        if (next_hp == 0) {
            obj.state_4 = 2;
            obj.anim_2a = 2;
            obj.timer_2 = 16;
            obj.flags_38 |= 0x08;
            obj.last_sfx = 0x900232;
        } else {
            obj.state_4 = 8;
            obj.anim_2a = 10;
        }
    }

    if (obj.z_10 != 0) {
        obj.vz_56 -= 0xC000;
        obj.z_10 += obj.vz_56;
        if (obj.z_10 < 0) {
            obj.z_10 = 0;
        }
    }

    switch (obj.state_4) {
        case 0:
            if (screen_shake_ff17c4 != 0) {
                SelectNextAction017EF0(obj, target_distance_d5, screen_shake_ff17c4, prng_d0);
                break;
            }
            obj.timer_2 -= 1;
            if (obj.timer_2 <= 0) {
                SelectNextAction017EF0(obj, target_distance_d5, screen_shake_ff17c4, prng_d0);
            }
            break;

        case 2:
            obj.flags_36 ^= 0x02;
            obj.timer_2 -= 1;
            if (obj.timer_2 <= 0) {
                obj.loot_spawned = true;
                obj.active_0 = 0;
            }
            break;

        case 4:
            if ((obj.flags_37 & 0x02) != 0) {
                SelectNextAction017EF0(obj, target_distance_d5, screen_shake_ff17c4, prng_d0);
            }
            break;

        case 6:
            if ((obj.anim_frame_30 == 12 || obj.anim_frame_30 == 20) && obj.subtick_2e == 0) {
                obj.last_sfx = 0x800202;
            }
            if (hit_target != nullptr) {
                ApplyAttackHitCallback017FC0(*hit_target);
            }
            if ((obj.flags_37 & 0x02) != 0) {
                SelectNextAction017EF0(obj, target_distance_d5, screen_shake_ff17c4, prng_d0);
            }
            break;

        case 8:
            if ((obj.flags_37 & 0x02) != 0) {
                obj.flags_36 &= ~0x40;
                obj.flags_39 &= ~0x08;
                SelectNextAction017EF0(obj, target_distance_d5, screen_shake_ff17c4, prng_d0);
            } else {
                obj.flags_36 |= 0x40;
            }
            break;

        default:
            break;
    }
}

} // namespace Thor::World::DestructibleFlags
```

## 4. Behavioral Parity & Invariants
1. **Guaranteed Minimum Scratch Damage:** Subtracting defense `+$92` from 12-bit incoming damage uses an unsigned/positive floor (`bhi.b` else `moveq #1, d0`), guaranteeing that every valid hit deducts at least 1 HP regardless of defense rating.
2. **Death Blink Cadence Before Loot Drop:** Upon reaching 0 HP, the entity sets invulnerability `+$38:3`, transitions to State 2, and immediately toggles visibility `+$36:1` on the same tick while counting down 16 frames before invoking `0x00F476` with embedded table `0x017F6C`.
3. **Three-Bank Persistent Bitfield Architecture:** Both `PersistentFlag_SetBit` (`0x017FF2`, taking `a0`) and `PersistentFlag_ResolveBit` (`0x018030`, taking `a6`) share the exact bank mapping (`1 -> $FF0DBC`, `2 -> $FF0E1C`, `3 -> $FF0E2C`) and bit addressing (`byte = +$A4 >> 3`, `bit = +$A4 & 7`), providing the global persistence backbone for all room switches, gates, crushers, chests, and destructible hazards.
