# Standalone Handoff: Milestone M15.478 — ROM Entity AI Enemy Projectile Spawner Subsystem: Elemental Slash Waves, Fast Arrows & High-Arc Mortar Lob Trajectories

**ROM Range:** `0x010CFE..0x010EEA` (492 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements enemy projectile instantiation and trajectory physics for sword slash waves and bow arrows:

1. **`0x010CFE..0x010DB2` (`AI_SpawnEnemySwordSlashWaveProjectile`):**
   - Exchanges registers `exg a6, a5` to allocate child slot from parent enemy `a5`.
   - Initializes child projectile slot via constructor `jsr $8D06.l` with Type `#$34`.
   - Sets sprite priority `+$18 = #$55E`.
   - Hitbox dimensions: `rx=6 (+$46)`, `ry=6 (+$48)`, `rz=6 (+$49)`, `height=8 (+$4A)`.
   - Clones parent facing `+$16` and world coordinates `+$08 / +$0C`.
   - Sets elevation `+$10(a6) = d1.w`, initializes animation via `0x009D00`.
   - Reads 4-way offset table at `(a4)` indexed by facing: adds offsets to subpixel anchor `+$72 / +$76`.
   - Unpacks attack parameters from `(a3)`: damage `+$A8`, state `+$2A`, horizontal/vertical speed `(d1, d2)`.
   - Projects 8-way directional velocity via `0x00E2D4` into `+$4E` (`vel_x`) and `+$52` (`vel_y`).
   - Sets upward float velocity `+$56 = 0x0000C000`.
   - Sets subtype `+$A6 = 1`, parent back-pointer `+$AA = a5`.
   - If state `+$2A == 4`: sets bit 1 on status byte `+$3C`.
   - Restores parent pointer `a6 = a5; rts`.

2. **`0x010DB2..0x010E4E` (`AI_SpawnEnemyBowArrow_Fast`):**
   - Initializes Type `#$3A` via `jsr $8D06.l`.
   - Clones priority `+$18(a6) = +$5A(a5)`.
   - Unpacks facing-dependent anchor offsets `(a4)` into `+$72 / +$76`.
   - Unpacks 6-byte parameter record from table `(a3)` indexed by `+$96(a5) * 6`:
     * `+$A6(a6) = (a3)+` (arrow subtype)
     * `+$02(a6) = (a3)+` (lifetime)
     * `+$A8(a6) = (a3)` (damage)
   - Sets fast horizontal speed `0x0400 / 0x0400`.
   - Sets launch upward velocity `+$56 = 0x00028000`.
   - Directional velocity projection via `0x00E2D4` into `+$4E / +$52`.
   - Sets parent back-pointer `+$AA = a5`.
   - Restores `a6 = a5; rts`.

3. **`0x010E4E..0x010EEA` (`AI_SpawnEnemyBowArrow_HighArc`):**
   - Initializes Type `#$3A` via `jsr $8D06.l`.
   - Same anchor offset unpacking and 6-byte parameter table lookup.
   - Sets low horizontal speed `0x0100 / 0x0100` and high upward launch velocity `+$56 = 0x00040000` (steep parabolic mortar lob trajectory).
   - Directional velocity projection via `0x00E2D4`.
   - Restores `a6 = a5; rts`.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `ENTITY_CONSTRUCTOR` | `0x008D06` | Universal entity constructor |
| `ANIMATION_SETUP` | `0x009D00` | Universal animation and surface latch setup |
| `DIRECTIONAL_VELOCITY_PROJ` | `0x00E2D4` | 8-way directional velocity projector |
| `SLOT_TYPE_OFFSET` | `+$00` | Type 0x34 (Blade Wave), Type 0x3A (Bow Arrow) |
| `SLOT_LAUNCH_VEL_OFFSET` | `+$56` | 32-bit vertical launch/float velocity |
| `SLOT_PARENT_PTR_OFFSET` | `+$AA` | Parent entity pointer |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::ai {

struct SlashWaveParams {
    uint16_t damage{0};
    uint16_t state_2a{0};
    int16_t speed_x{0};
    int16_t speed_y{0};
};

struct ArrowRecordParams {
    uint16_t subtype{0};
    uint16_t lifetime{0};
    uint16_t damage{0};
};

struct Point2D {
    int16_t x{0};
    int16_t y{0};
};

void spawn_enemy_slash_wave(
    EntitySlot& child,
    const EntitySlot& parent,
    int16_t elevation,
    std::span<const Point2D, 4> offset_table,
    const SlashWaveParams& params);

void spawn_enemy_fast_arrow(
    EntitySlot& child,
    const EntitySlot& parent,
    int16_t elevation,
    std::span<const Point2D, 4> offset_table,
    const ArrowRecordParams& params);

void spawn_enemy_high_arc_arrow(
    EntitySlot& child,
    const EntitySlot& parent,
    int16_t elevation,
    std::span<const Point2D, 4> offset_table,
    const ArrowRecordParams& params);

} // namespace thor::ai
```
