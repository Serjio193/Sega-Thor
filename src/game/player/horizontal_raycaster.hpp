#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>

namespace oasis::game::player {

struct HorizontalRaycasterConfig {
    static constexpr std::uint32_t kPosXRaycasterAddr = 0x009084U;
    static constexpr std::uint32_t kPosXRaycasterEndAddr = 0x009128U;
    static constexpr std::uint32_t kNegXRaycasterAddr = 0x009128U;
    static constexpr std::uint32_t kNegXRaycasterEndAddr = 0x0091C6U;
    static constexpr std::uint32_t kRaycastCleanupAddr = 0x0091C6U;
    static constexpr std::uint32_t kRaycastCleanupEndAddr = 0x0091F6U;
    static constexpr std::uint32_t kTerrainSurfaceLutAddr = 0x0096E8U;

    // Movement direction codes
    static constexpr std::uint8_t kDirCodePosX = 0x11U;
    static constexpr std::uint8_t kDirCodeNegX = 0x13U;

    // Offsets in 188-byte entity slot
    static constexpr std::size_t kPosXOffset = 0x08U;           // word (integer X)
    static constexpr std::size_t kSubpixelXOffset = 0x0AU;      // word (fractional X)
    static constexpr std::size_t kPosYOffset = 0x0CU;           // word (integer Y)
    static constexpr std::size_t kSurfaceHeightOffset = 0x14U;  // word
    static constexpr std::size_t kDirCodeOffset = 0x40U;        // byte
    static constexpr std::size_t kRadiusXOffset = 0x46U;        // word
    static constexpr std::size_t kTerrainByteOffset = 0x6EU;    // byte
    static constexpr std::size_t kTerrainAttrOffset = 0x6FU;    // byte
    static constexpr std::size_t kTerrainTypeOffset = 0x70U;    // byte
    static constexpr std::size_t kVelXOffset = 0x72U;           // 32-bit (16.16)

    // 16-byte Terrain surface type translation table at 0x0096E8
    static constexpr std::array<std::uint8_t, 16> kTerrainSurfaceLut = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU,
    };
};

[[nodiscard]] constexpr std::uint8_t lookup_terrain_surface_type(std::uint8_t raw_terrain_byte) noexcept {
    return HorizontalRaycasterConfig::kTerrainSurfaceLut[raw_terrain_byte & 0x0FU];
}

[[nodiscard]] constexpr std::uint16_t compute_tile_distance_right(std::uint16_t probe_x) noexcept {
    return static_cast<std::uint16_t>(7U - (probe_x & 7U));
}

[[nodiscard]] constexpr std::uint16_t compute_tile_distance_left(std::uint16_t probe_x) noexcept {
    return static_cast<std::uint16_t>(probe_x & 7U);
}

struct RaycastEnvironment {
    std::function<bool(std::int16_t tile_col, std::int16_t y_pos)> is_tile_solid{};
    std::function<std::pair<std::uint16_t, std::uint8_t>(std::int16_t x, std::int16_t y)> probe_surface{};
    std::function<std::int16_t(std::int16_t displacement)> clamp_boundary{};
    std::function<std::int16_t(std::int16_t displacement)> sweep_solid_entities{};
};

struct HorizontalRaycastResult {
    std::int16_t final_x{0};
    std::uint16_t final_subpixel{0U};
    std::size_t tiles_stepped{0U};
    bool hit_wall{false};
    std::uint16_t surface_height{0U};
    std::uint8_t terrain_type{0xFFU};
};

HorizontalRaycastResult step_horizontal_raycast(
    std::span<std::uint8_t> slot,
    const RaycastEnvironment& env = {}) noexcept;

HorizontalRaycastResult calculate_horizontal_raycast(
    std::int16_t start_x,
    std::uint16_t subpixel_x,
    std::int32_t vel_x_16_16,
    std::int16_t pos_y,
    std::uint16_t radius_x,
    const RaycastEnvironment& env = {}) noexcept;

} // namespace oasis::game::player
