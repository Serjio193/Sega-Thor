# STANDALONE HANDOFF: Milestone M15.521 — ROM World Interactive Objects: Sliding/Retracting Wall Trap, Falling Rock Boulder & Impact AABB (`0x0171C4..0x0173D6`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x0171C4..0x0173D6` (530 bytes total: 137 M68K instructions across 3 routines).
- **Core Responsibilities:**
  1. `0x0171C4..0x0172C2` (`AI_World_SlidingRetractingWallBridgeTrap`, 254 bytes, 67 insns):
     - Periodic sliding wall obstacle or retracting bridge trap.
     - **Sub-mode 0 (Initialization):** Sets sub-mode `+$66 = 1`, initializes timer `+$2 = +$A0`, height `z = +$8C`, records anchor coordinate `+$A6 = x`, clears offset `+$AA = 0`.
     - **Sub-mode 1 (Retracting / Retracted):**
       - Restores `x = +$A6` (anchor).
       - Decrements extension offset `+$AA` by 8 until 0.
       - Applies directional offset: if facing `+$16 == 1` adds `offset` to `x`, else subtracts `offset`.
       - Decrements timer `+$2`. While `+$2 > 0`, remains retracted.
       - When timer expires: checks persistent state bit via `0x018030`. If set (latched/locked), remains in sub-mode 1. If clear (or carry set), triggers extension: sets sub-mode `+$66 = 2`, sets timer `+$2 = +$9E`, and plays extend SFX `#$700391` via `0x00E7E8`.
     - **Sub-mode 2 (Extending / Extended):**
       - Restores `x = +$A6` (anchor). Sets flag `+$37:3`.
       - If offset `+$AA < 0x28` (40 px max extension): increments offset by 8 and clears flag `+$37:3` while in motion.
       - Applies directional offset to `x` according to `+$16 == 1`.
       - Calls animation tick `0x00BC8E` (`d0 = 0`).
       - Retract evaluation: checks persistent bit via `0x018030` and countdown timer `+$2`. If persistent bit becomes set or timer expires to 0, transitions back to sub-mode 1 and resets timer `+$2 = +$A0`.
  2. `0x0172C2..0x017344` (`AI_World_FallingRockBoulderTrap`, 130 bytes, 32 insns):
     - Falling ceiling rock / boulder obstacle with ballistic gravity, ground impact bounce, and screen shake.
     - **Sub-mode 0 (Initialization):** Sets `+$66 = 0xFF`, initializes height `z = +$8C`, sets aerial collision flag `+$38:6`.
     - **Active Tick (`z != 0`):**
       - Calls impact AABB subroutine `0x017344` to test collision vs Ali.
       - Applies gravity: `vz -= 0x0000C000` (-0.75 px/frame^2 in 16.16).
       - Integrates height: `z += vz`.
       - If `z < 0` (ground collision):
         - Clamps `z = 0`.
         - **First Bounce (`+$A6 == 0`):** Sets bounce flag `+$A6 = 0xFF`, sets `z = 1`, sets upward bounce velocity `vz = +0x00030000` (+3.0 px/frame), triggers screen shake amplitude 3 at `$FF17C4 = 3`, and plays heavy impact SFX `#$800386` via `0x00E7FC`.
         - **Second Landing (`+$A6 != 0`):** Clears `vz = 0`.
     - **Landed Handler (`z == 0`):** Increments fade / despawn counter `+$2E` each frame, unless camera/anchor scroll deltas `+$72 | +$76 != 0`.
  3. `0x017344..0x0173D6` (`AI_World_FallingRockImpactAABB`, 146 bytes, 38 insns):
     - Impact collision detection vs Ali for falling rocks and heavy environmental hazards.
     - Performs AABB collision query via `0x00B922`: box `[-10..+10, -10..+10]`, height `[0..24]`.
     - Immunity filter: if Ali is guarded (`Ali.+$38 & 8`) or invulnerable (`Ali.+$82 != 0`), ignores hit.
     - **Vulnerable Hit Response:**
       - Sets attacker pointer `Ali.+$7E = rock_ptr`.
       - Inflicts damage `Ali.+$86 = 0x8007` (bit 15 set | 7 HP).
       - Clears `Ali.+$82 = 0`, `Ali.+$83 = 0`, `Ali.+$9D = 0`.
       - Imparts upward launch velocity `Ali.+$56 = +0x00020000` (+2.0 px/frame).
       - Raises height `Ali.+$10 += 1` to separate from ground.
       - Computes knockback facing: flips bit 1 of current facing (`Ali.+$16 ^ 2 -> Ali.+$84`).
       - Computes directional velocities via `0x00E2D4` with magnitudes `x = 0x0310, y = 0x0300` -> `Ali.+$4E` (`vx`), `Ali.+$52` (`vy`).
       - Plays strike sound effect: if Ali entity type `Ali.+$0 == 2` plays `#$900237` via `0x00E7FC`, else `#$800346` via `0x00E7E8`.

## 2. ROM Evidence Reference
- **Evidence JSON:** `data/thor-evidence/rom_world_sliding_falling_traps_m15_521.json`
- **Extractor Tool:** `tools/extract_rom_world_sliding_falling_traps.py`
- **Test Suite:** `tests/rom_world_sliding_falling_traps_test.py` (15 tests, 15/15 PASS)

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <utility>

namespace Thor::WorldObjects::Traps {

// --- SFX Identifiers ---
inline constexpr uint32_t kSfxSlideWallExtend       = 0x00700391u; // Sliding wall extend sound
inline constexpr uint32_t kSfxRockHeavyImpact       = 0x00800386u; // Heavy boulder impact & bounce
inline constexpr uint32_t kSfxRockStrikeAli         = 0x00800346u; // Rock strike sound (Type != 2)
inline constexpr uint32_t kSfxRockStrikeAlt         = 0x00900237u; // Rock strike sound (Type == 2)

// --- Physics & Damage Constants ---
inline constexpr int32_t  kRockGravityStep          = 0x0000C000;  // -0.75 px/frame^2 (16.16)
inline constexpr int32_t  kRockBounceVelocity       = 0x00030000;  // +3.0 px/frame (16.16)
inline constexpr int32_t  kAliLaunchVelocity        = 0x00020000;  // +2.0 px/frame (16.16)
inline constexpr uint16_t kRockDamageWord           = 0x8007u;     // Bit 15 set | 7 HP
inline constexpr uint16_t kScreenShakeAmpRockBounce = 3u;

// --- AABB Extents ---
struct ImpactAABB {
    int16_t min_x{-10};
    int16_t min_y{-10};
    int16_t max_x{10};
    int16_t max_y{10};
    int16_t min_z{0};
    int16_t max_z{24};
};

inline constexpr ImpactAABB kFallingRockAABB{};

// --- Ali Context ---
struct AliHitContext {
    uint16_t entity_type_0{1};
    uint32_t attacker_ptr{0};
    uint16_t flag_38{0};        // Bit 3 = Guard
    uint8_t  cooldown_82{0};    // Invulnerability
    uint8_t  hitstun_83{0};
    uint8_t  flag_9d{0};
    uint16_t facing_16{0};
    int16_t  knockback_facing_84{0};
    int32_t  z{0};
    int32_t  vx{0};
    int32_t  vy{0};
    int32_t  vz{0};
    uint16_t damage_in{0};
};

// --- Sliding / Retracting Wall Trap ---
struct SlidingWallBridge {
    uint8_t  submode_66{0};
    uint16_t timer_2{0};
    uint16_t param_a0_retract_time{0};
    uint16_t param_9e_extend_time{0};
    uint16_t param_8c_height{0};
    uint16_t facing_16{0};
    uint8_t  flag_37{0};        // Bit 3 = extended lock
    int32_t  x{0};
    int32_t  z{0};
    int32_t  anchor_x_a6{0};
    int16_t  offset_aa{0};      // 0..40 (0x28)
    uint16_t persistent_bit{0};
};

inline void StepSlidingWallBridge(
    SlidingWallBridge& wall,
    bool persistent_flag_set,
    auto&& animate_fn,
    auto&& play_sfx_fn)
{
    if (wall.submode_66 == 0) {
        wall.submode_66 = 1;
        wall.timer_2 = wall.param_a0_retract_time;
        wall.z = wall.param_8c_height;
        wall.anchor_x_a6 = wall.x;
        wall.offset_aa = 0;
        return;
    }

    if (wall.submode_66 == 1) {
        wall.x = wall.anchor_x_a6;
        if (wall.offset_aa > 0) {
            wall.offset_aa = (wall.offset_aa >= 8) ? (wall.offset_aa - 8) : 0;
        }

        if (wall.facing_16 == 1) {
            wall.x += wall.offset_aa;
        } else {
            wall.x -= wall.offset_aa;
        }

        if (wall.timer_2 > 0) {
            wall.timer_2--;
            if (wall.timer_2 != 0) {
                return;
            }
        }

        if (persistent_flag_set) {
            return;
        }

        wall.submode_66 = 2;
        wall.timer_2 = wall.param_9e_extend_time;
        play_sfx_fn(kSfxSlideWallExtend);
        return;
    }

    if (wall.submode_66 == 2) {
        wall.x = wall.anchor_x_a6;
        wall.flag_37 |= (1 << 3);

        if (wall.offset_aa < 0x28) {
            wall.offset_aa += 8;
            if (wall.offset_aa > 0x28) wall.offset_aa = 0x28;
            wall.flag_37 &= ~(1 << 3);
        }

        if (wall.facing_16 == 1) {
            wall.x += wall.offset_aa;
        } else {
            wall.x -= wall.offset_aa;
        }

        animate_fn();

        bool retract = persistent_flag_set;
        if (!retract) {
            if (wall.timer_2 == 0) {
                retract = true;
            } else {
                wall.timer_2--;
                if (wall.timer_2 == 0) {
                    retract = true;
                }
            }
        }

        if (retract) {
            wall.submode_66 = 1;
            wall.timer_2 = wall.param_a0_retract_time;
        }
    }
}

// --- Falling Rock Boulder Trap ---
struct FallingRockBoulder {
    uint8_t  submode_66{0};
    uint8_t  bounced_a6{0};
    uint8_t  flag_38{0};        // Bit 6 = Aerial
    uint16_t param_8c_height{0};
    uint16_t fade_2e{0};
    int32_t  z{0};
    int32_t  vz{0};
    uint32_t ptr{0};
};

inline void StepFallingRockBoulder(
    FallingRockBoulder& rock,
    AliHitContext& ali,
    bool in_aabb_contact,
    int32_t scroll_dx,
    int32_t scroll_dy,
    auto&& shake_screen_fn,
    auto&& play_sfx_fn)
{
    if (rock.submode_66 == 0) {
        rock.submode_66 = 0xFF;
        rock.z = rock.param_8c_height;
        rock.flag_38 |= (1 << 6);
        return;
    }

    if (rock.z != 0) {
        // Impact check vs Ali
        if (in_aabb_contact && (ali.flag_38 & (1 << 3)) == 0 && ali.cooldown_82 == 0) {
            ali.attacker_ptr = rock.ptr;
            ali.damage_in = kRockDamageWord;
            ali.cooldown_82 = 0;
            ali.hitstun_83 = 0;
            ali.flag_9d = 0;
            ali.vz = kAliLaunchVelocity;
            ali.z += 1;
            ali.knockback_facing_84 = ali.facing_16 ^ 2;
            ali.vx = (ali.knockback_facing_84 == 0 || ali.knockback_facing_84 == 1) ? 0x310 : -0x310;
            ali.vy = (ali.knockback_facing_84 == 0 || ali.knockback_facing_84 == 2) ? 0x300 : -0x300;

            if (ali.entity_type_0 == 2) {
                play_sfx_fn(kSfxRockStrikeAlt);
            } else {
                play_sfx_fn(kSfxRockStrikeAli);
            }
        }

        rock.vz -= kRockGravityStep;
        rock.z += rock.vz;

        if (rock.z < 0) {
            rock.z = 0;
            if (rock.bounced_a6 == 0) {
                rock.bounced_a6 = 0xFF;
                rock.z = 1;
                rock.vz = kRockBounceVelocity;
                shake_screen_fn(kScreenShakeAmpRockBounce);
                play_sfx_fn(kSfxRockHeavyImpact);
            } else {
                rock.vz = 0;
            }
        }
    } else {
        if ((scroll_dx | scroll_dy) == 0) {
            rock.fade_2e++;
        }
    }
}

} // namespace Thor::WorldObjects::Traps
```
