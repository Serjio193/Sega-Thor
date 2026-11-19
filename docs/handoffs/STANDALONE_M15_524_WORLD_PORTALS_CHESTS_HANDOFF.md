# STANDALONE HANDOFF: Milestone M15.524 — ROM World Interactive Objects: Room Doors, Vortex Portals, Spirit Locks & Treasure Chests (`0x017A96..0x017E28`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x017A96..0x017E28` (914 bytes total: 225 M68K instructions across 3 routines).
- **Core Responsibilities:**
  1. `0x017A96..0x017CA6` (`AI_World_RoomDoorPortalWarpTrigger`, 528 bytes, 127 insns):
     - Handles room transition doorways (`type != 0x7C`) and vortex warp portals (`type == 0x7C`).
     - **Standard Doorways (`type != 0x7C`):**
       - Tests AABB contact vs Ali via `0x00B922`: box `[-3..+3, -1..+1, 0..1000]`.
       - Validates target entity type `+$0 == 2`, cutscene lock `$FF1985 == 0`, and player state `!= 0x5A`.
       - **Cross-Room Transition (`+$4E != $FF0E40`):**
         - Special Room `0x90B` Check: if `$FF0E40 == 0x90B` and bit 0 of `$FF0DC6 == 0`, calls `0x00C2EC(d5=0, d6=0x5000, d7=0x5800, d0=0x18)` and sets `$FF0DC6 |= 1`.
         - Plays transition sound SFX `#$800230` via sound driver `$60004`.
         - Sets transition flags `$FF17BB = 0xFF` and `$FF17BF = 0xFF`, and jumps to `0x00CA24`.
       - **Same-Room Teleporter (`+$4E == $FF0E40`):**
         - Teleports target coordinates to `(+$52, +$54)` via `0x009D00` and plays SFX `#$800230` via `$60004`.
     - **Vortex Portal (`type == 0x7C`):**
       - Evaluates control byte `+$4C` (`& 0x7F == 1` clears `+$4C`, sets anim 2; `& 0x7F == 2` sets `+$4C = 0x80`).
       - Disabled if animation `+$2A == 0`, cutscene locks `$FF1985` or `$FF1987 != 0`, or Ali is in an excluded action state (`0x0E, 0x28, 0x2A, 0x2C, 0x2E, 0x6C, 0x5A, 0x32, 0x34, 0x3E, 0x58`).
       - Computes homing pull vector `(d1, d2)` and distance `d4` via `0x00E45E`:
         - `d4 > 56`: outside suction radius.
         - `d4 in [41..56]`: quarter suction velocity (`>> 2`).
         - `d4 in [25..40]`: half suction velocity (`>> 1`).
         - `d4 in [2..24]`: full suction velocity.
         - `d4 <= 1` (Center Absorption): if Ali is grounded (`z == 0`) and not already in state `0x6E`, plays warp SFX `#$800230` via `$60004`, stores destination room/coords (`Ali.+$AE = +$4E`, `Ali.+$B0 = +$52`, `Ali.+$B2 = +$54`), sets Ali animation `0x78`, enters warp state `0x6E`, and clears facing `+$16 = 0`.
  2. `0x017CA6..0x017D12` (`AI_World_SpiritLockPersistentDespawner`, 108 bytes, 28 insns):
     - **Spirit / Key Lock Trigger (`0x017CA6`):**
       - If bit 7 of `+$4C` is set, toggles visibility `+$36:1` based on cutscene lock `$FF1985`.
       - When spirit active flag `$FF0DBA:1` is set, tests AABB collision via `0x00B922` `[-10..+10, -6..+6, 0..4]`.
       - On contact with Spirit Sparkle (`type == 0x16`): clears visibility bit 1 in `+$36`, sets animation `+$2A = 2`, and plays unlock SFX `#$80022F` via `0x00E7FC`.
     - **Persistent Despawner (`0x017D06`):**
       - Clears entity type `+$0 = 0` and calls persistent state bit setter `0x017FF2(a0 = a6)`.
  3. `0x017D12..0x017E28` (`AI_World_TreasureChest`, 278 bytes, 70 insns):
     - **Sub-mode 0 (Initialization):** Sets `+$66 = 0xFF`, loads height `z = +$A0 & 0x7F`. If bit 7 of `+$A0` is set (auto-open flag): enters state `+$4 = 0x0A`, advances animation `+$2A += 2`, sets timer `+$2 = 1`.
     - **Ballistic Gravity & Bounce:** While `z != 0`, applies gravity `vz -= 0xC000`, integrates `z += vz`. On ground impact (`z < 0`): clamps `z = 0`. First bounce (`+$A6 == 0`): sets `+$A6 = 0xFF`, `z = 1`, bounce velocity `vz = +0x00030000`, and plays chest bounce SFX `#$400376` via `0x00E7FC`. Second landing: clears `vz = 0`.
     - **Hit-to-Open (`+$4 == 0`):** When incoming damage `+$86 != 0`, sets state `+$4 = 1`, countdown timer `+$2 = 2`, advances animation `+$2A += 2`, and plays chest open SFX `#$900232` via `0x00E7FC`.
     - **Loot Spawning & Despawn:** When countdown timer `+$2` reaches 0 (or immediately in auto-open state `0x0A`):
       - If persistent mode `+$9E != 0`: calls persistent bit setter `0x017FF2`.
       - If loot item ID `+$8C != 0`: spawns item entity via `0x00F53C`. If `+$9E == 0`, copies parameters `+$A2 -> +$4E` and `+$A4 -> +$50` onto the spawned item.
       - Despawns chest (`+$0 = 0`).

## 2. ROM Evidence Reference
- **Evidence JSON:** `data/thor-evidence/rom_world_portals_chests_m15_524.json`
- **Extractor Tool:** `tools/extract_rom_world_portals_chests.py`
- **Test Suite:** `tests/rom_world_portals_chests_test.py` (14 tests, 14/14 PASS)

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace Thor::WorldObjects::PortalsAndChests {

// --- SFX & Entity Identifiers ---
inline constexpr uint32_t kSfxDoorPortalWarp     = 0x00800230u; // Room transition / portal warp SFX
inline constexpr uint32_t kSfxSpiritLockReveal   = 0x0080022Fu; // Spirit lock sparkle trigger SFX
inline constexpr uint32_t kSfxChestBounce        = 0x00400376u; // Treasure chest ground bounce SFX
inline constexpr uint32_t kSfxChestOpen          = 0x00900232u; // Treasure chest open SFX

inline constexpr uint16_t kTypeVortexPortal      = 0x007Cu;
inline constexpr uint16_t kTypeSpiritSparkle     = 0x0016u;
inline constexpr uint16_t kSpecialRoomId90B      = 0x090Bu;

inline constexpr int32_t  kChestGravityStep      = 0x0000C000;
inline constexpr int32_t  kChestBounceVelocity   = 0x00030000;

// --- Portal Excluded Player States ---
inline constexpr std::array<uint16_t, 11> kPortalExcludedAliStates{{
    0x0E, 0x28, 0x2A, 0x2C, 0x2E, 0x6C, 0x5A, 0x32, 0x34, 0x3E, 0x58
}};

inline bool IsAliExcludedFromPortal(uint16_t state) {
    return std::find(kPortalExcludedAliStates.begin(), kPortalExcludedAliStates.end(), state)
           != kPortalExcludedAliStates.end();
}

// --- Ali Portal / Door Context ---
struct AliPortalContext {
    uint16_t entity_type_0{2};
    uint16_t state_4{0};
    uint16_t anim_2a{0};
    uint16_t facing_16{0};
    uint16_t target_room_ae{0};
    int16_t  target_x_b0{0};
    int16_t  target_y_b2{0};
    int32_t  x{0};
    int32_t  y{0};
    int32_t  z{0};
    int32_t  delta_x_72{0};
    int32_t  delta_y_76{0};
};

// --- Room Door & Vortex Portal ---
struct RoomDoorPortal {
    uint16_t entity_type_0{0x40};
    uint8_t  flag_4c{0};
    uint16_t anim_2a{1};
    uint16_t target_room_4e{0};
    int16_t  teleport_x_52{0};
    int16_t  teleport_y_54{0};
    int32_t  x{0};
    int32_t  y{0};
};

inline void StepRoomDoorPortal(
    RoomDoorPortal& portal,
    AliPortalContext& ali,
    bool in_aabb_contact,
    uint16_t current_room,
    bool cutscene_locked,
    bool dialogue_locked,
    uint8_t& room_flag_dc6,
    auto&& cross_room_fn,
    auto&& same_room_teleport_fn,
    auto&& special_room_90b_fn,
    auto&& play_sfx_fn)
{
    if (portal.entity_type_0 != kTypeVortexPortal) {
        if (!in_aabb_contact) return;
        if (ali.entity_type_0 != 2 || cutscene_locked || ali.state_4 == 0x5A) return;

        if (portal.target_room_4e != current_room) {
            if (current_room == kSpecialRoomId90B && (room_flag_dc6 & 1) == 0) {
                special_room_90b_fn();
                room_flag_dc6 |= 1;
            }
            play_sfx_fn(kSfxDoorPortalWarp);
            cross_room_fn(portal.target_room_4e);
        } else {
            ali.x = portal.teleport_x_52;
            ali.y = portal.teleport_y_54;
            same_room_teleport_fn();
            play_sfx_fn(kSfxDoorPortalWarp);
        }
        return;
    }

    // Vortex Portal (type == 0x7C)
    const uint8_t sub_mode = portal.flag_4c & 0x7F;
    if (sub_mode == 1) {
        portal.flag_4c = 0;
        portal.anim_2a = 2;
    } else if (sub_mode == 2) {
        portal.flag_4c = 0x80;
    }

    if (portal.anim_2a == 0 || cutscene_locked || dialogue_locked) return;
    if (IsAliExcludedFromPortal(ali.state_4)) return;

    const int32_t dx = portal.x - ali.x;
    const int32_t dy = portal.y - ali.y;
    const int32_t dist = std::max(std::abs(dx), std::abs(dy));

    if (dist <= 1) {
        if (ali.state_4 == 0x6E || ali.z != 0) return;
        play_sfx_fn(kSfxDoorPortalWarp);
        ali.target_room_ae = portal.target_room_4e;
        ali.target_x_b0 = portal.teleport_x_52;
        ali.target_y_b2 = portal.teleport_y_54;
        ali.anim_2a = 0x78;
        ali.state_4 = 0x6E;
        ali.facing_16 = 0;
    } else if (dist <= 56) {
        int32_t pull_x = dx;
        int32_t pull_y = dy;
        if (dist > 24) { pull_x >>= 1; pull_y >>= 1; }
        if (dist > 40) { pull_x >>= 1; pull_y >>= 1; }
        ali.delta_x_72 += pull_x;
        ali.delta_y_76 += pull_y;
    }
}

// --- Spirit Lock Trigger ---
struct SpiritLockTrigger {
    uint16_t entity_type_0{0x35};
    uint8_t  flag_4c{0};
    uint8_t  flag_36{0};        // Bit 1 = Hidden / Masked
    uint16_t anim_2a{0};
};

inline void StepSpiritLockTrigger(
    SpiritLockTrigger& lock,
    bool cutscene_locked,
    bool spirit_sparkle_active,
    bool in_sparkle_aabb,
    uint16_t contact_entity_type,
    auto&& play_sfx_fn)
{
    if ((lock.flag_4c & 0x80) != 0) {
        if (cutscene_locked) {
            lock.flag_36 &= ~(1 << 1);
        } else {
            lock.flag_36 |= (1 << 1);
        }
    }

    if (spirit_sparkle_active && in_sparkle_aabb && contact_entity_type == kTypeSpiritSparkle) {
        lock.flag_36 &= ~(1 << 1);
        lock.anim_2a = 2;
        play_sfx_fn(kSfxSpiritLockReveal);
    }
}

// --- Treasure Chest ---
struct TreasureChest {
    uint16_t entity_type_0{0x1A};
    uint8_t  submode_66{0};
    uint8_t  bounced_a6{0};
    uint8_t  param_9e_persistent{0};
    uint8_t  param_8c_loot_id{0};
    uint8_t  param_a2{0};
    uint16_t param_a4{0};
    uint16_t param_a0_height_flags{0};
    uint16_t state_4{0};
    uint16_t timer_2{0};
    uint16_t anim_2a{0};
    uint16_t damage_86{0};
    int32_t  z{0};
    int32_t  vz{0};
};

inline void StepTreasureChest(
    TreasureChest& chest,
    auto&& set_persistent_fn,
    auto&& spawn_loot_fn,
    auto&& play_sfx_fn)
{
    if (chest.submode_66 == 0) {
        chest.submode_66 = 0xFF;
        const bool auto_open = (chest.param_a0_height_flags & 0x80) != 0;
        chest.z = chest.param_a0_height_flags & 0x7F;
        if (auto_open) {
            chest.state_4 = 0x0A;
            chest.anim_2a += 2;
            chest.timer_2 = 1;
        }
        return;
    }

    if (chest.z != 0) {
        chest.vz -= kChestGravityStep;
        chest.z += chest.vz;
        if (chest.z < 0) {
            chest.z = 0;
            if (chest.bounced_a6 == 0) {
                chest.bounced_a6 = 0xFF;
                chest.z = 1;
                chest.vz = kChestBounceVelocity;
                play_sfx_fn(kSfxChestBounce);
            } else {
                chest.vz = 0;
            }
        }
    }

    auto dispense_loot_and_despawn = [&]() {
        if (chest.param_9e_persistent != 0) {
            set_persistent_fn();
        }
        if (chest.param_8c_loot_id != 0) {
            spawn_loot_fn(
                chest.param_8c_loot_id,
                chest.param_9e_persistent == 0,
                chest.param_a2,
                chest.param_a4);
        }
        chest.entity_type_0 = 0;
    };

    if (chest.state_4 == 0x0A) {
        dispense_loot_and_despawn();
        return;
    }

    if (chest.state_4 == 0) {
        if (chest.damage_86 != 0) {
            chest.state_4 = 1;
            chest.timer_2 = 2;
            chest.anim_2a += 2;
            play_sfx_fn(kSfxChestOpen);
        }
        return;
    }

    if (chest.timer_2 > 0) {
        chest.timer_2--;
        if (chest.timer_2 == 0) {
            dispense_loot_and_despawn();
        }
    }
}

} // namespace Thor::WorldObjects::PortalsAndChests
```
