#include "game/world/room_layer_loader.hpp"

#include <bit>

namespace oasis::game::world {

namespace {

[[nodiscard]] RoomLayerSpec build_layer_spec(
    std::uint8_t w_byte,
    std::uint8_t h_byte,
    std::uint32_t& cursor) noexcept {
    RoomLayerSpec spec{};
    spec.tiles_w = static_cast<std::uint16_t>(static_cast<std::uint16_t>(w_byte & 0x7FU) << 5U);
    spec.tiles_h = static_cast<std::uint16_t>(static_cast<std::uint16_t>(h_byte) << 5U);
    spec.row_shift = compute_row_shift(spec.tiles_w);
    spec.tilemap_offset = cursor;
    spec.tilemap_bytes =
        static_cast<std::uint32_t>(spec.tiles_w) * static_cast<std::uint32_t>(spec.tiles_h) * 2U;
    cursor += spec.tilemap_bytes;

    spec.has_collision = (w_byte & 0x80U) != 0U;
    if (spec.has_collision && spec.tilemap_bytes > 0U) {
        spec.collision_offset = cursor;
        spec.collision_bytes = spec.tilemap_bytes / 2U;
        cursor += spec.collision_bytes;
    }
    return spec;
}

} // namespace

std::uint8_t compute_row_shift(std::uint16_t tiles_w) noexcept {
    if (tiles_w == 0U) {
        return 0U;
    }
    return static_cast<std::uint8_t>(std::countr_zero(static_cast<std::uint32_t>(tiles_w) * 2U));
}

RoomLayerLayout decode_room_layer_header(
    const std::array<std::uint8_t, 4>& header4) noexcept {
    RoomLayerLayout layout{};
    std::uint32_t cursor = 4U;
    layout.plane_a = build_layer_spec(header4[0], header4[1], cursor);
    layout.plane_b = build_layer_spec(header4[2], header4[3], cursor);
    return layout;
}

} // namespace oasis::game::world
