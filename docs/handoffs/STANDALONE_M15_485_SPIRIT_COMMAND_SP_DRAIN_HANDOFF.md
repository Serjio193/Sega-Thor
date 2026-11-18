# STANDALONE HANDOFF: Milestone M15.485 — ROM Spirit Button A Command Decoder, Shared SP Guard & Dytto Master Tick / SP Drain

## 1. Subsystem Overview
- **ROM Address Range:** `0x011A4C..0x011BA0` (340 bytes, 79 instructions).
- **Core Responsibilities:**
  1. `0x011A4C..0x011B28` (`Spirit_PollButtonACommandInput`, 220 bytes, 47 instructions): Evaluates cutscene/lock flags `$FF1982`, `$FF1983`, and `$FF19A7`. When unlocked, samples Button A (`$FF165E & 0x40`), guards post-lock/post-command held presses via `$FF19A0 = 0xFF` until release, ticks the 6-frame double-tap countdown `$FF19A1`, increments hold counter `$FF199E` (20-frame / `0x14` threshold), latches `$FF165E -> $FF19A3` and `$FF19FE -> $FF19A4` on initial press, and writes command code `$FF19A2` (`0` = none, `1` = single tap, `2` = double tap, `3` = 20+ frame hold release).
  2. `0x011B28..0x011B4E` (`Spirit_TryConsumePlayerSP`, 38 bytes, 11 instructions): Shared spirit ability SP-cost guard called across spirit handlers. Reads Ali's SP `+$8E(a5)`, subtracts `d0.w`, commits the updated SP and clears Carry (`CCR &= ~1`) if signed result `>= 1`, or leaves SP unchanged and sets Carry (`CCR |= 1`) if `< 1` (preserving CCR across `movem.l (a7)+, d1`).
  3. `0x011B4E..0x011BA0` (`AI_Dytto_MasterTickAndSpDrain`, 82 bytes, 21 instructions): Master update entry for Dytto (`0x0018`). Loads `a5 = $FF19E8`; if sub-entity flag `+$A2(a6) != 0` (e.g. projectile sub-entities), jumps directly to state dispatcher `0x011BA0`. Otherwise invokes `0x011A4C` and entity collision/bound helper `0x00E36E`, ticks the 45-frame (`0x2D`) timer `+$8E(a6)` to drain 1 SP from Ali `+$8E(a5)` (branching to spirit dismissal `0x012314` when SP is 0 before or after decrement), and toggles low-SP blink bit 1 (`0x02`) on `+$36(a6)` when `+$8E(a5) <= 1` (clearing bit 1 when `> 1`).

### RAM & Field Map
| Address / Offset | Size | Role |
|---|---|---|
| `$FF165E` | `uint8` | Controller port 1 button state; bit 6 (`0x40`) = Button A |
| `$FF1982`, `$FF1983`, `$FF19A7` | `uint8` | Cutscene / lock flags suppressing Button A command polling when non-zero |
| `$FF199E` | `uint16` | Button A hold duration counter (threshold `0x14` = 20 frames) |
| `$FF19A0` | `uint8` | Previous Button A sample (`0x00`, `0x40`, or `0xFF` suppression sentinel) |
| `$FF19A1` | `uint8` | 6-frame (`0x06`) double-tap countdown window |
| `$FF19A2` | `uint8` | Output spirit command code (`0` = none, `1` = single tap, `2` = double tap, `3` = hold release) |
| `$FF19A3` | `uint8` | Latched `$FF165E` input byte captured on initial Button A press |
| `$FF19A4` | `uint16` | Latched `$FF19FE` player facing word captured on initial Button A press |
| `$FF19E8` (`a5`) | struct | Player (Ali) entity record base; `+$8E(a5)` = current SP (`uint16`), `+$16(a5)` = `$FF19FE` |
| `a6` (`$FF1AA4`) | struct | Spirit entity record; `+$36(a6)` bit 1 = low-SP blink flag, `+$8E(a6)` = SP drain timer, `+$A2(a6)` = sub-entity bypass flag |

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_spirit_command_sp_drain_m15_485.json`
- Extractor: `tools/extract_rom_spirit_command_sp_drain.py`
- Test: `tests/rom_spirit_command_sp_drain_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::AI {

constexpr uint8_t  kSpiritButtonAMask           = 0x40;
constexpr uint16_t kSpiritHoldThresholdFrames   = 0x0014;  // 20 frames
constexpr uint8_t  kSpiritDoubleTapWindowFrames = 6;
constexpr uint16_t kDyttoSpDrainIntervalFrames  = 0x002D;  // 45 frames
constexpr uint8_t  kSpiritLowSpBlinkBitMask     = 0x02;
constexpr int16_t  kSpiritMinRemainingSp        = 1;

enum class SpiritCommandCode : uint8_t {
    None        = 0,
    SingleTap   = 1,
    DoubleTap   = 2,
    HoldRelease = 3,
};

struct SpiritCommandInputState {
    uint16_t hold_counter{0};        // $FF199E
    uint8_t  prev_button_a{0};       // $FF19A0 (0x00, 0x40, or 0xFF)
    uint8_t  double_tap_timer{0};    // $FF19A1
    uint8_t  command_out{0};         // $FF19A2
    uint8_t  latched_input{0};       // $FF19A3
    uint16_t latched_facing{0};      // $FF19A4
};

// Cleanroom specification of 0x011A4C..0x011B28 (Spirit_PollButtonACommandInput).
inline SpiritCommandCode PollButtonACommandInput(
    SpiritCommandInputState& state,
    uint8_t input_byte,              // $FF165E
    uint16_t player_facing,          // $FF19FE
    bool locked                      // $FF1982 || $FF1983 || $FF19A7
) noexcept {
    if (locked) {
        state.hold_counter = 0;
        state.double_tap_timer = 0;
        state.prev_button_a = 0xFF;
        state.command_out = 0;
        return SpiritCommandCode::None;
    }

    const uint8_t button_a = static_cast<uint8_t>(input_byte & kSpiritButtonAMask);
    uint8_t prev_a = state.prev_button_a;
    if ((prev_a & 0x80) != 0) {
        if (button_a != 0) {
            state.command_out = 0;
            return SpiritCommandCode::None;
        }
        prev_a = 0;
    }
    state.prev_button_a = button_a;

    if (state.double_tap_timer != 0) {
        if (--state.double_tap_timer == 0) {
            state.hold_counter = 0;
            state.prev_button_a = 0xFF;
            state.command_out = static_cast<uint8_t>(SpiritCommandCode::SingleTap);
            return SpiritCommandCode::SingleTap;
        }
    }

    if (button_a != 0) {
        ++state.hold_counter;
        if (prev_a != 0) {
            state.command_out = 0;
            return SpiritCommandCode::None;
        }
        if (state.double_tap_timer != 0) {
            state.hold_counter = 0;
            state.double_tap_timer = 0;
            state.prev_button_a = 0xFF;
            state.command_out = static_cast<uint8_t>(SpiritCommandCode::DoubleTap);
            return SpiritCommandCode::DoubleTap;
        }
        state.latched_input = input_byte;
        state.latched_facing = player_facing;
        state.command_out = 0;
        return SpiritCommandCode::None;
    }

    if (prev_a == 0) {
        state.hold_counter = 0;
        state.command_out = 0;
        return SpiritCommandCode::None;
    }

    if (state.hold_counter >= kSpiritHoldThresholdFrames) {
        state.hold_counter = 0;
        state.double_tap_timer = 0;
        state.prev_button_a = 0xFF;
        state.command_out = static_cast<uint8_t>(SpiritCommandCode::HoldRelease);
        return SpiritCommandCode::HoldRelease;
    }

    state.hold_counter = 0;
    state.double_tap_timer = kSpiritDoubleTapWindowFrames;
    state.command_out = 0;
    return SpiritCommandCode::None;
}

// Cleanroom specification of 0x011B28..0x011B4E (Spirit_TryConsumePlayerSP).
// Returns true on success (remaining SP >= 1, Carry clear), false on rejection (Carry set).
inline bool TryConsumePlayerSp(int16_t& player_sp, int16_t cost) noexcept {
    const int16_t remaining = static_cast<int16_t>(player_sp - cost);
    if (remaining < kSpiritMinRemainingSp) {
        return false;
    }
    player_sp = remaining;
    return true;
}

enum class DyttoMasterStepOutcome : uint8_t {
    ContinueStateDispatch,  // 0x011BA0
    TriggerDismissal,       // 0x012314
};

struct DyttoMasterEntityFields {
    uint8_t  render_flags_36{0};     // +$36(a6), bit 1 toggled when SP <= 1
    uint16_t drain_timer_8e{kDyttoSpDrainIntervalFrames};  // +$8E(a6)
    uint8_t  sub_entity_flag_a2{0};  // +$A2(a6)
};

// Cleanroom specification of 0x011B4E..0x011BA0 (AI_Dytto_MasterTickAndSpDrain)
// SP drain & blink logic (invoking PollButtonACommandInput + helper 0x00E36E when +$A2 == 0).
inline DyttoMasterStepOutcome StepDyttoSpDrainAndBlink(
    DyttoMasterEntityFields& spirit,
    uint16_t& player_sp              // +$8E(a5)
) noexcept {
    if (spirit.sub_entity_flag_a2 != 0) {
        return DyttoMasterStepOutcome::ContinueStateDispatch;
    }

    if (spirit.drain_timer_8e != 0) {
        --spirit.drain_timer_8e;
    } else {
        spirit.drain_timer_8e = kDyttoSpDrainIntervalFrames;
        if (player_sp == 0) {
            return DyttoMasterStepOutcome::TriggerDismissal;
        }
        --player_sp;
        if (player_sp == 0) {
            return DyttoMasterStepOutcome::TriggerDismissal;
        }
    }

    if (player_sp > 1) {
        spirit.render_flags_36 &= static_cast<uint8_t>(~kSpiritLowSpBlinkBitMask);
    } else {
        spirit.render_flags_36 ^= kSpiritLowSpBlinkBitMask;
    }
    return DyttoMasterStepOutcome::ContinueStateDispatch;
}

} // namespace Thor::AI
```
