#include "game/graphics_decompress.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using oasis::game::decompress_graphics;

int main() {
    {
        const std::array<std::uint8_t, 7> source{0x06, 0x00, 0x03, 'A', 'B', 'C', 0x00};
        std::array<std::uint8_t, 32> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 3);
        assert(output[0] == 'A' && output[1] == 'B' && output[2] == 'C');
    }

    {
        const std::array<std::uint8_t, 5> source{0x04, 0x00, 0x41, 'Z', 0x00};
        std::array<std::uint8_t, 32> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 5);
        for (std::size_t i = 0; i < result.output_size; ++i) assert(output[i] == 'Z');
    }

    {
        const std::array<std::uint8_t, 9> source{
            0x08, 0x00, 0x03, 'A', 'B', 'C', 0x80, 0x03, 0x00};
        std::array<std::uint8_t, 32> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 7);
        const std::array<std::uint8_t, 7> expected{'A', 'B', 'C', 'A', 'B', 'C', 'A'};
        for (std::size_t i = 0; i < expected.size(); ++i) assert(output[i] == expected[i]);
    }

    {
        // Format A: two chained 0b011xxxxx extension commands after one back-reference.
        const std::array<std::uint8_t, 11> source{
            0x0A, 0x00, 0x03, 'A', 'B', 'C', 0x80, 0x03, 0x62, 0x61, 0x00};
        std::array<std::uint8_t, 32> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 10);
        const std::array<std::uint8_t, 10> expected{
            'A', 'B', 'C', 'A', 'B', 'C', 'A', 'B', 'C', 'A'};
        for (std::size_t i = 0; i < expected.size(); ++i) assert(output[i] == expected[i]);
    }

    {
        // Format B: literal (0), then terminator token (1,1,00000).
        const std::array<std::uint8_t, 7> source{0x00, 0x00, 0x00, 0x06, 'X', 0x00, 0x00};
        std::array<std::uint8_t, 32> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 1);
        assert(output[0] == 'X');
    }

    {
        // Format B distance==1 special: repeat one byte 14 times, then terminate.
        // Initial control byte 0x03 reaches the special token. Refill word 0x0030
        // supplies four zero length bits and the following termination token.
        const std::array<std::uint8_t, 10> source{
            0x00, 0x00, 0x00, 0x03, 0x01, 0x30, 0x00, 'Q', 0x00, 0x00};
        std::array<std::uint8_t, 32> output{};
        const auto result = decompress_graphics(source, output);
        assert(result.source_consumed == source.size());
        assert(result.output_size == 14);
        for (std::size_t i = 0; i < result.output_size; ++i) assert(output[i] == 'Q');
    }

    return 0;
}
