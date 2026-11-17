#include "game/world/player_tile_box.hpp"

namespace oasis::game::world {

namespace {

struct TileBoxBounds {
    std::int32_t min_x{0};
    std::int32_t min_y{0};
    std::int32_t max_x{0};
    std::int32_t max_y{0};
};

[[nodiscard]] constexpr TileBoxBounds make_tile_box_bounds(
    std::int32_t tx1,
    std::int32_t ty1,
    std::int32_t tx2,
    std::int32_t ty2) noexcept {
    return TileBoxBounds{
        tx1 * (1 << kTileShift),
        ty1 * (1 << kTileShift),
        tx2 * (1 << kTileShift) + kTileMaxBias,
        ty2 * (1 << kTileShift) + kTileMaxBias,
    };
}

} // namespace

bool player_inside_tile_box(
    std::int32_t px,
    std::int32_t py,
    std::int32_t half_extent,
    std::int32_t tx1,
    std::int32_t ty1,
    std::int32_t tx2,
    std::int32_t ty2) noexcept {
    const TileBoxBounds box = make_tile_box_bounds(tx1, ty1, tx2, ty2);
    return (px - half_extent) >= box.min_x &&
           (px + half_extent) <= box.max_x &&
           (py - half_extent) >= box.min_y &&
           (py + half_extent) <= box.max_y;
}

bool player_overlaps_tile_box(
    std::int32_t px,
    std::int32_t py,
    std::int32_t half_extent,
    std::int32_t tx1,
    std::int32_t ty1,
    std::int32_t tx2,
    std::int32_t ty2) noexcept {
    const TileBoxBounds box = make_tile_box_bounds(tx1, ty1, tx2, ty2);
    return (px + half_extent) >= box.min_x &&
           (px - half_extent) <= box.max_x &&
           (py + half_extent) >= box.min_y &&
           (py - half_extent) <= box.max_y;
}

} // namespace oasis::game::world
