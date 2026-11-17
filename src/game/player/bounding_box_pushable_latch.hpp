#pragma once

#include "game/player/point_tile_integrator.hpp"
#include "game/player/slope_elevation_engine.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct BoundingBoxPushableLatchConfig {
    static constexpr std::uint32_t kSampleEntityBoundingBoxDispatchAddr = 0x009BC2U;
    static constexpr std::uint32_t kSampleBoundingTileBoxAddr = 0x009BF2U;
    static constexpr std::uint32_t kReadPointTileLayerAddr = 0x009C40U;
    static constexpr std::uint32_t kScanPushableBlockSolidCollisionAddr = 0x009C7CU;
    static constexpr std::uint32_t kLatchEntityGroundSurfaceAddr = 0x009D00U;

    static constexpr std::uint32_t kTerrainSurfaceLutAddr = 0x0096E8U;
    static constexpr std::uint32_t kPushableBlockPoolRam = 0x00FF2D8CU;
    static constexpr std::uint32_t kLayer1Ram = 0x00FF1766U;
    static constexpr std::uint32_t kLayer2Ram = 0x00FF1770U;
    static constexpr std::uint32_t kLayerSelectRam = 0x00FF16F0U;

    static constexpr std::uint16_t kPushableBlockType = 0x0056U;
    static constexpr std::size_t kPushableBlockSlotCount = 6U;
    static constexpr std::size_t kPushableBlockSlotStride = 90U; // 0x5A

    // Slot offsets
    static constexpr std::size_t kTypeOffset = 0x00U;           // word
    static constexpr std::size_t kXPosOffset = 0x08U;           // word
    static constexpr std::size_t kYPosOffset = 0x0CU;           // word
    static constexpr std::size_t kGroundHeightOffset = 0x14U;   // word
    static constexpr std::size_t kRadXOffset = 0x42U;           // word
    static constexpr std::size_t kRadYOffset = 0x44U;           // word
    static constexpr std::size_t kBoundingRadiusOffset = 0x46U; // word
    static constexpr std::size_t kAndAccumOffset = 0x6EU;       // byte
    static constexpr std::size_t kOrAccumOffset = 0x6FU;        // byte
    static constexpr std::size_t kSurfaceTypeOffset = 0x70U;    // byte

    // 16-byte LUT at 0x0096E8
    static constexpr std::array<std::uint8_t, 16> kTerrainSurfaceLut = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU
    };
};

struct TileBoxSampleResult {
    std::uint8_t or_accum{0U};
    std::uint8_t and_accum{0xFFU};
};

struct PushableBlockDesc {
    std::uint16_t type{0U};
    std::int16_t x{0};
    std::int16_t y{0};
    std::int16_t rad_x{0};
    std::int16_t rad_y{0};
};

[[nodiscard]] TileBoxSampleResult sample_bounding_tile_box(
    std::int16_t center_x,
    std::int16_t center_y,
    std::int16_t radius,
    const PointLayerTilemapDescriptor& layer) noexcept;

[[nodiscard]] TileBoxSampleResult sample_entity_bounding_tile_box_dispatch(
    std::span<const std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2) noexcept;

[[nodiscard]] std::uint8_t read_point_tile_layer(
    std::span<const std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2) noexcept;

[[nodiscard]] bool scan_pushable_block_solid_collision(
    std::int16_t entity_x,
    std::int16_t entity_y,
    std::int16_t entity_rad_x,
    std::int16_t entity_rad_y,
    std::span<const PushableBlockDesc> block_pool) noexcept;

[[nodiscard]] bool scan_pushable_block_solid_collision_raw(
    std::span<const std::uint8_t> entity_slot,
    std::span<const std::uint8_t> block_pool_bytes) noexcept;

void latch_entity_ground_surface(
    std::span<std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

} // namespace oasis::game::player
