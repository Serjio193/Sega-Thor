#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace oasis::game::player {

struct FlyingRaycasterConfig {
    static constexpr std::uint32_t kFlyingHorizontalRaycastAddr = 0x009708U;
    static constexpr std::uint32_t kFlyingHorizontalRaycastEndAddr = 0x0097E6U;
    static constexpr std::uint32_t kFlyingVerticalRaycastAddr = 0x0097E6U;
    static constexpr std::uint32_t kFlyingVerticalRaycastEndAddr = 0x0098C2U;
    static constexpr std::uint32_t kFlyingSurfaceSampleExitAddr = 0x0098C2U;
    static constexpr std::uint32_t kPointWallProbeAddr = 0x0098ECU;
    static constexpr std::uint32_t kEntityReturnDispatcherAddr = 0x009382U;

    // Direction codes
    static constexpr std::uint8_t kDirPosX = 0x11U;
    static constexpr std::uint8_t kDirNegX = 0x13U;
    static constexpr std::uint8_t kDirPosY = 0x12U;
    static constexpr std::uint8_t kDirNegY = 0x10U;

    // Collision bit masks
    static constexpr std::uint8_t kCollisionWallBit = 6U;
    static constexpr std::uint8_t kCollisionWallMask = 0x40U; // bit 6

    // Entity slot offsets
    static constexpr std::size_t kPosXOffset = 0x08U;             // word (X integer)
    static constexpr std::size_t kSubpixelXOffset = 0x0AU;        // word (X subpixel)
    static constexpr std::size_t kPosYOffset = 0x0CU;             // word (Y integer)
    static constexpr std::size_t kSubpixelYOffset = 0x0EU;        // word (Y subpixel)
    static constexpr std::size_t kSurfaceHeightOffset = 0x14U;    // word (ground height)
    static constexpr std::size_t kCollisionFlags40Offset = 0x40U; // byte (X direction / wall collision)
    static constexpr std::size_t kCollisionFlags41Offset = 0x41U; // byte (Y direction / wall collision)
    static constexpr std::size_t kRadiusOffset = 0x46U;           // word (collision radius)
    static constexpr std::size_t kSurfaceByte6EOffset = 0x6EU;    // byte
    static constexpr std::size_t kSurfaceByte6FOffset = 0x6FU;    // byte
    static constexpr std::size_t kVelXOffset = 0x72U;             // long (vel_x 16.16)
    static constexpr std::size_t kVelYOffset = 0x76U;             // long (vel_y 16.16)
};

using PointProbeFn = std::function<bool(std::int16_t x, std::int16_t y)>;
using ClampFn = std::function<std::int16_t(std::int16_t displacement)>;
using SweepFn = std::function<std::int16_t(std::int16_t displacement)>;
using SurfaceProbeFn = std::function<std::int16_t(std::int16_t x, std::int16_t y, std::int16_t radius)>;

struct FlyingRaycastEnvironment {
    PointProbeFn point_probe{};
    ClampFn clamp_boundary_x{};
    SweepFn sweep_entities_x{};
    ClampFn clamp_boundary_y{};
    SweepFn sweep_entities_y{};
    SurfaceProbeFn probe_surface_height{};
};

struct FlyingRaycastResult {
    std::int16_t final_x{0};
    std::uint16_t final_subpixel_x{0U};
    std::int16_t final_y{0};
    std::uint16_t final_subpixel_y{0U};
    bool hit_wall_x{false};
    bool hit_wall_y{false};
    std::uint8_t dir_code_x{0U};
    std::uint8_t dir_code_y{0U};
    std::uint32_t step_count_a3{0U};
    std::int16_t ground_surface_height{0};
};

[[nodiscard]] FlyingRaycastResult calculate_flying_horizontal_raycast(
    std::int16_t start_x,
    std::uint16_t subpixel_x,
    std::int32_t vel_x_16_16,
    std::int16_t cur_y,
    const FlyingRaycastEnvironment& env) noexcept;

[[nodiscard]] FlyingRaycastResult calculate_flying_vertical_raycast(
    std::int16_t cur_x,
    std::int16_t start_y,
    std::uint16_t subpixel_y,
    std::int32_t vel_y_16_16,
    const FlyingRaycastEnvironment& env) noexcept;

[[nodiscard]] FlyingRaycastResult calculate_flying_dual_axis_raycast(
    std::int16_t start_x,
    std::uint16_t subpixel_x,
    std::int32_t vel_x_16_16,
    std::int16_t start_y,
    std::uint16_t subpixel_y,
    std::int32_t vel_y_16_16,
    std::int16_t radius_46,
    const FlyingRaycastEnvironment& env) noexcept;

FlyingRaycastResult step_flying_entity_simulation(
    std::span<std::uint8_t> slot,
    const FlyingRaycastEnvironment& env) noexcept;

} // namespace oasis::game::player
