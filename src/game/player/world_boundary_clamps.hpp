#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct WorldBoundaryClampsConfig {
    static constexpr std::uint32_t kClampPosXAddr = 0x009D4EU;
    static constexpr std::uint32_t kClampNegXAddr = 0x009D70U;
    static constexpr std::uint32_t kClampPosYAddr = 0x009D92U;
    static constexpr std::uint32_t kClampNegYAddr = 0x009DB4U;
    static constexpr std::uint32_t kBoundaryClampEndAddr = 0x009DD6U;

    static constexpr std::uint8_t kBoundaryCollisionFlag = 0x48U; // bit 6 (wall hit 0x40) | bit 3 (world boundary 0x08)

    // Entity slot offsets
    static constexpr std::size_t kXPosOffset = 0x08U;           // word (X pos)
    static constexpr std::size_t kYPosOffset = 0x0CU;           // word (Y pos)
    static constexpr std::size_t kFlagsXOffset = 0x40U;         // byte (collision flags X)
    static constexpr std::size_t kFlagsYOffset = 0x41U;         // byte (collision flags Y)
    static constexpr std::size_t kBoundingRadiusOffset = 0x46U; // word (bounding radius)
};

struct WorldMapBounds {
    std::uint16_t width_tiles{0U};
    std::uint16_t height_tiles{0U};

    [[nodiscard]] constexpr std::uint16_t max_x() const noexcept {
        return (width_tiles > 0U) ? static_cast<std::uint16_t>((width_tiles << 3U) - 1U) : 0U;
    }

    [[nodiscard]] constexpr std::uint16_t max_y() const noexcept {
        return (height_tiles > 0U) ? static_cast<std::uint16_t>((height_tiles << 3U) - 1U) : 0U;
    }
};

struct ClampStepResult {
    std::uint16_t clamped_step{0U};
    bool boundary_hit{false};
};

[[nodiscard]] ClampStepResult clamp_step_pos_x(
    std::uint16_t dx,
    std::uint16_t x,
    std::uint16_t radius,
    std::uint16_t width_tiles,
    std::uint8_t& flags_x) noexcept;

[[nodiscard]] ClampStepResult clamp_step_neg_x(
    std::uint16_t dx,
    std::uint16_t x,
    std::uint16_t radius,
    std::uint8_t& flags_x) noexcept;

[[nodiscard]] ClampStepResult clamp_step_pos_y(
    std::uint16_t dy,
    std::uint16_t y,
    std::uint16_t radius,
    std::uint16_t height_tiles,
    std::uint8_t& flags_y) noexcept;

[[nodiscard]] ClampStepResult clamp_step_neg_y(
    std::uint16_t dy,
    std::uint16_t y,
    std::uint16_t radius,
    std::uint8_t& flags_y) noexcept;

ClampStepResult step_entity_clamp_pos_x(
    std::span<std::uint8_t> slot,
    std::uint16_t dx,
    std::uint16_t width_tiles) noexcept;

ClampStepResult step_entity_clamp_neg_x(
    std::span<std::uint8_t> slot,
    std::uint16_t dx) noexcept;

ClampStepResult step_entity_clamp_pos_y(
    std::span<std::uint8_t> slot,
    std::uint16_t dy,
    std::uint16_t height_tiles) noexcept;

ClampStepResult step_entity_clamp_neg_y(
    std::span<std::uint8_t> slot,
    std::uint16_t dy) noexcept;

} // namespace oasis::game::player
