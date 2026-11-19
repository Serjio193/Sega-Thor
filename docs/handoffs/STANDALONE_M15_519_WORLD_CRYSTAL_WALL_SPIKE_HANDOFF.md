# STANDALONE HANDOFF: Milestone M15.519 — ROM World Interactive Objects: Spirit Crystal Orb Switch, Breakable Wall Block & Floor Spike/Saw Hazard (`0x016CBE..0x016EBC`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x016CBE..0x016EBC` (510 bytes total: 132 M68K instructions across 3 routines).
- **Core Responsibilities:**
  1. `0x016CBE..0x016D5C` (`AI_World_SpiritCrystalOrbSwitch`, 158 bytes, 41 insns):
     - Interactive puzzle orb switch activated exclusively by Spirit elemental weapons / abilities.
     - **Sub-mode 0 (Initialization):** Sets height `z = +$8C`, queries persistent state bit via `0x018030`. If already set: sets solid collision flag `+$38 |= 8`, animation 2 (lit/activated), and flag `+$3C |= 2`. If not set: sets animation 0 (dormant/unlit).
     - **Sub-mode != 0 (Active Tick):**
       - Queries persistent flag; if already activated, skips processing.
       - Enforces 10-frame cooldown on `+$82`.
       - When incoming damage `+$86 != 0`: clears `+$86` and sets cooldown `+$82 = 10`.
       - Checks bit 13 of damage word (`btst #$D, d1`):
         - **Bit 13 == 0 (Physical Weapon):** Plays clink deflection SFX `#$900238` via `0x00E7FC`. Switch remains dormant.
         - **Bit 13 == 1 (Spirit Weapon):** Plays chime activation SFX `#$9003A1` via `0x00E7FC`, sets persistent bit in world RAM via `0x018030`, switches to animation 2 (lit), sets `+$3C |= 2`, and locks cooldown `+$82 = 0xFF`.
  2. `0x016D5C..0x016DCE` (`AI_World_BreakableWallBlock`, 114 bytes, 31 insns):
     - Breakable barrier/obstacle vulnerable exclusively to Spirit weapons.
     - **Sub-mode 0 (Initialization):** Loads obstacle health points from parameter `+$A0` into current HP `+$8A` and max HP `+$88`.
     - **Sub-mode != 0 (Active Tick):**
       - Queries persistent bit via `0x018030`; if already destroyed (carry set), returns.
       - Enforces 6-frame cooldown on `+$82`.
       - When incoming damage `+$86 != 0`: clears `+$86` and sets cooldown `+$82 = 6`.
       - Checks bit 13 of damage word (`btst #$D, d1`):
         - **Bit 13 == 0 (Physical Weapon):** Plays clink deflection SFX `#$900238` via `0x00E7FC`. No damage dealt.
         - **Bit 13 == 1 (Spirit Weapon):** Subtracts 12-bit damage points (`damage & 0x0FFF`) from current HP `+$8A`.
         - If remaining HP `<= 0`: sets persistent destruction bit via `0x018030`, elevates height `z += 16` (`+$10 += 16`), and invokes shatter debris spawner `0x00E96C(debris_type=1)`.
  3. `0x016DCE..0x016EBC` (`AI_World_FloorSpikeSawHazard`, 238 bytes, 60 insns):
     - Active floor trap / saw blade hazard with AABB collision, directional knockback, and blinking despawn.
     - **Sub-mode 0 (Active Hazard):**
       - Decrements sound timer `+$AA`. Every 53 frames (resets to 52 / `0x34`), plays saw buzz SFX `#$600262` via `0x00E7E8`.
       - Tests AABB contact vs Ali via `0x00B922` with box `[-14..+14, -8..+8]` and height check `z in [0..40]`.
       - Contact immunity check: if Ali is guarded (`Ali.+$38 & 8`), invulnerable (`Ali.+$82 != 0`), or in heavy hitstun (`Ali.+$83 >= 2`), hit is ignored.
       - **Vulnerable Contact Response:**
         - Sets attacker pointer `Ali.+$7E = hazard_ptr`, clears `Ali.+$82 = 0` and `Ali.+$9D = 0`.
         - Inflicts base damage `Ali.+$86 = 0x200F` (spike hazard flag `0x2000` | 15 damage).
         - If Ali is airborne (`Ali.+$10 != 0`) or already reacting (`Ali.+$83 != 0`), sets bit 15: `0x200F | 0x8000 = 0xA00F`.
         - Computes knockback angle via `0x00E338(ali_pos - hazard_pos) -> Ali.+$84`.
         - Computes directional impulse via `0x00E2D4` with speeds `x = 0x0400, y = 0x03C0` -> `Ali.+$4E` (`vx`), `Ali.+$52` (`vy`).
         - Imparts upward launch impulse `Ali.+$56 = +0x00020000` (+2.0 px/frame).
         - Sets hitstun timer `Ali.+$83 = 10` frames (0x0A).
         - Plays impale sound SFX `#$8004D9` via `0x00E7E8`.
     - **Sub-mode != 0 (Blinking Despawn):**
       - Increments timer `+$2`.
       - While timer `< 15`: toggles entity visibility bit 1 at `+$36` (`bchg #1, $36(a6)`).
       - At timer `>= 15`: despawns entity (`+$0 = 0`), queries `0x018030`, and sets persistent flag bit.

## 2. ROM Evidence Reference
- **Evidence JSON:** `data/thor-evidence/rom_world_crystal_wall_spike_m15_519.json`
- **Extractor Tool:** `tools/extract_rom_world_crystal_wall_spike.py`
- **Test Suite:** `tests/rom_world_crystal_wall_spike_test.py` (19 tests, 19/19 PASS)

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace Thor::WorldObjects {

// --- SFX Identifiers ---
inline constexpr uint32_t kSfxChimeActivation   = 0x009003A1u; // Spirit crystal chime
inline constexpr uint32_t kSfxClinkDeflection   = 0x00900238u; // Physical weapon clink
inline constexpr uint32_t kSfxSawBladeBuzz      = 0x00600262u; // Looping floor spike / saw buzz
inline constexpr uint32_t kSfxImpaleDamage      = 0x008004D9u; // Spike contact hit SFX

// --- Damage Flags & Masks ---
inline constexpr uint16_t kDamageMaskSpiritBit  = (1u << 13); // 0x2000
inline constexpr uint16_t kDamageMaskAirborne   = (1u << 15); // 0x8000
inline constexpr uint16_t kDamageMaskPoints     = 0x0FFFu;
inline constexpr uint16_t kHazardSpikeBaseDmg   = 0x200Fu; // Hazard type 0x2000 + 15 damage

// --- Collision Extents ---
struct AABBBox {
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;
    int16_t min_z;
    int16_t max_z;
};

inline constexpr AABBBox kFloorSpikeAABB{
    .min_x = -14,
    .min_y = -8,
    .max_x = 14,
    .max_y = 8,
    .min_z = 0,
    .max_z = 40,
};

// --- Target Ali State Context ---
struct AliHitContext {
    uint32_t attacker_ptr{0};
    uint16_t flag_38{0};        // Bit 3 = Guarded / immune
    uint8_t  cooldown_82{0};    // Invulnerability cooldown
    uint8_t  hitstun_83{0};     // Hitstun mode & timer
    uint8_t  flag_9d{0};
    int16_t  knockback_angle{0}; // +$84
    int32_t  x{0};
    int32_t  y{0};
    int32_t  z{0};              // +$10
    int32_t  vx{0};             // +$4E
    int32_t  vy{0};             // +$52
    int32_t  vz{0};             // +$56
    uint16_t damage_in{0};      // +$86
};

// --- Spirit Crystal Orb Switch ---
struct SpiritCrystalOrbSwitch {
    uint8_t  submode_66{0};
    uint8_t  cooldown_82{0};
    uint16_t param_8c_height{0};
    uint16_t anim_id{0};        // 0 = dormant, 2 = lit
    uint8_t  flag_38{0};        // Bit 3 = Solid
    uint8_t  flag_3c{0};        // Bit 1 = Active
    uint16_t damage_86{0};
    int32_t  z{0};
    uint16_t persistent_bit{0};
};

inline void StepSpiritCrystalOrbSwitch(
    SpiritCrystalOrbSwitch& orb,
    bool persistent_flag_set,
    auto&& set_persistent_fn,
    auto&& play_sfx_fn)
{
    if (orb.submode_66 == 0) {
        orb.submode_66 = 1;
        orb.z = orb.param_8c_height;
        if (persistent_flag_set) {
            orb.flag_38 |= (1 << 3);
            orb.anim_id = 2;
            orb.flag_3c |= (1 << 1);
        } else {
            orb.anim_id = 0;
        }
        return;
    }

    if (persistent_flag_set) {
        return;
    }

    if (orb.cooldown_82 > 0) {
        orb.cooldown_82--;
        return;
    }

    if (orb.damage_86 == 0) {
        return;
    }

    const uint16_t dmg = orb.damage_86;
    orb.damage_86 = 0;
    orb.cooldown_82 = 10;

    if ((dmg & kDamageMaskSpiritBit) != 0) {
        play_sfx_fn(kSfxChimeActivation);
        set_persistent_fn(orb.persistent_bit);
        orb.anim_id = 2;
        orb.flag_3c |= (1 << 1);
        orb.cooldown_82 = 0xFF; // locked on
    } else {
        play_sfx_fn(kSfxClinkDeflection);
    }
}

// --- Breakable Wall Block ---
struct BreakableWallBlock {
    uint8_t  submode_66{0};
    uint8_t  cooldown_82{0};
    uint16_t param_a0_hp{0};
    uint16_t hp_current_8a{0};
    uint16_t hp_max_88{0};
    uint16_t damage_86{0};
    int32_t  z{0};
    uint16_t persistent_bit{0};
};

inline void StepBreakableWallBlock(
    BreakableWallBlock& wall,
    bool persistent_flag_cleared,
    auto&& set_persistent_fn,
    auto&& spawn_debris_fn,
    auto&& play_sfx_fn)
{
    if (wall.submode_66 == 0) {
        wall.submode_66 = 1;
        wall.hp_current_8a = wall.param_a0_hp;
        wall.hp_max_88 = wall.param_a0_hp;
        return;
    }

    if (persistent_flag_cleared) {
        return;
    }

    if (wall.cooldown_82 > 0) {
        wall.cooldown_82--;
        return;
    }

    if (wall.damage_86 == 0) {
        return;
    }

    const uint16_t dmg = wall.damage_86;
    wall.damage_86 = 0;
    wall.cooldown_82 = 6;

    if ((dmg & kDamageMaskSpiritBit) != 0) {
        const uint16_t points = dmg & kDamageMaskPoints;
        if (wall.hp_current_8a <= points) {
            wall.hp_current_8a = 0;
            set_persistent_fn(wall.persistent_bit);
            wall.z += 16;
            spawn_debris_fn(1, wall.z);
        } else {
            wall.hp_current_8a -= points;
        }
    } else {
        play_sfx_fn(kSfxClinkDeflection);
    }
}

// --- Floor Spike / Saw Hazard ---
struct FloorSpikeSawHazard {
    uint16_t entity_type_0{0x45};
    uint8_t  submode_66{0};
    int16_t  sound_timer_aa{0x34}; // 52 frames
    uint16_t despawn_timer_2{0};
    uint8_t  flag_36{0};           // Bit 1 = Visible
    int32_t  x{0};
    int32_t  y{0};
    uint32_t ptr{0};
    uint16_t persistent_bit{0};
};

inline bool StepFloorSpikeSawHazard(
    FloorSpikeSawHazard& hazard,
    AliHitContext& ali,
    bool in_aabb_contact,
    auto&& compute_angle_fn,
    auto&& compute_velocity_fn,
    auto&& set_persistent_fn,
    auto&& play_sfx_fn)
{
    if (hazard.submode_66 == 0) {
        hazard.sound_timer_aa--;
        if (hazard.sound_timer_aa < 0) {
            hazard.sound_timer_aa = 0x34; // 52 frames
            play_sfx_fn(kSfxSawBladeBuzz);
        }

        if (!in_aabb_contact) {
            return false;
        }

        // Immunity / guard checks
        if ((ali.flag_38 & (1 << 3)) != 0 || ali.cooldown_82 != 0 || ali.hitstun_83 >= 2) {
            return false;
        }

        ali.attacker_ptr = hazard.ptr;
        ali.cooldown_82 = 0;
        ali.flag_9d = 0;

        uint16_t dmg = kHazardSpikeBaseDmg;
        const int32_t dx = ali.x - hazard.x;
        const int32_t dy = ali.y - hazard.y;
        ali.knockback_angle = compute_angle_fn(dx, dy);

        auto [vx, vy] = compute_velocity_fn(ali.knockback_angle, 0x0400, 0x03C0);
        ali.vx = vx;
        ali.vy = vy;
        ali.vz = 0x00020000; // +2.0 px launch

        if (ali.hitstun_83 != 0 || ali.z != 0) {
            dmg |= kDamageMaskAirborne;
        }

        ali.damage_in = dmg;
        ali.hitstun_83 = 10;
        play_sfx_fn(kSfxImpaleDamage);
        return true;
    }

    // Sub-mode != 0: Blinking Despawn
    hazard.despawn_timer_2++;
    if (hazard.despawn_timer_2 < 15) {
        hazard.flag_36 ^= (1 << 1); // Toggle visibility bit 1
    } else {
        hazard.entity_type_0 = 0; // Despawn
        set_persistent_fn(hazard.persistent_bit);
    }
    return false;
}

} // namespace Thor::WorldObjects
```
