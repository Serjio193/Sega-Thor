#include "game/genesis_graphics.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    {
        std::array<std::uint8_t, 32> bytes{};
        bytes[0] = 0x1F;
        bytes[1] = 0xA5;
        const auto tile = oasis::game::decode_genesis_4bpp_tile(bytes);
        assert(tile[0] == 0x1);
        assert(tile[1] == 0xF);
        assert(tile[2] == 0xA);
        assert(tile[3] == 0x5);
    }

    {
        std::array<std::uint8_t, 32> bytes{};
        // Max red:   0x000E
        // Max green: 0x00E0
        // Max blue:  0x0E00
        bytes[0] = 0x00; bytes[1] = 0x0E;
        bytes[2] = 0x00; bytes[3] = 0xE0;
        bytes[4] = 0x0E; bytes[5] = 0x00;
        bytes[6] = 0x0E; bytes[7] = 0xEE;

        const auto palette = oasis::game::decode_genesis_palette16(bytes);
        assert((palette[0] == oasis::game::Rgb8{255, 0, 0}));
        assert((palette[1] == oasis::game::Rgb8{0, 255, 0}));
        assert((palette[2] == oasis::game::Rgb8{0, 0, 255}));
        assert((palette[3] == oasis::game::Rgb8{255, 255, 255}));
    }

    return 0;
}
