#pragma once

#include <cstdint>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kStreamColumnAddr = 0x0023CCU;
inline constexpr std::uint32_t kStreamRowAddr = 0x002512U;
inline constexpr std::uint16_t kScrollColumnAutoInc = 0x8F80U;
inline constexpr std::uint16_t kScrollRowAutoInc = 0x8F02U;
inline constexpr std::uint16_t kScrollForwardAheadCols = 40U;
inline constexpr std::uint16_t kScrollForwardAheadColBytes = 0x0050U;
inline constexpr std::uint16_t kScrollTilesPerColumn = 29U;
inline constexpr std::uint16_t kScrollWrapSentinelHeight = 0x03E8U; // 1000
inline constexpr std::uint16_t kScrollForwardAheadRows = 28U;
inline constexpr std::uint16_t kScrollForwardAheadRowBytes = 0x0E00U;
inline constexpr std::uint16_t kScrollTilesPerRow = 41U;
inline constexpr std::uint16_t kVdpColByteMask = 0x007EU;
inline constexpr std::uint16_t kVdpRowByteMask = 0x0F80U;
inline constexpr std::uint16_t kVdpScrollRowStrideBytes = 0x0080U;
inline constexpr std::size_t kVdpScrollNametableWords = 2048U; // 0x1000 bytes

/// Plane scroll state matching ROM struct a3 (0x08..0x25).
struct VdpPlaneScrollState {
    std::int16_t src_col{0};           // +0x08
    std::int16_t src_row{0};           // +0x0A
    std::uint16_t vram_offset{0};      // +0x0C (0x0000..0x0FFE)
    std::uint16_t vram_base{0xC000U};  // +0x1A
    std::int16_t pending_dx_tiles{0};  // +0x1C
    std::int16_t pending_dy_tiles{0};  // +0x1E
    std::uint16_t map_width_tiles{0};  // +0x20
    std::uint16_t map_height_tiles{0}; // +0x22
    std::uint16_t row_shift{0};        // +0x24
};

/// Models 0x0023CC..0x002510: streams 1 column (29 tiles) into the 64x32 VDP
/// nametable, stepping src_col/vram_offset according to pending_dx_tiles.
bool stream_plane_column(
    VdpPlaneScrollState& state,
    std::span<const std::uint16_t> src_tilemap,
    std::span<std::uint16_t, kVdpScrollNametableWords> vdp_nametable) noexcept;

/// Models 0x002512..0x002630: streams 1 row (41 tiles) into the 64x32 VDP
/// nametable, stepping src_row/vram_offset according to pending_dy_tiles.
bool stream_plane_row(
    VdpPlaneScrollState& state,
    std::span<const std::uint16_t> src_tilemap,
    std::span<std::uint16_t, kVdpScrollNametableWords> vdp_nametable) noexcept;

} // namespace oasis::game::world
