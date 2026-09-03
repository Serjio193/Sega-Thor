#include "genesis/vdp.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>

int main() {
    using oasis::genesis::Vdp;
    using oasis::genesis::decode_tile_attributes;

    {
        const auto tile = decode_tile_attributes(0xF923U);
        assert(tile.tile_index == 0x0123U);
        assert(tile.palette == 3U);
        assert(tile.priority);
        assert(tile.flip_h);
        assert(tile.flip_v);
    }

    {
        const auto tile = decode_tile_attributes(0x0000U);
        assert(tile.tile_index == 0U);
        assert(tile.palette == 0U);
        assert(!tile.priority);
        assert(!tile.flip_h);
        assert(!tile.flip_v);
    }

    Vdp vdp;
    const std::array<std::uint8_t, 4> bytes{0x12, 0x34, 0xAB, 0xCD};
    vdp.write_vram(0x20, bytes);
    assert(vdp.read_vram_word(0x20) == 0x1234U);
    assert(vdp.read_vram_word(0x22) == 0xABCDU);

    const std::array<std::uint8_t, 2> color{0x0E, 0xE0};
    vdp.write_cram(0, color);
    assert(vdp.read_cram_word(0) == 0x0EE0U);

    const std::array<std::uint8_t, 2> scroll{0x01, 0x23};
    vdp.write_vsram(78, scroll);
    assert(vdp.read_vsram_word(78) == 0x0123U);

    bool threw = false;
    try {
        vdp.write_cram(Vdp::kCramSize, color);
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
