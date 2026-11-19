# STANDALONE HANDOFF: Milestone M15.512 — ROM Composite Serpent/Multi-Segment Boss Sub-Modes 2/3, Hit-Flash Palette Toggle, Defense-Mitigated HP Subtraction, Knockback Trigger & 12-Entry State Jump Table (`0x015C4A..0x015E0E`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x015C4A..0x015E0E` (452 bytes total: 106 M68K instructions in `0x015C4A..0x015DF4`, 426 bytes; 13-entry relative state jump table in `0x015DF4..0x015E0E`, 26 bytes).
- **Core Responsibilities:**
  1. `0x015C4A..0x015CB6` (`AI_SerpentBoss_SubMode3_Activation`, 108 bytes, 25 insns):
     - Sub-Mode 3 Cutscene/Intro Activation. Traverses 9 sequential child slots (stride `0xBC`):
       - Body slots 1..4: sets anim `$10` (`+$2A = 0x10`).
       - Head slot 5: sets anim `$12` (`+$2A = 0x12`), clears `+$36` and `+$38`.
       - Tail/orb slots 6..9: clears `+$36`.
     - Restores root entity pointer `a6` from `a4`, clears root `+$36`, `+$38`, and timer `+$2 = 0`.
  2. `0x015CB6..0x015CCA` (`AI_SerpentBoss_SubMode2_Countdown`, 20 bytes, 6 insns):
     - Sub-Mode 2 Countdown & Death Trigger.
     - Decrements timer `+$2`. When expired (`timer == 0`), triggers boss death via `0x00E96C` with mode `d0 = 2`.
  3. `0x015CCA..0x015DF4` (`AI_SerpentBoss_SubMode2_MasterTick`, 298 bytes, 75 insns):
     - Active game guard: checks bit 0 of `$FF0E1D.l` (branches to inactive handler `0x015AFC` if paused).
     - Clears temporary fields: hitstun timer `+$83 = 0`, invulnerability flag `+$82 = 0`, reaction `+$9C = 0`.
     - Fast bypass for State `$12`: if `+$04 == 0x12`, branches directly to `0x0160E2`.
     - Hit-flash palette restoration: decrements `+$AF`. When it reaches 0, restores normal palette `$0200` across 5 linked segments (root + body slots 1..4).
     - Incoming damage handling when pending damage `+$86 != 0`:
       - Sets flash palette `$4200` across all 5 linked segments.
       - Sets flash duration `+$AF = 2` frames.
       - State 2 rebound: if current state is State 2 (slither / charge), transitions to State 4 with anim `$34`, reflects timer as `17 - timer` (clamped `>= 1`), and negates velocity vectors `+$A6 = -+$A6` and `+$AA = -+$AA`.
       - Defense mitigation: masks 12-bit damage `d0 & 0x0FFF`, subtracts defense `+$92` clamped to minimum 1 (`mitigated = max(1, damage - defense)`).
       - HP subtraction: `new_hp = max(0, hp - mitigated)`.
       - Death transition on `new_hp == 0`: switches to State `$10`, applies upward Z impulse `+$56 = +0x00020000` (+2.0 px), sets invulnerability bit 3 in `+$38`. If cutscene flag `$FF1982 != 0`, calls Ali helper `0x015F7C`, inflicting damage `$800A` on Ali, knockback `+$52 = +0x00030000` (+3.0 px), reaction mode 2, and sets serpent anim `$34`.
     - State jump table dispatch: dispatches via relative table `0x015DF4` using `+$04`.
  4. `0x015DF4..0x015E0E` (`AI_SerpentBoss_StateJumpTable`, 26 bytes, 13 entries):
     - 13 relative entries for States `$00..$18` (12 primary states `$00..$16` + State `$18`):
       - State `$00` $\to$ `0x015E0E` (+26)
       - State `$02` $\to$ `0x015E56` (+96)
       - State `$04` $\to$ `0x015EEA` (+242)
       - State `$06` $\to$ `0x015F04` (+266)
       - State `$08` $\to$ `0x015E3C` (+64)
       - State `$0A` $\to$ `0x015F34` (+310)
       - State `$0C` $\to$ `0x015F52` (+338)
       - State `$0E` $\to$ `0x015FC4` (+450)
       - State `$10` $\to$ `0x01608C` (+648)
       - State `$12` $\to$ `0x0160E2` (+732)
       - State `$14` $\to$ `0x015FE0` (+472)
       - State `$16` $\to$ `0x015FFE` (+500)
       - State `$18` $\to$ `0x016070` (+612)

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_serpent_boss_master_damage_m15_512.json`
- Extractor Tool: `tools/extract_rom_serpent_boss_master_damage.py`
- Test Suite: `tests/rom_serpent_boss_master_damage_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace Thor::BossSerpent {

// --- Palette & SFX Constants ---
inline constexpr uint16_t kNormalTilePaletteAttr     = 0x0200u;
inline constexpr uint16_t kHitFlashTilePaletteAttr   = 0x4200u;
inline constexpr uint8_t  kHitFlashDurationFrames    = 2u;
inline constexpr uint16_t kDeathModeId               = 2u;

// --- Physics & Impulse Constants ---
inline constexpr int32_t  kDeathZImpulse             = 0x00020000; // +2.0 px/frame
inline constexpr int32_t  kAliCutsceneKnockbackVy    = 0x00030000; // +3.0 px/frame
inline constexpr uint16_t kAliCutsceneDamageWord     = 0x800Au;
inline constexpr uint16_t kSerpentReboundAnimId      = 0x0034u;

// --- State IDs ---
enum class SerpentState : uint16_t {
    State00 = 0x00,
    State02 = 0x02,
    State04 = 0x04,
    State06 = 0x06,
    State08 = 0x08,
    State0A = 0x0A,
    State0C = 0x0C,
    State0E = 0x0E,
    State10 = 0x10,
    State12 = 0x12,
    State14 = 0x14,
    State16 = 0x16,
    State18 = 0x18,
};

// --- Jump Table Targets ---
inline constexpr std::array<uint32_t, 13> kStateJumpTargets{{
    0x015E0Eu, // State 0x00
    0x015E56u, // State 0x02
    0x015EEAu, // State 0x04
    0x015F04u, // State 0x06
    0x015E3Cu, // State 0x08
    0x015F34u, // State 0x0A
    0x015F52u, // State 0x0C
    0x015FC4u, // State 0x0E
    0x01608Cu, // State 0x10
    0x0160E2u, // State 0x12
    0x015FE0u, // State 0x14
    0x015FFEu, // State 0x16
    0x016070u, // State 0x18
}};

// --- Step Result Structures ---
struct DamageStepResult {
    bool     took_damage{false};
    uint16_t damage_applied{0};
    bool     entered_death{false};
    bool     rebound_triggered{false};
    bool     ali_hit_in_cutscene{false};
    SerpentState new_state{SerpentState::State00};
};

// --- Pure Step Functions ---

inline uint16_t ComputeMitigatedDamage(uint16_t incoming_damage, uint16_t defense) noexcept {
    const uint16_t masked = incoming_damage & 0x0FFFu;
    if (masked > defense) {
        return masked - defense;
    }
    return 1u; // Clamped to minimum 1
}

inline DamageStepResult ProcessIncomingDamage(
    uint16_t incoming_damage,
    uint16_t& current_hp,
    uint16_t defense,
    SerpentState& current_state,
    uint16_t& timer,
    int32_t&  vel_a6,
    int32_t&  vel_aa,
    uint8_t&  flash_timer,
    bool      cutscene_active) noexcept
{
    DamageStepResult res{};
    if (incoming_damage == 0) {
        return res;
    }

    res.took_damage = true;
    flash_timer = kHitFlashDurationFrames;

    // State 2 rebound
    if (current_state == SerpentState::State02) {
        current_state = SerpentState::State04;
        res.rebound_triggered = true;
        const int16_t mirrored = 17 - static_cast<int16_t>(timer);
        timer = (mirrored <= 0) ? 1u : static_cast<uint16_t>(mirrored);
        vel_a6 = -vel_a6;
        vel_aa = -vel_aa;
    }

    const uint16_t mitigated = ComputeMitigatedDamage(incoming_damage, defense);
    res.damage_applied = mitigated;

    if (mitigated >= current_hp) {
        current_hp = 0;
        current_state = SerpentState::State10;
        res.entered_death = true;
        if (cutscene_active) {
            res.ali_hit_in_cutscene = true;
        }
    } else {
        current_hp -= mitigated;
    }

    res.new_state = current_state;
    return res;
}

} // namespace Thor::BossSerpent
```
