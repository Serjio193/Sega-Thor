#include "game/player/world_boundary_clamps.hpp"

namespace oasis::game::player {

namespace {

inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(buf[offset]) << 8U) |
             static_cast<std::uint16_t>(buf[offset + 1U]));
    }
    return 0U;
}

} // namespace

ClampStepResult clamp_step_pos_x(
    std::uint16_t dx,
    std::uint16_t x,
    std::uint16_t radius,
    std::uint16_t width_tiles,
    std::uint8_t& flags_x) noexcept {
    auto d7 = static_cast<std::uint16_t>(x + radius);
    auto d3 = static_cast<std::uint16_t>((width_tiles << 3U) - 1U);
    d3 = static_cast<std::uint16_t>(d3 - d7);

    // 0x009D5E: cmp.w d3, d0; bls.w $9d6e (unsigned <=)
    if (dx <= d3) {
        return {dx, false};
    }

    dx = d3;
    if (dx != 0U) {
        return {dx, false};
    }

    flags_x |= WorldBoundaryClampsConfig::kBoundaryCollisionFlag;
    return {0U, true};
}

ClampStepResult clamp_step_neg_x(
    std::uint16_t dx,
    std::uint16_t x,
    std::uint16_t radius,
    std::uint8_t& flags_x) noexcept {
    auto d3 = static_cast<std::int32_t>(static_cast<std::int16_t>(x)) -
              static_cast<std::int32_t>(static_cast<std::int16_t>(radius));

    // 0x009D76: bmi.w $9d88
    if (d3 < 0) {
        flags_x |= WorldBoundaryClampsConfig::kBoundaryCollisionFlag;
        return {0U, true};
    }

    auto rem = static_cast<std::uint16_t>(d3);
    // 0x009D7A: cmp.w d3, d0; bls.w $9d86
    if (dx <= rem) {
        return {dx, false};
    }

    dx = rem;
    if (dx != 0U) {
        return {dx, false};
    }

    flags_x |= WorldBoundaryClampsConfig::kBoundaryCollisionFlag;
    return {0U, true};
}

ClampStepResult clamp_step_pos_y(
    std::uint16_t dy,
    std::uint16_t y,
    std::uint16_t radius,
    std::uint16_t height_tiles,
    std::uint8_t& flags_y) noexcept {
    auto d7 = static_cast<std::uint16_t>(y + radius);
    auto d3 = static_cast<std::uint16_t>((height_tiles << 3U) - 1U);
    d3 = static_cast<std::uint16_t>(d3 - d7);

    // 0x009DA2: cmp.w d3, d0; bls.w $9db2 (unsigned <=)
    if (dy <= d3) {
        return {dy, false};
    }

    dy = d3;
    if (dy != 0U) {
        return {dy, false};
    }

    flags_y |= WorldBoundaryClampsConfig::kBoundaryCollisionFlag;
    return {0U, true};
}

ClampStepResult clamp_step_neg_y(
    std::uint16_t dy,
    std::uint16_t y,
    std::uint16_t radius,
    std::uint8_t& flags_y) noexcept {
    auto d3 = static_cast<std::int32_t>(static_cast<std::int16_t>(y)) -
              static_cast<std::int32_t>(static_cast<std::int16_t>(radius));

    // 0x009DBA: bmi.w $9dcc
    if (d3 < 0) {
        flags_y |= WorldBoundaryClampsConfig::kBoundaryCollisionFlag;
        return {0U, true};
    }

    auto rem = static_cast<std::uint16_t>(d3);
    // 0x009DBE: cmp.w d3, d0; bls.w $9dca
    if (dy <= rem) {
        return {dy, false};
    }

    dy = rem;
    if (dy != 0U) {
        return {dy, false};
    }

    flags_y |= WorldBoundaryClampsConfig::kBoundaryCollisionFlag;
    return {0U, true};
}

ClampStepResult step_entity_clamp_pos_x(
    std::span<std::uint8_t> slot,
    std::uint16_t dx,
    std::uint16_t width_tiles) noexcept {
    if (slot.size() < 0x48U) {
        return {dx, false};
    }
    auto x = read_u16_be(slot, WorldBoundaryClampsConfig::kXPosOffset);
    auto rad = read_u16_be(slot, WorldBoundaryClampsConfig::kBoundingRadiusOffset);
    auto flags_x = slot[WorldBoundaryClampsConfig::kFlagsXOffset];
    auto res = clamp_step_pos_x(dx, x, rad, width_tiles, flags_x);
    slot[WorldBoundaryClampsConfig::kFlagsXOffset] = flags_x;
    return res;
}

ClampStepResult step_entity_clamp_neg_x(
    std::span<std::uint8_t> slot,
    std::uint16_t dx) noexcept {
    if (slot.size() < 0x48U) {
        return {dx, false};
    }
    auto x = read_u16_be(slot, WorldBoundaryClampsConfig::kXPosOffset);
    auto rad = read_u16_be(slot, WorldBoundaryClampsConfig::kBoundingRadiusOffset);
    auto flags_x = slot[WorldBoundaryClampsConfig::kFlagsXOffset];
    auto res = clamp_step_neg_x(dx, x, rad, flags_x);
    slot[WorldBoundaryClampsConfig::kFlagsXOffset] = flags_x;
    return res;
}

ClampStepResult step_entity_clamp_pos_y(
    std::span<std::uint8_t> slot,
    std::uint16_t dy,
    std::uint16_t height_tiles) noexcept {
    if (slot.size() < 0x48U) {
        return {dy, false};
    }
    auto y = read_u16_be(slot, WorldBoundaryClampsConfig::kYPosOffset);
    auto rad = read_u16_be(slot, WorldBoundaryClampsConfig::kBoundingRadiusOffset);
    auto flags_y = slot[WorldBoundaryClampsConfig::kFlagsYOffset];
    auto res = clamp_step_pos_y(dy, y, rad, height_tiles, flags_y);
    slot[WorldBoundaryClampsConfig::kFlagsYOffset] = flags_y;
    return res;
}

ClampStepResult step_entity_clamp_neg_y(
    std::span<std::uint8_t> slot,
    std::uint16_t dy) noexcept {
    if (slot.size() < 0x48U) {
        return {dy, false};
    }
    auto y = read_u16_be(slot, WorldBoundaryClampsConfig::kYPosOffset);
    auto rad = read_u16_be(slot, WorldBoundaryClampsConfig::kBoundingRadiusOffset);
    auto flags_y = slot[WorldBoundaryClampsConfig::kFlagsYOffset];
    auto res = clamp_step_neg_y(dy, y, rad, flags_y);
    slot[WorldBoundaryClampsConfig::kFlagsYOffset] = flags_y;
    return res;
}

} // namespace oasis::game::player
