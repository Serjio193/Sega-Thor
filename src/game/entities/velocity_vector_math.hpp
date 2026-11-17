#pragma once

#include <cstdint>

namespace oasis::game::entities {

inline constexpr std::uint32_t kCompute8DirVelocityAddr = 0x00E2D4U;
inline constexpr std::uint32_t kVelocityDirTableAddr = 0x00E2F2U;
inline constexpr std::uint32_t kComputeCardinalFacingAddr = 0x00E338U;

/// 16.16 fixed-point velocity pair produced by ROM 0x00E2D4.
struct VelocityFixed16_16 {
    std::int32_t vx{0};
    std::int32_t vy{0};
};

/// Models 0x00E2D4: converts u16 (speed_x, speed_y) to 16.16 fixed-point
/// (speed << 8) and dispatches direction & 7 across N/E/S/W/NE/SE/SW/NW,
/// scaling diagonals by 0.75 via (v >> 1) + (v >> 2).
[[nodiscard]] VelocityFixed16_16 compute_8dir_velocity(
    std::uint16_t speed_x,
    std::uint16_t speed_y,
    std::uint8_t direction) noexcept;

/// Models 0x00E338: maps (vx, vy) to cardinal facing 0..3
/// (|vx| <= |vy| -> vy < 0 ? 0 : 2; else vx < 0 ? 3 : 1).
[[nodiscard]] std::uint8_t compute_cardinal_facing(
    std::int32_t vx,
    std::int32_t vy) noexcept;

} // namespace oasis::game::entities
