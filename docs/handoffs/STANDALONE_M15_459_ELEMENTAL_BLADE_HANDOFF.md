# STANDALONE HANDOFF: Milestone M15.459 — ROM Elemental Sword Wave Projectile Spawner, Parameter Evaluator & Trajectory Engine

## 1. Context & Range
- **Milestone:** M15.459
- **ROM Addresses:** `0x00F258..0x00F396` (318 bytes)
- **Target Subsystem:** Player Ali Elemental Sword 360-Spin Projectile Slash Wave Spawner & Parameter Evaluator:
  * Ground elemental blade entry (`0x00F258`) & ground sword-tip offset table (`0x00F262`)
  * Water elemental blade entry (`0x00F26A`) & water sword-tip offset table (`0x00F274`)
  * Shared projectile slot initializer at `$FF1B60` (`0x00F27C..0x00F396`)
  * Sword type parameter decoding: Fire Sword (`0x80000000`), Omega Sword (`0x40000000`), Hyper Sword (`0x20000000`)
  * 8-way directional velocity projection via `0x00E2D4`
  * Subpixel anchor offset computation and elevation float velocity `+0x0000C000`
- **Evidence JSON:** `data/thor-evidence/rom_elemental_blade_m15_459.json`
- **Unit Tests:** `tests/rom_elemental_blade_test.py`

---

## 2. Subroutines & ROM Architecture

### Subroutine 1: `ElementalBlade_GroundEntry` (`0x00F258..0x00F262`, 10 bytes)
- Called by Ground Sword Spin Attack when Button A charge is released (`0x007328`).
- Decrements weapon durability via `0x008530`.
- Loads ground sword-tip offset table pointer: `a2 = 0x00F262`.
- Branches to shared initializer: `bra.b 0x00F27C`.

### Subroutine 2: `ElementalBlade_GroundFacingOffsetsTable` (`0x00F262..0x00F26A`, 8 bytes)
4 packed words `(int8_t dx, int8_t dy)` representing sword-tip anchor relative to Ali:
| Facing | Direction | Raw Word | `offset_dx` | `offset_dy` |
|---|---|---|---|---|
| 0 | Up | `0x01D8` | `+1` | `-40` (spawns 40px above Ali) |
| 1 | Right | `0x10E6` | `+16` | `-26` (spawns 16px right, 26px above) |
| 2 | Down | `0xFD00` | `-3` | `0` (spawns at feet) |
| 3 | Left | `0xF0E6` | `-16` | `-26` (spawns 16px left, 26px above) |

### Subroutine 3: `ElementalBlade_WaterEntry` (`0x00F26A..0x00F274`, 10 bytes)
- Called by Water Sword Spin Attack (`0x00735E`).
- Decrements weapon durability via `0x008530`.
- Loads water sword-tip offset table pointer: `a2 = 0x00F274`.
- Branches to shared initializer: `bra.b 0x00F27C`.

### Subroutine 4: `ElementalBlade_WaterFacingOffsetsTable` (`0x00F274..0x00F27C`, 8 bytes)
4 packed words `(int8_t dx, int8_t dy)` adjusted for water swimming depth:
| Facing | Direction | Raw Word | `offset_dx` | `offset_dy` |
|---|---|---|---|---|
| 0 | Up | `0x03E0` | `+3` | `-32` |
| 1 | Right | `0x14EF` | `+20` | `-17` |
| 2 | Down | `0xFD04` | `-3` | `+4` |
| 3 | Left | `0xECEF` | `-20` | `-17` |

### Subroutine 5: `ElementalBlade_SharedInitializer` (`0x00F27C..0x00F396`, 282 bytes)
- Preserves caller pointer `a4 = a6` (Ali) and parameter `a3 = d0`.
- Allocates fixed projectile slot `a6 = $FF1B60`.
- Sets type `+$00(a6) = 0x0034` (Slash Wave Projectile).
- Invokes entity constructor `0x008D06`.
- Sets sprite priority `+$18(a6) = 0x055E`.
- Hitbox dimensions: `+$46 = 6, +$48 = 6, +$49 = 6, +$4A = 8`.
- Copies facing `+$16` and world coordinates `+$08, +$0C` from Ali `a4`.
- Elevation `+$10(a6) = d0.w` (from low word of input parameter).
- Initializes animation frame table via `0x009D00`.
- Decodes sword type bitmask on `d0`:
  * Default: `damage = 3, anim = 2, speed = (0x0C00, 0x0BC0)`.
  * Fire Sword (`bit 31` set): `damage = 0x2004, anim = 4, bset #1, $3C(a6)`.
  * Omega Sword (`bit 30` set): `+$A7 = 0xFF` (piercing), `damage = 0x8005, speed = (0x1000, 0x0F00)`.
  * Hyper Sword (`bit 29` set): `damage = 0x0000, anim = 4, bset #1, $3C(a6), speed = (0x0E00, 0x0D00)`.
- Projects 8-way directional velocity via `0x00E2D4` along facing `d3 = +$16(a6)` into `+$4E` and `+$52`.
- Sets flags: `+$3D = 0, +$38 = 6`.
- Unpacks sword-tip anchor from `(a2, facing * 2)`:
  * `anchor_x = dx << 16`
  * `anchor_y = (dy + elevation) << 16`
  * If bit 0 of `$FF17B8` set: doubles anchors (`anchor_x *= 2, anchor_y *= 2`).
  * Stores to `+$72(a6)` and `+$76(a6)`.
- Initializes upward elevation float velocity: `+$56(a6) = 0x0000C000`.
- Initializes lifetime counter: `+$A6(a6) = 0xF0` (240 frames).
- Stores owner pointer: `+$AA(a6) = a4` (Ali).
- Restores `a6 = a4` and returns.

---

## 3. Recommended C++20 Interface
```cpp
namespace thor::combat {

enum class SwordWaveType : uint32_t {
    Standard = 0,
    Fire = 0x80000000,
    Omega = 0x40000000,
    Hyper = 0x20000000
};

struct SwordWaveConfig {
    SwordWaveType type{SwordWaveType::Standard};
    uint16_t damage{3};
    uint16_t anim_id{2};
    bool piercing{false};
    bool flag_3c_bit1{false};
    uint16_t speed_cardinal{0x0C00};
    uint16_t speed_diagonal{0x0BC0};
};

SwordWaveConfig get_sword_wave_config(SwordWaveType type);

} // namespace thor::combat
```
