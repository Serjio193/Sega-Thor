# STANDALONE HANDOFF: Milestone M15.460 — ROM Player Ali Bow Arrow Projectile Spawner, Ground/Water Offsets & Parabolic Launch Physics

## 1. Context & Range
- **Milestone:** M15.460
- **ROM Addresses:** `0x00F396..0x00F476` (224 bytes)
- **Target Subsystem:** Player Ali Bow Arrow Projectile Spawner, Facing Launch Offset Tables & Elevation-Dependent Parabolic Trajectory Engine:
  * Ground bow arrow entry (`0x00F396`) & launch offset table `0x00F3A0`
  * Water bow arrow entry (`0x00F3A8`) & launch offset table `0x00F3B2`
  * Shared arrow projectile initializer at `$FF1B60` (`0x00F3BA..0x00F476`)
  * Subpixel anchor calculation with screen scale doubling (`bit 0 of $FF17B8`)
  * Elevation threshold (`16px`) for high vs low launch arcs
  * 8-way directional velocity projection via `0x00E2D4`
- **Evidence JSON:** `data/thor-evidence/rom_bow_arrow_engine_m15_460.json`
- **Unit Tests:** `tests/rom_bow_arrow_engine_test.py`

---

## 2. Subroutines & ROM Architecture

### Subroutine 1: `BowArrow_GroundEntry` (`0x00F396..0x00F3A0`, 10 bytes)
- Called by Ground Bow Release when Button B is released (`0x0073B4` / `0x0073C0`).
- Checks durability via `0x008530`.
- Loads ground launch offset table: `a2 = 0x00F3A0`.
- Branches to shared initializer: `bra.b 0x00F3BA`.

### Subroutine 2: `BowArrow_GroundOffsetsTable` (`0x00F3A0..0x00F3A8`, 8 bytes)
4 packed words `(int8_t dx, int8_t dy)` representing arrow tip offset:
| Facing | Direction | Raw Word | `offset_dx` | `offset_dy` |
|---|---|---|---|---|
| 0 | Up | `0x06C8` | `+6` | `-56` |
| 1 | Right | `0x20DE` | `+32` | `-34` |
| 2 | Down | `0xF6F8` | `-10` | `-8` |
| 3 | Left | `0xE0DE` | `-32` | `-34` |

### Subroutine 3: `BowArrow_WaterEntry` (`0x00F3A8..0x00F3B2`, 10 bytes)
- Called by Water Bow Release (`0x0073DE` / `0x0073EA`).
- Checks durability via `0x008530`.
- Loads water launch offset table: `a2 = 0x00F3B2`.
- Branches to shared initializer: `bra.b 0x00F3BA`.

### Subroutine 4: `BowArrow_WaterOffsetsTable` (`0x00F3B2..0x00F3BA`, 8 bytes)
4 packed words `(int8_t dx, int8_t dy)` adjusted for water swimming depth:
| Facing | Direction | Raw Word | `offset_dx` | `offset_dy` |
|---|---|---|---|---|
| 0 | Up | `0x0EEE` | `+14` | `-18` |
| 1 | Right | `0x1204` | `+18` | `+4` |
| 2 | Down | `0xFA14` | `-6` | `+20` |
| 3 | Left | `0xEE04` | `-18` | `+4` |

### Subroutine 5: `BowArrow_SharedInitializer` (`0x00F3BA..0x00F476`, 188 bytes)
- Preserves caller pointer `a4 = a6` (Ali).
- Allocates fixed projectile slot `a6 = $FF1B60`.
- Sets type `+$00(a6) = 0x003A` (Arrow Projectile Entity).
- Invokes constructor `0x008D06`.
- Copies Ali sprite priority: `+$18(a6) = +$5A(a4)`.
- Arrow subtype: `+$A6(a6) = d0.w` (0=Normal, 1=Fire Bow, 2=Bomb Arrow).
- Arrow lifetime/range: `+$02(a6) = d2.w`.
- Computes launch subpixel anchor from `(a2, facing * 2)`:
  * `anchor_x = dx << 16`
  * `anchor_y = (dy + d1) << 16`
  * If bit 0 of `$FF17B8` set: doubles anchors (`anchor_x *= 2, anchor_y *= 2`).
  * Stores to `+$72(a6)` and `+$76(a6)`.
- Sets initial elevation: `+$10(a6) = d1 << 16`.
- World coordinates: `+$08(a6) = +$08(a4), +$0C(a6) = +$0C(a4)`.
- Animation table setup: `0x009D00`.
- Combat stats: `damage_a8 = 0x8005, anim_2a = 0, state_4 = 0`.
- Elevation-dependent trajectory arc selection:
  * If `+$10(a6) >= 16` (high arc from elevation):
    - Horizontal speeds: `d1 = 0x0300, d2 = 0x0300`.
    - Upward elevation launch velocity: `+$56(a6) = 0x00020000` (+2.0 px/frame).
  * If `+$10(a6) < 16` (flat/low arc from ground):
    - Horizontal speeds: `d1 = 0x0400, d2 = 0x0400`.
    - Upward elevation launch velocity: `+$56(a6) = 0x00028000` (+2.5 px/frame).
- Projects 8-way directional velocity via `0x00E2D4` along facing `d3 = +$16(a4)` into `+$4E` and `+$52`.
- Stores owner Ali pointer: `+$AA(a6) = a4`.
- Restores `a6 = a4` and returns.

---

## 3. Recommended C++20 Interface
```cpp
namespace thor::combat {

enum class ArrowType : uint16_t {
    Normal = 0,
    Fire = 1,
    Bomb = 2
};

struct ArrowLaunchParams {
    ArrowType type{ArrowType::Normal};
    uint16_t lifetime{60};
    int16_t initial_elevation{0};
    uint16_t speed_horizontal{0x0400};
    int32_t launch_elevation_velocity{0x00028000};
};

ArrowLaunchParams calculate_arrow_launch_params(ArrowType type, int16_t elevation);

} // namespace thor::combat
```
