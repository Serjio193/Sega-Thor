#include "game/world/terrain_elevation_probe.hpp"

namespace oasis::game::world {

TileSpanAggregate aggregate_tile_span(std::span<const std::uint8_t> tiles) noexcept {
    if (tiles.empty()) {
        return {0x00U, 0x00U};
    }
    std::uint8_t or_bits = 0x00U;
    std::uint8_t and_bits = 0xFFU;
    for (const std::uint8_t tile : tiles) {
        or_bits |= tile;
        and_bits &= tile;
    }
    return {or_bits, and_bits};
}

ElevationProbeResult evaluate_terrain_elevation_probe(
    const ElevationProbeInput& input) noexcept {
    if ((input.entity_flags_38 & 0x01U) != 0U || input.entity_elev_70 < 0) {
        return {false, 0U, -1, 0, 0U};
    }

    const TileSpanAggregate agg = aggregate_tile_span(input.span_tiles);
    const std::int8_t tile_elev = kElevationNibbleLut16[agg.and_bits & 0x0FU];
    if (tile_elev < 0) {
        return {true, kProbeFlagBlockedBit6, tile_elev, 0, 0U};
    }

    const std::int8_t delta = static_cast<std::int8_t>(tile_elev - input.entity_elev_70);
    if (delta == 0) {
        const bool wall_bit7 = (agg.or_bits & 0x80U) != 0U && (input.entity_flags_38 & 0x40U) == 0U;
        const bool wall_bit4 = (agg.or_bits & 0x10U) != 0U;
        if (wall_bit7 || wall_bit4) {
            return {true, kProbeFlagBlockedBit6, tile_elev, delta, 0U};
        }
        return {false, 0U, tile_elev, delta, 0U};
    }

    if (delta >= 2) {
        return {true, kProbeFlagBlockedBit6, tile_elev, delta, 0U};
    }

    if (delta == 1) {
        if ((input.entity_flags_38 & 0x20U) != 0U || (agg.or_bits & 0x10U) != 0U) {
            return {true, kProbeFlagLedgeOrStepBit5, tile_elev, delta, 0U};
        }
        return {false, 0U, tile_elev, delta, 0U};
    }

    if (delta == -1) {
        if ((agg.or_bits & 0x10U) != 0U) {
            return {true, kProbeFlagBlockedBit6, tile_elev, delta, 0U};
        }
        return {false, 0U, tile_elev, delta, 0U};
    }

    // delta <= -2: ledge-drop eligibility check
    if ((input.entity_flags_38 & 0x10U) != 0U &&
        input.z_vel_10 == 0 &&
        (input.entity_elev_70 & 1) == 0 &&
        input.ledge_scan_depth_px <= kMaxLedgeScanPx) {
        const std::uint16_t h_ent =
            input.height_table[static_cast<std::size_t>(input.entity_elev_70 & 7)];
        const std::uint16_t h_tile =
            input.height_table[static_cast<std::size_t>(tile_elev & 7)];
        if (h_ent >= h_tile) {
            const std::uint16_t drop = static_cast<std::uint16_t>(h_ent - h_tile);
            if (drop >= input.ledge_scan_depth_px) {
                return {true, kProbeFlagLedgeOrStepBit5, tile_elev, delta, drop};
            }
        }
    }

    return {true, kProbeFlagBlockedBit6, tile_elev, delta, 0U};
}

} // namespace oasis::game::world
