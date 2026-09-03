#include "genesis/vdp.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>

int main() {
    using oasis::genesis::Vdp;
    using oasis::genesis::decode_plane_cell;
    using oasis::genesis::decode_sprite_attributes;
    using oasis::genesis::decode_tile_attributes;

    {
        const auto tile = decode_tile_attributes(0xF923U);
        assert(tile.tile_index == 0x0123U);
        assert(tile.palette == 3U);
        assert(tile.priority);
        assert(tile.flip_h);
        assert(tile.flip_v);

        const auto cell = decode_plane_cell(0xF923U);
        assert(cell.tile.tile_index == tile.tile_index);
        assert(cell.tile.palette == tile.palette);
    }

    {
        const auto tile = decode_tile_attributes(0x0000U);
        assert(tile.tile_index == 0U);
        assert(tile.palette == 0U);
        assert(!tile.priority);
        assert(!tile.flip_h);
        assert(!tile.flip_v);
    }

    {
        const auto sprite = decode_sprite_attributes(
            0x01A5U,
            static_cast<std::uint16_t>((2U << 10U) | (1U << 8U) | 0x35U),
            0xD456U,
            0x0188U);
        assert(sprite.y == 0x01A5U);
        assert(sprite.width_cells == 3U);
        assert(sprite.height_cells == 2U);
        assert(sprite.link == 0x35U);
        assert(sprite.tile.tile_index == 0x0456U);
        assert(sprite.tile.palette == 2U);
        assert(sprite.tile.priority);
        assert(!sprite.tile.flip_h);
        assert(sprite.tile.flip_v);
        assert(sprite.x == 0x0188U);
    }

    Vdp vdp;
    const std::array<std::uint8_t, 4> bytes{0x12, 0x34, 0xAB, 0xCD};
    vdp.write_vram(0x20, bytes);
    assert(vdp.read_vram_word(0x20) == 0x1234U);
    assert(vdp.read_vram_word(0x22) == 0xABCDU);

    vdp.write_vram_word(0x24, 0xCAFEU);
    assert(vdp.read_vram_word(0x24) == 0xCAFEU);

    vdp.write_cram_word(0, 0x0EE0U);
    assert(vdp.read_cram_word(0) == 0x0EE0U);

    vdp.write_vsram_word(78, 0x0123U);
    assert(vdp.read_vsram_word(78) == 0x0123U);

    bool threw = false;
    try {
        const std::array<std::uint8_t, 2> color{0x0E, 0xE0};
        vdp.write_cram(Vdp::kCramSize, color);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        vdp.write_vram_word(Vdp::kVramSize - 1, 0x1234U);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        (void)vdp.read_vsram_word(Vdp::kVsramSize - 1);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);

    return 0;
}
