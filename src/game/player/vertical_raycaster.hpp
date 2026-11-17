#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>

namespace oasis::game::player {

struct VerticalRaycasterConfig {
    static constexpr std::uint32_t kPosXClearAddr = 0x0091F6U;
    static constexpr std::uint32_t kPosYRaycasterAddr = 0x00920CU;
    static constexpr std::uint32_t kPosYRaycasterEndAddr = 0x0092B0U;
    static constexpr std::uint32_t kNegYRaycasterAddr = 0x0092B0U;
    static constexpr std::uint32_t kNegYRaycasterEndAddr = 0x00934EU;
    static constexpr std::uint32_t kFinalYWriteAddr = 0x00934EU;
    static constexpr std::uint32_t kFinalYWriteEndAddr = 0x009382U;
    static constexpr std::uint32_t kEntityReturnDispatcherAddr = 0x009382U;
    static constexpr std::uint32_t kEntityReturnDispatcherEndAddr = 0x00938EU;
    static constexpr std::uint32_t kTerrainSurfaceLutAddr = 0x0096E8U;
    static constexpr std::uint32_t kReturnVectorRamAddr = 0x00FF193CU;

    // Movement direction codes
    static constexpr std::uint8_t kDirCodePosY = 0x12U; // Down (+Y)
    static constexpr std::uint8_t kDirCodeNegY = 0x10U; // Up (-Y)

    // Offsets in 188-byte entity slot
    static constexpr std::size_t kPosXOffset = 0x08U;           // word (integer X)
    static constexpr std::size_t kSubpixelXOffset = 0x0AU;      // word (fractional X)
    static constexpr std::size_t kPosYOffset = 0x0CU;           // word (integer Y)
    static constexpr std::size_t kSubpixelYOffset = 0x0EU;      // word (fractional Y)
    static constexpr std::size_t kSurfaceHeightOffset = 0x14U;  // word
    static constexpr std::size_t kDirCodeXOffset = 0x40U;       // byte
    static constexpr std::size_t kDirCodeYOffset = 0x41U;       // byte
    static constexpr std::size_t kRadiusOffset = 0x46U;         // word (radius used for both X and Y)
    static constexpr std::size_t kTerrainByteOffset = 0x6EU;    // byte
    static constexpr std::size_t kTerrainAttrOffset = 0x6FU;    // byte
    static constexpr std::size_t kTerrainTypeOffset = 0x70U;    // byte
    static constexpr std::size_t kVelXOffset = 0x72U;           // 32-bit (16.16)
    static constexpr std::size_t kVelYOffset = 0x76U;           // 32-bit (16.16)

    // 16-byte Terrain surface type translation table at 0x0096E8
    static constexpr std::array<std::uint8_t, 16> kTerrainSurfaceLut = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU,
    };
};

[[nodiscard]] constexpr std::uint8_t lookup_terrain_surface_type(std::uint8_t raw_terrain_byte) noexcept {
    return VerticalRaycasterConfig::kTerrainSurfaceLut[raw_terrain_byte & 0x0FU];
}

[[nodiscard]] constexpr std::uint16_t compute_tile_row_distance_down(std::uint16_t probe_y) noexcept {
    return static_cast<std::uint16_t>(7U - (probe_y & 7U));
}

[[nodiscard]] constexpr std::uint16_t compute_tile_row_distance_up(std::uint16_t probe_y) noexcept {
    return static_cast<std::uint16_t>(probe_y & 7U);
}

[[nodiscard]] constexpr std::uint32_t dispatch_entity_return(std::uint32_t return_vector_ram) noexcept {
    return return_vector_ram;
}

struct VerticalRaycastEnvironment {
    std::function<bool(std::int16_t x_pos, std::int16_t tile_row)> is_tile_solid{};
    std::function<std::pair<std::uint16_t, std::uint8_t>(std::int16_t x, std::int16_t y)> probe_surface{};
    std::function<std::int16_t(std::int16_t displacement)> clamp_boundary{};
    std::function<std::int16_t(std::int16_t displacement)> sweep_solid_entities{};
};

struct VerticalRaycastResult {
    std::int16_t final_y{0};
    std::uint16_t final_subpixel_y{0U};
    std::size_t tiles_stepped{0U};
    bool hit_wall{false};
    std::uint16_t surface_height{0U};
    std::uint8_t terrain_type{0xFFU};
    std::uint32_t return_vector{0U};
};

VerticalRaycastResult calculate_vertical_raycast(
    std::int16_t pos_x,
    std::int16_t start_y,
    std::uint16_t subpixel_y,
    std::int32_t vel_y_16_16,
    std::uint16_t radius,
    const VerticalRaycastEnvironment& env = {}) noexcept;

VerticalRaycastResult step_vertical_raycast(
    std::span<std::uint8_t> slot,
    std::uint32_t return_vector_ram = 0U,
    const VerticalRaycastEnvironment& env = {}) noexcept;

} // namespace oasis::game::player
