#include "game/entities/viewport_culler_landing.hpp"

#include "game/entities/ai_steering_gravity.hpp"

#include <algorithm>

namespace oasis::game::entities {

namespace {

[[nodiscard]] bool is_point_offscreen(
    std::int16_t px,
    std::int16_t py,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::int16_t margin) noexcept {
    const std::int32_t min_x = static_cast<std::int32_t>(cam_x) - margin;
    const std::int32_t min_y = static_cast<std::int32_t>(cam_y) - margin;
    const std::int32_t max_x = min_x + kViewportWidthPx + static_cast<std::int32_t>(margin) * 2;
    const std::int32_t max_y = min_y + kViewportHeightPx + static_cast<std::int32_t>(margin) * 2;
    return px < min_x || py < min_y || px > max_x || py > max_y;
}

} // namespace

AirborneStepResult step_airborne_move_and_land(
    AirborneEntityState state,
    bool spin_when_idle_ff1971_bit0) noexcept {
    AirborneStepResult result{};
    result.next_state = state;

    if (state.vz_16_16 == 0) {
        result.status = ((state.flags_37 & 0x02U) != 0U)
            ? AirborneLandStatus::GroundedClear
            : AirborneLandStatus::Grounded;
        return result;
    }

    const ZGravityStepResult z_res = step_z_gravity(state.z_16_16, state.vz_16_16);
    result.next_state.z_16_16 = z_res.z;
    result.next_state.vz_16_16 = z_res.vz;

    if (z_res.landed) {
        result.next_state.z_16_16 = 0;
        result.next_state.vz_16_16 = 0;
        result.next_state.air_timer_2e = 0U;
        result.next_state.byte_83 = 0U;
        result.next_state.flags_39 = static_cast<std::uint8_t>(state.flags_39 | 0x04U);
        result.next_state.landing_stun_9a = kLandingStunFrames;
        if ((state.flags_39 & 0x02U) != 0U) {
            result.next_state.flags_39 = static_cast<std::uint8_t>(
                result.next_state.flags_39 & ~0x04U);
            result.sound_cmd = kLandingThudSfx;
        }
        result.status = AirborneLandStatus::JustLanded;
        return result;
    }

    result.next_state.x_16_16 += state.vx_16_16;
    result.next_state.y_16_16 += state.vy_16_16;
    if (state.vx_16_16 == 0 && state.vy_16_16 == 0 && spin_when_idle_ff1971_bit0) {
        result.next_state.facing = static_cast<std::uint16_t>((state.facing + 1U) & 0x03U);
    }
    result.status = AirborneLandStatus::Airborne;
    return result;
}

bool is_offscreen_with_z(
    std::int16_t x,
    std::int16_t y,
    std::int16_t z,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::int16_t margin) noexcept {
    return is_point_offscreen(x, static_cast<std::int16_t>(y - z), cam_x, cam_y, margin);
}

bool is_offscreen_ground(
    std::int16_t x,
    std::int16_t y,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::int16_t margin) noexcept {
    return is_point_offscreen(x, y, cam_x, cam_y, margin);
}

void copy_entity_struct_0xbc(
    std::span<const std::uint8_t, kEntityStructBytes> src,
    std::span<std::uint8_t, kEntityStructBytes> dst) noexcept {
    std::copy(src.begin(), src.end(), dst.begin());
}

} // namespace oasis::game::entities
