# STANDALONE HANDOFF: Milestone M15.503 — Bow Spirit Spawn Initializer, Boss/Flying Enemy AI Master Tick, Hit-Damage/Death Handler, 14-Entry State Jump Table, State 0 PRNG Action Selector & State 4 Wander Stepper

## 1. Subsystem Overview
- **ROM Address Range:** `0x0147E0..0x014A72` (658 bytes, 150 code instructions + 14-word jump table).
- **Core Responsibilities:**
  1. `0x0147E0..0x014848` (`AI_BowSpirit_SpawnInit`, 104 bytes, 21 instructions):
     - Entity spawn configuration for Bow Spirit (`0x1E`): base init via `0x008D06`, VRAM tile `$04F8`, HP/SP `100/10` (`$64/$0A`), spawn coords, sprite init via `0x009D00`.
     - State `2`, animation `$14`, facing `0`, flying property (bit 2 of `+$38`), clears leash flag `$FF1C1C`, and dispatches summon SFX `$80037E` via `0x00E7FC`.
  2. `0x014848..0x014914` (`AI_Boss_MasterTick_DamageDeathHandler`, 204 bytes, 55 instructions):
     - Master tick loop invoking helpers `0x00E682` and `0x00E36E`.
     - Damage filter: skips if hitstun `+$A2` set or special flag `+$B4` set.
     - Damage ingestion from `+$86`: bit 13 (`0x2000`) zero-clamps damage (defended hit). Increments hit counter `+$B6`, triggers flinch anim `$10` and flinch state `$0E` (reaction bit 6 of `+$36`).
     - Subtracts 12-bit damage from HP `+$8A`, clamps at 0, plays hit SFX `$A00250`.
     - **Death Trigger (HP == 0):** Sets death animation `$18`, death state `$1A`, invulnerable bit 3 of `+$38`, dead flag `+$A2`, spawns explosion particle `$7F` at `(x, y + 0x20, z + 0x20)` via `0x00E90E`, asserts boss death lock `$FF1983`, and signals script/camera trigger `0x00DAE6`.
     - State jump table dispatcher at `0x014908`: indexes 14-entry table at `0x014914`.
  3. `0x014914..0x014930` (`14-Entry State Jump Table`, 28 bytes, 14 16-bit relative words):
     - Dispatches states `0x00..0x1A` (step 2) to targets:
       - `0x00`: `+30` -> `0x014932` (Action Selector)
       - `0x02`: `+26` -> `0x014930` (Dummy RTS)
       - `0x04`: `+324` -> `0x014A5C` (Wander Stepper)
       - `0x06`: `+418` -> `0x014ABC`
       - `0x08`: `+462` -> `0x014AEA`
       - `0x0A`: `+562` -> `0x014B50`
       - `0x0C`: `+1018` -> `0x014D1A`
       - `0x0E`: `+1476` -> `0x014EE6` (Damage flinch recovery)
       - `0x10`: `+334` -> `0x014A72`
       - `0x12`: `+1268` -> `0x014E1A`
       - `0x14`: `+1252` -> `0x014E0C`
       - `0x16`: `+594` -> `0x014B7C`
       - `0x18`: `+652` -> `0x014BB8`
       - `0x1A`: `+1586` -> `0x014F60` (Death state)
  4. `0x014932..0x014A5C` (`AI_Boss_State00_ActionSelector`, 298 bytes, 67 instructions):
     - Evaluates 16-bit PRNG word (`0x00E268`) or cutscene lock `$FF1987`:
       - `P <= 0x2000` (12.5%): Action 1 (anim `$02`, state `$06`, SFX `$800251`)
       - `0x2000 < P <= 0x6000` (25.0%): Action 2 (anim `$12`, state `$10`)
       - `0x6000 < P <= 0x9000` (18.75%): Action 3 (anim `$18`, state `$16`, speed `$40000`, timer 24, SFX `$800251`)
       - `P > 0x9000` or cutscene lock (43.75%): Wander setup:
         - Midpoint: `(0x100 + AliX) / 2`, `(0x140 + AliY) / 2` with `[-16..+15]` PRNG jitter.
         - Chebyshev distance `max(|dx|, |dy|)` determines speed:
           - `> 32 px`: `$00020000` (2.0 fixed)
           - `17..32 px`: `$00010000` (1.0 fixed)
           - `<= 16 px`: `$00008000` (0.5 fixed)
         - Sets timer = 10, transitions to State `$04`.
  5. `0x014A5C..0x014A72` (`AI_Boss_State04_WanderStep`, 22 bytes, 7 instructions):
     - Kinematic steering step via `0x0150A0` toward target `(+$A6, +$AA)`.
     - Decrements timer `+$2`; upon reaching 0, transitions immediately back to State `0x00` (`0x014932`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_boss_ai_master_state0_m15_503.json`
- Extractor: `tools/extract_rom_boss_ai_master_state0.py`
- Unit Tests: `tests/rom_boss_ai_master_state0_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
#include <optional>

namespace Thor::AI::Boss {

enum class BossState : uint8_t {
    State00_ActionSelect = 0x00,
    State02_DummyRts     = 0x02,
    State04_WanderStep   = 0x04,
    State06_Action1      = 0x06,
    State08_Action2      = 0x08,
    State0A_Action3      = 0x0A,
    State0C_Special1     = 0x0C,
    State0E_Flinch       = 0x0E,
    State10_Special2     = 0x10,
    State12_Special3     = 0x12,
    State14_Special4     = 0x14,
    State16_Charge       = 0x16,
    State18_Special5     = 0x18,
    State1A_Death        = 0x1A
};

constexpr std::array<uint32_t, 14> STATE_JUMP_TARGETS = {
    0x014932, 0x014930, 0x014A5C, 0x014ABC, 0x014AEA, 0x014B50,
    0x014D1A, 0x014EE6, 0x014A72, 0x014E1A, 0x014E0C, 0x014B7C,
    0x014BB8, 0x014F60
};

struct BossEntity {
    uint16_t type{0x1E};
    uint16_t vram_tile{0x04F8};
    int16_t max_hp{100};
    int16_t hp{100};
    int16_t max_sp{10};
    int16_t sp{10};
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
    uint16_t facing{0};
    BossState state{BossState::State02_DummyRts};
    uint16_t anim_id{0x14};
    uint8_t flags_36{0};
    uint8_t flags_38{0x04}; // bit 2: flying
    bool invuln_a2{false};
    uint16_t timer{0};
    uint16_t hit_counter{0};
    int16_t target_x{0};
    int16_t target_y{0};
    int32_t speed_b0{0};
};

struct ActionDecision {
    BossState next_state;
    uint16_t anim_id;
    int32_t speed{0};
    uint16_t timer{0};
    uint32_t sfx{0};
    int16_t wander_x{0};
    int16_t wander_y{0};
};

/// Initialize Bow Spirit entity parameters (0x0147E0..0x014846).
inline void InitBowSpirit(BossEntity& ent, int16_t spawn_x, int16_t spawn_y, uint16_t facing) noexcept {
    ent.type = 0x1E;
    ent.vram_tile = 0x04F8;
    ent.max_hp = 100;
    ent.hp = 100;
    ent.max_sp = 10;
    ent.sp = 10;
    ent.x = spawn_x;
    ent.y = spawn_y;
    ent.z = 0;
    ent.facing = facing;
    ent.state = BossState::State02_DummyRts;
    ent.anim_id = 0x14;
    ent.flags_38 = 0x04;
}

/// Ingest damage, apply flinch or trigger death sequence (0x014848..0x014908).
inline bool ProcessBossDamage(BossEntity& ent, uint16_t raw_damage, bool cutscene_b4) noexcept {
    if (ent.invuln_a2 || cutscene_b4 || raw_damage == 0) {
        return false;
    }

    uint16_t effective_dmg = ((raw_damage & 0x2000) != 0) ? 0 : (raw_damage & 0x0FFF);

    ++ent.hit_counter;
    ent.anim_id = 0x10; // flinch anim
    ent.state = BossState::State0E_Flinch;
    ent.flags_36 |= 0x40;

    ent.hp = static_cast<int16_t>(std::max(0, ent.hp - effective_dmg));

    if (ent.hp <= 0) {
        ent.anim_id = 0x18; // death anim
        ent.state = BossState::State1A_Death;
        ent.flags_38 |= 0x08; // bit 3: invulnerable
        ent.invuln_a2 = true;
        ent.timer = 0;
        return true; // Death triggered
    }
    return false; // Survived
}

/// Select action for State 0x00 based on PRNG word or cutscene lock (0x014932..0x014A5A).
inline ActionDecision SelectState0Action(
    uint16_t prng_word,
    bool cutscene_lock,
    int16_t boss_x,
    int16_t boss_y,
    int16_t ali_x,
    int16_t ali_y,
    int8_t jitter_x = 0,
    int8_t jitter_y = 0
) noexcept {
    if (!cutscene_lock) {
        if (prng_word <= 0x2000) {
            return {.next_state = BossState::State06_Action1, .anim_id = 0x02, .sfx = 0x800251};
        } else if (prng_word <= 0x6000) {
            return {.next_state = BossState::State10_Special2, .anim_id = 0x12};
        } else if (prng_word <= 0x9000) {
            return {
                .next_state = BossState::State16_Charge,
                .anim_id = 0x18,
                .speed = 0x40000,
                .timer = 24,
                .sfx = 0x800251
            };
        }
    }

    // Default: Midpoint wander
    constexpr int16_t ORIGIN_X = 0x100;
    constexpr int16_t ORIGIN_Y = 0x140;
    int16_t target_x = ORIGIN_X + ((ali_x - ORIGIN_X) / 2) + jitter_x;
    int16_t target_y = ORIGIN_Y + ((ali_y - ORIGIN_Y) / 2) + jitter_y;

    int16_t cdx = std::abs(target_x - boss_x);
    int16_t cdy = std::abs(target_y - boss_y);
    int16_t chebyshev = std::max(cdx, cdy);

    int32_t speed = 0x00020000;
    if (chebyshev <= 0x10) {
        speed = 0x00008000;
    } else if (chebyshev <= 0x20) {
        speed = 0x00010000;
    }

    return {
        .next_state = BossState::State04_WanderStep,
        .anim_id = 0x14,
        .speed = speed,
        .timer = 10,
        .wander_x = target_x,
        .wander_y = target_y
    };
}

/// Step State 0x04 wander timer and check for expiration (0x014A5C..0x014A70).
inline bool StepState4Wander(BossEntity& ent) noexcept {
    if (ent.timer > 0) {
        --ent.timer;
    }
    if (ent.timer == 0) {
        ent.state = BossState::State00_ActionSelect;
        return true; // Reached zero, transition back to State 0
    }
    return false;
}

} // namespace Thor::AI::Boss
```
