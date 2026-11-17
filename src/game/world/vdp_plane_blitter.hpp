#pragma once

#include <cstdint>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kVdpPlaneBlitterAddr = 0x00D800U;
inline constexpr std::uint16_t kVdpNametableCols = 64U;
inline constexpr std::uint16_t kVdpNametableRows = 32U;
inline constexpr std::size_t kVdpNametableWords =
    static_cast<std::size_t>(kVdpNametableCols) * kVdpNametableRows; // 2048 words = 0x1000 bytes
inline constexpr std::uint16_t kVdpRowBytes = 0x80U;                 // 128 bytes
inline constexpr std::uint16_t kViewportBlitCols = 41U;
inline constexpr std::uint16_t kViewportBlitRows = 29U;
inline constexpr std::uint16_t kFullPlaneBlitCols = 64U;
inline constexpr std::uint16_t kFullPlaneBlitRows = 32U;

struct BlitDimensions {
    std::uint16_t cols{0};
    std::uint16_t rows{0};
};

/// Models 0x00D820..0x00D846: selects full-plane (64, 32) when force_full_bit1
/// is set or (tiles_h == 32 && tiles_w <= 64); otherwise viewport (41, 29).
[[nodiscard]] BlitDimensions select_blit_dimensions(
    std::uint16_t tiles_w,
    std::uint16_t tiles_h,
    bool force_full_bit1) noexcept;

/// Models 0x00D8C4..0x00D946: blits a (dims.cols x dims.rows) tilemap window
/// starting at (start_col, start_row) into a 64x32 VDP nametable with dual
/// modulo wrap-around on source (tiles_w, tiles_h) and VDP (64, 32).
bool blit_wrapped_plane_window(
    std::span<const std::uint16_t> src_tilemap,
    std::uint16_t tiles_w,
    std::uint16_t tiles_h,
    std::uint16_t start_col,
    std::uint16_t start_row,
    BlitDimensions dims,
    std::span<std::uint16_t, kVdpNametableWords> dst_nametable) noexcept;

} // namespace oasis::game::world
