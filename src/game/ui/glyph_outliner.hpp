#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::ui {

inline constexpr std::uint32_t kRasterizeOutlinedGlyphAddr = 0x00C222U;
inline constexpr std::uint32_t kPack8x8Tile4bppAddr = 0x00C2A6U;
inline constexpr std::uint32_t kClear18BytesAddr = 0x00C2C8U;
inline constexpr std::uint32_t kUnpack1bppRow16pxAddr = 0x00C2DAU;

inline constexpr std::uint32_t kGlyphTableRomAddr = 0x0016C1CAU;
inline constexpr std::size_t kGlyphStrideBytes = 28U;
inline constexpr std::size_t kGlyphRows = 14U;
inline constexpr std::size_t kCanvasStrideBytes = 18U;
inline constexpr std::size_t kCanvasHeightRows = 17U;
inline constexpr std::size_t kCanvasTotalBytes = kCanvasStrideBytes * kCanvasHeightRows; // 306

inline constexpr std::uint8_t kGlyphForegroundByte = 0xFFU;
inline constexpr std::uint8_t kGlyphOutlineByte = 0x0EU;
inline constexpr std::uint8_t kGlyphBackgroundByte = 0x00U;

inline constexpr std::size_t kTileOffsets4bpp[4] = {
    0x13U, // TL: row 1, col 1
    0xA3U, // BL: row 9, col 1
    0x1BU, // TR: row 1, col 9
    0xABU, // BR: row 9, col 9
};

/// Models 0x00C222..0x00C2EA: unpacks 14 16-bit 1-bpp glyph rows into an
/// 18x17 scratch canvas, applies 4-neighbor outline dilation (color 14 / 0x0E),
/// and packs the 16x16 interior into four 8x8 4-bpp Mega Drive tiles (128 bytes)
/// in column-major order: TL, BL, TR, BR.
[[nodiscard]] std::array<std::uint8_t, 128> rasterize_outlined_glyph(
    std::span<const std::uint16_t, kGlyphRows> rows) noexcept;

} // namespace oasis::game::ui
