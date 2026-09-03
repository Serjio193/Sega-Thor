#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace oasis::game {

struct Rgb8 {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};

    friend constexpr bool operator==(const Rgb8&, const Rgb8&) = default;
};

using Tile8x8 = std::array<std::uint8_t, 64>;
using Palette16 = std::array<Rgb8, 16>;

// Mega Drive tiles are 8x8, 4 bits per pixel, 32 bytes per tile.
[[nodiscard]] Tile8x8 decode_genesis_4bpp_tile(std::span<const std::uint8_t> bytes);

// Decodes 16 big-endian Mega Drive CRAM words (32 bytes).
[[nodiscard]] Palette16 decode_genesis_palette16(std::span<const std::uint8_t> bytes);

} // namespace oasis::game
