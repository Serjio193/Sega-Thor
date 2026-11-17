#pragma once

#include <array>
#include <cstdint>

namespace oasis::game::world {

inline constexpr std::uint32_t kTerrainProbeAddr = 0x009D00U;
inline constexpr std::uint32_t kTerrainSurfaceLutAddr = 0x0096E8U;
inline constexpr std::uint32_t kRoomClampRightAddr = 0x009D4EU;
inline constexpr std::uint32_t kRoomClampLeftAddr = 0x009D70U;
inline constexpr std::uint32_t kRoomClampBottomAddr = 0x009D92U;
inline constexpr std::uint32_t kRoomClampTopAddr = 0x009DB4U;

inline constexpr std::uint32_t kPrimaryLayerDescRam = 0xFF1766U;
inline constexpr std::uint32_t kAltLayerDescRam = 0xFF1770U;
inline constexpr std::uint8_t kRoomEdgeWallFlag = 0x48U;

/// 16-byte terrain surface effect LUT at ROM 0x0096E8 (mapped into entity +0x70).
inline constexpr std::array<std::uint8_t, 16> kTerrainSurfaceLut16 = {
    0xFFU, 0U, 2U, 1U, 4U, 0xFFU, 3U, 0xFFU,
    6U, 7U, 0xFFU, 0xFFU, 5U, 0xFFU, 0xFFU, 0xFFU
};

/// Result of 4-side room boundary clamp (0x009D4E..0x009DD4).
struct RoomClampResult {
    std::int32_t dx{0};
    std::int32_t dy{0};
    std::uint8_t col_x_flags{0}; // +0x40: bitmask | 0x48 when clamped horizontally
    std::uint8_t col_y_flags{0}; // +0x41: bitmask | 0x48 when clamped vertically
};

/// Selects the active layer descriptor RAM address ($FF1770 if alt_layer_bit7, else $FF1766).
[[nodiscard]] constexpr std::uint32_t select_terrain_layer_desc(bool alt_layer_bit7) noexcept {
    return alt_layer_bit7 ? kAltLayerDescRam : kPrimaryLayerDescRam;
}

/// Maps (tile_byte & 0x0F) through kTerrainSurfaceLut16 (0x0096E8).
[[nodiscard]] std::uint8_t lookup_terrain_surface_effect(std::uint8_t tile_byte) noexcept;

/// Models 0x009D4E..0x009DD4: clamps (x + dx +/- radius) and (y + dy +/- radius)
/// against [0, tiles * 8 - 1], setting 0x48 in col_x_flags / col_y_flags on clamp.
[[nodiscard]] RoomClampResult clamp_entity_to_room_bounds(
    std::int32_t x,
    std::int32_t y,
    std::int32_t dx,
    std::int32_t dy,
    std::int32_t radius,
    std::uint16_t room_tiles_w,
    std::uint16_t room_tiles_h) noexcept;

} // namespace oasis::game::world
