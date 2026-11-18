#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::font {

/// Configuration and constants for ROM Dynamic 1-Bit Font Glyph Expansion,
/// 4-Neighbor Outline Filter & 4bpp Tile Packer (0x00C222..0x00C2EC, 202 bytes).
struct FontGlyphPackerConfig {
    static constexpr std::uint32_t kEntryAddr = 0x00C222U;
    static constexpr std::uint32_t kExitRtsAddr = 0x00C2A4U;
    static constexpr std::uint32_t kPackTilesHelperAddr = 0x00C2A6U;
    static constexpr std::uint32_t kClearTwoRowsHelperAddr = 0x00C2C8U;
    static constexpr std::uint32_t kExpandScanlineHelperAddr = 0x00C2DAU;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C2ECU;
    static constexpr std::size_t kRoutineSizeBytes = 202U;

    // ROM Font Table
    static constexpr std::uint32_t kFontTableRomAddr = 0x0016C1CAU;
    static constexpr std::size_t kGlyphScanlines = 14U;
    static constexpr std::size_t kGlyphRecordSizeBytes = 28U; // 14 words

    // RAM Addresses
    static constexpr std::uint32_t kCanvasRamAddr = 0x00FF3028U;
    static constexpr std::uint32_t kTilesOutRamAddr = 0x00FF2FA8U;

    // Canvas dimensions
    static constexpr std::size_t kCanvasPitch = 18U;
    static constexpr std::size_t kCanvasRows = 18U;
    static constexpr std::size_t kCanvasSizeBytes = kCanvasPitch * kCanvasRows; // 324 bytes (0x144)

    // Outline filter
    static constexpr std::uint32_t kOutlineFilterStartRam = 0x00FF303AU; // Row 1, Col 0
    static constexpr std::size_t kOutlineFilterPixelCount = 288U;         // 16 rows * 18 cols
    static constexpr std::uint8_t kOutlineColorByte = 0x0EU;
    static constexpr std::uint8_t kForegroundPixelByte = 0xFFU;
    static constexpr std::uint8_t kBackgroundPixelByte = 0x00U;

    // Sega Genesis 4bpp Tile Format
    static constexpr std::size_t kTileWidth = 8U;
    static constexpr std::size_t kTileHeight = 8U;
    static constexpr std::size_t kBytesPerRow = 4U;                       // 8 pixels at 4bpp = 4 bytes
    static constexpr std::size_t kTileSizeBytes = kTileHeight * kBytesPerRow; // 32 bytes
    static constexpr std::size_t kNumTiles = 4U;                          // 16x16 glyph = 4 tiles (TL, BL, TR, BR)
    static constexpr std::size_t kTilesOutSizeBytes = kNumTiles * kTileSizeBytes; // 128 bytes (0x80)

    static constexpr std::uint8_t kForegroundNibble = 0x0FU;
    static constexpr std::uint8_t kOutlineNibble = 0x0EU;
    static constexpr std::uint8_t kBackgroundNibble = 0x00U;
};

using FontCanvas = std::array<std::uint8_t, FontGlyphPackerConfig::kCanvasSizeBytes>;
using FontTilesOut = std::array<std::uint8_t, FontGlyphPackerConfig::kTilesOutSizeBytes>;

/// 1-Bit to byte expansion into padded canvas buffer (0x00C22E..0x00C248).
/// Initializes rows 0..1 and rows 16..17 with 0x00.
/// Rows 2..15 expand 14 scanlines with leading 0x00, 16 pixels (0xFF or 0x00), and trailing 0x00.
void expand_glyph_scanlines_to_canvas(
    std::span<const std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines,
    FontCanvas& canvas) noexcept;

/// 4-neighbor cardinal outline filter applied in-place on the canvas (0x00C248..0x00C276).
/// Scans 288 pixels from row 1, col 0 ($FF303A) to row 16, col 17.
/// Writes 0x0E into background pixels that touch a foreground pixel (0xFF) on top, bottom, left, or right.
void apply_cardinal_outline_filter(FontCanvas& canvas) noexcept;

/// Packs the 16x16 processed glyph area into four 8x8 Sega Genesis 4bpp tiles (0x00C276..0x00C2A4).
/// Tile 0: Top-Left (Row 1..8, Col 1..8) -> $FF2FA8
/// Tile 1: Bottom-Left (Row 9..16, Col 1..8) -> $FF2FC8
/// Tile 2: Top-Right (Row 1..8, Col 9..16) -> $FF2FE8
/// Tile 3: Bottom-Right (Row 9..16, Col 9..16) -> $FF3008
void pack_canvas_to_4bpp_tiles(const FontCanvas& canvas, FontTilesOut& tiles_out) noexcept;

/// High-level single invocation that renders 14 scanlines into 4 packed Genesis tiles.
[[nodiscard]] FontTilesOut render_glyph_scanlines(
    std::span<const std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines,
    FontCanvas* out_canvas = nullptr) noexcept;

/// Reads 14 words for glyph_index from ROM font table and renders into 4 packed Genesis tiles.
[[nodiscard]] FontTilesOut render_glyph_from_rom(
    std::span<const std::uint8_t> rom_data,
    std::uint16_t glyph_index,
    FontCanvas* out_canvas = nullptr) noexcept;

/// Direct raw RAM window helpers:
/// Canvas written at ram_window[0x3028..0x316B].
/// Tiles written at ram_window[0x2FA8..0x3027].
void render_glyph_scanlines_in_ram(
    std::span<std::uint8_t> ram_window,
    std::span<const std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines);

void render_glyph_from_rom_in_ram(
    std::span<std::uint8_t> ram_window,
    std::span<const std::uint8_t> rom_data,
    std::uint16_t glyph_index);

} // namespace oasis::game::font
