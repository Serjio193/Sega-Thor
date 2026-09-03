#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game {

struct DecompressResult {
    std::size_t source_consumed{};
    std::size_t output_written{};
};

// Native translation of the USA reference routine at 0x00003820.
// Throws std::runtime_error for truncated/invalid input or output overflow.
[[nodiscard]] DecompressResult decompress_graphics(
    std::span<const std::uint8_t> source,
    std::span<std::uint8_t> destination);

} // namespace oasis::game
