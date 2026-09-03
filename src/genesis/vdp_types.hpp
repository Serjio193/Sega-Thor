#pragma once

#include <cstdint>

namespace oasis::genesis {

// Standard Mega Drive/Genesis pattern-name word used by planes and sprites.
// Bits: P CC V H NNNNNNNNNNN
struct TileAttributes {
    std::uint16_t tile_index{};
    std::uint8_t palette{};
    bool priority{};
    bool flip_h{};
    bool flip_v{};
};

[[nodiscard]] constexpr TileAttributes decode_tile_attributes(std::uint16_t value) noexcept {
    return {
        static_cast<std::uint16_t>(value & 0x07FFU),
        static_cast<std::uint8_t>((value >> 13U) & 0x03U),
        (value & 0x8000U) != 0,
        (value & 0x0800U) != 0,
        (value & 0x1000U) != 0,
    };
}

struct PlaneCell {
    TileAttributes tile{};
};

[[nodiscard]] constexpr PlaneCell decode_plane_cell(std::uint16_t value) noexcept {
    return {decode_tile_attributes(value)};
}

// Standard four-word Sprite Attribute Table entry.
// Coordinates are raw VDP coordinates; visible screen origin handling is a renderer concern.
struct SpriteAttributes {
    std::uint16_t y{};          // 9-bit raw VDP Y coordinate.
    std::uint8_t width_cells{}; // 1..4 tiles.
    std::uint8_t height_cells{};// 1..4 tiles.
    std::uint8_t link{};        // 0 terminates the sprite chain.
    TileAttributes tile{};
    std::uint16_t x{};          // 9-bit raw VDP X coordinate.
};

[[nodiscard]] constexpr SpriteAttributes decode_sprite_attributes(
    std::uint16_t word0,
    std::uint16_t word1,
    std::uint16_t word2,
    std::uint16_t word3) noexcept {
    return {
        static_cast<std::uint16_t>(word0 & 0x01FFU),
        static_cast<std::uint8_t>(((word1 >> 10U) & 0x03U) + 1U),
        static_cast<std::uint8_t>(((word1 >> 8U) & 0x03U) + 1U),
        static_cast<std::uint8_t>(word1 & 0x007FU),
        decode_tile_attributes(word2),
        static_cast<std::uint16_t>(word3 & 0x01FFU),
    };
}

} // namespace oasis::genesis
