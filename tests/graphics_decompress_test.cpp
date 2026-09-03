#include "game/graphics_decompress.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

using oasis::game::decompress_graphics;

int main() {
    {
        const std::array<std::uint8_t, 7> source{0x06, 0x00, 0x03, 'A', 'B', 'C', 0x00};
        std::array<std::uint8_t, 16> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 3);
        assert(output[0] == 'A' && output[1] == 'B' && output[2] == 'C');
    }

    {
        const std::array<std::uint8_t, 5> source{0x04, 0x00, 0x41, 'Z', 0x00};
        std::array<std::uint8_t, 16> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 5);
        for (std::size_t i = 0; i < result.output_size; ++i) assert(output[i] == 'Z');
    }

    {
        const std::array<std::uint8_t, 9> source{
            0x08, 0x00, 0x03, 'A', 'B', 'C', 0x80, 0x03, 0x00};
        std::array<std::uint8_t, 16> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 7);
        const std::array<std::uint8_t, 7> expected{'A', 'B', 'C', 'A', 'B', 'C', 'A'};
        for (std::size_t i = 0; i < expected.size(); ++i) assert(output[i] == expected[i]);
    }

    {
        // Initial flag bits: literal (0), then terminator token (1,1,00000).
        // The zero distance byte is followed by the zero end marker.
        const std::array<std::uint8_t, 7> source{0x00, 0x00, 0x00, 0x06, 'X', 0x00, 0x00};
        std::array<std::uint8_t, 16> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 1);
        assert(output[0] == 'X');
    }

    return 0;
}
