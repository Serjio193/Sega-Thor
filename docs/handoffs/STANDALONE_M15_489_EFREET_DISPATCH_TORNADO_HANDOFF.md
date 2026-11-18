# STANDALONE HANDOFF: Milestone M15.489 — ROM Dytto Water Tornado Steering/Bounce & Efreet Spirit Master 14-State Dispatcher / Command Starters

## 1. Subsystem Overview
- **ROM Address Range:** `0x0123E2..0x012680` (670 bytes: 188 instructions across `0x0123E2..0x0125AE` and `0x0125CA..0x012680` + 28-byte 14-entry relative jump table at `0x0125AE..0x0125CA`).
- **Core Responsibilities:**
  1. `0x0123E2..0x01254C` (`AI_Dytto_WaterTornadoSteeringAndTrigVelocity`, 362 bytes, 120 insns):
     - Calls `0x0123AE` (`d4 = 8` px hover tracking), loads angle `+$4E(a6)`, turn rate `+$50(a6)`, drift timer `+$51(a6)`, and speed `+$52(a6)`.
     - Reflects `+$40(a6)` bit 6 (horizontal wall: `(0x80 - d3)` if `<= 0x80`, else `(0x180 - d3)` + `neg.b d4`) and `+$41(a6)` bit 6 (vertical wall: `(0x100 - d3) & 0xFF` + `neg.b d4`).
     - Clamps/bounces against viewport `[cam_x..cam_x+$140, cam_y..cam_y+$E0]` using `$FF1716` or `$FF173E` (selected by `$FF16F0` bit 7): sets angle `0x00 / 0x80 / 0xC0 / 0x40`, halves turn rate `asr.b #1`, sets drift timer `1`.
     - If Button A (`$FF165E` bit 6) held: clears `$FF199E`, calls `0x00E706` for target angle toward Ali, steers shortest-arc with max step `8`, zeroes `+$50`, sets drift timer `10` (`0x0A`). Else decrements drift timer; on `0`, reloads `7` and sets `+$50 = (prng & 8) - 4` (`0x00E268`), adding `+$50` to angle.
     - Ramps `+$52 <= $80000` by `+$2000`, maps angle sectors (`$20/$60/$A0/$E0` -> `1, 0, 3, 2, 1`) into `+$16(a6)`, and integrates `(speed >> 7) * cos` into `+$72` and `-(speed >> 7) * sin` into `+$76` via `$05D706`.
  2. `0x01254C..0x0125CA` (`AI_Efreet_MasterTickAndDispatcher`, 126 bytes: 98 bytes / 26 insns + 28-byte jump table):
     - Loads `a5 = $FF19E8`; if `+$A2(a6) != 0` jumps directly to state dispatcher `0x0125A2`.
     - Calls `0x00E682` (hitstun early-return guard), `0x011A4C` (`Spirit_PollButtonACommandInput`), `0x00E36E`.
     - Ticks 35-frame (`0x23`) SP drain timer `+$8E(a6)` to deduct `1` SP from `+$8E(a5)` (dismissing via `0x012BA8` if SP is `0` before or after decrement); toggles `+$36(a6)` bit 1 when `SP <= 1` and clears it when `SP > 1`.
     - Dispatches `+$04(a6)` (`$00..$1A`) via the 14-entry signed word self-relative table at `0x0125AE..0x0125CA`.
  3. `0x0125CA..0x012608` (`AI_Efreet_TryStartDashCharge`, 62 bytes, 15 insns; `$FF19A2 == 2` at `0x0129A0`):
     - Consumes `8` SP via `0x011B28` (else buzzer `0x012978`), sets State `$08`, `+$2A = $0C`, copies Ali's facing `+$16(a5)` -> `+$16(a6)`, sets velocity `(0x0100, 0x0100)` via `0x00E2D4` into `+$4E/+$52`, plays SFX `#$800258` (`0x00E7FC`).
  4. `0x012608..0x01263C` (`AI_Efreet_TryStartFlameBreath`, 52 bytes, 12 insns; `$FF19A2 == 1` at `0x012998`):
     - Consumes `6` SP via `0x011B28` and requires `$FF1C1C` `+$00 == 0` (else `0x012978`), reserves `+$00 = $FFFE` into `+$A6(a6)`, calls `0x012C4E`, sets State `$0A`, `+$2A = 2`.
  5. `0x01263C..0x012672` (`AI_Efreet_TryStartFireballMeltdown`, 54 bytes, 12 insns; `$FF19A2 == 3` at `0x0129A8`):
     - Consumes `25` (`$19`) SP via `0x011B28` (else `0x012978`), sets State `$0C`, `+$2A = 8`, facing `2`, `+$AE = $60`, `+$02 = 0`, `+$AF = $FF0DB7 + $10`.
  6. `0x012672..0x012680` (`AI_Efreet_StartHitReaction`, 14 bytes, 3 insns):
     - Sets State `$10`, `+$2A = $0A`.

### Efreet 14-Entry Relative Jump Table (`0x0125AE..0x0125CA`)
| State | Slot Addr | Disp | Target | Role |
|---|---|---|---|---|
| `$00` | `0x0125AE` | `+0x03DA` | `0x012988` | Idle / command & follow |
| `$02` | `0x0125B0` | `+0x03D8` | `0x012988` | Idle / command & follow |
| `$04` | `0x0125B2` | `+0x00CE` | `0x012680` | State `$04` |
| `$06` | `0x0125B4` | `+0x063C` | `0x012BF0` | Dismissal sub-state |
| `$08` | `0x0125B6` | `+0x00F6` | `0x0126AC` | Dash Charge update |
| `$0A` | `0x0125B8` | `+0x01EC` | `0x0127A4` | Flame Breath update |
| `$0C` | `0x0125BA` | `+0x0248` | `0x012802` | Fireball Meltdown update |
| `$0E` | `0x0125BC` | `+0x0204` | `0x0127C0` | State `$0E` |
| `$10` | `0x0125BE` | `+0x03CA` | `0x012988` | Hit reaction / idle entry |
| `$12` | `0x0125C0` | `+0x010E` | `0x0126CE` | State `$12` |
| `$14` | `0x0125C2` | `+0x0140` | `0x012702` | State `$14` |
| `$16` | `0x0125C4` | `+0x0350` | `0x012914` | State `$16` |
| `$18` | `0x0125C6` | `+0x0252` | `0x012818` | State `$18` |
| `$1A` | `0x0125C8` | `+0x0284` | `0x01284C` | State `$1A` |

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_efreet_dispatch_tornado_m15_489.json`
- Extractor: `tools/extract_rom_efreet_dispatch_tornado.py`
- Test: `tests/rom_efreet_dispatch_tornado_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint32_t kTornadoMaxSpeedThreshold = 0x00080000u;
inline constexpr uint32_t kTornadoSpeedRampStep     = 0x00002000u;
inline constexpr uint8_t  kTornadoMaxAimTurnStep    = 8;
inline constexpr uint8_t  kTornadoAimResetTimer     = 10;
inline constexpr uint8_t  kTornadoDriftResetTimer   = 7;
inline constexpr int16_t  kTornadoViewportWidth     = 0x0140;
inline constexpr int16_t  kTornadoViewportHeight    = 0x00E0;

inline constexpr uint16_t kEfreetSpDrainIntervalFrames = 0x0023;   // 35 frames
inline constexpr int16_t  kEfreetDashSpCost            = 8;
inline constexpr int16_t  kEfreetFlameSpCost           = 6;
inline constexpr int16_t  kEfreetMeltdownSpCost        = 25;       // 0x19
inline constexpr uint16_t kEfreetFlameSlotSentinel     = 0xFFFEu;
inline constexpr uint32_t kEfreetDashSfxId             = 0x00800258u;

struct WaterTornadoMotionState {
    int16_t  x_08{0};
    int16_t  y_0c{0};
    uint16_t facing_16{0};
    uint8_t  flags_40{0};
    uint8_t  flags_41{0};
    uint16_t angle_4e{0};
    int8_t   turn_rate_50{0};
    uint8_t  drift_timer_51{0};
    uint32_t speed_52{0};
    int32_t  vel_x_72{0};
    int32_t  vel_y_76{0};
};

inline uint16_t MapTornadoAngleToFacing(uint8_t angle) noexcept {
    if (angle < 0x20u || angle >= 0xE0u) return 1;  // Right
    if (angle < 0x60u) return 0;                    // Up
    if (angle < 0xA0u) return 3;                    // Left
    return 2;                                       // Down
}

inline void StepTornadoReflectionAndViewportBounds(
    WaterTornadoMotionState& t,
    int16_t cam_x,
    int16_t cam_y) noexcept {
    uint16_t angle = t.angle_4e;
    int8_t turn = t.turn_rate_50;
    if ((t.flags_40 & 0x40u) != 0) {
        angle = (angle <= 0x80u)
            ? static_cast<uint16_t>(0x80u - angle)
            : static_cast<uint16_t>(0x180u - angle);
        turn = static_cast<int8_t>(-turn);
    }
    if ((t.flags_41 & 0x40u) != 0) {
        angle = static_cast<uint8_t>(0u - static_cast<uint8_t>(angle));
        turn = static_cast<int8_t>(-turn);
    }

    auto clamp_edge = [&](uint16_t new_angle) noexcept {
        angle = new_angle;
        turn = static_cast<int8_t>(turn >> 1);
        t.drift_timer_51 = 1;
    };
    if (t.x_08 < cam_x) clamp_edge(0x00u);
    if (t.x_08 > static_cast<int16_t>(cam_x + kTornadoViewportWidth)) clamp_edge(0x80u);
    if (t.y_0c < cam_y) clamp_edge(0xC0u);
    if (t.y_0c > static_cast<int16_t>(cam_y + kTornadoViewportHeight)) clamp_edge(0x40u);

    t.angle_4e = angle;
    t.turn_rate_50 = turn;
}

inline void StepTornadoAimedOrDriftSteering(
    WaterTornadoMotionState& t,
    bool button_a_held,
    uint8_t target_angle,
    uint16_t prng_val,
    uint16_t& hold_counter_ff199e) noexcept {
    uint8_t angle = static_cast<uint8_t>(t.angle_4e);
    if (button_a_held) {
        hold_counter_ff199e = 0;
        const uint8_t ccw = static_cast<uint8_t>(angle - target_angle);
        const uint8_t cw = static_cast<uint8_t>(target_angle - angle);
        const int8_t step = (ccw < cw)
            ? static_cast<int8_t>(-std::min<uint8_t>(ccw, kTornadoMaxAimTurnStep))
            : static_cast<int8_t>(std::min<uint8_t>(cw, kTornadoMaxAimTurnStep));
        angle = static_cast<uint8_t>(angle + step);
        t.turn_rate_50 = 0;
        t.drift_timer_51 = kTornadoAimResetTimer;
    } else {
        if (--t.drift_timer_51 == 0) {
            t.drift_timer_51 = kTornadoDriftResetTimer;
            t.turn_rate_50 = static_cast<int8_t>((prng_val & 0x08u) - 4);
        }
        angle = static_cast<uint8_t>(angle + t.turn_rate_50);
    }

    if (t.speed_52 <= kTornadoMaxSpeedThreshold) {
        t.speed_52 += kTornadoSpeedRampStep;
    }
    t.angle_4e = angle;
    t.facing_16 = MapTornadoAngleToFacing(angle);
}

struct EfreetEntityState {
    uint16_t timer_02{0};
    uint16_t state_04{0};
    uint16_t facing_16{0};
    uint16_t action_2a{0};
    uint8_t  flags_36{0};
    int32_t  vel_x_4e{0};
    int32_t  vel_y_52{0};
    uint16_t drain_timer_8e{kEfreetSpDrainIntervalFrames};
    uint8_t  sub_entity_a2{0};
    uint32_t flame_slot_ptr_a6{0};
    uint8_t  field_ae{0};
    uint8_t  field_af{0};
};

inline bool StepEfreetSpDrain(EfreetEntityState& efreet, int16_t& player_sp) noexcept {
    if (efreet.sub_entity_a2 != 0) {
        return true;
    }
    if (efreet.drain_timer_8e != 0) {
        --efreet.drain_timer_8e;
    } else {
        efreet.drain_timer_8e = kEfreetSpDrainIntervalFrames;
        if (player_sp == 0 || --player_sp == 0) {
            return false;  // Dismiss via 0x012BA8
        }
    }
    if (player_sp <= 1) {
        efreet.flags_36 ^= 0x02u;
    } else {
        efreet.flags_36 &= static_cast<uint8_t>(~0x02u);
    }
    return true;
}

inline void StartEfreetHitReaction(EfreetEntityState& efreet) noexcept {
    efreet.state_04 = 0x10;
    efreet.action_2a = 0x0A;
}

} // namespace Thor::AI
```
