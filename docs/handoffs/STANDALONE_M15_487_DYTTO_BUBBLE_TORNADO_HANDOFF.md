# STANDALONE HANDOFF: Milestone M15.487 — ROM Dytto Spirit Magic Bubble Projectile, Flame/Enemy Stun AABB Scanner & Water Tornado Vortex Subsystem

## 1. Subsystem Overview
- **ROM Address Range:** `0x011D8A..0x0120D4` (842 bytes, 203 instructions).
- **Core Responsibilities:**
  1. `0x011D8A..0x011DB6` (`AI_Dytto_TryStartMagicBubble`, 44 bytes, 10 instructions): Consumes 5 SP via `0x011B28` (branching to `0x01219A` on Carry set), reserves a Pool 1 slot via `0x00DA62` into `+$A4(a6)` (branching to `0x01219A` on Carry set), copies latched facing `$FF19A4` to `+$16(a6)`, and enters State `$0E` (`+$2A = 6`).
  2. `0x011DB6..0x011ED2` (`AI_Dytto_State0E_CastAndLaunchMagicBubble`, 284 bytes, 63 instructions): While `+$30(a6) < 0x18`, lowers `+$10(a6)` by 2 if non-negative after subtraction (otherwise calls `0x0123B4`). At `+$30 == 0x18`, calls `0x0123B4`, verifies reserved slot `$FF19E8 + +$A4(a4)` still holds reservation type `$002E`, converts it to Type `$0018` via `0x008D06`, sets sprite pointer `#$00141374`, priority `(parent_18 & $8000) | $07A8`, `+$3D = 8`, `+$36 = 0`, `+$38 = $0E`, `8x8x16` hitbox (`+$46=8, +$48=8, +$49=8, +$4A=$10`), `+$A2 = $FF`, State `$10` (`+$2A = $1A`), `+$10 += $0A`, launch offset `0x1800` (+`0x20000` X when facing Down=2) and velocity `(0x0400, 0x03C0)` via `0x00E2D4` into `+$72/+$76` and `+$4E/+$52`, probes `0x009D00`/`0x009AD6` to latch `+$14(a6) = d4`, and plays SFX `#$800354`. On `+$37` bit 1 (when `+$30 != 0x18`), returns Dytto to State `0` (`0x0121AA`).
  3. `0x011ED2..0x011FA6` (`AI_Dytto_State10_16_MagicBubbleFlightAndImpact`, 212 bytes, 56 instructions):
     - **State `$10` (`0x011ED2..0x011F98`):** Calls `0x00E36E`, transitions to State `$16` (`+$2A = $24`) on wall contact (`(+$40 | +$41)` bit 6), scans `0x00B922` (`d0.b = $F0`, box `[-9..9, -9..9, -1..17]`): triggers Flame `$004C` or Type `$0098` (`+$9E == 1`) with `+$66(a0) = $FF` + SFX `#$8004D4` -> State `$16`; otherwise checks enemy immunity (`+$3A` bit 1 -> State `$16` without stun/SFX; `+$38` bit 3 -> ignore and fly/cull), and stuns vulnerable enemies with `+$9D(a0) = ($FF0DB6 << 3) + 80`, `+$9C(a0) = ($FF0DB6 << 2) + 20`, SFX `#$8004D4` -> State `$16`. When no impact pops the bubble, viewport-culls via `0x00E5AE` (margin `0x18` -> `clr.w $0(a6)`) and integrates `+$4E/+$52` into `+$72/+$76`.
     - **State `$16` (`0x011F98..0x011FA6`):** Despawns bubble (`clr.w $0(a6)`) when `+$37` bit 1 is set.
  4. `0x011FA6..0x0120D4` (`AI_Dytto_WaterTornadoLifecycle`, 302 bytes, 74 instructions):
     - **Command 3 Start (`0x011FA6..0x011FF6`):** Clears `$FF199E`, lowers `+$10` by `0x28` (clamping 32-bit `+$10` to 0 on borrow), enters State `$08` (`+$2A = $0A`), plays SFX `#$80034E`, sets orbit angle `+$4E = ((5 - $FF19FE) & 3) << 6`, step `+$50 = 4`, radial step `+$51 = 20`, radius `+$52 = $20000`.
     - **State `$08` (`0x011FF6..0x01202A`):** Orbits via `0x0123E2` until `+$37` bit 1 -> transitions to State `$0A` (`+$2A = $0C`, `+$3D = $10`, `+$02 = ($FF0DB6 << 3) + 120`).
     - **State `$0A` (`0x01202A..0x012086`):** Drains `+$8E(a6)` by 8 (clamped `>= 0`), runs attack scan `0x00BCA4` (`d0 = 0`), orbits via `0x0123E2`, plays SFX `#$700340` when `(+$02 & 7) == 0`, and transitions to State `$0C` (`+$2A = $18`, `+$3D = 0`, `+$10 += $28`, `+$02 = $1E`) when Ali's SP `+$8E(a5) == 0` or `+$02` hits 0.
     - **State `$0C` (`0x012086..0x0120D4`):** Continues attack scan `0x00BCA4`, decays `+$52` by `$5000` (clamped `>= 0`), orbits via `0x0123E2`, alternates `+$3D` (`$10`/`0`) paired with `-/+$28` on `+$10` each tick, and exits to State `0` (`0x0121AA`) with `+$3D = 0` when `+$02` hits 0.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_dytto_bubble_tornado_m15_487.json`
- Extractor: `tools/extract_rom_dytto_bubble_tornado.py`
- Test: `tests/rom_dytto_bubble_tornado_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::AI {

constexpr int16_t  kMagicBubbleSpCost           = 5;
constexpr uint16_t kMagicBubbleLaunchFrame      = 0x18;
constexpr uint16_t kMagicBubbleReservationType  = 0x002E;
constexpr uint16_t kMagicBubbleEntityType       = 0x0018;
constexpr uint32_t kMagicBubbleSpritePtr        = 0x00141374;
constexpr uint16_t kMagicBubbleVramBase         = 0x07A8;
constexpr uint16_t kMagicBubbleLaunchOffset     = 0x1800;
constexpr int32_t  kMagicBubbleDownXNudge       = 0x00020000;
constexpr uint16_t kMagicBubbleSpeedX           = 0x0400;
constexpr uint16_t kMagicBubbleSpeedY           = 0x03C0;
constexpr uint8_t  kMagicBubbleCullMargin       = 0x18;

constexpr uint16_t kTornadoZDrop                = 0x28;
constexpr uint8_t  kTornadoOrbitStep            = 4;
constexpr uint8_t  kTornadoRadialStep           = 20;
constexpr int32_t  kTornadoOrbitRadius          = 0x00020000;
constexpr int32_t  kTornadoRadiusDecay          = 0x00005000;
constexpr uint16_t kTornadoBaseDuration         = 120;
constexpr uint16_t kTornadoWinddownDuration     = 30;
constexpr uint16_t kTornadoExtraDrainStep       = 8;

struct BubbleScanBox {
    uint8_t filter{0xF0};
    int8_t  x_min{-9}, y_min{-9};
    int8_t  x_max{9},  y_max{9};
    int8_t  z_min{-1}, z_max{17};
};

enum class BubbleImpactOutcome : uint8_t {
    ContinueFlight,
    DespawnOffscreen,
    EnterPopState16,
};

struct BubbleTargetFields {
    uint16_t type_id{0};      // +$00
    uint8_t  flags_38{0};     // +$38 bit 3: intangible
    uint8_t  flags_3a{0};     // +$3A bit 1: immune pop
    uint8_t  trigger_66{0};   // +$66 set to 0xFF on Flame/0x98
    uint8_t  stun_param_9c{0};
    uint8_t  stun_timer_9d{0};
    uint8_t  sub_type_9e{0};  // +$9E == 1 for special Type 0x98
};

inline BubbleImpactOutcome ResolveBubbleTargetImpact(
    BubbleTargetFields& target,
    uint8_t spirit_level,     // $FF0DB6
    bool& out_play_impact_sfx
) noexcept {
    out_play_impact_sfx = false;
    if (target.type_id == 0x004C || (target.type_id == 0x0098 && target.sub_type_9e == 1)) {
        target.trigger_66 = 0xFF;
        out_play_impact_sfx = true;
        return BubbleImpactOutcome::EnterPopState16;
    }
    if ((target.flags_3a & 0x02) != 0) {
        return BubbleImpactOutcome::EnterPopState16;
    }
    if ((target.flags_38 & 0x08) != 0) {
        return BubbleImpactOutcome::ContinueFlight;
    }
    target.stun_timer_9d = static_cast<uint8_t>((spirit_level << 3) + 0x50);
    target.stun_param_9c = static_cast<uint8_t>((spirit_level << 2) + 0x14);
    out_play_impact_sfx = true;
    return BubbleImpactOutcome::EnterPopState16;
}

inline uint16_t ComputeTornadoInitialAngle(uint16_t player_facing) noexcept {
    return static_cast<uint16_t>(((5 - player_facing) & 3) << 6);
}

inline uint16_t ComputeTornadoVortexDuration(uint8_t spirit_level) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(spirit_level) << 3) + kTornadoBaseDuration);
}

} // namespace Thor::AI
```
