#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kProbeTerrainXAddr = 0x00938EU;
inline constexpr std::uint32_t kProbeTerrainYUpAddr = 0x0094D2U;
inline constexpr std::uint32_t kProbeTerrainYDownAddr = 0x0095AAU;
inline constexpr std::uint32_t kElevationNibbleLutAddr = 0x0096E8U;
inline constexpr std::uint16_t kMaxLedgeScanPx = 0x0180U;
inline constexpr std::uint8_t kLedgeScanStepPx = 8U;
inline constexpr std::uint8_t kProbeFlagLedgeOrStepBit5 = 0x20U; // bit 5
inline constexpr std::uint8_t kProbeFlagBlockedBit6 = 0x40U;     // bit 6

/// 16-byte Gray-code elevation nibble LUT at ROM 0x0096E8.
inline constexpr std::array<std::int8_t, 16> kElevationNibbleLut16 = {
    -1, 0, 2, 1, 4, -1, 3, -1,
     6, 7, -1, -1, 5, -1, -1, -1
};

struct TileSpanAggregate {
    std::uint8_t or_bits{0};
    std::uint8_t and_bits{0};
};

struct ElevationProbeInput {
    std::uint8_t entity_flags_38{0};
    std::int8_t entity_elev_70{0};
    std::int32_t z_vel_10{0};
    std::span<const std::uint8_t> span_tiles{};
    std::array<std::uint16_t, 8> height_table{}; // $FF1706
    std::uint16_t ledge_scan_depth_px{0};
};

struct ElevationProbeResult {
    bool blocked{false};
    std::uint8_t flags{0};       // bit 5 (0x20) or bit 6 (0x40)
    std::int8_t tile_elev{-1};
    std::int8_t delta{0};
    std::uint16_t drop_height{0}; // $FF196E
};

/// Aggregates OR and AND across a span of collision tile bytes.
[[nodiscard]] TileSpanAggregate aggregate_tile_span(
    std::span<const std::uint8_t> tiles) noexcept;

/// Models 0x00938E / 0x0094D2 / 0x0095AA terrain elevation & ledge-drop decision core.
[[nodiscard]] ElevationProbeResult evaluate_terrain_elevation_probe(
    const ElevationProbeInput& input) noexcept;

} // namespace oasis::game::world
