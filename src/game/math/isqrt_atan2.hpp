#pragma once

#include <array>
#include <cstdint>

namespace oasis::game::math {

inline constexpr std::uint32_t kIsqrtU32Addr = 0x00E6BAU;
inline constexpr std::uint32_t kAtan2ToPointAddr = 0x00E6F0U;
inline constexpr std::uint32_t kAtan2ToPlayerAddr = 0x00E706U;
inline constexpr std::uint32_t kAtanOctantLutAddr = 0x00E768U;
inline constexpr std::uint32_t kTangentLutRomAddr = 0x0005D906U;

inline constexpr std::uint32_t kIsqrtGuessCap = 0x0400U; // 1024
inline constexpr std::uint8_t kAngleStep = 8U;
inline constexpr std::uint8_t kAngle90Deg = 64U;
inline constexpr std::uint8_t kAngle180Deg = 128U;
inline constexpr std::uint8_t kAngle270Deg = 192U;

// ROM 0x0005D906: 9-word 8.8 fixed-point tangent thresholds for 0..64 in octant 1.
inline constexpr std::array<std::uint16_t, 9> kTangentLut0x05D906 = {
    0x0019U, 0x004EU, 0x0089U, 0x00D2U, 0x0138U, 0x01DFU, 0x034CU, 0x0A27U, 0xFFFFU
};

/// Models 0x00E6BA..0x00E6EE: integer square root via Heron's method with initial guess min(val, 0x0400).
[[nodiscard]] std::uint16_t isqrt_u32(std::uint32_t val) noexcept;

/// Models 0x00E768..0x00E78C: octant-1 atan using 9-threshold tangent LUT (returns 0..64 in steps of 8).
[[nodiscard]] std::uint8_t atan_q1(std::int16_t dx, std::int16_t dy) noexcept;

/// Models 0x00E722..0x00E766: 256-step atan2 angle (0..255) where 0=Right, 64=Up, 128=Left, 192=Down.
/// Note: dy_up = src_y - dst_y (upwards in screen space).
[[nodiscard]] std::uint8_t atan2_256(std::int16_t dx, std::int16_t dy_up) noexcept;

/// Models 0x00E6F0..0x00E704: computes 256-step angle from (src_x, src_y) to (dst_x, dst_y).
[[nodiscard]] std::uint8_t atan2_to_point(
    std::int16_t src_x,
    std::int16_t src_y,
    std::int16_t dst_x,
    std::int16_t dst_y) noexcept;

/// Models 0x00E706..0x00E720: computes 256-step angle from entity (src_x, src_y) to player.
[[nodiscard]] std::uint8_t atan2_to_player(
    std::int16_t src_x,
    std::int16_t src_y,
    std::int16_t player_x,
    std::int16_t player_y) noexcept;

} // namespace oasis::game::math
