# STANDALONE HANDOFF: Milestone M15.510 — ROM Multi-Part Boss/Golem (`0x9A`) Phase Damage Handler, Plane A Tilemap Damage Patches, Tentacle/Arm Sub-State Machine (`0..4`) & Spore Homing (`0x015758..0x0159E2`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x015758..0x0159E2` (650 bytes total: 160 M68K instructions across 3 code segments, plus two 12-byte tilemap patch records).
- **Core Responsibilities:**
  1. `0x015758..0x0157FC` (`AI_GolemBoss_BodyHitDamage`, 164 bytes, 42 insns):
     - Checks pending damage in `+$86(a6)`. If zero or if already in State 4 hitstun (`+$04(a6) == 4`), bypasses damage and branches directly to Tentacle/Arm logic `0x01585C`.
     - Clears pending damage `+$86 = 0` and hit flag `+$82 = 0`.
     - Extracts 12-bit damage: `d0 = d0 & 0x0FFF`.
     - Subtracts damage from HP `+$8A(a6)`; clamps HP to 0 if negative.
     - On HP = 0: invokes death trigger `0x00E96C` with mode `d0 = 7` and exits immediately (`rts`).
     - On HP > 0: enters State 4 (`+$04 = 4`), anim `+$2A = 6`, hitstun timer `+$83 = 8`.
     - Phase 1 transition check: if `+$8E == 1` and `current_hp <= max_hp / 4`:
       - Increments `+$8E` to 2.
       - Emits debris particle 3 at `(body_x - 136, body_y - 64, body_z)` via `0x00E93A`.
       - Applies Plane A tilemap fracture patch 1 via `0x00DB68` using Record 1 (`0x0157FC`).
  2. `0x0157FC..0x015808` (`TilemapPatch_Phase1`, 12 bytes data):
     - Source Rect: `[left=6, top=32, right=11, bottom=37]` (5x5 tiles).
     - Dest Coord: `[x=33, y=12]`.
     - Plane A VRAM buffer context: `$FF1716`.
  3. `0x015808..0x015850` (`AI_GolemBoss_Phase0Check`, 72 bytes, 18 insns):
     - Phase 0 transition check: if `+$8E == 0` and `current_hp <= max_hp / 2`:
       - Increments `+$8E` to 1.
       - Sets global damage flag bit 1: `bset.b #1, $FF2A3E.l`.
       - Emits debris particle 3 at `(body_x - 128, body_y + 8, body_z)` via `0x00E93A`.
       - Applies Plane A tilemap fracture patch 0 via `0x00DB68` using Record 2 (`0x015850`).
  4. `0x015850..0x01585C` (`TilemapPatch_Phase0`, 12 bytes data):
     - Source Rect: `[left=0, top=32, right=5, bottom=37]` (5x5 tiles).
     - Dest Coord: `[x=32, y=17]`.
     - Plane A VRAM buffer context: `$FF1716`.
  5. `0x01585C..0x0159B4` (`AI_GolemBoss_TentacleArmAndSporeHoming`, States 0..4):
     - Dispatches on sub-state `+$04(a6)`:
       - **State 0 (Idle):** resets `+$04 = 0`, returns.
       - **State 1 (Extend / Attack):** decrements timer `+$2`. If zero, times out to State 2 (retract) with anim `+$2A = 10`. If active, computes steering toward Ali (`$FF19F0`, `$FF19F4`) via `0x00E45E`. If distance `d4 > 4`, advances at 2x speed (`vx = d1 * 2`, `vy = d2 * 2`) accumulating into `+$1E(a6)` and `+$76(a6)`. If distance `d4 <= 4`:
         - Checks Ali guard: if `ali.+$38 & (1 << 3)` or `ali.+$82 != 0`, blocked -> switches arm to State 2 (`+$2A = 10`).
         - Otherwise inflicts hit: sets attacker pointer `ali.+$7E = a6`, clears `ali.+$82 = 0`, sets damage `ali.+$86 = 10` (or `0x800A` if airborne `ali.+$10 != 0`), clears `ali.+$9D = 0`, reaction mode `ali.+$84 = 3`, knockback `ali.+$4E = -0x00050000`, `ali.+$52 = 0`, `ali.+$56 = +0x00020000`, hitstun `ali.+$83 = 14`, plays SFX `$800381` via `0x00E7FC`, and sets arm to State 3 cooldown (`+$2 = 8`, `+$2A = 10`).
       - **State 2 (Retract to Home):** steers toward home (`$FF1CE0`, `$FF1CE4`) at 2x speed. When distance `d4 <= 4`, resets arm to State 0 (idle).
       - **State 3 (Cooldown):** decrements timer `+$2`. When expired (`+$2 == 0`), transitions to State 2 (retract).
       - **State 4 (Hitstun Retract):** decrements hitstun timer `+$83`. While `+$83 > 0`, sets flashing bit `+$36:6` and steers toward home at 1x speed (clamps if `d4 <= 2`). Upon expiration, clears `+$36:6` and `+$39:3`, transitioning to State 2 (`+$2A = 10`).
  6. `0x0159B4..0x0159E2` (`AI_GolemBoss_SporeHomingReturn`, 46 bytes, 12 insns):
     - Sub-mode `+$A2 == 3`: loads parent entity pointer from `+$A6(a6)`.
     - Steers toward parent pos `(parent.+$8, parent.+$C)` via `0x00E45E`.
     - When distance `d4 > 10`: accumulates at 4x speed into spore 32-bit position `+$72` and `+$76`.
     - When distance `d4 <= 10`: despawns spore entity (`+$00(a6) = 0`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_golem_boss_damage_arm_m15_510.json`
- Extractor Tool: `tools/extract_rom_golem_boss_damage_arm.py`
- Test Suite: `tests/rom_golem_boss_damage_arm_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace Thor::BossGolem {

// --- SFX & Entity Identifiers ---
inline constexpr uint32_t kTentacleHitSfxId          = 0x00800381u;
inline constexpr uint16_t kGolemDeathModeId          = 7u;
inline constexpr uint8_t  kGolemDebrisParticleType   = 3u;

// --- Knockback Parameters ---
inline constexpr int32_t  kAliHitKnockbackVx         = -0x00050000; // -5.0 px/frame
inline constexpr int32_t  kAliHitKnockbackVy         = 0;
inline constexpr int32_t  kAliHitKnockbackVz         = 0x00020000;  // +2.0 px/frame
inline constexpr uint8_t  kAliHitHitstunFrames       = 14u;
inline constexpr uint16_t kAliHitBaseDamage          = 10u;
inline constexpr uint16_t kAirborneDamageMask        = 0x8000u;

// --- Arm Timing & Thresholds ---
inline constexpr uint16_t kArmCooldownFrames         = 8u;
inline constexpr uint16_t kArmHitDistanceThreshold   = 4u;
inline constexpr uint16_t kArmHomeDistanceThreshold  = 4u;
inline constexpr uint16_t kArmHitstunHomeThreshold   = 2u;
inline constexpr uint16_t kSporeHomingThreshold      = 10u;

// --- Tilemap Patch Definitions ---
struct TilemapRect {
    uint16_t left{0};
    uint16_t top{0};
    uint16_t right{0};
    uint16_t bottom{0};
};

struct TilemapPatch {
    TilemapRect source_rect;
    uint16_t    dest_x{0};
    uint16_t    dest_y{0};
    uint16_t    width_tiles{0};
    uint16_t    height_tiles{0};
};

// Record 2 at 0x015850 (Phase 0 -> Phase 1, 50% HP threshold)
inline constexpr TilemapPatch kTilemapPatchPhase0{
    .source_rect  = { .left = 0, .top = 32, .right = 5, .bottom = 37 },
    .dest_x       = 32,
    .dest_y       = 17,
    .width_tiles  = 5,
    .height_tiles = 5,
};

// Record 1 at 0x0157FC (Phase 1 -> Phase 2, 25% HP threshold)
inline constexpr TilemapPatch kTilemapPatchPhase1{
    .source_rect  = { .left = 6, .top = 32, .right = 11, .bottom = 37 },
    .dest_x       = 33,
    .dest_y       = 12,
    .width_tiles  = 5,
    .height_tiles = 5,
};

// --- Golem Body Damage Result ---
struct BodyDamageResult {
    bool     jump_to_tentacle_arm{true};
    bool     triggered_death{false};
    uint8_t  new_phase{0};
    bool     phase_advanced{false};
    bool     debris_emitted{false};
    int16_t  debris_x{0};
    int16_t  debris_y{0};
    int16_t  debris_z{0};
    std::optional<TilemapPatch> applied_patch;
};

// --- Tentacle Arm State Enum ---
enum class ArmState : uint16_t {
    Idle     = 0,
    Extend   = 1,
    Retract  = 2,
    Cooldown = 3,
    Hitstun  = 4,
};

// --- Arm Step Result ---
struct ArmStepResult {
    ArmState new_state{ArmState::Idle};
    uint16_t new_anim{0};
    bool     hit_ali{false};
    bool     play_sfx{false};
    uint32_t sfx_id{0};
};

// --- Pure Step Functions ---

inline BodyDamageResult ProcessGolemHitDamage(
    uint16_t pending_damage,
    uint16_t current_state,
    uint16_t& current_hp,
    uint16_t max_hp,
    uint8_t&  fracture_phase,
    int16_t   body_x,
    int16_t   body_y,
    int16_t   body_z,
    bool&     flag_ff2a3e_bit1) noexcept
{
    BodyDamageResult res{};
    if (pending_damage == 0) {
        res.jump_to_tentacle_arm = true;
        return res;
    }

    if (current_state == 4) {
        // Bypass if already in hitstun state 4
        res.jump_to_tentacle_arm = true;
        return res;
    }

    const uint16_t damage = pending_damage & 0x0FFFu;
    if (damage >= current_hp) {
        current_hp = 0;
        res.triggered_death = true;
        res.jump_to_tentacle_arm = false;
        return res;
    }

    current_hp -= damage;
    res.jump_to_tentacle_arm = true;

    if (fracture_phase == 0) {
        const uint16_t half_hp = max_hp >> 1;
        if (current_hp <= half_hp) {
            fracture_phase = 1;
            flag_ff2a3e_bit1 = true;
            res.phase_advanced = true;
            res.new_phase = 1;
            res.debris_emitted = true;
            res.debris_x = body_x - 128;
            res.debris_y = body_y + 8;
            res.debris_z = body_z;
            res.applied_patch = kTilemapPatchPhase0;
        }
    } else if (fracture_phase == 1) {
        const uint16_t quarter_hp = max_hp >> 2;
        if (current_hp <= quarter_hp) {
            fracture_phase = 2;
            res.phase_advanced = true;
            res.new_phase = 2;
            res.debris_emitted = true;
            res.debris_x = body_x - 136;
            res.debris_y = body_y - 64;
            res.debris_z = body_z;
            res.applied_patch = kTilemapPatchPhase1;
        }
    }

    return res;
}

} // namespace Thor::BossGolem
```
