#include "game/math/isqrt_atan2.hpp"

namespace oasis::game::math {

std::uint16_t isqrt_u32(std::uint32_t val) noexcept {
    if (val == 0U) {
        return 0U;
    }

    std::uint32_t d2 = (val >= kIsqrtGuessCap) ? kIsqrtGuessCap : val;
    while (true) {
        const std::uint32_t d1 = d2;
        d2 = ((val / d1) + d1) >> 1U;
        if (d2 >= d1) {
            return static_cast<std::uint16_t>(d1);
        }
    }
}

std::uint8_t atan_q1(std::int16_t dx, std::int16_t dy) noexcept {
    if (dx == 0) {
        return kAngle90Deg; // 0x40 = 64
    }

    const std::uint32_t ratio = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(dy)) << 8U)
        / static_cast<std::uint32_t>(static_cast<std::uint16_t>(dx));

    std::uint8_t angle = 0U;
    for (const std::uint16_t thresh : kTangentLut0x05D906) {
        if (ratio <= thresh) {
            return angle;
        }
        angle = static_cast<std::uint8_t>(angle + kAngleStep);
    }
    return angle;
}

std::uint8_t atan2_256(std::int16_t dx, std::int16_t dy_up) noexcept {
    if (dx >= 0) {
        if (dy_up >= 0) {
            // Quadrant 1 (Right..Up): 0..64
            return atan_q1(dx, dy_up);
        }
        // Quadrant 4 (Down..Right): 192..255 (-angle mod 256)
        const std::uint8_t q1 = atan_q1(dx, static_cast<std::int16_t>(-dy_up));
        return static_cast<std::uint8_t>(-static_cast<std::int16_t>(q1));
    }

    const auto abs_dx = static_cast<std::int16_t>(-dx);
    if (dy_up >= 0) {
        // Quadrant 2 (Up..Left): 64..128 (128 - angle)
        const std::uint8_t q1 = atan_q1(abs_dx, dy_up);
        return static_cast<std::uint8_t>(kAngle180Deg - q1);
    }

    // Quadrant 3 (Left..Down): 128..192 (128 + angle)
    const std::uint8_t q1 = atan_q1(abs_dx, static_cast<std::int16_t>(-dy_up));
    return static_cast<std::uint8_t>(kAngle180Deg + q1);
}

std::uint8_t atan2_to_point(
    std::int16_t src_x,
    std::int16_t src_y,
    std::int16_t dst_x,
    std::int16_t dst_y) noexcept {
    const auto dx = static_cast<std::int16_t>(dst_x - src_x);
    const auto dy_up = static_cast<std::int16_t>(src_y - dst_y);
    return atan2_256(dx, dy_up);
}

std::uint8_t atan2_to_player(
    std::int16_t src_x,
    std::int16_t src_y,
    std::int16_t player_x,
    std::int16_t player_y) noexcept {
    return atan2_to_point(src_x, src_y, player_x, player_y);
}

} // namespace oasis::game::math
