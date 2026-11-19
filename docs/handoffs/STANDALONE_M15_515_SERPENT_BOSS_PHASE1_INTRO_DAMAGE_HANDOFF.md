# STANDALONE HANDOFF: Milestone M15.515 — ROM Serpent Boss Phase 1 Intro Cutscene, Palette Crossfades, Active Master Tick, 5-Segment Hit-Flash, Death Transition & 13-Entry State Jump Table (`0x016294..0x0164E2`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x016294..0x0164E2` (590 bytes total: 142 M68K instructions in `0x016294..0x0164C8`, 564 bytes; 13-entry relative state jump table in `0x0164C8..0x0164E2`, 26 bytes).
- **Core Responsibilities:**
  1. `0x016294..0x0163A4` (`AI_SerpentBoss_Phase1IntroCutscene`, 272 bytes, 62 insns):
     - Cutscene Sequencer. If active combat flag `$FF0E1D:0` is set, branches immediately to active tick `0x0163A4`.
     - Otherwise increments cutscene timer `+$B0(a6)` and advances through 6 stages:
       - **Stage 1 (`timer < 90`):** calls standby/idle `0x015AFC`.
       - **Stage 2 (`timer in 90..122`):** crossfades CRAM line 3 `$FF136C` from `$5D0E6` to `$5D106` via `0x00302E` with `step = timer - 90`. At `step == 0`, plays rumble SFX `$700219`. At `step == 32`, sets `$FF20AE = 0x12` and calls `$DAE6`.
       - **Stage 3 (`timer in 123..149`):** calls standby/idle `0x015AFC`.
       - **Stage 4 (`timer in 150..182`):** crossfades CRAM line 4 `$FF13AC` from `$5D0E6` to `$5D106` with `step = timer - 150`. At `step == 32`, sets normal palette `$0200` across 5 linked segments (root + 4 body slots) and plays rumble SFX `$700219`.
       - **Stage 5 (`timer in 183..210`):** calls standby/idle `0x015AFC`.
       - **Stage 6 (`timer >= 211`):** cutscene finalization. Calls Sub-Mode 3 init `0x015C4A` (sets body anims `$10`, head anim `$12`), copies 32 bytes from `$5D126` to `$FF13AC`, sets bit 0 of `$FF164D`, triggers room/audio event `$102` via `$DAC2`, and activates active combat by setting bit 0 of `$FF0E1D`.
  2. `0x0163A4..0x0164C8` (`AI_SerpentBoss_Phase1ActiveMasterTick`, 292 bytes, 80 insns):
     - Active Combat Master Tick.
     - Clears temporary fields: `+$72 = 0`, `+$76 = 0`, `+$83 = 0`, `+$9C = 0`.
     - Invulnerability timer `+$AE`: asserts `+$82 = 0xFF` while `+$AE > 0`; clears `+$82 = 0` when `+$AE == 0`.
     - Hit-flash palette restoration `+$AF`: restores normal palette `$0200` across 5 linked segments upon reaching 0 (provided HP != 0).
     - Incoming damage check when `+$86 != 0`:
       - Sets flash palette `$4200` across all 5 linked segments.
       - Sets flash timer `+$AF = 2`.
       - Heavy damage check: if bit 14 or 15 set (`damage & 0xC000 != 0`), sets `+$AE = 2` and `+$82 = 0xFF` (gives 2 frames of invulnerability).
       - Defense mitigation: masks 12-bit damage `d0 & 0x0FFF`, subtracts defense `+$92` (clamped to minimum 1: `mitigated = max(1, damage - defense)`).
       - HP subtraction: `new_hp = max(0, hp - mitigated)`.
       - Death transition on `new_hp == 0`:
         - Sets State `$16` (`+$04 = 0x16`).
         - Sets anim 8 (`+$2A = 8`).
         - Applies upward Z impulse `+$56 = +0x00020000` (+2.0 px).
         - Sets flags `+$38 |= 0x0C` (bits 2 and 3).
         - Sets global defeat flag `$FF210A = 0x0FFF`.
         - Calls entity transition `0x00E96C` with mode `d0 = 10` for each of the 4 child slots (stride `$BC`).
         - Exits immediately (`rts`), skipping jump table dispatch.
     - State jump table dispatch: dispatches via relative table `0x0164C8` using `+$04`.
  3. `0x0164C8..0x0164E2` (`AI_SerpentBoss_Phase1StateJumpTable`, 26 bytes, 13 entries):
     - 13 relative offset entries for States `$00..$18`:
       - State `$00` $\to$ `0x0164E2` (+26)
       - State `$02` $\to$ `0x01652E` (+100)
       - State `$04` $\to$ `0x01658A` (+190)
       - State `$06` $\to$ `0x0165AE` (+224)
       - State `$08` $\to$ `0x0165C4` (+244)
       - State `$0A` $\to$ `0x0165D0` (+254)
       - State `$0C` $\to$ `0x0165E2` (+270)
       - State `$0E` $\to$ `0x01663A` (+356)
       - State `$10` $\to$ `0x016690` (+440)
       - State `$12` $\to$ `0x0166D2` (+504)
       - State `$14` $\to$ `0x0166F6` (+538)
       - State `$16` $\to$ `0x0164F2` (+20)
       - State `$18` $\to$ `0x01651C` (+60)

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_serpent_boss_phase1_intro_damage_m15_515.json`
- Extractor Tool: `tools/extract_rom_serpent_boss_phase1_intro_damage.py`
- Test Suite: `tests/rom_serpent_boss_phase1_intro_damage_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>

namespace Thor::BossSerpent::Phase1 {

// --- Palette & Audio Constants ---
inline constexpr uint16_t kNormalTilePaletteAttr     = 0x0200u;
inline constexpr uint16_t kHitFlashTilePaletteAttr   = 0x4200u;
inline constexpr uint32_t kIntroRumbleSfxId          = 0x00700219u;
inline constexpr uint16_t kMusicEventId              = 0x0102u;
inline constexpr uint16_t kDeathAnimId               = 8u;
inline constexpr int32_t  kDeathZImpulse             = 0x00020000; // +2.0 px/frame
inline constexpr uint16_t kChildDeathModeId          = 10u;
inline constexpr uint16_t kDefeatFlagWord            = 0x0FFFu;

// --- Cutscene Stage Timers ---
inline constexpr uint16_t kCutsceneStage1Max         = 90u;
inline constexpr uint16_t kCutsceneStage2Max         = 123u;
inline constexpr uint16_t kCutsceneStage3Max         = 150u;
inline constexpr uint16_t kCutsceneStage4Max         = 183u;
inline constexpr uint16_t kCutsceneStage5Max         = 211u;

// --- Jump Table Targets for Phase 1 ---
inline constexpr std::array<uint32_t, 13> kPhase1StateJumpTargets{{
    0x0164E2u, // State 0x00
    0x01652Eu, // State 0x02
    0x01658Au, // State 0x04
    0x0165AEu, // State 0x06
    0x0165C4u, // State 0x08
    0x0165D0u, // State 0x0A
    0x0165E2u, // State 0x0C
    0x01663Au, // State 0x0E
    0x016690u, // State 0x10
    0x0166D2u, // State 0x12
    0x0166F6u, // State 0x14
    0x0164F2u, // State 0x16
    0x01651Cu, // State 0x18
}};

// --- Step Result Structures ---
struct CutsceneStepResult {
    bool     combat_activated{false};
    bool     play_rumble_sfx{false};
    bool     trigger_dae6{false};
    bool     set_normal_palette{false};
    uint8_t  cram_ff136c_crossfade_step{0};
    uint8_t  cram_ff13ac_crossfade_step{0};
};

struct ActiveTickResult {
    bool     took_damage{false};
    uint16_t damage_applied{0};
    bool     heavy_invuln_triggered{false};
    bool     death_triggered{false};
    uint16_t state_dispatch{0};
};

// --- Pure Step Functions ---

inline CutsceneStepResult StepPhase1Cutscene(
    uint16_t& timer_b0,
    bool combat_already_active) noexcept
{
    CutsceneStepResult res{};
    if (combat_already_active) {
        res.combat_activated = true;
        return res;
    }

    ++timer_b0;

    if (timer_b0 >= kCutsceneStage1Max && timer_b0 < kCutsceneStage2Max) {
        const uint8_t step = static_cast<uint8_t>(timer_b0 - kCutsceneStage1Max);
        res.cram_ff136c_crossfade_step = step;
        if (step == 0) {
            res.play_rumble_sfx = true;
        } else if (step == 32u) {
            res.trigger_dae6 = true;
        }
    } else if (timer_b0 >= kCutsceneStage3Max && timer_b0 < kCutsceneStage4Max) {
        const uint8_t step = static_cast<uint8_t>(timer_b0 - kCutsceneStage3Max);
        res.cram_ff13ac_crossfade_step = step;
        if (step == 32u) {
            res.set_normal_palette = true;
            res.play_rumble_sfx = true;
        }
    } else if (timer_b0 >= kCutsceneStage5Max) {
        res.combat_activated = true;
    }

    return res;
}

inline ActiveTickResult ProcessPhase1Damage(
    uint16_t incoming_damage,
    uint16_t& current_hp,
    uint16_t defense,
    uint16_t current_state,
    uint8_t&  flash_timer,
    uint8_t&  invuln_timer) noexcept
{
    ActiveTickResult res{};
    if (incoming_damage == 0) {
        res.state_dispatch = current_state;
        return res;
    }

    res.took_damage = true;
    flash_timer = 2u;

    if ((incoming_damage & 0xC000u) != 0) {
        invuln_timer = 2u;
        res.heavy_invuln_triggered = true;
    }

    const uint16_t masked = incoming_damage & 0x0FFFu;
    const uint16_t mitigated = (masked > defense) ? (masked - defense) : 1u;
    res.damage_applied = mitigated;

    if (mitigated >= current_hp) {
        current_hp = 0;
        res.death_triggered = true;
        res.state_dispatch = 0x16u; // State 0x16
    } else {
        current_hp -= mitigated;
        res.state_dispatch = current_state;
    }

    return res;
}

} // namespace Thor::BossSerpent::Phase1
```
