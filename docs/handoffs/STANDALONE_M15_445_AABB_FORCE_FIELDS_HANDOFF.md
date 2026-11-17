# STANDALONE HANDOFF: M15.445 — ROM 3D AABB Entity Force Vector Field, Conveyor Belt & Elevation Current Displacement Subsystem

- **Source Routine:** `entity_3d_aabb_force_field_subsystem` (`0x00DDA2..0x00DEEC`, 330 bytes).
- **Domain:** Entity Physics / Environmental Force Fields / Conveyor Belts / Currents.
- **Evidence Reference:** `data/thor-evidence/rom_aabb_force_fields_m15_445.json`.
- **Test Reference:** `tests/rom_aabb_force_fields_test.py`.

---

## 1. Architectural Role & Context

In Beyond Oasis / The Story of Thor, environmental objects (such as conveyor belts, water currents, wind tunnels, floor traps, falling ramps, and push hazards) apply directional force vectors to characters and objects located within specified 3D bounding areas.

The subsystem at `0x00DDA2..0x00DEEC` provides three unified entrypoints that iterate over all 21 active entity slots in the master entity pool (`$FF19E8`, stride `$BC` / 188 bytes, slot 0 = Player Ali, slots 1..20 = NPCs / enemies / dynamic objects):

1. **`entity_apply_box_force_vector` (`0x00DDA2..0x00DE00`, 94 bytes):**
   - Standard environmental force field (used by water flumes, wind currents, and ice slides).
   - Verifies entity active status (`tst.w $0(a0); ble skip`).
   - Performs 3D Axis-Aligned Bounding Box (AABB) intersection check:
     - `min_y <= entity.y <= max_y`
     - `min_x <= entity.x <= max_x`
     - `min_z <= entity.z <= max_z`
   - If inside the volume: adds 32-bit vector `d1` to entity X accumulator (`+$72(a0)`) and `d2` to entity Y accumulator (`+$76(a0)`).

2. **`entity_apply_box_force_vector_with_elevation_boost` (`0x00DE00..0x00DE6C`, 108 bytes):**
   - Performs the same 3D AABB check.
   - Adds single force `(d1, d2)`.
   - If entity has non-zero elevation (`$10(a0) != 0`, e.g. jumping or suspended in air), applies `(d1, d2)` a second time, providing a 2x force boost to airborne entities!

3. **`entity_apply_conveyor_current_force` (`0x00DE6C..0x00DEEC`, 128 bytes):**
   - Conveyor belt and water whirlpool engine.
   - Evaluates immunity flags at `+$3A(a0)`:
     - If bit 0 is set (`btst #0, $3a(a0)`), entity is immune (e.g. pinned or heavy object) -> skipped.
     - If bit 3 is set (`btst #3, $3a(a0)`), entity is immune (e.g. flying or rooted) -> skipped.
   - Performs 3D AABB check.
   - Applies force with airborne doubling (`$10(a0) != 0`).

---

## 2. Register Conventions

| Register | Direction | Semantic Meaning |
|---|---|---|
| `a0` | Scratch | Current entity pointer (`$FF19E8 + i * 0xBC`) |
| `d0` | Scratch | Loop counter (initialized to `#$14` for 21 entities) |
| `d1` | In | 32-bit X force vector / displacement accumulator addend |
| `d2` | In | 32-bit Y force vector / displacement accumulator addend |
| `d3` | In | Low word: min Y; High word (swapped): min X |
| `d4` | In | Low word: max Y; High word (swapped): max X |
| `d5` | In | Low word: max Z; High word (swapped): min Z |
| `d6` | Scratch | Entity coordinate scratch register |

---

## 3. Entity Memory Layout

| Offset | Size | Type | Field Semantic |
|---|---|---|---|
| `+$00` | Word | `int16_t` | Entity status / type (`> 0` indicates active / alive) |
| `+$08` | Word | `int16_t` | Entity X position |
| `+$0C` | Word | `int16_t` | Entity Y position |
| `+$10` | Long | `int32_t` | Entity Z / elevation |
| `+$3A` | Byte | `uint8_t` | Immunity bitflags (bit 0: conveyor immune, bit 3: current immune) |
| `+$72` | Long | `int32_t` | Accumulated X velocity / displacement delta |
| `+$76` | Long | `int32_t` | Accumulated Y velocity / displacement delta |

---

## 4. C++20 Cleanroom Interface Design

```cpp
#pragma once

#include <cstdint>
#include <span>

namespace thor::physics {

struct BoundingBox3D {
    int16_t min_x{0};
    int16_t max_x{0};
    int16_t min_y{0};
    int16_t max_y{0};
    int16_t min_z{0};
    int16_t max_z{0};

    [[nodiscard]] constexpr bool contains(int16_t x, int16_t y, int16_t z) const noexcept {
        return (y >= min_y && y <= max_y &&
                x >= min_x && x <= max_x &&
                z >= min_z && z <= max_z);
    }
};

struct EntitySlot {
    int16_t status{0};
    int16_t pos_x{0};
    int16_t pos_y{0};
    int32_t pos_z{0};
    uint8_t flags_3a{0};
    int32_t accum_dx{0};
    int32_t accum_dy{0};
};

inline void apply_box_force_vector(
    std::span<EntitySlot> entities,
    const BoundingBox3D& box,
    int32_t force_x,
    int32_t force_y
) {
    for (auto& ent : entities) {
        if (ent.status <= 0) continue;
        if (!box.contains(ent.pos_x, ent.pos_y, static_cast<int16_t>(ent.pos_z))) continue;
        ent.accum_dx += force_x;
        ent.accum_dy += force_y;
    }
}

inline void apply_box_force_vector_with_elevation_boost(
    std::span<EntitySlot> entities,
    const BoundingBox3D& box,
    int32_t force_x,
    int32_t force_y
) {
    for (auto& ent : entities) {
        if (ent.status <= 0) continue;
        if (!box.contains(ent.pos_x, ent.pos_y, static_cast<int16_t>(ent.pos_z))) continue;
        ent.accum_dx += force_x;
        ent.accum_dy += force_y;
        if (ent.pos_z != 0) {
            ent.accum_dx += force_x;
            ent.accum_dy += force_y;
        }
    }
}

inline void apply_conveyor_current_force(
    std::span<EntitySlot> entities,
    const BoundingBox3D& box,
    int32_t force_x,
    int32_t force_y
) {
    for (auto& ent : entities) {
        if (ent.status <= 0) continue;
        if ((ent.flags_3a & 0x01) != 0 || (ent.flags_3a & 0x08) != 0) continue;
        if (!box.contains(ent.pos_x, ent.pos_y, static_cast<int16_t>(ent.pos_z))) continue;
        ent.accum_dx += force_x;
        ent.accum_dy += force_y;
        if (ent.pos_z != 0) {
            ent.accum_dx += force_x;
            ent.accum_dy += force_y;
        }
    }
}

} // namespace thor::physics
```
