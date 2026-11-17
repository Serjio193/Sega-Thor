# STANDALONE HANDOFF: ROM Tile Rect vs Player Ali AABB Containment & Overlap Collision Testers (0x00C17A..0x00C222)

## 1. Routine Overview
- **ROM Range:** `0x00C17A..0x00C222` (168 bytes).
- **Function:** Tile Rectangle vs Player Ali AABB Spatial Query Engine. Provides dual geometric tests: `player_inside_rect` (`0x00C17A`, strict AABB containment test) and `player_overlaps_rect` (`0x00C1CE`, AABB intersection / overlap test). Used by over 150 scripts, triggers, door transitions, and area bounds across Beyond Oasis.
- **Evidence JSON:** `data/thor-evidence/rom_player_rect_collision_m15_426.json`.

## 2. Ingress Entry Points
1. `0x00C17A` (`player_inside_rect`):
   - Tests if Player AABB (`$FF19E8`, radius `+0x46(a0)`) is **completely contained** within the tile rectangle.
   - Sets Carry flag if fully inside (`ori.b #1, ccr`); clears Carry if any part is outside (`andi.b #0xFE, ccr`).
2. `0x00C1CE` (`player_overlaps_rect`):
   - Tests if Player AABB **intersects / overlaps** the tile rectangle.
   - Sets Carry flag if overlapping (`ori.b #1, ccr`); clears Carry if completely disjoint (`andi.b #0xFE, ccr`).

## 3. Algorithm & Logic
1. **Coordinate Unpacking:**
   - Input register `d1`: `(tile_x_min << 16) | tile_y_min`.
   - Input register `d2`: `(tile_x_max << 16) | tile_y_max`.
   - Scaled by 8: `asl.l #3, d1`, `asl.l #3, d2`.
   - Pixel bias added: `addi.l #$00070007, d2` (gives inclusive tile pixel boundaries `+7`).
   - World pixel rectangle:
     * `rect_x = [tile_x_min * 8, tile_x_max * 8 + 7]`
     * `rect_y = [tile_y_min * 8, tile_y_max * 8 + 7]`
2. **Player AABB Construction:**
   - Reads Player position `X = +0x08(a0)`, `Y = +0x0C(a0)` and collision radius `rad = +0x46(a0)` from `$FF19E8`.
   - `px_min = X - rad`, `px_max = X + rad`.
   - `py_min = Y - rad`, `py_max = Y + rad`.
3. **Containment Evaluation (`0x00C17A`):**
   - Fails if `px_min < rect_x_min` or `px_max > rect_x_max`.
   - Fails if `py_min < rect_y_min` or `py_max > rect_y_max`.
   - If all pass: Carry = 1.
4. **Overlap Evaluation (`0x00C1CE`):**
   - Fails if `px_min > rect_x_max` or `px_max < rect_x_min`.
   - Fails if `py_min > rect_y_max` or `py_max < rect_y_min`.
   - If all pass: Carry = 1.

## 4. Recommended C++20 Interface
```cpp
namespace thor::collision {

struct TileRect {
    uint16_t x_min{0};
    uint16_t y_min{0};
    uint16_t x_max{0};
    uint16_t y_max{0};

    constexpr PixelAABB to_pixel_aabb() const no_except {
        return PixelAABB{
            .x_min = static_cast<int32_t>(x_min * 8),
            .y_min = static_cast<int32_t>(y_min * 8),
            .x_max = static_cast<int32_t>(x_max * 8 + 7),
            .y_max = static_cast<int32_t>(y_max * 8 + 7),
        };
    }
};

bool is_player_inside_rect(const TileRect& rect, const EntityState& player);
bool does_player_overlap_rect(const TileRect& rect, const EntityState& player);

} // namespace thor::collision
```
