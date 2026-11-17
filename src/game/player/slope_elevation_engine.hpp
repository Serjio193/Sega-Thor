#pragma once

#include "game/player/point_tile_integrator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct SlopeElevationEngineConfig {
    static constexpr std::uint32_t kRampSearchHandler1Addr = 0x0099F0U;
    static constexpr std::uint32_t kRampSearchHandler2Addr = 0x009A60U;
    static constexpr std::uint32_t kCustomElevationProbeAddr = 0x009AD0U;
    static constexpr std::uint32_t kEntityElevationProbeAddr = 0x009AD6U;
    static constexpr std::uint32_t kLut2Addr = 0x0096F8U;

    static constexpr std::uint32_t kElevationTableRam = 0x00FF1706U;
    static constexpr std::uint32_t kLayer1Ram = 0x00FF1766U;
    static constexpr std::uint32_t kLayer2Ram = 0x00FF1770U;
    static constexpr std::uint32_t kLayerSelectRam = 0x00FF16F0U;

    static constexpr std::uint32_t kMaxRampScanTiles = 48U; // moveq #0x2F, d6
    static constexpr std::uint8_t kRampBit4Mask = 0x10U;

    // Entity slot offsets
    static constexpr std::size_t kXPosOffset = 0x08U;          // word (X pos)
    static constexpr std::size_t kYPosOffset = 0x0CU;          // word (Y pos)
    static constexpr std::size_t kAirHeightOffset = 0x10U;     // word (air height)
    static constexpr std::size_t kGroundHeightOffset = 0x14U;  // word (ground height)

    static constexpr std::array<std::uint8_t, 16> kLut2SurfaceCodes = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0x04U, 0x03U, 0x03U,
        0x06U, 0x06U, 0x06U, 0x06U, 0x05U, 0x05U, 0x05U, 0x05U
    };

    static constexpr std::int16_t kVoidElevation = static_cast<std::int16_t>(0xF000);
    static constexpr std::size_t kVoidElevationEntry = 7U;

    static constexpr std::array<std::int16_t, 8> kDefaultElevationTable = {
        0, 8, 16, 24, 32, 40, 48, static_cast<std::int16_t>(0xF000)
    };
};

enum class RampSearchBranch {
    ContinueSlopeTraversal, // bra.w 0x009944
    BlockedWall,            // bra.w 0x009918 (sets Carry)
    PassableGroundCommit    // bra.w 0x0099AA (clears Carry, stores elevation)
};

struct RampSearchResult {
    RampSearchBranch branch{RampSearchBranch::BlockedWall};
    std::int16_t final_y{0};
    std::int16_t air_height_delta{0};
    std::int16_t new_elevation{0};
    std::int16_t scan_offset{0};
    bool found_candidate{false};
};

[[nodiscard]] std::int16_t probe_tile_elevation(
    std::int16_t x,
    std::int16_t y,
    std::int16_t entity_y,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

[[nodiscard]] std::int16_t probe_entity_elevation(
    std::span<const std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

[[nodiscard]] RampSearchResult evaluate_ramp_search_handler_1(
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

[[nodiscard]] RampSearchResult evaluate_ramp_search_handler_2(
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

RampSearchResult step_ramp_search_handler_1(
    std::span<std::uint8_t> slot,
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

RampSearchResult step_ramp_search_handler_2(
    std::span<std::uint8_t> slot,
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = SlopeElevationEngineConfig::kDefaultElevationTable) noexcept;

} // namespace oasis::game::player
