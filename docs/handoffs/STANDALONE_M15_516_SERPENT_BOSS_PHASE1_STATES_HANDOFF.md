# STANDALONE HANDOFF: Milestone M15.516 — ROM Serpent Boss Phase 1 States 0x00..0x18 (Hover, Bounds-Clamped Glide, Dive Leap, Ground Slam Tremor & 4-Shot Fan Barrage)

## 1. Subsystem Overview
- **ROM Address Range:** `0x0164E2..0x016764` (642 bytes, 171 M68K instructions across 8 routines + 4-byte angle table `0x016734..0x016738`).
- **Core Responsibilities:**
  1. `0x0164E2..0x0164F2` (`AI_SerpentBoss_State00_HoverWait`, 16 bytes, 5 instructions):
     - Calls movement/hover helper `0x016764` (`d5 = 0`).
     - Decrements countdown timer `+$2(a6)`.
     - When `+$2 <= 0`, transitions to pattern sequencer action at `0x016862`.
  2. `0x0164F2..0x01651C` (`AI_SerpentBoss_State16_DeathGravityFall`, 42 bytes, 11 instructions):
     - Applies gravity `-0x0000C000` to vertical velocity `+$56`, integrating into height `+$10`.
     - Upon ground impact (`+$10 <= 0`), clamps `+$10 = 0`, calls sound cleanup `0x00F740`, and triggers screen effect `0x00E96C` mode 12 (`d0 = 0x0C`).
  3. `0x01651C..0x01652E` (`AI_SerpentBoss_State18_SettleWait`, 18 bytes, 5 instructions):
     - Calls `0x016764` (`d5 = 0`).
     - When event bit 1 in `+$F3(a6)` is set, transitions to pattern sequencer `0x01685A`.
  4. `0x01652E..0x01656A` (`AI_SerpentBoss_State02_ArenaGlide`, 60 bytes, 15 instructions):
     - Calls `0x016764` with vertical parameter `d5 = -20`.
     - Applies horizontal velocity `+$A2 << 16` clamped to arena limits `[0x60..0x1A0]` via `0x01656A`.
     - Increments step counter `+$2(a6)`. Every 8th frame (`+$2 & 7 == 0`), triggers glide SFX `0x70035E` via `0x00E7FC`.
     - On event bit `+$F3:1`, transitions to `0x01685A`.
  5. `0x01656A..0x01658A` (`AI_SerpentBoss_Helper_ClampXPosition`, 32 bytes, 11 instructions):
     - Clamps X-velocity to arena bounds `[0x0060..0x01A0]` (96..416 px).
     - If moving right (`d1 >= 0`) and position `> 0x1A0`, sets `d1 = 0`.
     - If moving left (`d1 < 0`) and position `< 0x60`, sets `d1 = 0`.
     - Accumulates valid displacement into `+$72(a6)`.
  6. `0x01658A..0x0165E2` (`AI_SerpentBoss_States04_06_08_0A_WindupArmWait`, 88 bytes, 22 instructions):
     - State `0x04`: on `+$F3:1` $\to$ enters State `0x06`, sets timer `+$2 = 20`, calls anim helper `0x016A46(0x2A)`.
     - State `0x06`: decrements `+$2`; on expiration (0) $\to$ enters State `0x08`, calls anim `0x016A46(0x0E)`.
     - State `0x08`: on `+$F3:1` $\to$ transitions to pattern sequencer `0x01685A`.
     - State `0x0A`: inspects child arm slot `$FF2084`: checks `+$4 == 0`. When 0 $\to$ transitions to `0x016862`.
  7. `0x0165E2..0x0166D2` (`AI_SerpentBoss_States0C_0E_10_DiveLeapGroundSlam`, 240 bytes, 57 instructions):
     - State `0x0C`: on `+$F3:1`, computes 42-frame trajectory vector `(ali_pos - boss_pos) / 42` (`divs #$2A`) into `+$4E/+$52`, sets upward launch velocity `+$56 = 0x00100000` (+16.0 px), enters State `0x0E`, anim `0x016A46(0x26)`.
     - State `0x0E`: while arm active flag `+$EC > 0`, applies gravity `-0x0000C000`. While `+$10 > 0x30`, integrates Y velocity and clamped X velocity. When landing (`+$10 <= 0x30`), clamps `+$10 = 0x30`, clears `+$56 = 0`, enters State `0x10`, anim `0x016A46(0x22)`.
     - State `0x10`: when arm frame `+$EC == 4`, triggers screen shake `$FF17C4 = 0x0C`, plays impact SFX `0x90021C` and rumble SFX `0x80035D` via `0x00E7FC`. On `+$F3:1` $\to$ transitions to `0x01685A`.
  8. `0x0166D2..0x016764` (`AI_SerpentBoss_States12_14_FanBarrage`, 146 bytes, 35 instructions + 4-byte table):
     - State `0x12`: on `+$F3:1` $\to$ enters State `0x14`, sets substate `+$2A = 2`, anim `0x016A46(0x2A)`.
     - State `0x14`: at anim frame `+$30 == 8`, checks burst counter `+$A0`:
       - If `+$A0 > 0`: increments `+$2E`, decrements `+$A0`. On even count, selects angle from 4-byte table `0x016734` (`(+$A0 >> 1) & 3`) and spawns projectile via `0x0167BE` with `speed = 0x00030000` (3.0 px).
       - If `+$A0 == 0`: spawns center aimed shot (`angle = 0xC0`, `speed = 0x00040000`).
     - On animation finished flag `+$37:1`, resets to State `0x08`, substate `+$2A = 0`, anim `0x016A46(0x0E)`.
  9. Table `0x016734..0x016738`: 4 bytes `[0xB5, 0xCB, 0x9F, 0xE1]` (181, 203, 159, 225 steps).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_serpent_boss_phase1_states_m15_516.json`
- Extractor: `tools/extract_rom_serpent_boss_phase1_states.py`
- Unit Tests: `tests/rom_serpent_boss_phase1_states_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <tuple>

namespace Thor::AI::SerpentPhase1 {

constexpr std::array<uint8_t, 4> FAN_ANGLES = {0xB5, 0xCB, 0x9F, 0xE1};

struct SerpentBossEntity {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    int32_t vx{0};
    int32_t vy{0};
    int32_t vz{0};
    int32_t x_accum{0};
    int32_t y_accum{0};
    int16_t vel_a2{0};
    int16_t timer{0};
    uint16_t state{0};
    uint16_t substate_2a{0};
    uint16_t anim_frame_30{0};
    uint8_t flags_37{0};
    uint8_t flags_f3{0};
    int16_t arm_active_ec{0};
    uint8_t burst_counter_a0{0};
    uint16_t shot_counter_2e{0};
};

struct PlayerTarget {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
};

struct SpawnedProjectile {
    uint8_t angle{0};
    int32_t speed{0};
    bool is_aimed{false};
};

/// Clamps arena X movement to [0x60..0x1A0] (96..416 px) (0x01656A..0x016588).
inline int32_t ClampXVelocity(int16_t current_x, int32_t dx) noexcept {
    if (dx >= 0) {
        if (current_x > 0x01A0) return 0;
    } else {
        if (current_x < 0x0060) return 0;
    }
    return dx;
}

/// Emulates State $00 Idle/Hover Wait (0x0164E2..0x0164F2).
inline bool StepHoverWait(SerpentBossEntity& boss) noexcept {
    boss.timer -= 1;
    return (boss.timer <= 0); // true triggers next pattern action (0x016862)
}

/// Emulates State $16 Death Gravity Fall (0x0164F2..0x01651C).
inline bool StepDeathGravityFall(SerpentBossEntity& boss) noexcept {
    if (boss.z <= 0) return true;

    boss.vz -= 0x0000C000;
    boss.z += boss.vz;

    if (boss.z > 0) return false;

    boss.z = 0;
    return true; // ground impact cleanup
}

/// Emulates State $02 Arena Glide (0x01652E..0x01656A).
/// Returns pair: (exit_to_sequencer, should_play_footstep_sfx).
inline std::pair<bool, bool> StepArenaGlide(SerpentBossEntity& boss) noexcept {
    int32_t dx = static_cast<int32_t>(boss.vel_a2) << 16;
    int32_t clamped_dx = ClampXVelocity(boss.x, dx);
    boss.x_accum += clamped_dx;
    boss.x += static_cast<int16_t>(clamped_dx >> 16);

    boss.timer += 1;
    bool sfx = ((boss.timer & 7) == 0);
    bool exit = ((boss.flags_f3 & 0x02) != 0);

    return {exit, sfx};
}

/// Emulates Dive Leap & Ground Slam (States $0C, $0E, $10) (0x0165E2..0x0166D2).
enum class DiveStateResult { InProgress, LeapInitiated, LandedGround, ShakeTriggered, ExitSequencer };

inline DiveStateResult StepDiveLeapGroundSlam(
    SerpentBossEntity& boss,
    const PlayerTarget& ali
) noexcept {
    if (boss.state == 0x0C) {
        if ((boss.flags_f3 & 0x02) != 0) {
            boss.state = 0x0E;
            int32_t d1 = (static_cast<int32_t>(ali.x) - boss.x) >> 6;
            int32_t d2 = (static_cast<int32_t>(ali.y) - boss.y) >> 6;
            boss.vx = (d1 / 42) << 6;
            boss.vy = (d2 / 42) << 6;
            boss.vz = 0x00100000;
            return DiveStateResult::LeapInitiated;
        }
        return DiveStateResult::InProgress;
    } else if (boss.state == 0x0E) {
        if (boss.arm_active_ec <= 0) return DiveStateResult::InProgress;

        boss.vz -= 0x0000C000;
        boss.z += boss.vz;

        if (boss.z > 0x30) {
            boss.y_accum += boss.vy;
            int32_t eff_vx = ClampXVelocity(boss.x, boss.vx);
            boss.x_accum += eff_vx;
            return DiveStateResult::InProgress;
        } else {
            boss.z = 0x30;
            boss.vz = 0;
            boss.state = 0x10;
            return DiveStateResult::LandedGround;
        }
    } else if (boss.state == 0x10) {
        bool shake = (boss.arm_active_ec == 4);
        if ((boss.flags_f3 & 0x02) != 0) {
            return DiveStateResult::ExitSequencer;
        }
        return shake ? DiveStateResult::ShakeTriggered : DiveStateResult::InProgress;
    }
    return DiveStateResult::InProgress;
}

/// Emulates Fan Barrage (States $12, $14) (0x0166D2..0x016764).
inline std::pair<bool, std::optional<SpawnedProjectile>> StepFanBarrage(
    SerpentBossEntity& boss
) noexcept {
    if (boss.state == 0x12) {
        if ((boss.flags_f3 & 0x02) != 0) {
            boss.state = 0x14;
            boss.substate_2a = 2;
        }
        return {false, std::nullopt};
    } else if (boss.state == 0x14) {
        std::optional<SpawnedProjectile> shot = std::nullopt;
        if (boss.anim_frame_30 == 8) {
            if (boss.burst_counter_a0 > 0) {
                boss.shot_counter_2e += 1;
                boss.burst_counter_a0 -= 1;
                if (boss.burst_counter_a0 == 0) boss.shot_counter_2e = 0;

                if ((boss.burst_counter_a0 & 1) == 0) {
                    size_t idx = (boss.burst_counter_a0 >> 1) & 3;
                    shot = SpawnedProjectile{FAN_ANGLES[idx], 0x00030000, false};
                }
            } else {
                shot = SpawnedProjectile{0xC0, 0x00040000, true};
            }
        }

        if ((boss.flags_37 & 0x02) != 0) {
            boss.state = 8;
            boss.substate_2a = 0;
            return {true, shot}; // Barrage complete
        }
        return {false, shot};
    }
    return {false, std::nullopt};
}

} // namespace Thor::AI::SerpentPhase1
```

## 4. Behavioral Parity & Invariants
1. **Arena Boundary Hard Clamping:** Movement is strictly bounded to `[96..416]` px (`0x60..0x1A0`), preventing the serpent from clipping into wall colliders during lateral glides or dive leaps.
2. **Exact 42-Frame Div Trajectory:** The dive leap uses an integer signed division (`divs.w #$2A`) to partition horizontal travel across exactly 42 frames with initial upward launch impulse `0x00100000`.
3. **Ground Slam Elevation Clamp:** Landing occurs at `z = 0x30` (48 px above floor plane), immediately stabilizing vertical motion (`vz = 0`) and triggering screen shake `$FF17C4 = 0x0C`.
4. **4-Angle Fan Pattern Interleaving:** Projectile emission triggers on animation frame 8; even burst counters step through `{0xB5, 0xCB, 0x9F, 0xE1}` at speed `0x30000`, while the final burst fires an aimed center projectile (`angle = 0xC0`, speed `0x40000`).
