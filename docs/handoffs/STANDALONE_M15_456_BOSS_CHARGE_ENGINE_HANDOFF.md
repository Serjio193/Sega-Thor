# STANDALONE HANDOFF: Milestone M15.456 — ROM Boss Type $3A Combat State Engine (Part 2), Velocity Damping & Floor Bounce Recoil

## 1. Context & Range
- **Milestone:** M15.456
- **ROM Addresses:** `0x00EE94..0x00F054` (448 bytes, 120 instructions)
- **Target Subsystem:** Boss Entity Type `$3A` (Colossus / Minotaur Boss) Combat State Transitions, Subpixel Velocity Air Drag Damping, 3D Parabolic Trajectory Gravity Integration, Floor Inelastic Bounce Recoil & Spawner Notification Engine.
- **Evidence JSON:** `data/thor-evidence/rom_boss_charge_engine_m15_456.json`
- **Unit Tests:** `tests/rom_boss_charge_engine_test.py`

---

## 2. Subroutines & ROM Architecture

### Subroutine 1: `Boss3A_State4_Update` (`0x00EE94..0x00EF1E`, 136 bytes, 36 insns)
- Integrates trajectory physics via `0x00EFA2`.
- Checks action frame completion flag: `btst.b #1, $37(a6)`.
- If set:
  * Clears flip bit 11 in `$18(a6)`: `andi.w #$F7FF, $18(a6)`.
  * Sets state flags: `bset.b #3, $36(a6)`, `bset.b #3, $38(a6)`, `bset.b #1, $3C(a6)`.
  * Clears bits 13 and 14 in `$18(a6)`: `andi.w #$9FFF, $18(a6)`.
  * Sets state `$04(a6) = 6`.
  * Checks subtype `$A6(a6)`:
    - If `0`: `$2A(a6) = 6`, SFX `#$B00224` via spatial dispatcher `0x00E78E`.
    - If `1`: `$2A(a6) = 10`, SFX `#$B00224` via spatial dispatcher `0x00E78E`.
    - Else (>1): `$04(a6) = 8`, `$2A(a6) = 8`, Roar SFX `#$B00225`, Ground quake SFX `#$600382`, screen shake `$FF17C4 = 4`.

### Subroutine 2: `Boss3A_State2_Update` (`0x00EF1E..0x00EF38`, 26 bytes, 7 insns)
- Integrates trajectory physics via `0x00EFA2`.
- Checks action frame completion flag: `btst.b #1, $37(a6)`.
- If set: transitions to State 4 (`$04(a6) = 4, $2A(a6) = 4`).

### Subroutine 3: `Boss3A_State0_CountdownTimer` (`0x00EF38..0x00EF5E`, 38 bytes, 11 insns)
- Integrates trajectory physics via `0x00EFA2`.
- Decrements countdown timer `+$02(a6)`.
- When timer reaches 0: transitions to State 2 (`$04(a6) = 2, $2A(a6) = 2`).

### Subroutine 4: `Boss3A_VelocityDampingIntegration` (`0x00EF5E..0x00EFA2`, 68 bytes, 21 insns)
- Computes 0.75x velocity air drag damping per frame using shift-add arithmetic:
  ```m68k
  d1 = vel >> 1
  d0 = d1
  d1 >>= 1
  d1 += d0    ; d1 = 0.5 * vel + 0.25 * vel = 0.75 * vel
  ```
- Applies damping to:
  * Horizontal X: `+$4E(a6) = d1; +$72(a6) += d1`
  * Horizontal Y: `+$52(a6) = d1; +$76(a6) += d1`
  * Elevation Z: `+$56(a6) = d1; +$10(a6) += d0` (adds `d0 = vel_z >> 1` to elevation).
  * Floor clamp: if `+$10(a6) < 0`, clamps `+$10(a6) = 0`.

### Subroutine 5: `Boss3A_TrajectoryFloorBounceRecoil` (`0x00EFA2..0x00F038`, 150 bytes, 38 insns)
- Syncs integer world positions from subpixel anchors via `0x00E36E`.
- Adds horizontal velocity `+$4E(a6)` to `+$72(a6)` and `+$52(a6)` to `+$76(a6)`.
- Applies gravity: `d0 = +$56(a6) - 0xC000`.
- If on ground (`+$10(a6) == 0`):
  * If bit 3 of `$36(a6)` set: branches to despawn (`0x00F038`).
  * If `d0 < 0` (falling downward): branches to stop all velocities (`0x00F020`).
- If in air:
  * Updates `+$56(a6) = d0` and `+$10(a6) += d0`.
  * If `+$10(a6) >= 0`: branches to viewport culling check (`0x00F02C`).
  * If `+$10(a6) < 0` (floor collision impact):
    - Clamps elevation: `+$10(a6) = 0`.
    - If bit 3 of `$36(a6)` set: branches to despawn (`0x00F038`).
    - Inelastic bounce recoil: `d0 = (-d0) >> 1` (50% restitution).
    - Sets new upward velocity: `+$56(a6) = d0`.
    - Horizontal friction: `+$4E(a6) >>= 1; +$52(a6) >>= 1` (50% reduction).
    - Toggles sprite h-flip flag: `+$18(a6) ^= 0x0800`.
    - Plays bounce impact SFX `#$400376` via `0x00E7E8`.
    - If recoil velocity `d0 < 0x4000`: clears velocities (`+$4E = 0, +$52 = 0, +$56 = 0`).
- Viewport culling check:
  * Margin `d7 = 0x30` (48 pixels).
  * Calls `0x00E610`. If out of bounds (`carry set`): branches to despawn (`0x00F038`).

### Subroutine 6: `Boss3A_DespawnAndNotifySpawner` (`0x00F038..0x00F054`, 30 bytes, 7 insns)
- Reads parent spawner pointer `a0 = +$AA(a6)`.
- If `a0 != a6` and `a0 != 0xFF19E8` (not self and not player Ali):
  * Writes notification word: `+$06(a0) = 0x0010`.
- Deallocates entity slot: `+$00(a6) = 0`.

---

## 3. Key Constants
| Constant | Value | Description |
|---|---|---|
| Gravity Acceleration | `-0x0000C000` | Downward acceleration per tick |
| Air Drag Damping | `0.75` | 75% retention `(v >> 1) + (v >> 2)` |
| Bounce Restitution | `0.5` | Inelastic floor recoil `(-v) >> 1` |
| Rest Velocity Threshold | `0x00004000` | Recoil speed below which motion ceases |
| Culling Margin | `48` (`0x30`) | Camera viewport margin in pixels |
| SFX Swing | `0x00B00224` | Audio sound ID for swing |
| SFX Roar | `0x00B00225` | Audio sound ID for roar |
| SFX Quake | `0x00600382` | Audio sound ID for ground quake |
| SFX Bounce | `0x00400376` | Audio sound ID for rock ground bounce |
| Screen Shake Frames | `4` | Written to `$FF17C4` |
| Spawner Message | `0x0010` | Written to `+$06(parent)` on despawn |
