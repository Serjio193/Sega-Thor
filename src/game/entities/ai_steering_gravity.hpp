#pragma once

#include "game/entities/velocity_vector_math.hpp"

#include <cstdint>

namespace oasis::game::entities {

inline constexpr std::uint32_t kFacingAndManhattanAddr = 0x00E398U;
inline constexpr std::uint32_t kSteer4WayTowardPlayerAddr = 0x00E3E8U;
inline constexpr std::uint32_t kAimFixedPointVectorAddr = 0x00E45EU;
inline constexpr std::uint32_t kLaunchKnockbackArcAddr = 0x00E4D0U;
inline constexpr std::uint32_t kStepZGravityAddr = 0x00E502U;

inline constexpr std::int32_t kMinChaseTimerFrames = 16;
inline constexpr std::int32_t kAimMajorStep16_16 = 0x00020000; // 2.0 px/frame
inline constexpr std::uint16_t kLaunchSpeedX = 0x0300U;
inline constexpr std::uint16_t kLaunchSpeedY = 0x02E0U;
inline constexpr std::int32_t kLaunchZVel16_16 = 0x00060000;
inline constexpr std::int32_t kGravityZ16_16 = 0x0000C000;

struct FacingAndDistance {
    std::uint8_t facing{0U}; // 0=Up, 1=Right, 2=Down, 3=Left
    std::int32_t manhattan{0};
};

struct Steer4WayResult {
    std::uint8_t facing{0U};
    std::int32_t vx{0};
    std::int32_t vy{0};
    std::int32_t chase_timer{0};
};

struct KnockbackArcState {
    std::int32_t z{0};
    std::int32_t vx{0};
    std::int32_t vy{0};
    std::int32_t vz{0};
};

struct ZGravityStepResult {
    std::int32_t z{0};
    std::int32_t vz{0};
    bool landed{false};
};

/// Models 0x00E398..0x00E3E6: computes cardinal facing (0..3) and |dx| + |dy|.
[[nodiscard]] FacingAndDistance facing_and_manhattan_to_player(
    std::int32_t enemy_x,
    std::int32_t enemy_y,
    std::int32_t player_x,
    std::int32_t player_y) noexcept;

/// Models 0x00E3E8..0x00E45C: steers 4-way toward player with chase timer
/// max(16, max(|dx|, |dy|)).
[[nodiscard]] Steer4WayResult steer_4way_toward_player(
    std::int32_t enemy_x,
    std::int32_t enemy_y,
    std::int32_t player_x,
    std::int32_t player_y,
    std::uint16_t speed_x,
    std::uint16_t speed_y) noexcept;

/// Models 0x00E45E..0x00E4CE: 16.16 normalized 2.0 px/frame major-axis aim vector.
[[nodiscard]] VelocityFixed16_16 aim_fixed_point_vector(
    std::int32_t enemy_x,
    std::int32_t enemy_y,
    std::int32_t target_x,
    std::int32_t target_y) noexcept;

/// Models 0x00E4D0..0x00E500: launches knockback arc with 0x300/0x2E0 XY and 0x00060000 Z.
[[nodiscard]] KnockbackArcState launch_knockback_arc(std::uint8_t dir8) noexcept;

/// Models 0x00E502..0x00E520: steps Z gravity (vz -= 0x0000C000, skips 0 via -1, lands z < 0).
[[nodiscard]] ZGravityStepResult step_z_gravity(
    std::int32_t z_16_16,
    std::int32_t vz_16_16) noexcept;

} // namespace oasis::game::entities
