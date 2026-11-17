#include "game/entities/ai_steering_gravity.hpp"

#include <algorithm>
#include <cstdlib>

namespace oasis::game::entities {

FacingAndDistance facing_and_manhattan_to_player(
    std::int32_t enemy_x,
    std::int32_t enemy_y,
    std::int32_t player_x,
    std::int32_t player_y) noexcept {
    const std::int32_t dx = player_x - enemy_x;
    const std::int32_t dy = player_y - enemy_y;
    return FacingAndDistance{
        compute_cardinal_facing(dx, dy),
        std::abs(dx) + std::abs(dy),
    };
}

Steer4WayResult steer_4way_toward_player(
    std::int32_t enemy_x,
    std::int32_t enemy_y,
    std::int32_t player_x,
    std::int32_t player_y,
    std::uint16_t speed_x,
    std::uint16_t speed_y) noexcept {
    const std::int32_t dx = player_x - enemy_x;
    const std::int32_t dy = player_y - enemy_y;
    const std::uint8_t facing = compute_cardinal_facing(dx, dy);
    const VelocityFixed16_16 vel = compute_8dir_velocity(speed_x, speed_y, facing);
    const std::int32_t max_axis = std::max(std::abs(dx), std::abs(dy));
    return Steer4WayResult{
        facing,
        vel.vx,
        vel.vy,
        std::max(kMinChaseTimerFrames, max_axis),
    };
}

VelocityFixed16_16 aim_fixed_point_vector(
    std::int32_t enemy_x,
    std::int32_t enemy_y,
    std::int32_t target_x,
    std::int32_t target_y) noexcept {
    const std::int32_t dx = target_x - enemy_x;
    const std::int32_t dy = target_y - enemy_y;
    if (dx == 0 && dy == 0) {
        return VelocityFixed16_16{};
    }
    const std::int32_t major = std::max(std::abs(dx), std::abs(dy));
    return VelocityFixed16_16{
        static_cast<std::int32_t>((static_cast<std::int64_t>(dx) * kAimMajorStep16_16) / major),
        static_cast<std::int32_t>((static_cast<std::int64_t>(dy) * kAimMajorStep16_16) / major),
    };
}

KnockbackArcState launch_knockback_arc(std::uint8_t dir8) noexcept {
    const VelocityFixed16_16 vel = compute_8dir_velocity(kLaunchSpeedX, kLaunchSpeedY, dir8);
    return KnockbackArcState{0, vel.vx, vel.vy, kLaunchZVel16_16};
}

ZGravityStepResult step_z_gravity(
    std::int32_t z_16_16,
    std::int32_t vz_16_16) noexcept {
    const std::int32_t next_z = z_16_16 + vz_16_16;
    if (next_z < 0) {
        return ZGravityStepResult{0, 0, true};
    }
    std::int32_t next_vz = vz_16_16 - kGravityZ16_16;
    if (next_vz == 0) {
        next_vz = -1;
    }
    return ZGravityStepResult{next_z, next_vz, false};
}

} // namespace oasis::game::entities
