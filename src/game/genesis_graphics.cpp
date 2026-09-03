#include "game/genesis_graphics.hpp"

#include <stdexcept>

namespace oasis::game {
namespace {

std::uint8_t expand_3bit(std::uint8_t value) {
    return static_cast<std::uint8_t>((static_cast<unsigned>(value) * 255U + 3U) / 7U);
}

} // namespace

Tile8x8 decode_genesis_4bpp_tile(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 32) throw std::runtime_error("Genesis tile requires 32 bytes");

    Tile8x8 pixels{};
    for (std::size_t i = 0; i < 32; ++i) {
        pixels[i * 2] = static_cast<std::uint8_t>((bytes[i] >> 4U) & 0x0FU);
        pixels[i * 2 + 1] = static_cast<std::uint8_t>(bytes[i] & 0x0FU);
    }
    return pixels;
}

Palette16 decode_genesis_palette16(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 32) throw std::runtime_error("Genesis 16-color palette requires 32 bytes");

    Palette16 palette{};
    for (std::size_t i = 0; i < palette.size(); ++i) {
        const std::uint16_t word = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(bytes[i * 2]) << 8U) | bytes[i * 2 + 1]);
        const auto red = static_cast<std::uint8_t>((word >> 1U) & 0x07U);
        const auto green = static_cast<std::uint8_t>((word >> 5U) & 0x07U);
        const auto blue = static_cast<std::uint8_t>((word >> 9U) & 0x07U);
        palette[i] = {expand_3bit(red), expand_3bit(green), expand_3bit(blue)};
    }
    return palette;
}

} // namespace oasis::game
