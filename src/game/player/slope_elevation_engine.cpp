#include "game/player/slope_elevation_engine.hpp"

namespace oasis::game::player {

namespace {

inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        std::uint16_t u = (static_cast<std::uint16_t>(buf[offset]) << 8U) |
                          static_cast<std::uint16_t>(buf[offset + 1U]);
        return static_cast<std::int16_t>(u);
    }
    return 0;
}

inline void write_i16_be(std::span<std::uint8_t> buf, std::size_t offset, std::int16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        auto u = static_cast<std::uint16_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

inline std::int16_t get_table_elevation(
    std::span<const std::int16_t> table,
    std::size_t idx,
    std::int16_t fallback) noexcept {
    if (idx < table.size()) {
        return table[idx];
    }
    return fallback;
}

} // namespace

std::int16_t probe_tile_elevation(
    std::int16_t x,
    std::int16_t y,
    std::int16_t entity_y,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table) noexcept {
    auto void_h = get_table_elevation(
        elevation_table,
        SlopeElevationEngineConfig::kVoidElevationEntry,
        SlopeElevationEngineConfig::kVoidElevation);

    if (layer.base == nullptr || layer.base_size == 0U) {
        return void_h;
    }

    auto col = static_cast<std::int16_t>(x >> 3);
    auto row = static_cast<std::int16_t>(y >> 3);
    auto offset = static_cast<std::int32_t>((row << layer.row_shift) + col);

    if (offset < 0 || static_cast<std::size_t>(offset) >= layer.base_size) {
        return void_h;
    }

    auto raw = layer.base[offset];
    auto attr = static_cast<std::uint8_t>(raw & 0x0FU);
    auto surface_code = SlopeElevationEngineConfig::kLut2SurfaceCodes[attr];

    if (surface_code == 0xFFU) {
        // 0x009BAC: entry 7 in elevation table (void/pit)
        return void_h;
    }

    // Even surface code -> flat surface (0x009BB0)
    if ((surface_code & 1U) == 0U) {
        return get_table_elevation(elevation_table, surface_code, 0);
    }

    // Odd surface code -> slope fractional interpolation (0x009B26)
    auto d2 = static_cast<std::int16_t>(entity_y & 7);
    auto d3 = static_cast<std::int16_t>(7 - (entity_y & 7));
    d2 = static_cast<std::int16_t>(d2 - 8);

    auto curr_offset = offset;
    auto d0 = attr;
    auto up_attr = d0;

    // Upward scan loop (0x009B4A..0x009B58)
    while (true) {
        d2 = static_cast<std::int16_t>(d2 + 8);
        curr_offset -= layer.row_stride;
        if (curr_offset < 0 || static_cast<std::size_t>(curr_offset) >= layer.base_size) {
            break;
        }
        up_attr = static_cast<std::uint8_t>(layer.base[curr_offset] & 0x0FU);
        if (up_attr != d0) {
            break;
        }
    }

    // 0x009B60..0x009B6A: compare surface codes
    auto up_code = SlopeElevationEngineConfig::kLut2SurfaceCodes[up_attr];
    auto cur_code = SlopeElevationEngineConfig::kLut2SurfaceCodes[d0];
    if (up_code < cur_code) {
        // 0x009BA6 -> 0x009BB0: fallback to flat surface
        return get_table_elevation(elevation_table, surface_code, 0);
    }

    // Downward scan loop (0x009B70..0x009B80)
    curr_offset = offset;
    d3 = static_cast<std::int16_t>(d3 - 8);
    while (true) {
        d3 = static_cast<std::int16_t>(d3 + 8);
        curr_offset += layer.row_stride;
        if (curr_offset < 0 || static_cast<std::size_t>(curr_offset) >= layer.base_size) {
            break;
        }
        auto down_attr = static_cast<std::uint8_t>(layer.base[curr_offset] & 0x0FU);
        if (down_attr != d0) {
            break;
        }
    }

    // 0x009B82: d2 = total Y span
    auto total_span = static_cast<std::int16_t>(d2 + d3);
    if (total_span == 0) {
        total_span = 1;
    }

    // 0x009B8A..0x009B9C: fractional interpolation via mulu/divu
    auto base_idx = static_cast<std::size_t>(surface_code - 1U);
    auto next_idx = base_idx + 2U;
    auto base_h = get_table_elevation(elevation_table, base_idx, 0);
    auto next_h = get_table_elevation(elevation_table, next_idx, base_h);
    auto height_delta = static_cast<std::int32_t>(next_h) - static_cast<std::int32_t>(base_h);
    auto fraction = static_cast<std::int16_t>((height_delta * d3) / total_span);

    return static_cast<std::int16_t>(base_h + fraction);
}

std::int16_t probe_entity_elevation(
    std::span<const std::uint8_t> slot,
    std::uint8_t layer_select_ff16f0,
    const PointLayerTilemapDescriptor& layer1,
    const PointLayerTilemapDescriptor& layer2,
    std::span<const std::int16_t> elevation_table) noexcept {
    const auto& layer = ((layer_select_ff16f0 & 0x80U) != 0U) ? layer2 : layer1;
    auto x = read_i16_be(slot, SlopeElevationEngineConfig::kXPosOffset);
    auto y = read_i16_be(slot, SlopeElevationEngineConfig::kYPosOffset);
    return probe_tile_elevation(x, y, y, layer, elevation_table);
}

RampSearchResult evaluate_ramp_search_handler_1(
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table) noexcept {
    auto start_tile_y = static_cast<std::int16_t>((y >> 3) + 1);
    auto tile_x = static_cast<std::int16_t>(x >> 3);
    auto tile_offset = static_cast<std::int32_t>((start_tile_y << layer.row_shift) + tile_x);

    std::int16_t d5 = 0;
    bool found = false;

    for (std::uint32_t i = 0; i < SlopeElevationEngineConfig::kMaxRampScanTiles; ++i) {
        d5 = static_cast<std::int16_t>(d5 + 8);
        if (layer.base != nullptr && tile_offset >= 0 &&
            static_cast<std::size_t>(tile_offset) < layer.base_size) {
            std::uint8_t raw = layer.base[tile_offset];
            if ((raw & SlopeElevationEngineConfig::kRampBit4Mask) == 0U) {
                found = true;
                break;
            }
        }
        tile_offset += layer.row_stride;
    }

    if (!found) {
        return {RampSearchBranch::BlockedWall, y, 0, 0, d5, false};
    }

    auto candidate_y = static_cast<std::int16_t>(y + d5);
    auto new_elevation = probe_tile_elevation(x, candidate_y, candidate_y, layer, elevation_table);

    if (new_elevation < 0) {
        return {RampSearchBranch::BlockedWall, y, 0, new_elevation, d5, true};
    }

    if (prev_elevation < 0) {
        return {RampSearchBranch::PassableGroundCommit, y, 0, new_elevation, d5, true};
    }

    auto diff = static_cast<std::int16_t>(prev_elevation - new_elevation);
    if (diff <= 0) {
        return {RampSearchBranch::BlockedWall, y, 0, new_elevation, d5, true};
    }

    diff = static_cast<std::int16_t>(diff - d5);
    if (diff < 0) {
        return {RampSearchBranch::BlockedWall, y, 0, new_elevation, d5, true};
    }

    return {
        RampSearchBranch::ContinueSlopeTraversal,
        candidate_y,
        d5,
        new_elevation,
        d5,
        true
    };
}

RampSearchResult evaluate_ramp_search_handler_2(
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table) noexcept {
    auto start_tile_y = static_cast<std::int16_t>((y >> 3) + 1);
    auto tile_x = static_cast<std::int16_t>(x >> 3);
    auto tile_offset = static_cast<std::int32_t>((start_tile_y << layer.row_shift) + tile_x);

    std::int16_t d5 = 0;
    bool found = false;

    for (std::uint32_t i = 0; i < SlopeElevationEngineConfig::kMaxRampScanTiles; ++i) {
        d5 = static_cast<std::int16_t>(d5 + 8);
        if (layer.base != nullptr && tile_offset >= 0 &&
            static_cast<std::size_t>(tile_offset) < layer.base_size) {
            std::uint8_t raw = layer.base[tile_offset];
            if ((raw & SlopeElevationEngineConfig::kRampBit4Mask) == 0U) {
                found = true;
                break;
            }
        }
        tile_offset += layer.row_stride;
    }

    if (!found) {
        return {RampSearchBranch::PassableGroundCommit, y, 0, 0, d5, false};
    }

    auto candidate_y = static_cast<std::int16_t>(y + d5);
    auto new_elevation = probe_tile_elevation(x, candidate_y, candidate_y, layer, elevation_table);

    if (new_elevation < 0) {
        return {RampSearchBranch::PassableGroundCommit, y, 0, new_elevation, d5, true};
    }

    if (prev_elevation < 0) {
        return {RampSearchBranch::PassableGroundCommit, y, 0, new_elevation, d5, true};
    }

    auto diff = static_cast<std::int16_t>(prev_elevation - new_elevation);
    if (diff <= 0) {
        return {RampSearchBranch::PassableGroundCommit, y, 0, new_elevation, d5, true};
    }

    diff = static_cast<std::int16_t>(diff - d5);
    if (diff < 0) {
        return {RampSearchBranch::PassableGroundCommit, y, 0, new_elevation, d5, true};
    }

    return {
        RampSearchBranch::ContinueSlopeTraversal,
        candidate_y,
        d5,
        new_elevation,
        d5,
        true
    };
}

RampSearchResult step_ramp_search_handler_1(
    std::span<std::uint8_t> slot,
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table) noexcept {
    auto res = evaluate_ramp_search_handler_1(x, y, prev_elevation, layer, elevation_table);

    if (slot.size() >= 0x7AU) {
        if (res.branch == RampSearchBranch::ContinueSlopeTraversal) {
            write_i16_be(slot, SlopeElevationEngineConfig::kYPosOffset, res.final_y);
            auto cur_air_h = read_i16_be(slot, SlopeElevationEngineConfig::kAirHeightOffset);
            write_i16_be(slot, SlopeElevationEngineConfig::kAirHeightOffset,
                         static_cast<std::int16_t>(cur_air_h + res.air_height_delta));
        } else if (res.branch == RampSearchBranch::PassableGroundCommit) {
            write_i16_be(slot, SlopeElevationEngineConfig::kGroundHeightOffset, res.new_elevation);
        }
    }

    return res;
}

RampSearchResult step_ramp_search_handler_2(
    std::span<std::uint8_t> slot,
    std::int16_t x,
    std::int16_t y,
    std::int16_t prev_elevation,
    const PointLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table) noexcept {
    auto res = evaluate_ramp_search_handler_2(x, y, prev_elevation, layer, elevation_table);

    if (slot.size() >= 0x7AU) {
        if (res.branch == RampSearchBranch::ContinueSlopeTraversal) {
            write_i16_be(slot, SlopeElevationEngineConfig::kYPosOffset, res.final_y);
            auto cur_air_h = read_i16_be(slot, SlopeElevationEngineConfig::kAirHeightOffset);
            write_i16_be(slot, SlopeElevationEngineConfig::kAirHeightOffset,
                         static_cast<std::int16_t>(cur_air_h + res.air_height_delta));
        } else if (res.branch == RampSearchBranch::PassableGroundCommit) {
            write_i16_be(slot, SlopeElevationEngineConfig::kGroundHeightOffset, res.new_elevation);
        }
    }

    return res;
}

} // namespace oasis::game::player
