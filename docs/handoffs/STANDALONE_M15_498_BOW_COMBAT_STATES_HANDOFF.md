# STANDALONE HANDOFF: Milestone M15.498 — ROM Bow Spirit (0x1E) Combat States $08, $04 Gravity Leap/Chomp & $06

## 1. Subsystem Overview
- **ROM Address Range:** `0x013B36..0x013CB4` (382 bytes, 95 instructions, zero embedded data tables).
- **Core Responsibilities:**
  1. `0x013B36..0x013BF2` (`AI_Bow_State08_ChompAttackAndTrail`, 188 bytes, 48 instructions):
     - **State `$08` (`0x013B36..0x013BB2`):** Decrements timer `+$02(a6)` (exiting to `0x013BB2` on 0). If not yet in chomp animation `+$2A == $24`, probes 17 enemy slots at `$FF1CD8` via `0x00BBA8` using table `0x05887C` and sets `+$2A = $24` on Carry set. Once `+$2A == $24`, loads `+$90(a6) = (($FF0DB9 + 1) >> 2)`, installs hit callback `$FF1940 = 0x00013BDE`, runs `0x00BCAE` (`d0 = 2`), clears `+$90(a6)`, refunds `+$02(a6) += 1` (preventing timer expiration mid-chomp), and exits to `0x013BB2` when `+$37` bit 1 is set. Every continuing tick integrates `+$4E/+$52 -> +$72/+$76` and `+$56 -> +$10` (clamped `>= 0`) and updates Bow's stem/trail chain via `0x014200` (`d7 = $0C`).
     - **Exit Helper (`0x013BB2..0x013BDE`):** Resets State `0`, Anim `0`, substate `+$9E = 2`, zeroes `+$4E/+$52/+$56`, clears bit 2 of `+$38`, and updates trail via `0x014200` (`d7 = 9`).
     - **Chomp Hit Callback (`0x013BDE..0x013BF2`):** Calls `0x00BE98` (`hit_apply_handler`) and zeroes enemy `+$4E/+$52(a0)` whenever `+$56(a0) == 0` (grounded targets).
  2. `0x013BF2..0x013C7C` (`AI_Bow_State04_GravityLeapAndSeek`, 138 bytes, 35 instructions):
     - **State `$04` (`0x013BF2..0x013C52`):** Applies `-0x0000C000` gravity to `+$56(a6)` and adds to `+$10(a6)` (clamping both `+$10` and `+$56` to `0` on borrow). Calls `0x013FFC` (anchor chain sync) and `0x00E3A4` against `$FF1C1C` (`+$4E/+$52(a4)`) to compute Manhattan distance `d5`. Transitions to State `$06` (`0x013C52`) when `d5 <= 6`, `d5 > +$AC(a6)`, or `--(+$02(a6)) == 0`; otherwise updates `+$AC(a6) = d5` and integrates `+$4E/+$52 -> +$72/+$76`.
     - **State `$06` Entry (`0x013C52..0x013C7C`):** Sets State `$06`, Anim `$12`, clears `+$16` and `+$10`, frees `$FF1C1C` slot (`+$0(a4) = 0`), and plays SFX `#$0080037E`.
  3. `0x013C7C..0x013CB4` (`AI_Bow_State06_PopupAndRestorePos`, 56 bytes, 12 instructions):
     - **State `$06` (`0x013C7C..0x013CB4`):** Waits for `+$37` bit 1; when set, transitions to State `$02`, Anim `$14`, restores `+$08/+$0C` from backup `+$AE/+$B2`, clears `+$10 = 0`, calls `0x009D00` terrain probe, and plays SFX `#$0080037E`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_bow_combat_states_m15_498.json`
- Extractor: `tools/extract_rom_bow_combat_states.py`
- Test: `tests/rom_bow_combat_states_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::AI {

constexpr uint16_t kBowChompAnimId            = 0x0024;
constexpr uint8_t  kBowChompAttackIndex       = 2;
constexpr uint8_t  kBowState08ExitSubstate9E  = 2;
constexpr int32_t  kBowGravityStep            = 0x0000C000;
constexpr uint16_t kBowLeapProximityPx        = 6;
constexpr uint16_t kBowState06AnimBurrow      = 0x0012;
constexpr uint16_t kBowState02AnimPopup       = 0x0014;
constexpr uint32_t kBowBurrowPopupSfx         = 0x0080037E;

inline uint16_t ComputeBowChompBonusDamage(uint8_t bow_level_db9) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(bow_level_db9) + 1U) >> 2);
}

inline void StepBowGravityZ(int32_t& z_pos_10, int32_t& vz_56) noexcept {
    vz_56 -= kBowGravityStep;
    const int32_t next_z = z_pos_10 + vz_56;
    if (next_z < 0) {
        z_pos_10 = 0;
        vz_56 = 0;
    } else {
        z_pos_10 = next_z;
    }
}

inline bool ShouldFinishBowGravityLeap(
    uint16_t dist_d5,
    uint16_t prev_min_dist_ac,
    uint16_t& timer_02
) noexcept {
    if (dist_d5 <= kBowLeapProximityPx || dist_d5 > prev_min_dist_ac) {
        return true;
    }
    --timer_02;
    return timer_02 == 0;
}

} // namespace Thor::AI
```
