# STANDALONE HANDOFF: Milestone M15.518 — ROM Interactive World Objects: Moving Bridge/Dust Streamer, Falling Stalactite Ceiling Trap, Floor Pressure Switch & Hit-Toggle Crystal Switch

## 1. Subsystem Overview
- **ROM Address Range:** `0x016A64..0x016CBE` (602 bytes, 174 M68K instructions across 4 routines).
- **Core Responsibilities:**
  1. `0x016A64..0x016ADE` (`Object_MovingBridge_DustStreamer`, 122 bytes, 39 instructions):
     - Mode `+$9E`:
       - `0` (X-axis): adds PRNG height jitter `(PRNG << 1)` into Z height `+$10(a6)`, integrates velocity `+$8(a6) += +$4E(a6)`. When crossing target bound `+$A0(a6)` (`>= bound` if `vx >= 0`, `<= bound` if `vx < 0`), despawns entity (`+$0 = 0`).
       - `1` (Y-axis): adds PRNG height jitter `(PRNG << 2)` into Z height `+$10(a6)`, integrates velocity `+$C(a6) += +$52(a6)`. When crossing target bound `+$A0(a6)` (`>= bound` if `vy >= 0`, `<= bound` if `vy < 0`), despawns entity (`+$0 = 0`).
  2. `0x016ADE..0x016BD0` (`Object_FallingStalactite_Trap`, 242 bytes, 64 instructions):
     - Sub-state `+$9E` (0..3):
       - `0` (Ceiling Init): sets `z = 0x00F0` (240), wait timer `(PRNG & 0x3F) + 4`, `vz = 0`. If proximity mode `+$A0 != 0`, sets `timer = 1` and `vz = -0x18`. Sets flag `+$36:1`, enters State `1`.
       - `1` (Viewport Cull & Countdown): if `+$A0 == 0`, checks viewport culling `0x00E5AE(4)` (aborts if offscreen). Decrements timer; on expiration, selects random anim 0 or 4 based on `$FF1971:0`, clears `+$36:1`, enters State `2`.
       - `2` (Falling Ballistics & Collision): applies gravity `-0x0000C000` to `+$56`, integrates Z. Executes AABB check `[-1..+1, -1..+1]` vs Ali via `0x00B852`: if target is non-flying (`+$38:2 == 0`), shatters (`+$2A += 2`, State `3`). On ground impact (`z < 0`), clamps `z = 1`, advances anim `+$2A += 2`, enters State `3`.
       - `3` (Shatter & Respawn/Despawn): on anim finished flag `+$37:1`, respawns to State `0` if `+$A0 == 0`, else despawns (`+$0 = 0`).
  3. `0x016BD0..0x016C5C` (`Object_FloorPressureSwitch`, 140 bytes, 45 instructions):
     - Sets height `z = 0xFFF0` (-16). Mode `+$9E`:
       - `0` (Momentary): executes AABB vs Ali `[-6..+10, +6..+22]` via `0x00B852`. If player on switch: sets persistent bit via `0x018030` (`bset`), anim 2 (pressed). If off switch: clears bit (`bclr`), anim 0 (released).
       - `1` (One-Shot Latched): checks persistent bit via `0x018030`. If already set: anim 2. If unpressed: executes AABB test; upon player contact, sets persistent bit, plays latch SFX `0x900229` via `0x00E7FC`, anim 2.
  4. `0x016C5C..0x016CBE` (`Object_HitToggleCrystalSwitch`, 98 bytes, 26 instructions):
     - Retrieves persistent bit address/index via `0x018030`.
     - Cooldown timer `+$2`: while `> 0`, decrements; on reaching 0, clears invulnerability guard `+$82 = 0`.
     - On weapon hit (`+$86 != 0`): clears damage and flinch, sets guard `+$82 = 0xFF`, sets cooldown `+$2 = 10`, toggles persistent bit (`bchg`), plays chime SFX `0x900229` via `0x00E7FC`. Updates anim: 2 if bit set, 0 if clear.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_world_traps_switches_m15_518.json`
- Extractor: `tools/extract_rom_world_traps_switches.py`
- Unit Tests: `tests/rom_world_traps_switches_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <optional>
#include <tuple>

namespace Thor::World::InteractiveObjects {

struct DustStreamerEntity {
    uint16_t active_0{1};
    uint8_t mode_9e{0}; // 0 = X-axis, 1 = Y-axis
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    int32_t vx_4e{0};
    int32_t vy_52{0};
    int16_t bound_a0{0};
};

/// Emulates Moving Bridge / Dust Streamer Particle (0x016A64..0x016ADE).
inline bool StepDustStreamer(DustStreamerEntity& obj, uint16_t prng_val) noexcept {
    if (obj.mode_9e == 0) {
        obj.z += static_cast<int32_t>(prng_val << 1);
        obj.x += static_cast<int16_t>(obj.vx_4e);
        if (obj.vx_4e >= 0) {
            if (obj.x >= obj.bound_a0) { obj.active_0 = 0; return false; }
        } else {
            if (obj.x <= obj.bound_a0) { obj.active_0 = 0; return false; }
        }
        return true;
    } else if (obj.mode_9e == 1) {
        obj.z += static_cast<int32_t>(prng_val << 2);
        obj.y += static_cast<int16_t>(obj.vy_52);
        if (obj.vy_52 >= 0) {
            if (obj.y >= obj.bound_a0) { obj.active_0 = 0; return false; }
        } else {
            if (obj.y <= obj.bound_a0) { obj.active_0 = 0; return false; }
        }
        return true;
    }
    return false;
}

struct StalactiteEntity {
    uint16_t active_0{1};
    uint8_t state_9e{0};
    int16_t x{0};
    int16_t y{0};
    int32_t z{240};
    int32_t vz{0};
    int16_t timer{0};
    uint16_t anim_id{0};
    uint8_t flags_36{0};
    uint8_t flags_37{0};
    uint8_t bound_a0{0};
};

struct PlayerTarget {
    int16_t x{0};
    int16_t y{0};
    uint8_t flags_38{0};
};

enum class StalactiteResult {
    Initialized,
    CulledOffscreen,
    WaitingCountdown,
    StartFall,
    Falling,
    AliImpact,
    FloorImpact,
    Shattering,
    Respawned,
    Despawned
};

/// Emulates Falling Stalactite Ceiling Trap (0x016ADE..0x016BD0).
template <typename PRNGFn, typename VisibilityCheckFn>
inline StalactiteResult StepStalactiteTrap(
    StalactiteEntity& obj,
    const PlayerTarget* ali,
    PRNGFn&& prng_fn,
    VisibilityCheckFn&& is_visible_fn,
    uint8_t global_bit_1971 = 0
) noexcept {
    if (obj.state_9e == 0) {
        obj.z = 240;
        uint16_t p = prng_fn();
        obj.timer = static_cast<int16_t>((p & 0x3F) + 4);
        obj.vz = 0;
        if (obj.bound_a0 != 0) {
            obj.timer = 1;
            obj.vz = -0x18;
        }
        obj.flags_36 |= 0x02;
        obj.state_9e = 1;
        return StalactiteResult::Initialized;
    } else if (obj.state_9e == 1) {
        if (obj.bound_a0 == 0 && !is_visible_fn()) {
            obj.z = 240;
            return StalactiteResult::CulledOffscreen;
        }
        obj.timer -= 1;
        if (obj.timer != 0) {
            obj.z = 240;
            return StalactiteResult::WaitingCountdown;
        }
        obj.anim_id = ((global_bit_1971 & 1) != 0) ? 4 : 0;
        obj.flags_36 &= ~0x02;
        obj.state_9e = 2;
        obj.z = 240;
        return StalactiteResult::StartFall;
    } else if (obj.state_9e == 2) {
        obj.vz -= 0x0000C000;
        obj.z += obj.vz;

        if (obj.z < 0) {
            obj.z = 1;
            obj.anim_id += 2;
            obj.state_9e = 3;
            return StalactiteResult::FloorImpact;
        }

        if (ali != nullptr) {
            int16_t dx = std::abs(static_cast<int16_t>(obj.x - ali->x));
            int16_t dy = std::abs(static_cast<int16_t>(obj.y - ali->y));
            if (dx <= 1 && dy <= 1) {
                if ((ali->flags_38 & 0x04) == 0) { // non-flying target
                    obj.anim_id += 2;
                    obj.state_9e = 3;
                    return StalactiteResult::AliImpact;
                }
            }
        }
        return StalactiteResult::Falling;
    } else if (obj.state_9e == 3) {
        if ((obj.flags_37 & 0x02) != 0) {
            if (obj.bound_a0 == 0) {
                obj.state_9e = 0;
                return StalactiteResult::Respawned;
            } else {
                obj.active_0 = 0;
                return StalactiteResult::Despawned;
            }
        }
        return StalactiteResult::Shattering;
    }
    return StalactiteResult::Shattering;
}

struct PressureSwitchEntity {
    uint8_t mode_9e{0}; // 0 = momentary, 1 = one-shot latched
    int16_t x{0};
    int16_t y{0};
    int16_t z{-16};
    uint16_t anim_id{0};
};

/// Emulates Floor Pressure Switch (0x016BD0..0x016C5C).
/// Returns pair: (is_pressed, should_play_latch_sfx).
inline std::pair<bool, bool> StepFloorPressureSwitch(
    PressureSwitchEntity& obj,
    int16_t ali_x,
    int16_t ali_y,
    uint8_t& persistent_byte,
    uint8_t bit_idx
) noexcept {
    obj.z = -16;
    int16_t dx = ali_x - obj.x;
    int16_t dy = ali_y - obj.y;
    bool is_on_switch = (dx >= -6 && dx <= 10 && dy >= 6 && dy <= 22);

    if (obj.mode_9e == 0) {
        // Momentary
        if (is_on_switch) {
            persistent_byte |= (1 << bit_idx);
            obj.anim_id = 2;
            return {true, false};
        } else {
            persistent_byte &= ~(1 << bit_idx);
            obj.anim_id = 0;
            return {false, false};
        }
    } else if (obj.mode_9e == 1) {
        // One-shot latch
        bool already_set = ((persistent_byte & (1 << bit_idx)) != 0);
        if (already_set) {
            obj.anim_id = 2;
            return {true, false};
        }
        if (is_on_switch) {
            persistent_byte |= (1 << bit_idx);
            obj.anim_id = 2;
            return {true, true}; // play SFX 0x900229
        } else {
            obj.anim_id = 0;
            return {false, false};
        }
    }
    return {false, false};
}

struct CrystalSwitchEntity {
    int16_t timer_2{0};
    uint16_t damage_86{0};
    uint8_t flinch_83{0};
    uint8_t guard_82{0};
    uint16_t anim_id{0};
};

/// Emulates Hit-Toggle Crystal Switch (0x016C5C..0x016CBE).
/// Returns pair: (is_active, should_play_chime_sfx).
inline std::pair<bool, bool> StepHitToggleCrystalSwitch(
    CrystalSwitchEntity& obj,
    uint8_t& persistent_byte,
    uint8_t bit_idx
) noexcept {
    bool sfx = false;
    if (obj.timer_2 != 0) {
        obj.timer_2 -= 1;
        if (obj.timer_2 == 0) {
            obj.guard_82 = 0;
        }
    } else {
        if (obj.damage_86 != 0) {
            obj.damage_86 = 0;
            obj.flinch_83 = 0;
            obj.guard_82 = 0xFF;
            obj.timer_2 = 10;
            persistent_byte ^= (1 << bit_idx); // toggle bit
            sfx = true; // play SFX 0x900229
        }
    }

    bool is_active = ((persistent_byte & (1 << bit_idx)) != 0);
    obj.anim_id = is_active ? 2 : 0;
    return {is_active, sfx};
}

} // namespace Thor::World::InteractiveObjects
```

## 4. Behavioral Parity & Invariants
1. **Axis Velocity Jitter:** Moving bridge particles introduce pseudo-random vertical oscillation (`PRNG << 1` on X-axis, `PRNG << 2` on Y-axis) before checking directional boundary crossing (`+$A0`).
2. **Ceiling Trap Culling:** Normal stalactites (`+$A0 == 0`) remain perched at ceiling elevation (240 px) until visible in camera view, then select random animation variants (0 or 4) via `$FF1971:0` upon release.
3. **Collision Disconnect for Flying Targets:** Stalactite collisions only trigger against ground-bound targets (`+$38:2 == 0`), letting flying entities or jumped players pass safely if elevation aligns.
4. **Persistent Bit Semantics:** Pressure switches and crystal switches map directly to game progress bitfields via `0x018030`, preserving world puzzle states across room transitions.
