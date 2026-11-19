# STANDALONE HANDOFF: Milestone M15.509 — ROM Multi-Part Boss/Golem (`0x9A`) Vacuum Pull, Boulder Barrage Spawner, Body Knockback & Camera-Relative SFX Helpers

## 1. Subsystem Overview
- **ROM Address Range:** `0x0154E8..0x015758` (624 bytes, 151 M68K instructions across 7 routines).
- **Core Responsibilities:**
  1. `0x0154E8..0x01552E` (`AI_GolemBoss_State04_VacuumPull`, 70 bytes, 18 instructions):
     - Decrements pull duration timer `+$2(a6)`.
     - When timer reaches 0 or below, transitions to reset helper `0x01563C` (State 0, timer `0x28`).
     - Every 4th frame (`d0 & 3 == 0`), triggers spore/debris emission via `0x01569E`.
     - Computes directional pull vector toward boss via `0x00E45E`.
     - Scales pull displacement by 2x for grounded Ali (`+$10(a5) == 0`) or 4x for airborne Ali (`+$10(a5) != 0`).
     - Inverts delta (`neg.l`) and applies directly to Ali position accumulation registers `+$72(a5)` and `+$76(a5)`.
  2. `0x01552E..0x01563C` (`AI_GolemBoss_State02_BoulderBarrage`, 270 bytes, 60 instructions):
     - Decrements interval timer `+$2(a6)`.
     - When timer expires, decrements burst counter `+$A6(a6)`.
     - If `+$A6 == 0`, burst is complete; exits via `0x01563C` (resets to State 0, timer `0x28`).
     - Generates next interval: `(PRNG & 0x0F) + 2` frames (2..17 frames).
     - Allocates child entity in Pool 2 via `0x00DA2A` + `0x00E66E` at `(boss_x - 0x30, boss_y, z=10)` with bounds extent 6, height 16.
     - Aiming logic: checks decremented counter `(+$A6 & 7) == 0`:
       - If multiple of 8: aimed directly at Ali via `0x010684` with speed `0x000C0000`, vertical launch velocity `+$56 = 0x00080000`, and spawn height `z = 6`.
       - Otherwise: PRNG scatter velocities:
         - `x_vel = (PRNG1 << 1) - 0x000A8000`
         - `y_vel = (PRNG2 << 2) - 0x00010000`
         - `z_vel = PRNG3 + 0x00080000`
         - Spawn height `z = 10`.
     - Configures boulder anim `0x0C`, sub-mode `+$A2 = 2`, and plays SFX `0x70035C` via camera-relative helper `0x01571A`.
  3. `0x01563C..0x015648` (`AI_GolemBoss_ResetToState0`, 12 bytes, 3 instructions):
     - Clears state `+$4(a6) = 0` (State 0) and sets cooldown timer `+$2(a6) = 0x0028` (40 frames).
  4. `0x015648..0x01569E` (`AI_GolemBoss_BodyContactKnockback`, 86 bytes, 23 instructions):
     - Primary body contact callback triggered on collision with Ali.
     - Checks if Ali is invulnerable (`+$38:3`) or actively guarding (`+$82 != 0`):
       - If immune/guarding: nudges Ali position horizontally by `-0x00040000` (-4.0 px) without damage or flinch.
       - If vulnerable: inflicts 10 damage (`+$86 = 0x800A`), clears guard/flinch flags, sets knockback mode `+$84 = 3`, horizontal impulse `+$4E = -0x00100000` (-16.0 px), vertical impulse `+$56 = 0x00030000` (+3.0 px), and triggers impact SFX `0x800208` via `0x00060004`.
  5. `0x01569E..0x01571A` (`AI_GolemBoss_SpawnSporeRock`, 124 bytes, 31 instructions):
     - Spawns falling debris entity in Pool 2 via `0x00DA2A` + `0x00E66E`.
     - Positions at `(boss_x - 176 + (PRNG1 >> 4), boss_y + (PRNG2 >> 2) + (PRNG2 >> 3), z = 30)`.
     - Sets flying flag `+$38 |= 4`, anim `0x10`, State 0, sub-mode `+$A2 = 3`, links parent pointer, and triggers SFX `0x80034F` via `0x01571A`.
  6. `0x01571A..0x01573C` (`AI_GolemBoss_PlayCameraRelativeSFX`, 34 bytes, 9 instructions):
     - Camera-relative positional audio helper. Reads screen anchor from `$FF17AE`, offsets X by `-0x30` (-48 px), and calls audio dispatcher `0x00E7A6`.
  7. `0x01573C..0x015758` (`AI_GolemBoss_CheckSubMode4`, 28 bytes, 7 instructions):
     - Inspects previous entity slot `-$BC(a6)` in Pool 2 for Type `0x34` and anim timer `+$30 == 8`.
     - On match, invokes screen-shake/particle dispatcher `0x00E96C` with mode `d0 = 3`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_golem_boss_vacuum_barrage_m15_509.json`
- Extractor: `tools/extract_rom_golem_boss_vacuum_barrage.py`
- Unit Tests: `tests/rom_golem_boss_vacuum_barrage_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <optional>
#include <tuple>

namespace Thor::AI::GolemBoss {

struct GolemBossEntity {
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
    int16_t timer{0};
    uint8_t state{0};
    uint8_t burst_counter{0};
    uint8_t sub_mode{0};
};

struct PlayerTarget {
    int32_t x{0};
    int32_t y{0};
    int16_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    uint8_t flags_38{0};
    bool guard_82{false};
    uint16_t damage_86{0};
    uint8_t knockback_mode_84{0};
    uint32_t sfx{0};
};

struct SpawnedBoulder {
    int16_t x{0};
    int16_t y{0};
    int16_t z{10};
    uint8_t bounds{6};
    uint16_t height{16};
    uint8_t anim_id{0x0C};
    uint8_t state{0};
    uint8_t sub_mode{2};
    bool is_aimed{false};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    int32_t speed{0};
    uint32_t sfx{0x70035C};
};

struct SpawnedSporeRock {
    int16_t x{0};
    int16_t y{0};
    int16_t z{30};
    uint8_t bounds{4};
    uint8_t anim_id{0x10};
    uint8_t state{0};
    uint8_t sub_mode{3};
    uint8_t flags_38{0x04}; // bit 2 flying
    uint32_t sfx{0x80034F};
};

/// Resets boss to State 0 with cooldown delay 0x28 (40 frames).
inline void ResetToState0(GolemBossEntity& boss) noexcept {
    boss.state = 0;
    boss.timer = 0x0028;
}

/// Emulates State $04 Vacuum Pull & Spore Emitter (0x0154E8..0x01552E).
/// Returns pair: (should_spawn_spore, reset_occurred).
template <typename DirectionCalcFn>
inline std::pair<bool, bool> StepVacuumPull(
    GolemBossEntity& boss,
    PlayerTarget& ali,
    DirectionCalcFn&& calc_dir_fn
) noexcept {
    boss.timer -= 1;
    if (boss.timer < 0) {
        ResetToState0(boss);
        return {false, true};
    }

    bool spawn_spore = ((boss.timer & 3) == 0);

    // Compute direction vector from Ali to Boss
    auto [dx, dy] = calc_dir_fn(ali.x, ali.y, boss.x, boss.y);

    // Scale by 2x for grounded Ali, 4x for airborne Ali
    int32_t scale = (ali.z != 0) ? 4 : 2;
    int32_t pull_x = -(dx * scale);
    int32_t pull_y = -(dy * scale);

    ali.x += pull_x;
    ali.y += pull_y;

    return {spawn_spore, false};
}

/// Emulates State $02 Boulder Barrage Spawner (0x01552E..0x01563C).
/// Returns pair: (optional_spawned_boulder, reset_occurred).
template <typename PRNGFn, typename AimCalcFn>
inline std::pair<std::optional<SpawnedBoulder>, bool> StepBoulderBarrage(
    GolemBossEntity& boss,
    const PlayerTarget& ali,
    PRNGFn&& prng_fn,
    AimCalcFn&& aim_fn,
    bool pool_alloc_ok = true
) noexcept {
    boss.timer -= 1;
    if (boss.timer >= 0) {
        return {std::nullopt, false};
    }

    boss.burst_counter -= 1;
    if (boss.burst_counter == 0) {
        ResetToState0(boss);
        return {std::nullopt, true};
    }

    // Next interval: (PRNG & 0xF) + 2
    uint16_t p = prng_fn();
    boss.timer = static_cast<int16_t>((p & 0x0F) + 2);

    if (!pool_alloc_ok) {
        return {std::nullopt, false};
    }

    SpawnedBoulder b;
    b.x = boss.x - 0x30;
    b.y = boss.y;

    // Every 8th boulder is aimed directly at Ali
    if ((boss.burst_counter & 7) == 0) {
        b.is_aimed = true;
        b.speed = 0x000C0000;
        b.z = 6;
        b.z_vel = 0x00080000;
        auto [vx, vy] = aim_fn(b.x, b.y, static_cast<int16_t>(ali.x), static_cast<int16_t>(ali.y), b.speed);
        b.x_vel = vx;
        b.y_vel = vy;
    } else {
        b.is_aimed = false;
        b.z = 10;
        int32_t p1 = static_cast<int16_t>(prng_fn());
        int32_t p2 = static_cast<int16_t>(prng_fn());
        int32_t p3 = static_cast<int16_t>(prng_fn());
        b.x_vel = (p1 << 1) - 0x000A8000;
        b.y_vel = (p2 << 2) - 0x00010000;
        b.z_vel = p3 + 0x00080000;
    }

    return {b, false};
}

/// Emulates Body Contact Knockback Callback (0x015648..0x01569E).
enum class ContactResult { Nudged, KnockedBack };

inline ContactResult ApplyBodyContactKnockback(
    PlayerTarget& ali,
    const GolemBossEntity& /*boss*/
) noexcept {
    bool is_immune = ((ali.flags_38 & 0x08) != 0) || ali.guard_82;

    if (is_immune) {
        // Continuous horizontal push delta of -4.0 px
        ali.x -= 0x00040000;
        return ContactResult::Nudged;
    }

    ali.guard_82 = false;
    ali.damage_86 = 0x800A; // 10 HP damage
    ali.knockback_mode_84 = 3;
    ali.x_vel = -0x00100000; // -16.0 px impulse
    ali.y_vel = 0;
    ali.z_vel = 0x00030000;  // +3.0 px upward lift
    ali.sfx = 0x800208;
    return ContactResult::KnockedBack;
}

/// Emulates Spore/Rock Debris Spawner (0x01569E..0x01571A).
template <typename PRNGFn>
inline std::optional<SpawnedSporeRock> SpawnSporeRock(
    const GolemBossEntity& boss,
    PRNGFn&& prng_fn,
    bool pool_alloc_ok = true
) noexcept {
    if (!pool_alloc_ok) {
        return std::nullopt;
    }

    int32_t p1 = static_cast<int16_t>(prng_fn());
    int32_t p2 = static_cast<int16_t>(prng_fn());

    int16_t dx = static_cast<int16_t>(-176 + (p1 >> 4));
    // Exact 3/8 scaling: (p2 >> 2) + (p2 >> 3)
    int16_t dy = static_cast<int16_t>((p2 >> 2) + (p2 >> 3));

    SpawnedSporeRock r;
    r.x = boss.x + dx;
    r.y = boss.y + dy;
    r.z = 30;
    return r;
}

/// Emulates Camera-Relative SFX Coordinates (0x01571A..0x01573C).
inline std::pair<int16_t, int16_t> GetCameraRelativeSFXCoords(
    int16_t camera_anchor_x,
    int16_t camera_anchor_y
) noexcept {
    return {static_cast<int16_t>(camera_anchor_x - 0x30), camera_anchor_y};
}

/// Emulates Sub-Mode 4 Check on Previous Slot -$BC(a6) (0x01573C..0x015758).
inline std::optional<uint16_t> CheckSubMode4(
    uint8_t prev_type,
    uint16_t prev_anim_timer
) noexcept {
    if (prev_type == 0x34 && prev_anim_timer == 8) {
        return 3; // Mode 3 screen shake/particles
    }
    return std::nullopt;
}

} // namespace Thor::AI::GolemBoss
```

## 4. Behavioral Parity & Invariants
1. **Airborne vs Grounded Pull Scaling:** Airborne player entities (`z != 0`) experience double the horizontal pull delta (4x vs 2x), creating strong suction hazard during jumps.
2. **Deterministic Aiming Cadence:** Every 8th boulder in the burst (`(burst_counter & 7) == 0`) fires with high fixed velocity (`speed = $C0000`, `vz = $80000`) aimed directly at player coordinates. Remaining boulders scatter with randomized pseudo-normal spreads.
3. **Guard / Invulnerability Nudge:** Rather than ignoring immune players, body collision continuously displaces them leftward by -4.0 px (`-$40000`), preventing clipping or trapping inside the boss collider without dealing spurious damage.
4. **Sub-Mode 4 Inter-Entity Check:** The previous entity slot `-$BC` (4 bytes back in entity pool stride) synchronizes tentacle/body destruction with particle effects when reaching animation frame 8.
