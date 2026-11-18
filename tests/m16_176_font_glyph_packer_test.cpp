#include "game/font/font_glyph_packer.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::font;

static void test_constants_and_geometry() {
    static_assert(FontGlyphPackerConfig::kEntryAddr == 0x00C222U);
    static_assert(FontGlyphPackerConfig::kExitRtsAddr == 0x00C2A4U);
    static_assert(FontGlyphPackerConfig::kPackTilesHelperAddr == 0x00C2A6U);
    static_assert(FontGlyphPackerConfig::kClearTwoRowsHelperAddr == 0x00C2C8U);
    static_assert(FontGlyphPackerConfig::kExpandScanlineHelperAddr == 0x00C2DAU);
    static_assert(FontGlyphPackerConfig::kRangeEndAddr == 0x00C2ECU);
    static_assert(FontGlyphPackerConfig::kRoutineSizeBytes == 202U);

    static_assert(FontGlyphPackerConfig::kFontTableRomAddr == 0x0016C1CAU);
    static_assert(FontGlyphPackerConfig::kGlyphScanlines == 14U);
    static_assert(FontGlyphPackerConfig::kGlyphRecordSizeBytes == 28U);

    static_assert(FontGlyphPackerConfig::kCanvasRamAddr == 0x00FF3028U);
    static_assert(FontGlyphPackerConfig::kTilesOutRamAddr == 0x00FF2FA8U);

    static_assert(FontGlyphPackerConfig::kCanvasPitch == 18U);
    static_assert(FontGlyphPackerConfig::kCanvasRows == 18U);
    static_assert(FontGlyphPackerConfig::kCanvasSizeBytes == 324U);

    static_assert(FontGlyphPackerConfig::kOutlineFilterStartRam == 0x00FF303AU);
    static_assert(FontGlyphPackerConfig::kOutlineFilterPixelCount == 288U);

    static_assert(FontGlyphPackerConfig::kTileWidth == 8U);
    static_assert(FontGlyphPackerConfig::kTileHeight == 8U);
    static_assert(FontGlyphPackerConfig::kBytesPerRow == 4U);
    static_assert(FontGlyphPackerConfig::kTileSizeBytes == 32U);
    static_assert(FontGlyphPackerConfig::kNumTiles == 4U);
    static_assert(FontGlyphPackerConfig::kTilesOutSizeBytes == 128U);

    std::cout << "[PASS] test_constants_and_geometry\n";
}

static void test_blank_glyph() {
    std::array<std::uint16_t, 14> scanlines{};
    scanlines.fill(0U);

    FontCanvas canvas{};
    const FontTilesOut tiles = render_glyph_scanlines(scanlines, &canvas);

    for (std::size_t i = 0; i < canvas.size(); ++i) {
        assert(canvas[i] == 0x00U);
    }
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        assert(tiles[i] == 0x00U);
    }

    std::cout << "[PASS] test_blank_glyph\n";
}

static void test_single_isolated_pixel_outline() {
    // Single foreground pixel at scanline 3, bit 8 (middle of the glyph)
    // Scanline 3 maps to canvas row 2 + 3 = 5.
    // Bit 8 maps to canvas col 1 + 8 = 9.
    std::array<std::uint16_t, 14> scanlines{};
    scanlines[3] = static_cast<std::uint16_t>(1U << (15U - 8U)); // 0x0080

    FontCanvas canvas{};
    const FontTilesOut tiles = render_glyph_scanlines(scanlines, &canvas);

    constexpr std::size_t pitch = FontGlyphPackerConfig::kCanvasPitch;
    const std::size_t center_idx = 5U * pitch + 9U;

    // Center pixel must be foreground 0xFF
    assert(canvas[center_idx] == 0xFFU);

    // 4 cardinal neighbors must be outline 0x0E
    assert(canvas[center_idx - pitch] == 0x0EU); // Top (row 4, col 9)
    assert(canvas[center_idx + pitch] == 0x0EU); // Bottom (row 6, col 9)
    assert(canvas[center_idx - 1U] == 0x0EU);    // Left (row 5, col 8)
    assert(canvas[center_idx + 1U] == 0x0EU);    // Right (row 5, col 10)

    // Diagonal neighbors must remain background 0x00
    assert(canvas[center_idx - pitch - 1U] == 0x00U); // Top-left
    assert(canvas[center_idx - pitch + 1U] == 0x00U); // Top-right
    assert(canvas[center_idx + pitch - 1U] == 0x00U); // Bottom-left
    assert(canvas[center_idx + pitch + 1U] == 0x00U); // Bottom-right

    // Verify tile quadrant packing for this pixel:
    // Canvas col 9 is the first column (col 0) of Tile 2 (TR).
    // Canvas row 5 is row 4 of Tile 2.
    // In Tile 2:
    // - Row 3 (canvas row 4): col 0 is 0x0E, col 1 is 0x00 -> byte 0 is 0xE0
    const std::size_t tile2_offset = 2U * FontGlyphPackerConfig::kTileSizeBytes;
    assert(tiles[tile2_offset + 3U * 4U + 0U] == 0xE0U);

    // - Row 4 (canvas row 5): col 0 is 0xFF, col 1 is 0x0E -> byte 0 is 0xFE
    assert(tiles[tile2_offset + 4U * 4U + 0U] == 0xFEU);

    // - Row 5 (canvas row 6): col 0 is 0x0E, col 1 is 0x00 -> byte 0 is 0xE0
    assert(tiles[tile2_offset + 5U * 4U + 0U] == 0xE0U);

    // Left neighbor is at canvas col 8, which is the last column (col 7) of Tile 0 (TL)!
    // In Tile 0:
    // - Row 4 (canvas row 5): col 6 is 0x00, col 7 is 0x0E -> byte 3 is 0x0E
    const std::size_t tile0_offset = 0U * FontGlyphPackerConfig::kTileSizeBytes;
    assert(tiles[tile0_offset + 4U * 4U + 3U] == 0x0EU);

    std::cout << "[PASS] test_single_isolated_pixel_outline\n";
}

static void test_solid_box_glyph() {
    // All 14 scanlines fully set: 14x16 block of foreground pixels
    std::array<std::uint16_t, 14> scanlines{};
    scanlines.fill(0xFFFFU);

    FontCanvas canvas{};
    const FontTilesOut tiles = render_glyph_scanlines(scanlines, &canvas);
    (void)tiles;

    constexpr std::size_t pitch = FontGlyphPackerConfig::kCanvasPitch;

    // Top border row 0 must remain 0x00
    for (std::size_t c = 0; c < pitch; ++c) {
        assert(canvas[0U * pitch + c] == 0x00U);
    }
    // Bottom border row 17 must remain 0x00
    for (std::size_t c = 0; c < pitch; ++c) {
        assert(canvas[17U * pitch + c] == 0x00U);
    }

    // Row 1 (above solid box) cols 1..16 should be outline 0x0E
    for (std::size_t c = 1; c <= 16; ++c) {
        assert(canvas[1U * pitch + c] == 0x0EU);
    }
    // Row 16 (below solid box) cols 1..16 should be outline 0x0E
    for (std::size_t c = 1; c <= 16; ++c) {
        assert(canvas[16U * pitch + c] == 0x0EU);
    }

    // Left and right edges of rows 2..15 should be outline 0x0E
    for (std::size_t r = 2; r <= 15; ++r) {
        assert(canvas[r * pitch + 0U] == 0x0EU);  // Col 0 (left outline)
        assert(canvas[r * pitch + 17U] == 0x0EU); // Col 17 (right outline)
        // Interior cols 1..16 must be foreground 0xFF
        for (std::size_t c = 1; c <= 16; ++c) {
            assert(canvas[r * pitch + c] == 0xFFU);
        }
    }

    std::cout << "[PASS] test_solid_box_glyph\n";
}

static void test_quadrant_separation() {
    // 1. Mark in TL only (glyph row 0, col 0)
    {
        std::array<std::uint16_t, 14> scanlines{};
        scanlines[0] = 0x8000U;
        const FontTilesOut tiles = render_glyph_scanlines(scanlines);

        bool tl_has_data = false;
        for (std::size_t i = 0; i < 32; ++i) if (tiles[i] != 0) tl_has_data = true;
        bool bl_has_data = false;
        for (std::size_t i = 32; i < 64; ++i) if (tiles[i] != 0) bl_has_data = true;
        bool tr_has_data = false;
        for (std::size_t i = 64; i < 96; ++i) if (tiles[i] != 0) tr_has_data = true;
        bool br_has_data = false;
        for (std::size_t i = 96; i < 128; ++i) if (tiles[i] != 0) br_has_data = true;

        assert(tl_has_data);
        assert(!bl_has_data);
        assert(!tr_has_data);
        assert(!br_has_data);
    }

    // 2. Mark in BL only (glyph row 10, col 0 -> canvas row 12, col 1: inside Tile 1)
    {
        std::array<std::uint16_t, 14> scanlines{};
        scanlines[10] = 0x8000U;
        const FontTilesOut tiles = render_glyph_scanlines(scanlines);

        bool tl_has_data = false;
        for (std::size_t i = 0; i < 32; ++i) if (tiles[i] != 0) tl_has_data = true;
        bool bl_has_data = false;
        for (std::size_t i = 32; i < 64; ++i) if (tiles[i] != 0) bl_has_data = true;
        bool tr_has_data = false;
        for (std::size_t i = 64; i < 96; ++i) if (tiles[i] != 0) tr_has_data = true;
        bool br_has_data = false;
        for (std::size_t i = 96; i < 128; ++i) if (tiles[i] != 0) br_has_data = true;

        assert(!tl_has_data);
        assert(bl_has_data);
        assert(!tr_has_data);
        assert(!br_has_data);
    }

    // 3. Mark in TR only (glyph row 0, col 10 -> canvas row 2, col 11: inside Tile 2)
    {
        std::array<std::uint16_t, 14> scanlines{};
        scanlines[0] = 0x0020U;
        const FontTilesOut tiles = render_glyph_scanlines(scanlines);

        bool tl_has_data = false;
        for (std::size_t i = 0; i < 32; ++i) if (tiles[i] != 0) tl_has_data = true;
        bool bl_has_data = false;
        for (std::size_t i = 32; i < 64; ++i) if (tiles[i] != 0) bl_has_data = true;
        bool tr_has_data = false;
        for (std::size_t i = 64; i < 96; ++i) if (tiles[i] != 0) tr_has_data = true;
        bool br_has_data = false;
        for (std::size_t i = 96; i < 128; ++i) if (tiles[i] != 0) br_has_data = true;

        assert(!tl_has_data);
        assert(!bl_has_data);
        assert(tr_has_data);
        assert(!br_has_data);
    }

    // 4. Mark in BR only (glyph row 10, col 10 -> canvas row 12, col 11: inside Tile 3)
    {
        std::array<std::uint16_t, 14> scanlines{};
        scanlines[10] = 0x0020U;
        const FontTilesOut tiles = render_glyph_scanlines(scanlines);

        bool tl_has_data = false;
        for (std::size_t i = 0; i < 32; ++i) if (tiles[i] != 0) tl_has_data = true;
        bool bl_has_data = false;
        for (std::size_t i = 32; i < 64; ++i) if (tiles[i] != 0) bl_has_data = true;
        bool tr_has_data = false;
        for (std::size_t i = 64; i < 96; ++i) if (tiles[i] != 0) tr_has_data = true;
        bool br_has_data = false;
        for (std::size_t i = 96; i < 128; ++i) if (tiles[i] != 0) br_has_data = true;

        assert(!tl_has_data);
        assert(!bl_has_data);
        assert(!tr_has_data);
        assert(br_has_data);
    }

    std::cout << "[PASS] test_quadrant_separation\n";
}

static void test_rom_glyph_rasterization() {
    // Construct synthetic ROM buffer with Glyph 0 (ellipsis punctuation)
    // 0x16C1CA + 28 bytes
    std::vector<std::uint8_t> rom(0x16C1CAU + 56U, 0x00U);

    // Glyph 0: scanlines 6 and 7 have 0x3398 (0011 0011 1001 1000b)
    const std::size_t g0_base = 0x16C1CAU;
    rom[g0_base + 6U * 2U + 0U] = 0x33U;
    rom[g0_base + 6U * 2U + 1U] = 0x98U;
    rom[g0_base + 7U * 2U + 0U] = 0x33U;
    rom[g0_base + 7U * 2U + 1U] = 0x98U;

    FontCanvas canvas{};
    const FontTilesOut tiles = render_glyph_from_rom(rom, 0U, &canvas);

    // Verify dots in Tile 0 (TL):
    // Row 6 of Tile 0 (canvas row 7, top outline above scanline 6):
    assert(tiles[6U * 4U + 0U] == 0x00U);
    assert(tiles[6U * 4U + 1U] == 0xEEU);
    assert(tiles[6U * 4U + 2U] == 0x00U);
    assert(tiles[6U * 4U + 3U] == 0xEEU);

    // Row 7 of Tile 0 (canvas row 8, first scanline of dots):
    assert(tiles[7U * 4U + 0U] == 0x0EU);
    assert(tiles[7U * 4U + 1U] == 0xFFU);
    assert(tiles[7U * 4U + 2U] == 0xEEU);
    assert(tiles[7U * 4U + 3U] == 0xFFU);

    // Verify dots in Tile 1 (BL):
    // Row 0 of Tile 1 (canvas row 9, second scanline of dots):
    const std::size_t tile1_base = 1U * FontGlyphPackerConfig::kTileSizeBytes;
    assert(tiles[tile1_base + 0U * 4U + 0U] == 0x0EU);
    assert(tiles[tile1_base + 0U * 4U + 1U] == 0xFFU);
    assert(tiles[tile1_base + 0U * 4U + 2U] == 0xEEU);
    assert(tiles[tile1_base + 0U * 4U + 3U] == 0xFFU);

    // Row 1 of Tile 1 (canvas row 10, bottom outline below scanline 7):
    assert(tiles[tile1_base + 1U * 4U + 0U] == 0x00U);
    assert(tiles[tile1_base + 1U * 4U + 1U] == 0xEEU);
    assert(tiles[tile1_base + 1U * 4U + 2U] == 0x00U);
    assert(tiles[tile1_base + 1U * 4U + 3U] == 0xEEU);

    // Out of bounds ROM access returns blank tiles
    const FontTilesOut oob_tiles = render_glyph_from_rom(rom, 100U);
    for (std::size_t i = 0; i < oob_tiles.size(); ++i) {
        assert(oob_tiles[i] == 0U);
    }

    std::cout << "[PASS] test_rom_glyph_rasterization\n";
}

static void test_ram_window_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0xA5U); // 64KB Genesis RAM window

    std::array<std::uint16_t, 14> scanlines{};
    scanlines[2] = 0x0F00U;

    render_glyph_scanlines_in_ram(ram, scanlines);

    constexpr std::size_t canvas_off = FontGlyphPackerConfig::kCanvasRamAddr & 0xFFFFU;
    constexpr std::size_t tiles_off = FontGlyphPackerConfig::kTilesOutRamAddr & 0xFFFFU;

    // Verify canvas area was written
    assert(ram[canvas_off + 2U * FontGlyphPackerConfig::kCanvasPitch + 0U] == 0x00U);
    // Verify tiles area was written
    assert(ram[tiles_off] != 0xA5U || ram[tiles_off + 1] != 0xA5U);

    // Check untouched areas before and after
    assert(ram[tiles_off - 1U] == 0xA5U);
    assert(ram[canvas_off + FontGlyphPackerConfig::kCanvasSizeBytes] == 0xA5U);

    std::cout << "[PASS] test_ram_window_integration\n";
}

int main() {
    std::cout << "Starting M16.176 Font Glyph Packer tests...\n";
    test_constants_and_geometry();
    test_blank_glyph();
    test_single_isolated_pixel_outline();
    test_solid_box_glyph();
    test_quadrant_separation();
    test_rom_glyph_rasterization();
    test_ram_window_integration();
    std::cout << "All M16.176 Font Glyph Packer tests PASS!\n";
    return 0;
}
