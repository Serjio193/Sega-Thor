#include "game/font/font_glyph_packer.hpp"

#include <algorithm>
#include <cstdint>

namespace oasis::game::font {

void expand_glyph_scanlines_to_canvas(
    std::span<const std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines,
    FontCanvas& canvas) noexcept {
    canvas.fill(0x00U);

    for (std::size_t r = 0; r < FontGlyphPackerConfig::kGlyphScanlines; ++r) {
        const std::uint16_t line = scanlines[r];
        const std::size_t canvas_row = 2U + r;
        const std::size_t row_offset = canvas_row * FontGlyphPackerConfig::kCanvasPitch;

        // Leading pixel is 0x00 at col 0
        for (std::size_t bit = 0; bit < 16U; ++bit) {
            const bool is_set = ((line >> (15U - bit)) & 1U) != 0U;
            canvas[row_offset + 1U + bit] = is_set ? FontGlyphPackerConfig::kForegroundPixelByte
                                                   : FontGlyphPackerConfig::kBackgroundPixelByte;
        }
        // Trailing pixel is 0x00 at col 17
    }
}

void apply_cardinal_outline_filter(FontCanvas& canvas) noexcept {
    constexpr std::size_t start_idx = FontGlyphPackerConfig::kCanvasPitch; // Row 1, Col 0 (index 18)
    constexpr std::size_t count = FontGlyphPackerConfig::kOutlineFilterPixelCount; // 288 pixels

    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t curr = start_idx + i;
        if ((canvas[curr] & 0x80U) != 0U) {
            continue; // Foreground pixel (0xFF)
        }

        const std::uint8_t top = canvas[curr - FontGlyphPackerConfig::kCanvasPitch];
        const std::uint8_t bottom = canvas[curr + FontGlyphPackerConfig::kCanvasPitch];
        const std::uint8_t left = canvas[curr - 1U];
        const std::uint8_t right = canvas[curr + 1U];

        if (((top | bottom | left | right) & 0x80U) != 0U) {
            canvas[curr] = FontGlyphPackerConfig::kOutlineColorByte;
        }
    }
}

static void pack_one_tile(
    const FontCanvas& canvas,
    std::size_t start_row,
    std::size_t start_col,
    std::span<std::uint8_t, FontGlyphPackerConfig::kTileSizeBytes> out_tile) noexcept {
    std::size_t out_idx = 0;
    for (std::size_t r = 0; r < FontGlyphPackerConfig::kTileHeight; ++r) {
        const std::size_t row_base = (start_row + r) * FontGlyphPackerConfig::kCanvasPitch + start_col;
        for (std::size_t b = 0; b < FontGlyphPackerConfig::kBytesPerRow; ++b) {
            const std::uint8_t p0 = canvas[row_base + b * 2U];
            const std::uint8_t p1 = canvas[row_base + b * 2U + 1U];

            // In M68K: (p0 << 4) | (p1 & 0x0F)
            const std::uint8_t hi = static_cast<std::uint8_t>((p0 << 4U) & 0xF0U);
            const std::uint8_t lo = static_cast<std::uint8_t>(p1 & 0x0FU);
            out_tile[out_idx++] = static_cast<std::uint8_t>(hi | lo);
        }
    }
}

void pack_canvas_to_4bpp_tiles(const FontCanvas& canvas, FontTilesOut& tiles_out) noexcept {
    struct QuadCoord {
        std::size_t row;
        std::size_t col;
    };
    constexpr std::array<QuadCoord, 4> kQuadrants{{
        {1U, 1U}, // Tile 0: Top-Left
        {9U, 1U}, // Tile 1: Bottom-Left
        {1U, 9U}, // Tile 2: Top-Right
        {9U, 9U}  // Tile 3: Bottom-Right
    }};

    for (std::size_t t = 0; t < 4U; ++t) {
        const auto& q = kQuadrants[t];
        auto tile_span = std::span<std::uint8_t, FontGlyphPackerConfig::kTileSizeBytes>(
            tiles_out.data() + t * FontGlyphPackerConfig::kTileSizeBytes,
            FontGlyphPackerConfig::kTileSizeBytes);
        pack_one_tile(canvas, q.row, q.col, tile_span);
    }
}

FontTilesOut render_glyph_scanlines(
    std::span<const std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines,
    FontCanvas* out_canvas) noexcept {
    FontCanvas local_canvas{};
    expand_glyph_scanlines_to_canvas(scanlines, local_canvas);
    apply_cardinal_outline_filter(local_canvas);

    FontTilesOut tiles{};
    pack_canvas_to_4bpp_tiles(local_canvas, tiles);

    if (out_canvas != nullptr) {
        *out_canvas = local_canvas;
    }
    return tiles;
}

FontTilesOut render_glyph_from_rom(
    std::span<const std::uint8_t> rom_data,
    std::uint16_t glyph_index,
    FontCanvas* out_canvas) noexcept {
    const std::size_t offset = FontGlyphPackerConfig::kFontTableRomAddr +
                               static_cast<std::size_t>(glyph_index) * FontGlyphPackerConfig::kGlyphRecordSizeBytes;

    if (offset + FontGlyphPackerConfig::kGlyphRecordSizeBytes > rom_data.size()) {
        if (out_canvas != nullptr) {
            out_canvas->fill(0U);
        }
        return FontTilesOut{};
    }

    std::array<std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines{};
    for (std::size_t i = 0; i < FontGlyphPackerConfig::kGlyphScanlines; ++i) {
        const std::size_t w_off = offset + i * 2U;
        scanlines[i] = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(rom_data[w_off]) << 8U) |
            static_cast<std::uint16_t>(rom_data[w_off + 1U]));
    }

    return render_glyph_scanlines(scanlines, out_canvas);
}

void render_glyph_scanlines_in_ram(
    std::span<std::uint8_t> ram_window,
    std::span<const std::uint16_t, FontGlyphPackerConfig::kGlyphScanlines> scanlines) {
    constexpr std::size_t kCanvasOffset = FontGlyphPackerConfig::kCanvasRamAddr & 0xFFFFU;
    constexpr std::size_t kTilesOffset = FontGlyphPackerConfig::kTilesOutRamAddr & 0xFFFFU;

    if (ram_window.size() < kCanvasOffset + FontGlyphPackerConfig::kCanvasSizeBytes) {
        return;
    }

    FontCanvas canvas{};
    FontTilesOut tiles = render_glyph_scanlines(scanlines, &canvas);

    std::copy_n(canvas.begin(), canvas.size(), ram_window.begin() + kCanvasOffset);
    std::copy_n(tiles.begin(), tiles.size(), ram_window.begin() + kTilesOffset);
}

void render_glyph_from_rom_in_ram(
    std::span<std::uint8_t> ram_window,
    std::span<const std::uint8_t> rom_data,
    std::uint16_t glyph_index) {
    constexpr std::size_t kCanvasOffset = FontGlyphPackerConfig::kCanvasRamAddr & 0xFFFFU;
    constexpr std::size_t kTilesOffset = FontGlyphPackerConfig::kTilesOutRamAddr & 0xFFFFU;

    if (ram_window.size() < kCanvasOffset + FontGlyphPackerConfig::kCanvasSizeBytes) {
        return;
    }

    FontCanvas canvas{};
    FontTilesOut tiles = render_glyph_from_rom(rom_data, glyph_index, &canvas);

    std::copy_n(canvas.begin(), canvas.size(), ram_window.begin() + kCanvasOffset);
    std::copy_n(tiles.begin(), tiles.size(), ram_window.begin() + kTilesOffset);
}

} // namespace oasis::game::font
