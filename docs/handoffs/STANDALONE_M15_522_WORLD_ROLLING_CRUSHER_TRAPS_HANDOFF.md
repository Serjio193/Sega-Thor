# STANDALONE HANDOFF: Milestone M15.522 — ROM World Rolling/Sliding Crusher & Ceiling Crusher Traps

## 1. Subsystem Overview
- **ROM Address Range:** `0x0173D6..0x01778C` (950 bytes, 242 M68K instructions across 4 routines).
- **Core Responsibilities:**
  1. `0x0173D6..0x017528` (`AI_WorldObject_RollingSlidingBoulder_Crusher`, 338 bytes, 84 instructions):
     - Dispatches on mode `+$9E` (0, 1, 2, 3, 4, 5).
     - Mode 0 (Idle / Rest): increments ticker `+$2E` while deltas `+$72 | +$76 == 0`.
     - Mode 5 (Spawn Init): sets `+$37 = 0x40`, clears flying bit `+$38:7`, sets `+$38:2`, anim `+$2A = 2`, mode `+$9E = 1`, clears facing `+$16 = 0`, sets velocities `+$4E = +$A0`, `+$52 = +$8C`.
     - Mode 1 (Rolling / Sliding Motion): copies velocities to movement deltas `+$72/+$76`. Tests wall collision `(+$40 | +$41) & 0x40`: on wall collision, sets flying bit `+$38:2`, anim `+$2A = 4`, mode `+$9E = 2`, triggers screen shake `$FF17C4 = 4`, plays impact SFX `#$800346`. Else runs AABB collision test `0x00B922 [-16..+16, -16..+16, 0..32]` vs player: inflicts 20 HP damage (`$8014`), upward impulse `vz = $10000`, `z += 1`, transfers velocities `+$4E/+$52`, computes facing via `0x00E338`, plays SFX `#$800346` (`#$900237` for Ali), transitions to wall impact/shatter mode.
     - Mode 2 (Shatter / Despawn): when animation finished `+$37:1 != 0`, despawns entity (`+$0 = 0`).
  2. `0x017528..0x017634` (`AI_WorldObject_CeilingCrusher_Respawning`, 268 bytes, 70 instructions):
     - Mode `+$9E == 3`, sub-state `+$A6` (0..3).
     - Sub-state 0 (Raised Ceiling Wait): sets anim `+$2A = 6`, ceiling height `z = +$8C`, sets flag `+$36:3`. Polls persistent room bit via `0x018030`: when set, advances `+$A6 = 1` and sets drop timer `+$2 = +$A0`.
     - Sub-state 1 (Countdown to Drop & Player Snap): decrements `+$2`. When 0: advances `+$A6 = 2`, clears vz `+$56 = 0`, clears `+$36:3`, sets `+$36:2`, clears shadow scale `+$3D = 0`, snaps coordinates `x = $FF19F0`, `y = $FF19F4 + 1` to target player, plays drop SFX `#$800350`.
     - Sub-state 2 (Falling & Ground Impact): computes dynamic shadow scale `+$3D = (3 - min(z >> 6, 3)) << 3`, applies gravity `vz -= $C000`, integrates `z += vz`. On ground impact (`z < 0`): advances `+$A6 = 3`, clamps `z = 0`, sets flying bit `+$38:2`, anim `+$2A = 4`, triggers screen shake `$FF17C4 = 4`, plays impact SFX `#$800346`. Runs AABB collision test `[-16..+16, -16..+16, 0..32]` calling `Object_HeavyCrush_DamageCallback` (`0x017722`).
     - Sub-state 3 (Reset / Retract): on anim finished `+$37:1 != 0`, clears `+$A6 = 0`, clears `+$38:2`, and loops back to Sub-state 0 to raise back up.
  3. `0x017634..0x017722` (`AI_WorldObject_CeilingCrusher_OneShot`, 238 bytes, 64 instructions):
     - Mode `+$9E == 4`, sub-state `+$A6` (0..3).
     - Identical to respawning crusher with two critical distinctions:
       1) Sub-state 1 drop initiation drops at its fixed placement without snapping `x,y` to player coordinates.
       2) Sub-state 3 on impact animation finished despawns entity (`+$0 = 0`) permanently instead of resetting.
  4. `0x017722..0x01778C` (`Object_HeavyCrush_DamageCallback`, 106 bytes, 24 instructions):
     - Heavy crush damage collision callback routine. Verifies target vulnerability (checks invulnerability `+$38:3`, special immunity `+$3A:0`, and cooldown `+$82`).
     - Sets attacker reference `+$7E = a6`, deals massive 40 HP crush damage (`+$86 = $8028`), clears defense cooldowns/flags (`+$82 = 0`, `+$83 = 0`, `+$9D = 0`).
     - Applies upward launch `vz = $8000`, `z += 1`, zeroes target horizontal velocity (`+$4E = 0`, `+$52 = 0`), sets knockback facing `+$84` from target orientation `+$16`.
     - Plays SFX `#$900237` for Ali (`+$0 == 2`) or `#$800346` for other entities.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_world_rolling_crusher_traps_m15_522.json`
- Extractor: `tools/extract_rom_world_rolling_crusher_traps.py`
- Unit Tests: `tests/rom_world_rolling_crusher_traps_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <algorithm>
#include <tuple>

namespace Thor::World::RollingCrusherTraps {

struct TargetEntity {
    uint16_t type_0{2};           // 2 = Ali/player
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    int32_t vz_56{0};
    int32_t vx_4e{0};
    int32_t vy_52{0};
    uint16_t facing_16{0};
    uint16_t knockback_facing_84{0};
    uint8_t flags_38{0};          // bit 3: invulnerability
    uint8_t flags_3a{0};          // bit 0: immunity
    uint8_t invuln_82{0};         // hit cooldown
    uint8_t reaction_83{0};
    uint8_t flinch_9d{0};
    uintptr_t attacker_7e{0};
    uint16_t damage_86{0};
    uint32_t hit_sfx{0};
};

struct RollingBoulderEntity {
    uint16_t active_0{1};
    uint8_t mode_9e{5};           // 0=Idle, 1=Rolling, 2=Shatter, 5=Init
    uint16_t anim_2a{0};
    uint16_t counter_2e{0};
    uint8_t flags_37{0};          // bit 1: anim finished
    uint8_t flags_38{0};          // bit 2: flying
    uint16_t facing_16{0};
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    int32_t vx_4e{0};
    int32_t vy_52{0};
    int32_t delta_x_72{0};
    int32_t delta_y_76{0};
    uint8_t col_40{0};
    uint8_t col_41{0};
    uint16_t screen_shake{0};
    uint32_t sfx{0};
};

/// Emulates Heavy Crush Damage Callback (0x017722..0x01778C).
inline bool ApplyHeavyCrushDamage(uintptr_t attacker_addr, TargetEntity& target) noexcept {
    if ((target.flags_38 & 0x08) != 0) return false; // invulnerability
    if ((target.flags_3a & 0x01) != 0) return false; // immunity
    if (target.invuln_82 != 0) return false;         // cooldown

    target.attacker_7e = attacker_addr;
    target.damage_86 = 0x8028;                       // 40 HP damage
    target.invuln_82 = 0;
    target.reaction_83 = 0;
    target.flinch_9d = 0;
    target.vz_56 = 0x8000;
    target.z += 1;
    target.vx_4e = 0;
    target.vy_52 = 0;
    target.knockback_facing_84 = target.facing_16;
    target.hit_sfx = (target.type_0 == 2) ? 0x900237 : 0x800346;
    return true;
}

/// Emulates Rolling / Sliding Boulder or Crusher (0x0173D6..0x017528).
inline void StepRollingBoulder(
    RollingBoulderEntity& boulder,
    TargetEntity* target,
    int16_t spawn_vx_a0,
    int16_t spawn_vy_8c
) noexcept {
    if (boulder.mode_9e == 0) {
        boulder.counter_2e += 1;
        if ((boulder.delta_x_72 | boulder.delta_y_76) != 0) {
            boulder.counter_2e -= 1;
        }
    } else if (boulder.mode_9e == 5) {
        boulder.flags_37 = 0x40;
        boulder.flags_38 = (boulder.flags_38 & ~0x80) | 0x04;
        boulder.anim_2a = 2;
        boulder.mode_9e = 1;
        boulder.facing_16 = 0;
        boulder.vx_4e = spawn_vx_a0;
        boulder.vy_52 = spawn_vy_8c;
    } else if (boulder.mode_9e == 1) {
        boulder.delta_x_72 = boulder.vx_4e;
        boulder.delta_y_76 = boulder.vy_52;

        if (((boulder.col_40 | boulder.col_41) & 0x40) != 0) {
            boulder.flags_38 |= 0x04;
            boulder.anim_2a = 4;
            boulder.mode_9e = 2;
            boulder.screen_shake = 4;
            boulder.sfx = 0x800346;
            return;
        }

        if (target != nullptr) {
            int16_t dx = std::abs(target->x - boulder.x);
            int16_t dy = std::abs(target->y - boulder.y);
            int32_t dz = target->z - boulder.z;

            if (dx <= 16 && dy <= 16 && dz >= 0 && dz <= 32) {
                if ((target->flags_38 & 0x08) == 0 && target->invuln_82 == 0) {
                    target->attacker_7e = reinterpret_cast<uintptr_t>(&boulder);
                    target->damage_86 = 0x8014; // 20 HP
                    target->invuln_82 = 0;
                    target->reaction_83 = 0;
                    target->flinch_9d = 0;
                    target->vz_56 = 0x10000;
                    target->z += 1;
                    target->vx_4e = boulder.vx_4e;
                    target->vy_52 = boulder.vy_52;
                    target->knockback_facing_84 = 2;
                    target->hit_sfx = (target->type_0 == 2) ? 0x900237 : 0x800346;

                    boulder.flags_38 |= 0x04;
                    boulder.anim_2a = 4;
                    boulder.mode_9e = 2;
                    boulder.screen_shake = 4;
                    boulder.sfx = 0x800346;
                }
            }
        }
    } else if (boulder.mode_9e == 2) {
        if ((boulder.flags_37 & 0x02) != 0) {
            boulder.active_0 = 0;
        }
    }
}

struct CeilingCrusherEntity {
    uint16_t active_0{1};
    uint8_t mode_9e{3};           // 3 = Respawning, 4 = One-Shot
    uint8_t state_a6{0};          // 0..3
    uint16_t anim_2a{6};
    uint16_t timer_2{0};
    uint8_t flags_36{0};          // bit 3: raised, bit 2: falling
    uint8_t flags_37{0};          // bit 1: anim finished
    uint8_t flags_38{0};          // bit 2: flying / landing
    uint8_t shadow_scale_3d{0};
    int16_t x{0};
    int16_t y{0};
    int32_t z_32{0};              // 16.16 fixed point
    int32_t vz_56{0};             // 16.16 velocity
    int16_t ceiling_z_8c{192};
    int16_t timer_param_a0{30};
    uint16_t screen_shake{0};
    uint32_t sfx{0};
};

/// Emulates Ceiling Crusher (0x017528..0x017722).
inline void StepCeilingCrusher(
    CeilingCrusherEntity& crusher,
    bool room_persistent_bit,
    int16_t player_x,
    int16_t player_y,
    TargetEntity* target
) noexcept {
    bool is_one_shot = (crusher.mode_9e == 4);

    if (crusher.state_a6 == 0) {
        crusher.anim_2a = 6;
        crusher.z_32 = static_cast<int32_t>(crusher.ceiling_z_8c) << 16;
        crusher.flags_36 |= 0x08;

        if (room_persistent_bit) {
            crusher.state_a6 = 1;
            crusher.timer_2 = crusher.timer_param_a0;
        }
    } else if (crusher.state_a6 == 1) {
        if (crusher.timer_2 > 0) {
            crusher.timer_2 -= 1;
            if (crusher.timer_2 == 0) {
                crusher.state_a6 = 2;
                crusher.vz_56 = 0;
                crusher.flags_36 = (crusher.flags_36 & ~0x08) | 0x04;
                crusher.shadow_scale_3d = 0;
                if (!is_one_shot) {
                    crusher.x = player_x;
                    crusher.y = player_y + 1;
                }
                crusher.sfx = 0x800350;
            }
        }
    } else if (crusher.state_a6 == 2) {
        int32_t z_px = crusher.z_32 >> 16;
        int32_t d0 = std::min<int32_t>(z_px >> 6, 3);
        crusher.shadow_scale_3d = static_cast<uint8_t>((3 - d0) << 3);

        crusher.vz_56 -= 0xC000;
        crusher.z_32 += crusher.vz_56;

        if (crusher.z_32 < 0) {
            crusher.state_a6 = 3;
            crusher.z_32 = 0;
            crusher.flags_38 |= 0x04;
            crusher.anim_2a = 4;
            crusher.screen_shake = 4;
            crusher.sfx = 0x800346;
        }

        if (target != nullptr) {
            int16_t dx = std::abs(target->x - crusher.x);
            int16_t dy = std::abs(target->y - crusher.y);
            int32_t dz = target->z - (crusher.z_32 >> 16);
            if (dx <= 16 && dy <= 16 && dz >= 0 && dz <= 32) {
                ApplyHeavyCrushDamage(reinterpret_cast<uintptr_t>(&crusher), *target);
            }
        }
    } else if (crusher.state_a6 == 3) {
        if ((crusher.flags_37 & 0x02) != 0) {
            if (is_one_shot) {
                crusher.active_0 = 0;
            } else {
                crusher.state_a6 = 0;
                crusher.flags_38 &= ~0x04;
                crusher.anim_2a = 6;
                crusher.z_32 = static_cast<int32_t>(crusher.ceiling_z_8c) << 16;
                crusher.flags_36 |= 0x08;
            }
        }
    }
}

} // namespace Thor::World::RollingCrusherTraps
```

## 4. Behavioral Parity & Invariants
1. **Momentum Transfer vs Stopping Power:** When a rolling boulder strikes Ali, it imparts its full horizontal velocity `(vx, vy)` and an upward pop `vz = 0x10000`, knocking Ali forward while the boulder shatters into impact State 2. In contrast, ceiling crushers zero all horizontal velocity (`vx = 0, vy = 0`) and launch Ali upward with `vz = 0x8000`.
2. **Shadow Scaling Dynamics:** The ceiling crusher computes its ground shadow footprint via `+$3D = (3 - clamp(z >> 6, 3)) << 3`. When high above the ground (`z >= 192`), the shadow is minimal (scale 0); as it accelerates downward under gravity (`-$C000`), the shadow expands to 8, 16, and reaches maximum scale 24 (0x18) upon touchdown.
3. **Targeting Asymmetry:** Respawning crushers (`mode 3`) track Ali directly by snapping to `($FF19F0, $FF19F4 + 1)` upon drop release, creating an active dodging hazard that resets after each impact. One-shot crushers (`mode 4`) drop at fixed room coordinates and despawn permanently upon landing animation finish.
4. **Crush Damage Severity:** Ordinary traps inflict 10 or 20 HP, but the heavy crush callback deals 40 HP flat damage (`$8028`), resets invulnerability and reaction state registers (`$82, $83, $9D`), and triggers Ali's unique crushing impact voice SFX `#$900237`.
