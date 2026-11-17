#include "game/entities/velocity_vector_math.hpp"

namespace oasis::game::entities {

namespace {

[[nodiscard]] constexpr std::int32_t scale_diagonal_075(std::int32_t v) noexcept {
    return (v >> 1) + (v >> 2);
}

} // namespace

VelocityFixed16_16 compute_8dir_velocity(
    std::uint16_t speed_x,
    std::uint16_t speed_y,
    std::uint8_t direction) noexcept {
    const std::int32_t sx = static_cast<std::int32_t>(speed_x) << 8;
    const std::int32_t sy = static_cast<std::int32_t>(speed_y) << 8;

    switch (direction & 7U) {
    case 0U: // N
        return {0, -sy};
    case 1U: // E
        return {sx, 0};
    case 2U: // S
        return {0, sy};
    case 3U: // W
        return {-sx, 0};
    case 4U: // NE
        return {scale_diagonal_075(sx), -scale_diagonal_075(sy)};
    case 5U: // SE
        return {scale_diagonal_075(sx), scale_diagonal_075(sy)};
    case 6U: // SW
        return {-scale_diagonal_075(sx), scale_diagonal_075(sy)};
    case 7U: // NW
        return {-scale_diagonal_075(sx), -scale_diagonal_075(sy)};
    default:
        return {};
    }
}

std::uint8_t compute_cardinal_facing(
    std::int32_t vx,
    std::int32_t vy) noexcept {
    const std::int64_t abs_vx = vx < 0 ? -static_cast<std::int64_t>(vx)
                                       : static_cast<std::int64_t>(vx);
    const std::int64_t abs_vy = vy < 0 ? -static_cast<std::int64_t>(vy)
                                       : static_cast<std::int64_t>(vy);
    if (abs_vx <= abs_vy) {
        return vy < 0 ? 0U : 2U;
    }
    return vx < 0 ? 3U : 1U;
}

} // namespace oasis::game::entities
