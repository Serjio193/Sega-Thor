#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace oasis::game::player {

struct PointTileIntegratorConfig {
    static constexpr std::uint32_t kPointTileSolidityProbeAddr = 0x0098ECU;
    static constexpr std::uint32_t kSamplePointTileAttrAddr = 0x0099B8U;
    static constexpr std::uint32_t kReadPointTileRawAddr = 0x0099D6U;
    static constexpr std::uint32_t kElevationProbeCallAddr = 0x009AD0U;

    static constexpr std::uint8_t kRampFlagMask = 0x90U;
    static constexpr std::uint8_t kTileAttrMask = 0x0FU;

    // Entity slot offsets
    static constexpr std::size_t kYPosOffset = 0x0CU;          // word (Y pos)
    static constexpr std::size_t kAirHeightOffset = 0x10U;     // word (air height)
    static constexpr std::size_t kGroundHeightOffset = 0x14U;  // word (ground height)
};

struct PointLayerTilemapDescriptor {
    const std::uint8_t* base{nullptr};
    std::size_t base_size{0U};
    std::int16_t row_stride{0};
    std::uint16_t row_shift{0U};
};

using PointElevationProbeFn = std::function<std::int16_t(std::int16_t x, std::int16_t y)>;
using RampHandlerFn = std::function<bool(std::int16_t x, std::int16_t y)>;

struct PointTileEnvironment {
    PointElevationProbeFn probe_elevation{};
    RampHandlerFn handle_ramp{};
};

struct PointTileProbeResult {
    bool is_solid{false}; // Carry flag: true = blocked, false = passable
    std::int16_t final_y{0};
    std::int16_t final_air_height{0};
    std::int16_t final_ground_height{0};
    bool is_ramp{false};
};

[[nodiscard]] std::uint8_t read_point_tile_raw(
    const PointLayerTilemapDescriptor& layer,
    std::int16_t x,
    std::int16_t y) noexcept;

[[nodiscard]] std::uint8_t sample_point_tile_attr(
    const PointLayerTilemapDescriptor& layer,
    std::int16_t x,
    std::int16_t y) noexcept;

[[nodiscard]] PointTileProbeResult evaluate_point_tile_solidity(
    std::int16_t x,
    std::int16_t y,
    std::int16_t cur_ground_height,
    std::int16_t cur_air_height,
    const PointLayerTilemapDescriptor& layer,
    const PointTileEnvironment& env) noexcept;

PointTileProbeResult step_point_tile_solidity_probe(
    std::span<std::uint8_t> slot,
    std::int16_t x,
    std::int16_t y,
    const PointLayerTilemapDescriptor& layer,
    const PointTileEnvironment& env) noexcept;

} // namespace oasis::game::player
