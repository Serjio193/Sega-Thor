# STANDALONE HANDOFF: Milestone M15.517 — ROM Serpent Boss Phase 1 Body Contact AABB, Homing Orb Spawner, PRNG Action Selector & 4-Limb Animation Broadcaster (`0x016764..0x016A64`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x016764..0x016A64` (768 bytes total: 173 M68K instructions across 4 routines).
- **Core Responsibilities:**
  1. `0x016764..0x0167BE` (`AI_SerpentBoss_Phase1BodyContactAABB`, 90 bytes, 22 insns):
     - Evaluates body contact AABB vs Ali via `0x00B852` with bounding box `[-64..+64, -half_extent..+half_extent]` where `half_extent = boss.+$44`.
     - Guard check: if Ali is invulnerable (`ali.+$38 & (1 << 3)`), ignores hit.
     - Vulnerable Ali knockback: sets `ali.+$7E = a6`, inflicts damage `ali.+$86 = 0x8014` (20 HP, bit 15 set), hitstun `ali.+$83 = 5`, clears `ali.+$9D = 0`, modes `ali.+$16 = 2` and `ali.+$84 = 2`, knockback `ali.+$4E = 0`, `ali.+$52 = +0x00040000` (+4.0 px), and `ali.+$56 = -1` (`0xFFFF`).
  2. `0x0167BE..0x01685A` (`AI_SerpentBoss_Phase1HomingOrbSpawner`, 156 bytes, 31 insns):
     - Homing Orb Spawner. Allocates Pool 2 slot via `0x00DA2A` + `0x00E66E`.
     - Initializes orb: State `$12`, palette attribute `$0200`, `+$3D = 8`, sub-mode `+$66 = 2`, clears flags `+$36` and `+$37`, sets flying bit `+$38 |= 4`, mode `+$16 = 1`, bounds `8x8x16` (half-extent 8), coordinates `(caller_x - 1, caller_y + 16)`, angle `+$7A = d1`, speed `+$56 = d2`, anim `$16`, clears `+$86 = 0`, lifetime jitter `+$A1 += (prng & 7)`, initial velocity `vx = 0, vy = +0x00020000` (+2.0 px), and plays SFX `#$70035C`.
  3. `0x01685A..0x016A46` (`AI_SerpentBoss_Phase1ActionSelector`, 492 bytes, 110 insns):
     - Phase 1 Action Selector.
     - Broadcasts anim `$28` across 4 child limbs via `0x016A46`.
     - Event lock check: if dialogue/cutscene lock `$FF1987 != 0`, forces State 0 idle, anim 0, timer 15 frames.
     - Distance and orientation check to Ali via `0x00E398` (`d4` = distance, `d3` = height, `d1` = dx, `d2` = dy):
       - **Glide Trigger (`d4 < 24`):**
         - If `d3 < 32`: jumps to Dive Leap at `0x0169E2`.
         - If `d3 >= 32`: clears timer `+$2 = 0`, enters State 2 Glide:
           - `d1 >= 0` (Ali to right): `+$9E = d1 + 1`, 4-limb anims `$20/$1A/$1E/$1C`, State 2, anim 10, `+$A2 = 2`, SFX `#$70021A`.
           - `d1 < 0` (Ali to left): `+$9E = d1 - 1`, 4-limb anims `$1A/$20/$1C/$1E`, State 2, anim 10, `+$A2 = -2`, SFX `#$70021A`.
       - **Ranged Action Rolls (`d4 >= 24`):**
         - If `d2 < 0` (Ali above boss): jumps to Dive Leap at `0x0169E2`.
         - If `d2 >= 0`: rolls `0..99` via PRNG `0x00E2A2`:
           - **`0..29` (Fan Barrage):** sets State `$12`, anim `$C`, broadcasts anim `$C` to 4 limbs, SFX `#$700219`. If arm child slot `$FF2084` is valid (type `$30`, state `!=$12`): sets `+$A0 = 0, +$A1 = $28` (40f); else sets `+$A0 = $1E, +$A1 = $16`.
           - **`30..59` (Arm Grab):** if arm child slot `$FF2084` is valid, sets boss State `$0A`, anim 8, arm State 8, anim `$32`, timer 5, calculates aim velocities `delta_x = ((ali_x - 4) - arm_x) >> 4`, `delta_y = ((ali_y - 16) - arm_y) >> 4`, plays SFX `#$700219`. If arm is invalid, falls back to Fan Barrage.
           - **`60..79` (State `$04`):** sets State `$04`, broadcasts anim `$C` to 4 limbs.
           - **`80..89` (Dive Leap):** sets State `$0C`, broadcasts anim `$C` to 4 limbs, plays SFX `#$700219`.
           - **`90..99` (Idle):** sets State 0, anim 0, timer 15 frames.
  4. `0x016A46..0x016A64` (`AI_SerpentBoss_4LimbAnimBroadcaster`, 30 bytes, 10 insns):
     - Broadcasts anim ID in `d0` to `+$2A` across 4 sequential child entity slots (stride `0xBC`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_serpent_boss_phase1_actions_m15_517.json`
- Extractor Tool: `tools/extract_rom_serpent_boss_phase1_actions.py`
- Test Suite: `tests/rom_serpent_boss_phase1_actions_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace Thor::BossSerpent::Phase1 {

// --- SFX & Entity Identifiers ---
inline constexpr uint32_t kOrbLaunchSfxId            = 0x0070035Cu;
inline constexpr uint32_t kGlideHissSfxId            = 0x0070021Au;
inline constexpr uint32_t kRoarRumbleSfxId           = 0x00700219u;
inline constexpr uint16_t kBodyContactDamageWord     = 0x8014u; // 20 HP
inline constexpr int32_t  kBodyContactKnockbackVy    = 0x00040000; // +4.0 px/frame
inline constexpr int16_t  kBodyContactKnockbackVz    = -1;

// --- State Action Enum ---
enum class Phase1Action : uint16_t {
    IdleLock       = 0,
    GlideRight     = 1,
    GlideLeft      = 2,
    DiveLeap       = 3,
    FanBarrage     = 4,
    ArmGrab        = 5,
    State04        = 6,
    State0Idle     = 7,
};

// --- 4-Limb Animation Profiles ---
inline constexpr std::array<uint16_t, 4> kGlideRightLimbAnims{{ 0x20, 0x1A, 0x1E, 0x1C }};
inline constexpr std::array<uint16_t, 4> kGlideLeftLimbAnims{{ 0x1A, 0x20, 0x1C, 0x1E }};

// --- Action Selection Pure Function ---
struct ActionDecision {
    Phase1Action action{Phase1Action::State0Idle};
    uint16_t     boss_state{0};
    uint16_t     boss_anim{0};
    uint16_t     boss_timer{0};
    int16_t      sub_mode_a2{0};
    uint8_t      field_9e{0};
    uint8_t      broadcast_anim{0};
    uint32_t     play_sfx{0};
};

inline ActionDecision EvaluatePhase1Action(
    int16_t d4_dist,
    int16_t d3_height,
    int16_t d1_dx,
    int16_t d2_dy,
    uint8_t prng_roll,
    bool    dialogue_locked,
    bool    arm_valid) noexcept
{
    ActionDecision dec{};
    if (dialogue_locked) {
        dec.action = Phase1Action::IdleLock;
        dec.boss_state = 0;
        dec.boss_anim = 0;
        dec.boss_timer = 15;
        return dec;
    }

    if (d4_dist < 24) {
        if (d3_height < 32) {
            dec.action = Phase1Action::DiveLeap;
            dec.boss_state = 0x0C;
            dec.broadcast_anim = 0x0C;
            dec.play_sfx = kRoarRumbleSfxId;
            return dec;
        }
        dec.boss_state = 2;
        dec.boss_anim = 10;
        dec.boss_timer = 0;
        dec.play_sfx = kGlideHissSfxId;
        if (d1_dx >= 0) {
            dec.action = Phase1Action::GlideRight;
            dec.sub_mode_a2 = 2;
            dec.field_9e = static_cast<uint8_t>(d1_dx + 1);
        } else {
            dec.action = Phase1Action::GlideLeft;
            dec.sub_mode_a2 = -2;
            dec.field_9e = static_cast<uint8_t>(d1_dx - 1);
        }
        return dec;
    }

    if (d2_dy < 0) {
        dec.action = Phase1Action::DiveLeap;
        dec.boss_state = 0x0C;
        dec.broadcast_anim = 0x0C;
        dec.play_sfx = kRoarRumbleSfxId;
        return dec;
    }

    if (prng_roll < 30u) {
        dec.action = Phase1Action::FanBarrage;
        dec.boss_state = 0x12;
        dec.boss_anim = 0x0C;
        dec.broadcast_anim = 0x0C;
        dec.play_sfx = kRoarRumbleSfxId;
    } else if (prng_roll < 60u) {
        if (arm_valid) {
            dec.action = Phase1Action::ArmGrab;
            dec.boss_state = 0x0A;
            dec.boss_anim = 8;
            dec.play_sfx = kRoarRumbleSfxId;
        } else {
            dec.action = Phase1Action::FanBarrage;
            dec.boss_state = 0x12;
            dec.boss_anim = 0x0C;
            dec.broadcast_anim = 0x0C;
            dec.play_sfx = kRoarRumbleSfxId;
        }
    } else if (prng_roll < 80u) {
        dec.action = Phase1Action::State04;
        dec.boss_state = 0x04;
        dec.broadcast_anim = 0x0C;
    } else if (prng_roll < 90u) {
        dec.action = Phase1Action::DiveLeap;
        dec.boss_state = 0x0C;
        dec.broadcast_anim = 0x0C;
        dec.play_sfx = kRoarRumbleSfxId;
    } else {
        dec.action = Phase1Action::State0Idle;
        dec.boss_state = 0;
        dec.boss_anim = 0;
        dec.boss_timer = 15;
    }

    return dec;
}

} // namespace Thor::BossSerpent::Phase1
```
