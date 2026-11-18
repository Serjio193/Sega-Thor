#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::collision {

/// Configuration and constants for ROM Tile Rect vs Player Ali AABB Containment
/// & Overlap Collision Testers (0x00C17A..0x00C222, 168 bytes).
struct PlayerRectCollisionConfig {
    static constexpr std::uint32_t kPlayerInsideRectAddr = 0x00C17AU;
    static constexpr std::uint32_t kPlayerInsideFailAddr = 0x00C1C4U;
    static constexpr std::uint32_t kPlayerInsideRtsAddr = 0x00C1CCU;

    static constexpr std::uint32_t kPlayerOverlapsRectAddr = 0x00C1CEU;
    static constexpr std::uint32_t kPlayerOverlapsFailAddr = 0x00C218U;
    static constexpr std::uint32_t kPlayerOverlapsRtsAddr = 0x00C220U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C222U;
    static constexpr std::size_t kRoutineSizeBytes = 168U;

    // RAM Addresses & Offsets
    static constexpr std::uint32_t kPlayerEntityRam = 0x00FF19E8U;
    static constexpr std::size_t kPlayerWorldXOffset = 0x08U;
    static constexpr std::size_t kPlayerWorldYOffset = 0x0CU;
    static constexpr std::size_t kPlayerCollisionRadOffset = 0x46U;

    // Tile Coordinate Scaling
    static constexpr std::uint32_t kTileScaleShift = 3U; // * 8
    static constexpr std::uint32_t kMaxExtentBias = 0x00070007U;
    static constexpr std::int16_t kTilePixelSpan = 7;
};

/// 2D Tile Rectangle defined in tile grid coordinates.
struct TileRect {
    std::uint16_t x_min{0U};
    std::uint16_t y_min{0U};
    std::uint16_t x_max{0U};
    std::uint16_t y_max{0U};

    [[nodiscard]] constexpr bool operator==(const TileRect&) const noexcept = default;
};

/// 2D Pixel Axis-Aligned Bounding Box (inclusive).
struct PixelAabb2D {
    std::int16_t x_min{0};
    std::int16_t y_min{0};
    std::int16_t x_max{0};
    std::int16_t y_max{0};

    [[nodiscard]] constexpr bool operator==(const PixelAabb2D&) const noexcept = default;
};

/// Converts tile rectangle to inclusive pixel bounding box:
/// [x_min * 8, x_max * 8 + 7] and [y_min * 8, y_max * 8 + 7].
[[nodiscard]] constexpr PixelAabb2D unpack_tile_rect_pixel_bounds(const TileRect& rect) noexcept {
    return PixelAabb2D{
        .x_min = static_cast<std::int16_t>(rect.x_min << PlayerRectCollisionConfig::kTileScaleShift),
        .y_min = static_cast<std::int16_t>(rect.y_min << PlayerRectCollisionConfig::kTileScaleShift),
        .x_max = static_cast<std::int16_t>((rect.x_max << PlayerRectCollisionConfig::kTileScaleShift) + PlayerRectCollisionConfig::kTilePixelSpan),
        .y_max = static_cast<std::int16_t>((rect.y_max << PlayerRectCollisionConfig::kTileScaleShift) + PlayerRectCollisionConfig::kTilePixelSpan)
    };
}

/// Unpacks from packed 68000 register format:
/// d1 = (x_min << 16) | y_min
/// d2 = (x_max << 16) | y_max
[[nodiscard]] constexpr PixelAabb2D unpack_registers_pixel_bounds(
    std::uint32_t reg_d1,
    std::uint32_t reg_d2) noexcept {
    const auto d1_scaled = static_cast<std::uint32_t>(reg_d1 << 3U);
    const auto d2_scaled = static_cast<std::uint32_t>(reg_d2 << 3U) + PlayerRectCollisionConfig::kMaxExtentBias;

    const auto y_min = static_cast<std::int16_t>(d1_scaled & 0xFFFFU);
    const auto y_max = static_cast<std::int16_t>(d2_scaled & 0xFFFFU);
    const auto x_min = static_cast<std::int16_t>((d1_scaled >> 16U) & 0xFFFFU);
    const auto x_max = static_cast<std::int16_t>((d2_scaled >> 16U) & 0xFFFFU);

    return PixelAabb2D{
        .x_min = x_min,
        .y_min = y_min,
        .x_max = x_max,
        .y_max = y_max
    };
}

/// Player state required for rectangle collision queries.
struct PlayerCollisionState {
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t radius{0};

    [[nodiscard]] constexpr PixelAabb2D to_aabb() const noexcept {
        return PixelAabb2D{
            .x_min = static_cast<std::int16_t>(world_x - radius),
            .y_min = static_cast<std::int16_t>(world_y - radius),
            .x_max = static_cast<std::int16_t>(world_x + radius),
            .y_max = static_cast<std::int16_t>(world_y + radius)
        };
    }

    [[nodiscard]] constexpr bool operator==(const PlayerCollisionState&) const noexcept = default;
};

/// Evaluates if Player AABB is strictly contained inside the tile pixel bounds (0x00C17A).
[[nodiscard]] constexpr bool is_player_inside_pixel_bounds(
    const PixelAabb2D& rect_bounds,
    const PlayerCollisionState& player) noexcept {
    const PixelAabb2D player_aabb = player.to_aabb();
    return (player_aabb.x_min >= rect_bounds.x_min) &&
           (player_aabb.x_max <= rect_bounds.x_max) &&
           (player_aabb.y_min >= rect_bounds.y_min) &&
           (player_aabb.y_max <= rect_bounds.y_max);
}

/// Evaluates if Player AABB overlaps / intersects the tile pixel bounds (0x00C1CE).
[[nodiscard]] constexpr bool does_player_overlap_pixel_bounds(
    const PixelAabb2D& rect_bounds,
    const PlayerCollisionState& player) noexcept {
    const PixelAabb2D player_aabb = player.to_aabb();
    return (player_aabb.x_min <= rect_bounds.x_max) &&
           (player_aabb.x_max >= rect_bounds.x_min) &&
           (player_aabb.y_min <= rect_bounds.y_max) &&
           (player_aabb.y_max >= rect_bounds.y_min);
}

[[nodiscard]] constexpr bool is_player_inside_rect(
    const TileRect& rect,
    const PlayerCollisionState& player) noexcept {
    return is_player_inside_pixel_bounds(unpack_tile_rect_pixel_bounds(rect), player);
}

[[nodiscard]] constexpr bool does_player_overlap_rect(
    const TileRect& rect,
    const PlayerCollisionState& player) noexcept {
    return does_player_overlap_pixel_bounds(unpack_tile_rect_pixel_bounds(rect), player);
}

[[nodiscard]] constexpr bool is_player_inside_packed(
    std::uint32_t reg_d1,
    std::uint32_t reg_d2,
    const PlayerCollisionState& player) noexcept {
    return is_player_inside_pixel_bounds(unpack_registers_pixel_bounds(reg_d1, reg_d2), player);
}

[[nodiscard]] constexpr bool does_player_overlap_packed(
    std::uint32_t reg_d1,
    std::uint32_t reg_d2,
    const PlayerCollisionState& player) noexcept {
    return does_player_overlap_pixel_bounds(unpack_registers_pixel_bounds(reg_d1, reg_d2), player);
}

// ============================================================================
// Raw 64KB RAM Window Integration Helpers ($FF19E8)
// ============================================================================

[[nodiscard]] PlayerCollisionState read_player_collision_raw(std::span<const std::uint8_t> ram);
void write_player_collision_raw(std::span<std::uint8_t> ram, const PlayerCollisionState& player);

[[nodiscard]] bool is_player_inside_rect_raw(std::span<const std::uint8_t> ram, const TileRect& rect);
[[nodiscard]] bool does_player_overlap_rect_raw(std::span<const std::uint8_t> ram, const TileRect& rect);

[[nodiscard]] bool is_player_inside_packed_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t reg_d1,
    std::uint32_t reg_d2);

[[nodiscard]] bool does_player_overlap_packed_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t reg_d1,
    std::uint32_t reg_d2);

} // namespace oasis::game::collision
