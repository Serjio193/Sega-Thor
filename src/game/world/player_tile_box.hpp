#pragma once

#include <cstdint>

namespace oasis::game::world {

inline constexpr std::uint32_t kPlayerInsideTileBoxAddr = 0x00C17AU;
inline constexpr std::uint32_t kPlayerOverlapsTileBoxAddr = 0x00C1CEU;
inline constexpr std::uint32_t kPlayerEntityRamAddr = 0x00FF19E8U;

inline constexpr std::int32_t kTileShift = 3;
inline constexpr std::int32_t kTileMaxBias = 7; // packed 0x00070007
inline constexpr std::uint32_t kTileMaxBiasPacked = 0x00070007U;

/// Models 0x00C17A..0x00C1CC: returns true when Ali's AABB
/// [px - half_extent, px + half_extent] x [py - half_extent, py + half_extent]
/// is fully inside [tx1 * 8, tx2 * 8 + 7] x [ty1 * 8, ty2 * 8 + 7].
[[nodiscard]] bool player_inside_tile_box(
    std::int32_t px,
    std::int32_t py,
    std::int32_t half_extent,
    std::int32_t tx1,
    std::int32_t ty1,
    std::int32_t tx2,
    std::int32_t ty2) noexcept;

/// Models 0x00C1CE..0x00C220: returns true when Ali's AABB
/// [px - half_extent, px + half_extent] x [py - half_extent, py + half_extent]
/// overlaps [tx1 * 8, tx2 * 8 + 7] x [ty1 * 8, ty2 * 8 + 7].
[[nodiscard]] bool player_overlaps_tile_box(
    std::int32_t px,
    std::int32_t py,
    std::int32_t half_extent,
    std::int32_t tx1,
    std::int32_t ty1,
    std::int32_t tx2,
    std::int32_t ty2) noexcept;

} // namespace oasis::game::world
