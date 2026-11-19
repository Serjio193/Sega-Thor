# STANDALONE HANDOFF: Milestone M15.523 — ROM World Interactive Objects: Patrolling/Pacing Trap, Persistent Gate & Hidden Bridge, and Bomb-Destructible Barrier (`0x01778C..0x017A96`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x01778C..0x017A96` (778 bytes total: 204 M68K instructions across 3 routines).
- **Core Responsibilities:**
  1. `0x01778C..0x0178EA` (`AI_World_PatrollingPacingTrap`, 350 bytes, 93 insns):
     - Patrolling trap / reciprocating bridge platform moving back and forth between scanned limits and home base.
     - **State 0 (Home Initialization):** Saves home coordinates `+$A8 = x`, `+$AA = y`, height `z = +$8C`, sets animation 0, transitions to State 1 (`+$A6 = 1`).
     - **State 1 (Tile Boundary Scan & Motion Launch):**
       - Converts position to tile coordinates (`x >> 3, y >> 3`).
       - Scans surrounding tiles via `0x00C1CE` using a 16-byte signed 4-way offset table at `0x01783E`.
       - If path is obstructed (carry clear), returns.
       - If path is open (carry set): calculates directional velocity vector via `0x00E2D4` using speed parameter `+$A0` -> `+$4E` (`vx`), `+$52` (`vy`), sets animation 2, travel countdown timer `+$2 = +$9E`, transitions to State 2 (`+$A6 = 2`), and plays trap start SFX `#$800399` via `0x00E7E8`.
     - **State 2 (Acceleration, Deceleration & Travel):**
       - Calls animation tick `0x00BC8E` (`d0 = 0`).
       - Decrements travel timer `+$2`.
       - Applies acceleration before midpoint (`timer >= +$9E / 2`) and deceleration after midpoint (`timer < +$9E / 2`) by adding/subtracting velocity steps to `+$4E/+$52`, and integrating delta displacements into `+$72/+$76`.
       - On timer expiration (`+$2 == 0`): flips facing bit 1 in `+$17` (`bchg #1, $17(a6)`), sets animation 4, and transitions to State 3 (`+$A6 = 3`).
     - **State 3 (Homing Return):**
       - Calls animation tick `0x00BC8E` (`d0 = 0`).
       - Steers back toward home base `(+$A8, +$AA)` via `0x00E45E`.
       - While distance `d4 != 0`, applies homing step velocities to `+$72/+$76`.
       - Upon arrival (`d4 == 0`): sets animation 0, flips facing bit 1 in `+$17`, and transitions back to State 1 (`+$A6 = 1`).
  2. `0x0178EA..0x0179D4` (`AI_World_PersistentGateHiddenBridge`, 234 bytes, 59 insns):
     - Controller for persistent gates, secret hidden bridges, and enemy-cleared puzzle barriers.
     - **Sub-mode 0 (Initialization):** Sets sub-mode `+$66 = 1`, animation `+$2A = 2 * (+$8C)`, elevates height `z += 1`.
     - **Jump Table Dispatch (`0x017920`):** Dispatches based on mode parameter `+$9E`:
       - **Entry 0 (Persistent State Toggle):** Queries world persistent flag via `0x018030`. If set: sets visibility `+$36 |= 2`, clears solid collision `+$38 &= ~4`, sets elevated height `z = 0x2710` (10000 px). If clear: clears visibility `+$36 &= ~2`, sets solid collision `+$38 |= 4`, clears height `z = 0`. If `z` changes, triggers gate rumble sound SFX `#$B00385` via sound driver `$60004`.
       - **Entry 1 (Enemy Clear Threshold Gate):** Waits for initial 10-frame debounce (`+$2 < 10`). At timer >= 10, queries living enemies count via `0x0101EC`. If living enemies `<= +$A0`, calls gate unlock callback `0x017D06` and plays secret chime SFX `#$A00228` via `$60004`.
       - **Entry 2 (Delayed Trigger):** Counts 15-frame delay on `+$2`, then calls unlock callback `0x017D06` with chime SFX `#$A00228`.
  3. `0x0179D4..0x017A96` (`AI_World_BombDestructibleBarrier`, 194 bytes, 52 insns):
     - Specialized barrier destructible exclusively by Player Bomb explosions.
     - **Sub-mode 0 (Initialization):** Invokes gate init `0x0178F2`. If parameter `+$9E != 0`, transitions directly to Sub-mode 3 and toggles visibility bit 1 in `+$36`.
     - **Sub-modes 1 & 3 (Damage Evaluation):**
       - When incoming damage `+$86 != 0`, clears `+$86`.
       - Verifies attacker identity: attacker pointer must match the active Bomb entity slot at `$FF1AA4` (`+$7E == $FF1AA4`) and entity type must be Bomb (`+$0 == 0x1E`).
       - **If Attacker is a Valid Bomb:** Calls persistent barrier clearance callback `0x017FF2` (`a0 = a6`), plays explosion SFX `#$900202` via `0x00E7FC`.
         - In Sub-mode 1: locks cooldown `+$82 = 0xFF`, sets destruction animation `+$2A = 6`, transitions to Sub-mode 2 (despawns `+$0 = 0` once animation finished `+$37:1`).
         - In Sub-mode 3: instantly despawns entity (`+$0 = 0`).
       - **If Attacker is Non-Bomb (physical weapon / magic):** Clears cooldown `+$82 = 0` and plays deflection clink SFX `#$900238` via `0x00E7FC`. Barrier remains intact.

## 2. ROM Evidence Reference
- **Evidence JSON:** `data/thor-evidence/rom_world_patrol_gate_bomb_m15_523.json`
- **Extractor Tool:** `tools/extract_rom_world_patrol_gate_bomb.py`
- **Test Suite:** `tests/rom_world_patrol_gate_bomb_test.py` (14 tests, 14/14 PASS)

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <utility>

namespace Thor::WorldObjects::GatesAndTraps {

// --- SFX & Entity Identifiers ---
inline constexpr uint32_t kSfxPatrolTrapStart      = 0x00800399u; // Pacing trap launch sound
inline constexpr uint32_t kSfxGateRumbleMove       = 0x00B00385u; // Gate elevation / rumble sound
inline constexpr uint32_t kSfxSecretSolvedChime    = 0x00A00228u; // Puzzle solved chime SFX
inline constexpr uint32_t kSfxBombExplosionClear   = 0x00900202u; // Bomb barrier shatter SFX
inline constexpr uint32_t kSfxDeflectionClink      = 0x00900238u; // Physical strike clink SFX

inline constexpr uint32_t kBombAttackerSlot        = 0x00FF1AA4u;
inline constexpr uint16_t kBombEntityType          = 0x001Eu;
inline constexpr int32_t  kHiddenBridgeZ           = 0x2710; // 10000 px

// --- Embedded 4-Way Tile Scan Vector Table (0x01783E) ---
struct TileScanVector {
    int8_t d1x, d1y;
    int8_t d2x, d2y;
};

inline constexpr std::array<TileScanVector, 4> kPatrolTileScanVectors{{
    { -1, -24,   1,  -1 }, // Direction 0 (Up)
    {  1,  -1,  24,   1 }, // Direction 1 (Right)
    { -1,   1,   1,  24 }, // Direction 2 (Down)
    { -24, -1,  -1,   1 }, // Direction 3 (Left)
}};

// --- Patrolling / Pacing Trap ---
struct PatrollingPacingTrap {
    uint8_t  state_a6{0};
    uint8_t  flag_17{0};        // Bit 1 = Facing orientation flip
    uint16_t anim_2a{0};
    uint16_t timer_2{0};
    uint16_t facing_16{0};
    uint16_t param_8c_height{0};
    uint16_t param_a0_speed{0x400};
    uint8_t  param_9e_travel_time{0};
    int32_t  x{0};
    int32_t  y{0};
    int32_t  z{0};
    int32_t  home_x_a8{0};
    int32_t  home_y_aa{0};
    int32_t  vx_4e{0};
    int32_t  vy_52{0};
    int32_t  delta_x_72{0};
    int32_t  delta_y_76{0};
};

inline void StepPatrollingPacingTrap(
    PatrollingPacingTrap& trap,
    bool tile_scan_ok,
    auto&& animate_fn,
    auto&& play_sfx_fn)
{
    if (trap.state_a6 == 0) {
        trap.home_x_a8 = trap.x;
        trap.home_y_aa = trap.y;
        trap.z = trap.param_8c_height;
        trap.anim_2a = 0;
        trap.state_a6 = 1;
        return;
    }

    if (trap.state_a6 == 1) {
        if (!tile_scan_ok) return;

        const int32_t speed = trap.param_a0_speed;
        trap.vx_4e = (trap.facing_16 == 0 || trap.facing_16 == 1) ? speed : -speed;
        trap.vy_52 = (trap.facing_16 == 0 || trap.facing_16 == 2) ? speed : -speed;
        trap.anim_2a = 2;
        trap.timer_2 = trap.param_9e_travel_time;
        trap.state_a6 = 2;
        play_sfx_fn(kSfxPatrolTrapStart);
        return;
    }

    if (trap.state_a6 == 2) {
        animate_fn();
        if (trap.timer_2 <= 1) {
            trap.flag_17 ^= (1 << 1);
            trap.anim_2a = 4;
            trap.state_a6 = 3;
            return;
        }
        trap.timer_2--;

        const uint8_t midpoint = trap.param_9e_travel_time >> 1;
        constexpr int32_t kAccelStep = 0x20;
        if (trap.timer_2 >= midpoint) {
            trap.vx_4e += kAccelStep;
            trap.vy_52 += kAccelStep;
        } else {
            trap.vx_4e -= kAccelStep;
            trap.vy_52 -= kAccelStep;
        }
        trap.delta_x_72 += trap.vx_4e;
        trap.delta_y_76 += trap.vy_52;
        return;
    }

    if (trap.state_a6 == 3) {
        animate_fn();
        const int32_t dx = trap.home_x_a8 - trap.x;
        const int32_t dy = trap.home_y_aa - trap.y;
        if (dx != 0 || dy != 0) {
            trap.delta_x_72 = (dx > 0) ? 0x100 : ((dx < 0) ? -0x100 : 0);
            trap.delta_y_76 = (dy > 0) ? 0x100 : ((dy < 0) ? -0x100 : 0);
            trap.x += (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
            trap.y += (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
        } else {
            trap.anim_2a = 0;
            trap.flag_17 ^= (1 << 1);
            trap.state_a6 = 1;
        }
    }
}

// --- Persistent Gate & Hidden Bridge ---
struct PersistentGateHiddenBridge {
    uint8_t  submode_66{0};
    uint8_t  mode_9e{0};        // Jump table entry selector
    uint16_t timer_2{0};
    uint16_t anim_2a{0};
    uint16_t param_8c{0};
    uint16_t param_a0_threshold{0};
    uint8_t  flag_36{0};        // Bit 1 = Visible
    uint8_t  flag_38{0};        // Bit 2 = Solid
    int32_t  z{0};
    uint16_t persistent_bit{0};
};

inline void StepPersistentGateHiddenBridge(
    PersistentGateHiddenBridge& gate,
    bool persistent_flag_set,
    int living_enemies,
    auto&& unlock_callback_fn,
    auto&& play_sfx_fn)
{
    if (gate.submode_66 == 0) {
        gate.submode_66 = 1;
        gate.anim_2a = gate.param_8c * 2;
        gate.z += 1;
        return;
    }

    if (gate.mode_9e == 0) {
        const int32_t old_z = gate.z;
        if (persistent_flag_set) {
            gate.flag_36 |= (1 << 1);
            gate.flag_38 &= ~(1 << 2);
            gate.z = kHiddenBridgeZ;
        } else {
            gate.flag_36 &= ~(1 << 1);
            gate.flag_38 |= (1 << 2);
            gate.z = 0;
        }
        if (gate.z != old_z) {
            play_sfx_fn(kSfxGateRumbleMove);
        }
        return;
    }

    if (gate.mode_9e == 1) {
        if (gate.timer_2 < 10) {
            gate.timer_2++;
            return;
        }
        if (living_enemies <= gate.param_a0_threshold) {
            unlock_callback_fn();
            play_sfx_fn(kSfxSecretSolvedChime);
        }
        return;
    }

    if (gate.mode_9e == 2) {
        if (gate.timer_2 != 0) {
            gate.timer_2++;
            if (gate.timer_2 >= 15) {
                unlock_callback_fn();
                play_sfx_fn(kSfxSecretSolvedChime);
            }
        }
    }
}

// --- Bomb-Destructible Barrier ---
struct BombDestructibleBarrier {
    uint16_t entity_type_0{0x24};
    uint8_t  submode_66{0};
    uint8_t  mode_9e{0};
    uint8_t  cooldown_82{0};
    uint8_t  flag_36{0};
    uint8_t  flag_37{0};        // Bit 1 = Animation complete
    uint16_t anim_2a{0};
    uint16_t damage_86{0};
    uint16_t param_8c{0};
    int32_t  z{0};
    uint16_t persistent_bit{0};
};

inline void StepBombDestructibleBarrier(
    BombDestructibleBarrier& barrier,
    uint32_t attacker_ptr,
    uint16_t attacker_type,
    auto&& clear_callback_fn,
    auto&& play_sfx_fn)
{
    if (barrier.submode_66 == 0) {
        barrier.submode_66 = 1;
        barrier.anim_2a = barrier.param_8c * 2;
        barrier.z += 1;
        if (barrier.mode_9e != 0) {
            barrier.submode_66 = 3;
            barrier.flag_36 ^= (1 << 1);
        }
        return;
    }

    if (barrier.submode_66 == 1) {
        if (barrier.damage_86 == 0) return;
        barrier.damage_86 = 0;

        const bool is_bomb = (attacker_ptr == kBombAttackerSlot && attacker_type == kBombEntityType);
        if (is_bomb) {
            clear_callback_fn();
            barrier.cooldown_82 = 0xFF;
            barrier.anim_2a = 6;
            barrier.submode_66 = 2;
            play_sfx_fn(kSfxBombExplosionClear);
        } else {
            barrier.cooldown_82 = 0;
            play_sfx_fn(kSfxDeflectionClink);
        }
        return;
    }

    if (barrier.submode_66 == 2) {
        if ((barrier.flag_37 & (1 << 1)) != 0) {
            barrier.entity_type_0 = 0; // Despawn
        }
        return;
    }

    if (barrier.submode_66 == 3) {
        if (barrier.damage_86 == 0) return;
        barrier.damage_86 = 0;

        const bool is_bomb = (attacker_ptr == kBombAttackerSlot && attacker_type == kBombEntityType);
        if (is_bomb) {
            clear_callback_fn();
            barrier.entity_type_0 = 0; // Instant despawn
            play_sfx_fn(kSfxBombExplosionClear);
        } else {
            barrier.cooldown_82 = 0;
            play_sfx_fn(kSfxDeflectionClink);
        }
    }
}

} // namespace Thor::WorldObjects::GatesAndTraps
```
