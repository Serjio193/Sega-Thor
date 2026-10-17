#include "game/genesis_graphics.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    {
        std::array<std::uint8_t, 32> bytes{};
        for (std::size_t row = 0; row < 8; ++row) {
            bytes[row * 4] = 0x12;
            bytes[row * 4 + 1] = 0x34;
            bytes[row * 4 + 2] = 0x56;
            bytes[row * 4 + 3] = 0x78;
        }
        const auto tile = oasis::game::decode_genesis_4bpp_tile(bytes);
        for (std::size_t row = 0; row < 8; ++row) {
            for (std::size_t column = 0; column < 8; ++column) {
                assert(tile[row * 8 + column] == column + 1);
            }
        }
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
