#include "game/ui/glyph_outliner.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

using namespace oasis::game::ui;

[[nodiscard]] std::uint8_t read_tile_pixel(
    const std::array<std::uint8_t, 128>& tiles,
    std::size_t tile_idx,
    std::size_t row,
    std::size_t col) noexcept {
    const std::uint8_t byte = tiles[tile_idx * 32U + row * 4U + (col / 2U)];
    return (col % 2U == 0U) ? static_cast<std::uint8_t>(byte >> 4U)
                            : static_cast<std::uint8_t>(byte & 0x0FU);
}

void test_single_pixel_4_neighbor_outline() {
    // Single foreground pixel at row 2 (index 2 -> canvas row 3), col 3 (bit 12)
    std::array<std::uint16_t, kGlyphRows> rows{};
    rows[2] = static_cast<std::uint16_t>(1U << 12U); // x=3

    const auto tiles = rasterize_outlined_glyph(rows);

    // TL tile (index 0): covers canvas rows 1..8 (glyph rows 0..7), cols 1..8 (x 0..7)
    assert(read_tile_pixel(tiles, 0U, 2U, 3U) == 15U); // foreground
    assert(read_tile_pixel(tiles, 0U, 1U, 3U) == 14U); // top neighbor
    assert(read_tile_pixel(tiles, 0U, 3U, 3U) == 14U); // bottom neighbor
    assert(read_tile_pixel(tiles, 0U, 2U, 2U) == 14U); // left neighbor
    assert(read_tile_pixel(tiles, 0U, 2U, 4U) == 14U); // right neighbor

    // Diagonals must remain 0 (4-neighbor only)
    assert(read_tile_pixel(tiles, 0U, 1U, 2U) == 0U);
    assert(read_tile_pixel(tiles, 0U, 1U, 4U) == 0U);
    assert(read_tile_pixel(tiles, 0U, 3U, 2U) == 0U);
    assert(read_tile_pixel(tiles, 0U, 3U, 4U) == 0U);
}

void test_column_major_4_tile_packing_and_bottom_outline() {
    // Sample glyph 1 pattern from ROM evidence + bottom row pixel to test BL/TR/BR
    std::array<std::uint16_t, kGlyphRows> rows = {
        0x01F8U, 0x0180U, 0x0180U, 0x0180U, 0x0180U, 0x0180U, 0x0180U,
        0x0180U, 0x0180U, 0x0180U, 0x0180U, 0x0180U, 0x0180U, 0x0180U
    };
    const auto tiles = rasterize_outlined_glyph(rows);

    // Row 0 of 0x01F8 = 0000 0001 1111 1000 -> x=7 is in TL (col 7), x=8..12 in TR (cols 0..4)
    assert(read_tile_pixel(tiles, 0U, 0U, 7U) == 15U); // TL col 7
    assert(read_tile_pixel(tiles, 0U, 0U, 6U) == 14U); // TL outline left of x=7
    assert(read_tile_pixel(tiles, 2U, 0U, 0U) == 15U); // TR col 0 (x=8)
    assert(read_tile_pixel(tiles, 2U, 0U, 4U) == 15U); // TR col 4 (x=12)
    assert(read_tile_pixel(tiles, 2U, 0U, 5U) == 14U); // TR outline right of x=12

    // BL (index 1) row 5 = glyph row 13 (0x0180 -> x=7,8): x=7 in BL col 7, x=8 in BR col 0
    assert(read_tile_pixel(tiles, 1U, 5U, 7U) == 15U); // BL
    assert(read_tile_pixel(tiles, 3U, 5U, 0U) == 15U); // BR
    // Row 6 of BL/BR = canvas row 15 (1px below glyph row 13) gets outline 14
    assert(read_tile_pixel(tiles, 1U, 6U, 7U) == 14U);
    assert(read_tile_pixel(tiles, 3U, 6U, 0U) == 14U);
}

} // namespace

int main() {
    test_single_pixel_4_neighbor_outline();
    test_column_major_4_tile_packing_and_bottom_outline();
    return 0;
}
