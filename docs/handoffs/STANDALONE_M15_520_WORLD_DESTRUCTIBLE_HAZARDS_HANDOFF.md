# STANDALONE HANDOFF: Milestone M15.520 — ROM Interactive World Objects: Timed Switch, Destructible Block, Periodic Pop-Up Hazard & Periodic Toggle/Vent Hazard

## 1. Subsystem Overview
- **ROM Address Range:** `0x016EBC..0x0171C4` (776 bytes, 203 M68K instructions across 4 routines).
- **Core Responsibilities:**
  1. `0x016EBC..0x016F10` (`AI_WorldObject_TimedSwitch`, 84 bytes, 23 instructions):
     - One-shot initialization via `+$A6`: sets `z = +$8C`, height `+$4A = 16` (if `+$9E == 0`) or sets layer priority `+$3C:0` (if `+$9E == 1`).
     - Active trigger: when state `+$66 != 0`, increments timer `+$2`.
     - Upon reaching 6 frames: despawns entity (`+$0 = 0`) and commits room persistent bit via `0x018030`.
  2. `0x016F10..0x016F7C` (`AI_WorldObject_Destructible`, 108 bytes, 30 instructions):
     - One-shot initialization via `+$A6`: sets `z = +$8C`, max and current HP from parameter `+$A0`.
     - Hit processing: manages 6-frame invulnerability cooldown `+$82`. On weapon damage (`+$86 != 0`): clears damage and flinch, sets cooldown `+$82 = 6`, subtracts 12-bit masked damage from HP `+$8A`.
     - On zero HP (`+$8A <= 0`): commits persistent bit via `0x018030`, despawns (`+$0 = 0`), and if mode `+$9E != 0`, adds 24 to `z` and invokes shatter particle effect `0x00E96C(mode 0)`.
  3. `0x016F7C..0x0170E0` (`AI_WorldObject_PeriodicPopUpHazard`, 356 bytes, 90 instructions):
     - Sub-state `+$66` (0..2):
       - `0`: sets `+$66 = 1`, timer `+$2 = +$A0` $\to$ enters State 1.
       - `1` (Hidden Wait): decrements timer `+$2`. On expiration, checks persistent bit via `0x018030`; if clear, pops up $\to$ State 2 (timer `+$2 = +$9E`, anim 2, layer `+$3C:0` set, flying `+$38:2` cleared, height `+$4A = 48`, plays SFX `0x6004DF` and `0x500260`).
       - `2` (Active Hazard): checks persistent bit (retracts immediately if set). Decrements timer; on expiration, retracts $\to$ State 1 (timer `+$2 = +$A0`, anim 0, layer `+$3C:0` clear, flying `+$38:2` set, height `+$4A = 1`). In active phase, tests AABB vs Ali `[-8..+8, -2..+2]` with `z < 47`: inflicts 10 HP damage, directional knockback `(0x400, 0x3C0)`, upward impulse `vz = 0x20000`, reaction mode `+$83 = 8`, and plays SFX `0x80035B`. If Ali is airborne (`z != 0`) or already flinching, adds heavy bit `+$86 |= 0x8000`.
  4. `0x0170E0..0x0171C4` (`AI_WorldObject_PeriodicToggleVentHazard`, 228 bytes, 60 instructions):
     - Sub-state `+$66` (0..2):
       - `0`: sets `+$66 = 1`, timer `+$2 = +$A0`, `z = +$8C` $\to$ enters State 1.
       - `1` (Inactive Wait): calls anim updater `0x00BC8E`, clears anim `+$2A` on finish, decrements timer `+$2`. On expiration, checks persistent bit; if clear, opens vent $\to$ State 2 (timer `+$2 = +$9E`, anim 2, layer `+$3C:1` set, hazard depth `+$49 = 56`, plays SFX `0x80034F`).
       - `2` (Active Venting): checks persistent bit (closes immediately if set). Calls anim updater `0x00BC8E`, loops anim 4 on finish, decrements timer `+$2`. On expiration, closes vent $\to$ State 1 (timer `+$2 = +$A0`, closing anim 6, clears layer `+$3C:1` and hazard depth `+$49`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_world_destructible_hazards_m15_520.json`
- Extractor: `tools/extract_rom_world_destructible_hazards.py`
- Unit Tests: `tests/rom_world_destructible_hazards_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <optional>
#include <tuple>

namespace Thor::World::DestructibleHazards {

struct TimedSwitchEntity {
    uint16_t active_0{1};
    bool init_done_a6{false};
    uint8_t state_66{0};
    uint8_t mode_9e{0};
    int16_t z_8c{0};
    int32_t z{0};
    uint16_t height_4a{0};
    uint8_t flags_3c{0};
    int16_t timer_2{0};
};

/// Emulates Timed World Switch Helper (0x016EBC..0x016F10).
inline bool StepTimedSwitch(
    TimedSwitchEntity& obj,
    uint8_t& persistent_byte,
    uint8_t bit_idx
) noexcept {
    if (!obj.init_done_a6) {
        obj.init_done_a6 = true;
        obj.z = obj.z_8c;
        if (obj.mode_9e == 0) {
            obj.height_4a = 16;
        } else if (obj.mode_9e == 1) {
            obj.flags_3c |= 0x01;
        }
        return true;
    }

    if (obj.state_66 == 0) return true;

    obj.timer_2 += 1;
    if (obj.timer_2 >= 6) {
        obj.active_0 = 0;
        persistent_byte |= (1 << bit_idx);
        return false; // despawned
    }
    return true;
}

struct DestructibleBlockEntity {
    uint16_t active_0{1};
    bool init_done_a6{false};
    uint8_t mode_9e{0};
    int16_t z_8c{0};
    int32_t z{0};
    uint16_t max_hp_a0{1};
    int16_t hp_8a{1};
    uint16_t max_hp_88{1};
    uint8_t guard_82{0};
    uint16_t damage_86{0};
};

/// Emulates Destructible Block (0x016F10..0x016F7C).
/// Returns pair: (is_alive, should_trigger_shatter_effect).
inline std::pair<bool, bool> StepDestructibleBlock(
    DestructibleBlockEntity& obj,
    uint8_t& persistent_byte,
    uint8_t bit_idx
) noexcept {
    if (!obj.init_done_a6) {
        obj.init_done_a6 = true;
        obj.z = obj.z_8c;
        obj.hp_8a = static_cast<int16_t>(obj.max_hp_a0);
        obj.max_hp_88 = obj.max_hp_a0;
        return {true, false};
    }

    if (obj.guard_82 != 0) {
        obj.guard_82 -= 1;
        return {true, false};
    }

    if (obj.damage_86 == 0) return {true, false};

    uint16_t dmg = obj.damage_86 & 0x0FFF;
    obj.damage_86 = 0;
    obj.guard_82 = 6;
    obj.hp_8a -= static_cast<int16_t>(dmg);

    if (obj.hp_8a > 0) return {true, false};

    persistent_byte |= (1 << bit_idx);
    obj.active_0 = 0;
    bool shatter = false;
    if (obj.mode_9e != 0) {
        obj.z += 24;
        shatter = true;
    }
    return {false, shatter};
}

struct PlayerTarget {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    uint8_t flags_38{0};
    bool guard_82{false};
    uint8_t reaction_83{0};
    uint16_t damage_86{0};
    int32_t knockback_x{0};
    int32_t knockback_y{0};
    int32_t vz{0};
    const void* attacker{nullptr};
};

struct PopUpHazardEntity {
    uint8_t state_66{0};
    int16_t x{0};
    int16_t y{0};
    int16_t timer_a0{30};
    int16_t timer_9e{20};
    int16_t timer_2{0};
    uint16_t anim_id{0};
    uint8_t flags_3c{0};
    uint8_t flags_38{0};
    uint16_t height_4a{1};
};

/// Emulates Periodic Pop-Up Spike/Hazard Trap (0x016F7C..0x0170E0).
inline std::tuple<bool, bool, bool> StepPopUpHazard(
    PopUpHazardEntity& obj,
    uint8_t persistent_byte,
    uint8_t bit_idx,
    PlayerTarget* ali
) noexcept {
    bool pop_sfx = false;
    bool hit_sfx = false;

    if (obj.state_66 == 0) {
        obj.state_66 = 1;
        obj.timer_2 = obj.timer_a0;
        return {false, false, false};
    } else if (obj.state_66 == 1) {
        if (obj.timer_2 != 0) {
            obj.timer_2 -= 1;
            if (obj.timer_2 != 0) return {false, false, false};
        }
        if ((persistent_byte & (1 << bit_idx)) != 0) return {false, false, false};

        obj.state_66 = 2;
        obj.timer_2 = obj.timer_9e;
        obj.anim_id = 2;
        obj.flags_3c |= 0x01;
        obj.flags_38 &= ~0x04;
        obj.height_4a = 48;
        pop_sfx = true;
    } else if (obj.state_66 == 2) {
        bool suppressed = ((persistent_byte & (1 << bit_idx)) != 0);
        if (suppressed || obj.timer_2 <= 1) {
            obj.state_66 = 1;
            obj.timer_2 = obj.timer_a0;
            obj.anim_id = 0;
            obj.flags_3c &= ~0x01;
            obj.flags_38 |= 0x04;
            obj.height_4a = 1;
            return {false, false, false};
        }
        obj.timer_2 -= 1;

        if (ali != nullptr) {
            int16_t dx = ali->x - obj.x;
            int16_t dy = ali->y - obj.y;
            if (dx >= -8 && dx <= 8 && dy >= -2 && dy <= 2 && ali->z < 47) {
                bool is_immune = ((ali->flags_38 & 0x08) != 0) || ali->guard_82;
                if (!is_immune && ali->reaction_83 < 2) {
                    ali->attacker = &obj;
                    ali->guard_82 = false;
                    ali->damage_86 = 10;
                    ali->knockback_x = 0x400;
                    ali->knockback_y = 0x3C0;
                    ali->vz = 0x20000;
                    if (ali->reaction_83 != 0 || ali->z != 0) {
                        ali->damage_86 |= 0x8000;
                    }
                    ali->reaction_83 = 8;
                    hit_sfx = true;
                }
            }
        }
    }
    return {obj.state_66 == 2, pop_sfx, hit_sfx};
}

struct ToggleVentEntity {
    uint8_t state_66{0};
    int16_t z_8c{0};
    int32_t z{0};
    int16_t timer_a0{40};
    int16_t timer_9e{30};
    int16_t timer_2{0};
    uint16_t anim_id{0};
    uint8_t flags_3c{0};
    uint8_t depth_49{0};
    uint8_t flags_37{0};
};

/// Emulates Periodic Toggle Steam/Flame Vent Hazard (0x0170E0..0x0171C4).
inline std::pair<bool, bool> StepToggleVentHazard(
    ToggleVentEntity& obj,
    uint8_t persistent_byte,
    uint8_t bit_idx
) noexcept {
    bool vent_sfx = false;

    if (obj.state_66 == 0) {
        obj.state_66 = 1;
        obj.timer_2 = obj.timer_a0;
        obj.z = obj.z_8c;
        return {false, false};
    } else if (obj.state_66 == 1) {
        if (obj.anim_id != 0 && (obj.flags_37 & 0x02) != 0) obj.anim_id = 0;
        if (obj.timer_2 != 0) {
            obj.timer_2 -= 1;
            if (obj.timer_2 != 0) return {false, false};
        }
        if ((persistent_byte & (1 << bit_idx)) != 0) return {false, false};

        obj.state_66 = 2;
        obj.timer_2 = obj.timer_9e;
        obj.anim_id = 2;
        obj.flags_3c |= 0x02;
        obj.depth_49 = 56;
        vent_sfx = true;
    } else if (obj.state_66 == 2) {
        bool suppressed = ((persistent_byte & (1 << bit_idx)) != 0);
        if (!suppressed && obj.anim_id != 4 && (obj.flags_37 & 0x02) != 0) {
            obj.anim_id = 4;
        }
        if (suppressed || obj.timer_2 <= 1) {
            obj.state_66 = 1;
            obj.timer_2 = obj.timer_a0;
            obj.anim_id = 6;
            obj.flags_3c &= ~0x02;
            obj.depth_49 = 0;
            return {false, false};
        }
        obj.timer_2 -= 1;
    }
    return {obj.state_66 == 2, vent_sfx};
}

} // namespace Thor::World::DestructibleHazards
```

## 4. Behavioral Parity & Invariants
1. **One-Shot Initialization Pattern:** Both timed switches and destructible blocks use `+$A6` as an execution gate to prevent re-initializing base height `+$8C` or max HP on room reentry.
2. **Hit Invulnerability Cadence:** Destructible entities strictly enforce a 6-frame invulnerability timer upon taking damage, ensuring multi-hit weapons (e.g. fire sword whirls) deal deterministic step damage rather than instantaneous destruction.
3. **Z-Elevation Hazard Clearance:** Pop-up hazards only hit when Ali is within `[-8..+8, -2..+2]` and below 47 px elevation (`z < 47`), enabling players to leap safely over extended spikes.
4. **Persistent Room Synchronization:** Persistent bits queried via `0x018030` actively suppress both pop-up traps and vent hazards, ensuring defeated puzzles remain deactivated.
