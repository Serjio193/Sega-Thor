#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct VerticalWallProbeConfig {
    static constexpr std::uint32_t kTileSolidProbeNegYAddr = 0x0094D2U;
    static constexpr std::uint32_t kTileSolidProbeNegYEndAddr = 0x0095AAU;
    static constexpr std::uint32_t kTileSolidProbePosYAddr = 0x0095AAU;
    static constexpr std::uint32_t kTileSolidProbePosYEndAddr = 0x009678U;
    static constexpr std::uint32_t kScanRowSolidityAddr = 0x009678U;
    static constexpr std::uint32_t kScanRowSolidityEndAddr = 0x0096BAU;
    static constexpr std::uint32_t kSampleRowSpanAddr = 0x0096BAU;
    static constexpr std::uint32_t kSampleRowSpanEndAddr = 0x0096E8U;

    static constexpr std::uint32_t kElevationLutRam = 0x00FF1706U;
    static constexpr std::uint32_t kLedgeDropHeightRam = 0x00FF196EU;
    static constexpr std::uint32_t kTerrainSurfaceLutAddr = 0x0096E8U;

    // Entity slot offsets
    static constexpr std::size_t kPosXOffset = 0x08U;             // word (X pos)
    static constexpr std::size_t kPosZOffset = 0x10U;             // long (Z pos / air height)
    static constexpr std::size_t kFlags38Offset = 0x38U;          // byte
    static constexpr std::size_t kCollisionFlags41Offset = 0x41U; // byte (vertical collision flags)
    static constexpr std::size_t kSurfaceTypeOffset = 0x70U;      // byte
    static constexpr std::size_t kVelYOffset = 0x76U;             // long (vel_y)

    // Flag bits in +0x38
    static constexpr std::uint8_t kFlagIntangibleBit = 0U;       // bit 0: intangible (passable)
    static constexpr std::uint8_t kFlagLedgeDropCapableBit = 4U; // bit 4: can drop from ledges
    static constexpr std::uint8_t kFlagNoClimbBit = 5U;          // bit 5: cannot climb 1-step incline
    static constexpr std::uint8_t kFlagSlopeCheckBit = 6U;       // bit 6: block steep slope edges

    // Flag bits in +0x41
    static constexpr std::uint8_t kCollisionLedgeDropBit = 5U;   // bit 5: ledge drop triggered (0x20)
    static constexpr std::uint8_t kCollisionWallBit = 6U;        // bit 6: solid wall hit (0x40)

    static constexpr std::uint8_t kCollisionLedgeDropMask = 0x20U;
    static constexpr std::uint8_t kCollisionWallMask = 0x40U;

    // 16-byte Terrain surface type translation table at 0x0096E8
    static constexpr std::array<std::uint8_t, 16> kTerrainSurfaceLut = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU,
    };
};

[[nodiscard]] constexpr std::uint8_t lookup_vertical_terrain_surface_type(std::uint8_t raw_terrain_byte) noexcept {
    return VerticalWallProbeConfig::kTerrainSurfaceLut[raw_terrain_byte & 0x0FU];
}

struct VerticalWallProbeResult {
    bool is_solid{false};              // Carry flag: true = blocked, false = passable
    bool wall_hit{false};              // bit 6 on +0x41
    bool ledge_drop_triggered{false};  // bit 5 on +0x41
    std::uint8_t flags_41{0U};         // updated +0x41 bits to set
    std::int16_t ledge_drop_height{0}; // drop height written to $FF196E if ledge drop
    std::uint8_t target_surface_type{0xFFU};
};

struct VerticalWallProbeParams {
    std::uint8_t flags_38{0U};
    std::uint8_t cur_surface_type{0U};
    std::uint8_t target_raw_terrain{0U};
    std::uint8_t accumulated_or_d4{0U};   // d4 from scan
    std::int32_t pos_z{0};                // +0x10(a6) air height
    std::int16_t scan_extent_a4{0};       // extent from row scan
    std::span<const std::int16_t> elevation_table{}; // RAM $FF1706 table
};

[[nodiscard]] VerticalWallProbeResult evaluate_vertical_solid_probe(
    const VerticalWallProbeParams& params) noexcept;

VerticalWallProbeResult step_vertical_wall_probe(
    std::span<std::uint8_t> slot,
    std::uint8_t target_raw_terrain,
    std::uint8_t accumulated_or_d4 = 0U,
    std::int16_t scan_extent_a4 = 0,
    std::span<const std::int16_t> elevation_table = {},
    std::uint16_t* ledge_drop_ram = nullptr) noexcept;

struct TileRowScanResult {
    std::uint8_t accumulated_or{0U};     // d4
    std::uint8_t accumulated_and{0xFFU}; // d5
    std::int16_t scan_extent_a4{0};      // a4
};

struct VerticalLayerTilemapDescriptor {
    const std::uint8_t* base{nullptr};
    std::size_t base_size{0U};
    std::int16_t row_stride{0};
    std::uint16_t row_shift{0U};
};

[[nodiscard]] TileRowScanResult sample_tile_row_span(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3) noexcept;

[[nodiscard]] TileRowScanResult scan_tile_row_solidity(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3) noexcept;

[[nodiscard]] VerticalWallProbeResult probe_vertical_wall_neg_y(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3,
    std::uint8_t flags_38,
    std::uint8_t cur_surface_type,
    std::int32_t pos_z,
    std::span<const std::int16_t> elevation_table = {}) noexcept;

[[nodiscard]] VerticalWallProbeResult probe_vertical_wall_pos_y(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3,
    std::uint8_t flags_38,
    std::uint8_t cur_surface_type,
    std::int32_t pos_z,
    std::span<const std::int16_t> elevation_table = {}) noexcept;

VerticalWallProbeResult step_vertical_wall_probe_neg_y(
    std::span<std::uint8_t> slot,
    std::int16_t row_d0,
    std::int16_t radius_d3,
    const VerticalLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = {},
    std::uint16_t* ledge_drop_ram = nullptr) noexcept;

VerticalWallProbeResult step_vertical_wall_probe_pos_y(
    std::span<std::uint8_t> slot,
    std::int16_t row_d0,
    std::int16_t radius_d3,
    const VerticalLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table = {},
    std::uint16_t* ledge_drop_ram = nullptr) noexcept;

} // namespace oasis::game::player
