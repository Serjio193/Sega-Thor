# STANDALONE HANDOFF: Milestone M15.513 — ROM Composite Serpent/Multi-Segment Boss States 0x00..0x16 (Grab, Slam, Spit, Death Gravity Fall & 4-Segment Despawn Stagger)

## 1. Subsystem Overview
- **ROM Address Range:** `0x015E0E..0x0160E2` (724 bytes, 168 M68K instructions across 8 routines).
- **Core Responsibilities:**
  1. `0x015E0E..0x015E3C` (`AI_CompositeSerpent_State00_AnchorSync`, 46 bytes, 11 instructions):
     - Tracks anchor offset from root entity at `$FF1CD8`.
     - Target anchor position: `(root.x - 0x20, root.y + 0x18)`.
     - Accumulates subpixel deltas into `+$72(a6)` and `+$76(a6)`.
     - Syncs Z position `+$10(a6)` directly from `root.z`.
  2. `0x015E3C..0x015E56` (`AI_CompositeSerpent_State08_Ascend`, 26 bytes, 7 instructions):
     - Ascends by 3 units (`+$10(a6) += 3`) per frame while countdown timer `+$2(a6)` is non-zero.
     - On timer expiration, transitions to State `0x02` (Hover & Grab Probe) with timer 17 (0x11).
  3. `0x015E56..0x015EEA` (`AI_CompositeSerpent_State02_HoverGrabProbe`, 148 bytes, 34 instructions):
     - Invokes boundary helper `0x016168`, descending `+$10(a6) -= 4` until ground boundary is reached.
     - Evaluates Ali target entity at `$FF19E8`:
       - Grounded check: `ali.z == 0` (airborne Ali immediately causes grab miss).
       - Invulnerability check: `ali.flags_38 & 0x08 == 0` (immune Ali causes grab miss).
       - Proximity check: `|head.x + 4 - ali.x| <= 16` and `|head.y + 16 - ali.y| <= 16`.
       - Grab Success: enters State `0x06` (Grab, anim `$36`, timer 17), negates multipliers `+$A6` and `+$AA`, locks player action `$FF1982 = 1`, marks Ali grabbed flag `+$36:1`, and sets Ali flying/invulnerable flags `+$38 |= 0x0C`.
       - Grab Miss: enters State `0x04` (Return Ascend, anim `$34`, timer 17) and negates multipliers `+$A6` and `+$AA`.
  4. `0x015EEA..0x015F04` (`AI_CompositeSerpent_State04_ReturnAscend`, 26 bytes, 7 instructions):
     - Ascends `+$10(a6) += 4` via boundary helper `0x016168` until top boundary, then returns to State `0x00` (anim `$12`).
  5. `0x015F04..0x015F34` (`AI_CompositeSerpent_State06_SlamSpitSetup`, 48 bytes, 12 instructions):
     - Synchronizes grabbed Ali position to serpent jaws via `0x016184`.
     - Ascends `+$10(a6) += 4` until top boundary.
     - Inspects arena coordinate `$FF1CE0`:
       - If `$FF1CE0 <= 0x100`: branches to Rightward sequence (State `0x0A`, timer 7).
       - If `$FF1CE0 > 0x100`: branches to Leftward sequence (State `0x14`, timer 7).
  6. `0x015F34..0x015FE0` (`AI_CompositeSerpent_States0A_0C_0E_RightwardSlam`, 172 bytes, 40 instructions):
     - State `0x0A`: Leftward windup (`+$72 -= 3`) for 7 frames $\to$ enters State `0x0C` (timer 5).
     - State `0x0C`: Rightward lunge (`+$72 += 16`) for 5 frames $\to$ enters State `0x0E` (timer 13, anim `$12`, invulnerable `+$38:3`).
     - Release & Rightward Slam: clears player lock `$FF1982 = 0`, clears grabbed flag `+$36:1` and flying/invuln bits `+$38 & 0xF3`, inflicts 20 HP damage (`+$86 = 0x8014`), launches Ali rightward (`vx = 0x00200000`, `vz = -1`, `z -= 32`, `x += 4`, facing 1, knockback mode 1).
     - State `0x0E`: Leftward recovery (`+$72 -= 4`) for 13 frames $\to$ resets to State `0x00`, clears invulnerability.
  7. `0x015FE0..0x01608C` (`AI_CompositeSerpent_States14_16_LeftwardSlam`, 172 bytes, 40 instructions):
     - State `0x14`: Rightward windup (`+$72 += 3`) for 7 frames $\to$ enters State `0x16` (timer 5).
     - State `0x16`: Leftward lunge (`+$72 -= 16`) for 5 frames $\to$ enters State `0x18` (recovery, timer 13, anim `$12`, invulnerable `+$38:3`).
     - Release & Leftward Slam: clears player lock `$FF1982 = 0`, clears grabbed flag `+$36:1` and bits `+$38 & 0xF3`, inflicts 20 HP damage (`+$86 = 0x8014`), launches Ali leftward (`vx = -0x00200000`, `vz = -1`, `z -= 32`, `x -= 4`, facing 1, knockback mode 3).
     - Recovery: Rightward recovery (`+$72 += 4`) for 13 frames $\to$ resets to State `0x00`, clears invulnerability.
  8. `0x01608C..0x0160E2` (`AI_CompositeSerpent_State10_DeathGravityFall`, 86 bytes, 17 instructions):
     - Death gravity fall: applies gravity `-$C000` to `+$56` into Z position `+$10`.
     - Upon ground impact (`+$10 <= 0`): clamps `+$10 = 0`, triggers screen effect `0x00E96C` mode 2, and arms 4 child segments for staggered death sequence:
       - Segment 1: mode `+$122 = 3`, timer `+$BE = 4` (offset `+0xBC`)
       - Segment 2: mode `+$1DE = 3`, timer `+$17A = 8` (offset `+0x178`)
       - Segment 3: mode `+$29A = 3`, timer `+$236 = 12` (offset `+0x234`)
       - Segment 4: mode `+$356 = 3`, timer `+$2F2 = 16` (offset `+0x2F0`)

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_serpent_boss_states_m15_513.json`
- Extractor: `tools/extract_rom_serpent_boss_states.py`
- Unit Tests: `tests/rom_serpent_boss_states_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <cmath>

namespace Thor::AI::SerpentStates {

struct RootEntity {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
};

struct PlayerTarget {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    int16_t delta_x{0};
    uint8_t flags_36{0};
    uint8_t flags_38{0};
    uint16_t damage_86{0};
    uint16_t facing_16{0};
    uint16_t knockback_mode_84{0};
    bool lock_1982{false};
};

struct ChildSegment {
    uint8_t mode_66{0};
    uint16_t timer_2{0};
};

struct SerpentHeadEntity {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    int32_t z_vel{0};
    int16_t delta_x_72{0};
    int16_t delta_y_76{0};
    uint16_t state{0};
    uint16_t timer{0};
    uint16_t anim_id{0};
    uint8_t flags_38{0};
    int32_t multiplier_a6{1};
    int32_t multiplier_aa{1};
};

/// Emulates State $00 Anchor Offset Sync (0x015E0E..0x015E3C).
inline void SyncAnchorOffset(SerpentHeadEntity& head, const RootEntity& root) noexcept {
    head.delta_x_72 = static_cast<int16_t>((root.x - 0x20) - head.x);
    head.delta_y_76 = static_cast<int16_t>((root.y + 0x18) - head.y);
    head.z = root.z;
}

/// Emulates State $08 Ascend (0x015E3C..0x015E56).
inline bool StepAscend(SerpentHeadEntity& head) noexcept {
    head.timer -= 1;
    if (head.timer != 0) {
        head.z += 3;
        return false;
    }
    head.state = 2;
    head.timer = 17;
    return true;
}

/// Emulates State $02 Hover & Ali Grab Probe (0x015E56..0x015EEA).
enum class GrabProbeResult { Descending, GrabMiss, GrabSuccess };

inline GrabProbeResult StepHoverGrabProbe(
    SerpentHeadEntity& head,
    PlayerTarget& ali,
    bool at_ground_boundary
) noexcept {
    if (!at_ground_boundary) {
        head.z -= 4;
        return GrabProbeResult::Descending;
    }

    bool is_airborne = (ali.z != 0);
    bool is_immune = ((ali.flags_38 & 0x08) != 0);
    int16_t dx = std::abs(static_cast<int16_t>((head.x + 4) - ali.x));
    int16_t dy = std::abs(static_cast<int16_t>((head.y + 16) - ali.y));

    if (is_airborne || is_immune || dx > 16 || dy > 16) {
        head.state = 4;
        head.anim_id = 0x34;
        head.timer = 17;
        head.multiplier_a6 = -head.multiplier_a6;
        head.multiplier_aa = -head.multiplier_aa;
        return GrabProbeResult::GrabMiss;
    }

    head.state = 6;
    head.anim_id = 0x36;
    head.timer = 17;
    head.multiplier_a6 = -head.multiplier_a6;
    head.multiplier_aa = -head.multiplier_aa;

    ali.lock_1982 = true;
    ali.flags_36 |= 0x02; // grabbed
    ali.flags_38 |= 0x0C; // flying / invulnerable
    return GrabProbeResult::GrabSuccess;
}

/// Emulates State $04 Return Ascend (0x015EEA..0x015F04).
inline bool StepReturnAscend(SerpentHeadEntity& head, bool at_top_boundary) noexcept {
    if (!at_top_boundary) {
        head.z += 4;
        return false;
    }
    head.state = 0;
    head.anim_id = 0x12;
    return true;
}

/// Emulates State $06 Slam Setup (0x015F04..0x015F34).
inline bool SetupSlamDirection(
    SerpentHeadEntity& head,
    int16_t arena_x,
    bool at_top_boundary
) noexcept {
    if (!at_top_boundary) {
        head.z += 4;
        return false;
    }
    head.timer = 7;
    head.state = (arena_x <= 0x100) ? 0x0A : 0x14;
    return true;
}

/// Emulates Rightward Slam Sequence (States $0A, $0C, $0E) (0x015F34..0x015FE0).
inline void StepRightwardSlam(SerpentHeadEntity& head, PlayerTarget& ali) noexcept {
    head.timer -= 1;
    if (head.state == 0x0A) {
        if (head.timer != 0) {
            head.delta_x_72 -= 3;
        } else {
            head.state = 0x0C;
            head.timer = 5;
        }
    } else if (head.state == 0x0C) {
        if (head.timer != 0) {
            head.delta_x_72 += 16;
        } else {
            head.state = 0x0E;
            head.timer = 13;
            head.anim_id = 0x12;
            head.flags_38 |= 0x08;

            ali.lock_1982 = false;
            ali.flags_36 &= ~0x02;
            ali.flags_38 &= 0xF3;
            ali.damage_86 = 0x8014; // 20 HP
            ali.x_vel = 0x00200000;
            ali.y_vel = 0;
            ali.z_vel = -1;
            ali.z -= 32;
            ali.delta_x += 4;
            ali.facing_16 = 1;
            ali.knockback_mode_84 = 1;
        }
    } else if (head.state == 0x0E) {
        if (head.timer != 0) {
            head.delta_x_72 -= 4;
        } else {
            head.state = 0;
            head.flags_38 &= ~0x08;
        }
    }
}

/// Emulates Leftward Slam Sequence (States $14, $16, recovery) (0x015FE0..0x01608C).
inline void StepLeftwardSlam(SerpentHeadEntity& head, PlayerTarget& ali) noexcept {
    head.timer -= 1;
    if (head.state == 0x14) {
        if (head.timer != 0) {
            head.delta_x_72 += 3;
        } else {
            head.state = 0x16;
            head.timer = 5;
        }
    } else if (head.state == 0x16) {
        if (head.timer != 0) {
            head.delta_x_72 -= 16;
        } else {
            head.state = 0x18;
            head.timer = 13;
            head.anim_id = 0x12;
            head.flags_38 |= 0x08;

            ali.lock_1982 = false;
            ali.flags_36 &= ~0x02;
            ali.flags_38 &= 0xF3;
            ali.damage_86 = 0x8014; // 20 HP
            ali.x_vel = -0x00200000;
            ali.y_vel = 0;
            ali.z_vel = -1;
            ali.z -= 32;
            ali.delta_x -= 4;
            ali.facing_16 = 1;
            ali.knockback_mode_84 = 3;
        }
    } else if (head.state == 0x18) {
        if (head.timer != 0) {
            head.delta_x_72 += 4;
        } else {
            head.state = 0;
            head.flags_38 &= ~0x08;
        }
    }
}

/// Emulates State $10 Death Fall & 4-Segment Staggered Despawn (0x01608C..0x0160E2).
inline bool StepDeathGravityFall(
    SerpentHeadEntity& head,
    std::array<ChildSegment, 4>& segments
) noexcept {
    if (head.z <= 0) return true;

    head.z_vel -= 0x0000C000;
    head.z += head.z_vel;

    if (head.z > 0) return false;

    head.z = 0;
    constexpr std::array<uint16_t, 4> TIMERS = {4, 8, 12, 16};
    for (size_t i = 0; i < 4; ++i) {
        segments[i].mode_66 = 3;
        segments[i].timer_2 = TIMERS[i];
    }
    return true;
}

} // namespace Thor::AI::SerpentStates
```

## 4. Behavioral Parity & Invariants
1. **Strict Grab Filter:** Airborne player (`z != 0`) or active invulnerability prevents grab regardless of proximity; miss handler immediately transitions to ascending escape (anim `$34`).
2. **Deterministic Arena Splitting:** Arena coordinate threshold `$FF1CE0 > 0x100` deterministically routes the slam attack leftward or rightward to guarantee the player is hurled toward open floor space rather than walls.
3. **High-Impact Kinetic Ejection:** The slam inflicts 20 damage (`$8014`) with a fixed horizontal impulse of 32.0 px (`0x00200000` or `-0x00200000`), forcing knockback modes 1 or 3 and clamping Z downward by 32 px.
4. **Cascading Segment Self-Destruction:** On head ground collision in State `$10`, a 4-beat staggered detonation sequence is primed into child segment slots at intervals of 4 frames (`4, 8, 12, 16`).
