#include "game/world/terrain_probe_and_clamp.hpp"

namespace oasis::game::world {

std::uint8_t lookup_terrain_surface_effect(std::uint8_t tile_byte) noexcept {
    return kTerrainSurfaceLut16[tile_byte & 0x0FU];
}

RoomClampResult clamp_entity_to_room_bounds(
    std::int32_t x,
    std::int32_t y,
    std::int32_t dx,
    std::int32_t dy,
    std::int32_t radius,
    std::uint16_t room_tiles_w,
    std::uint16_t room_tiles_h) noexcept {
    RoomClampResult out{};
    out.dx = dx;
    out.dy = dy;

    const std::int32_t max_x = static_cast<std::int32_t>(room_tiles_w) * 8 - 1;
    const std::int32_t max_y = static_cast<std::int32_t>(room_tiles_h) * 8 - 1;

    const std::int32_t next_x = x + dx;
    if (next_x + radius > max_x) {
        out.dx = (max_x - radius) - x;
        out.col_x_flags |= kRoomEdgeWallFlag;
    } else if (next_x - radius < 0) {
        out.dx = radius - x;
        out.col_x_flags |= kRoomEdgeWallFlag;
    }

    const std::int32_t next_y = y + dy;
    if (next_y + radius > max_y) {
        out.dy = (max_y - radius) - y;
        out.col_y_flags |= kRoomEdgeWallFlag;
    } else if (next_y - radius < 0) {
        out.dy = radius - y;
        out.col_y_flags |= kRoomEdgeWallFlag;
    }

    return out;
}

} // namespace oasis::game::world
