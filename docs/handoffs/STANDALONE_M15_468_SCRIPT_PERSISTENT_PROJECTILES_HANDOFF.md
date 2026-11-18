# Standalone Handoff: Milestone M15.468 — ROM Script Virtual Machine Opcodes $15, $16, $18, $19: Persistent Gater, Spawner & Ballistic Projectile Engine

**ROM Range:** `0x00FEDE..0x010034` (342 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements persistent entity gating, packed entity instantiation, and dynamic particle/projectile spawning within the Cutscene & Event Script Virtual Machine:
1. **Opcode `$15` (`0x00FEDE..0x00FF1A`): Persistent Entity Spawn Gater:**
   - Evaluates a persistent bit across RAM banks:
     - `+1(a0) == 1`: Bank 1 (`$FF0DBC`)
     - `+1(a0) == 2`: Bank 2 (`$FF0E1C`)
     - `+1(a0) == 3`: Bank 3 (`$FF0E2C`)
   - Tests bit at `+4(a0)` (`bit = +4(a0) & 7, byte = +4(a0) >> 3`).
   - If bit is set (entity already defeated or item collected), skips the entire 18-byte (`$12`) entity spawn instruction (`lea.l $12(a0), a0; rts`).
   - If bit is clear, falls directly into Opcode `$16` to instantiate the entity.
2. **Opcode `$16` (`0x00FF1A..0x00FF98`): Persistent Entity Spawner:**
   - 18-byte packed entity declaration.
   - Allocates entity slot via `$DA2A`.
   - Initializes entity type `+2(a0)` via constructor `$8D06`.
   - Records persistent tracking info: bank `+$A2 = +1(a0)`, bit index `+$A4 = +4(a0)`.
   - Unpacks flags `+$17`, `+$9E`, `+$A0`, spatial position `(x, y)` at `+$08/+$0C`, drop item ID `+$8C`, sprite priority `+$18`.
   - Sets default HP: `+$88 = 10, +$8A = 10`.
   - Runs animation setup `$9D00`.
   - If entity type `>= 0x16` (22), activates AI from table `$DF54`.
   - Advances script PC by 18 bytes (`lea.l $12(a0), a0; rts`).
3. **Opcode `$18` (`0x00FF98..0x00FFDE`): Dynamic Script Particle Spawner:**
   - Allocates particle/effect slot via `$D9F0` (yields frame via `$F91C` on exhaustion).
   - Sets flag `+$4C = (a0)+`, type `(a0)+` (constructor `$8D06`).
   - Stores custom init routine pointer `+$56 = (a0)+`.
   - Sets coordinates `(x, y)` at `+$08/+$0C` and priority `+$18`.
   - Calls sprite/pattern setup `$9AD6`, storing VRAM tile pattern index into `+$14`.
   - If custom init pointer `+$56` is non-zero, executes `jsr (a0)`.
4. **Opcode `$19` (`0x00FFDE..0x010034`): Ballistic Projectile 3D Velocity Spawner:**
   - Allocates projectile slot via `$D9F0` (yields frame on exhaustion).
   - Sets flag `+$4C = (a0)+`, type `(a0)+` (constructor `$8D06`).
   - Forces physics update callback `+$56 = 0x017A96` (standard ballistic/gravity physics routine).
   - Sets spatial coordinates `(x, y)` and priority `+$18`.
   - Unpacks 3D velocity vector:
     - `+$4E = *(a0)+` (X velocity)
     - `+$52 = *(a0)+` (Y velocity)
     - `+$54 = *(a0)+` (Z / elevation velocity)
   - Calls sprite setup `$9AD6` (stores tile to `+$14`) and executes `0x017A96`.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `BANK1_BITS` | `$FF0DBC` | Persistent bitmask bank 1 |
| `BANK2_BITS` | `$FF0E1C` | Persistent bitmask bank 2 |
| `BANK3_BITS` | `$FF0E2C` | Persistent bitmask bank 3 |
| `BALLISTIC_PHYSICS_ROUTINE` | `0x017A96` | Standard ballistic arc physics updater |
| `AI_TABLE` | `0x00DF54` | AI controller jump table for enemy types `>= 0x16` |
| `SPRITE_TILE_SETUP` | `0x009AD6` | Sprite descriptor & VRAM tile mapping helper |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

struct PersistentEntityRecord {
    uint8_t bank_id{0};
    uint16_t entity_type{0};
    uint16_t bit_index{0};
    int16_t pos_x{0};
    int16_t pos_y{0};
    uint16_t drop_id{0};
    uint16_t priority{0};
};

struct BallisticProjectileParams {
    uint8_t flag_4c{0};
    uint16_t entity_type{0};
    int16_t pos_x{0};
    int16_t pos_y{0};
    uint16_t priority{0};
    int16_t vel_x{0};
    int16_t vel_y{0};
    int16_t vel_z{0};
};

bool evaluate_opcode_15_gater(
    const uint8_t*& pc,
    const ScriptEnv& env);

void execute_opcode_16_spawn_persistent(
    const uint8_t*& pc,
    const PersistentEntityRecord& record);

bool execute_opcode_18_spawn_particle(
    const uint8_t*& pc,
    uint8_t flag_4c,
    uint16_t entity_type,
    uint32_t init_ptr,
    int16_t x, int16_t y,
    uint16_t priority);

bool execute_opcode_19_spawn_ballistic_3d(
    const uint8_t*& pc,
    const BallisticProjectileParams& params);

} // namespace thor::script
```
