#include "game/player/vertical_wall_probe.hpp"

namespace oasis::game::player {

namespace {

inline std::int32_t read_i32_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 4U <= buf.size()) {
        std::uint32_t u = (static_cast<std::uint32_t>(buf[offset]) << 24U) |
                          (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
                          (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
                          static_cast<std::uint32_t>(buf[offset + 3U]);
        return static_cast<std::int32_t>(u);
    }
    return 0;
}

inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        std::uint16_t u = (static_cast<std::uint16_t>(buf[offset]) << 8U) |
                          static_cast<std::uint16_t>(buf[offset + 1U]);
        return static_cast<std::int16_t>(u);
    }
    return 0;
}

inline void write_i32_be(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) noexcept {
    if (offset + 4U <= buf.size()) {
        auto u = static_cast<std::uint32_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

} // namespace

TileRowScanResult sample_tile_row_span(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3) noexcept {
    TileRowScanResult res{};
    if (layer.base == nullptr || layer.base_size == 0U) {
        return res;
    }

    std::int16_t start_col = static_cast<std::int16_t>((center_x - radius_d3) >> 3);
    std::int16_t end_col = static_cast<std::int16_t>((center_x + radius_d3) >> 3);
    std::int16_t count = static_cast<std::int16_t>(end_col - start_col);
    if (count < 0) {
        return res;
    }

    std::size_t row_offset = (layer.row_shift != 0U)
        ? (static_cast<std::size_t>(static_cast<std::uint16_t>(row_d0)) << layer.row_shift)
        : (static_cast<std::size_t>(static_cast<std::uint16_t>(row_d0)) *
           static_cast<std::size_t>(layer.row_stride));

    std::size_t offset = row_offset + static_cast<std::size_t>(static_cast<std::uint16_t>(start_col));

    for (std::int16_t i = 0; i <= count; ++i) {
        if (offset < layer.base_size) {
            std::uint8_t byte_val = layer.base[offset];
            res.accumulated_or |= byte_val;
            res.accumulated_and &= byte_val;
        }
        offset += 1U;
    }

    return res;
}

TileRowScanResult scan_tile_row_solidity(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3) noexcept {
    TileRowScanResult res = sample_tile_row_span(layer, row_d0, center_x, radius_d3);

    // 0x009684: btst.b #4, d5; beq.w $96b6; btst.b #7, d5; beq.w $96b6
    if ((res.accumulated_and & 0x90U) != 0x90U) {
        res.scan_extent_a4 = 0;
        return res;
    }

    std::int16_t d7 = 0;
    std::int16_t cur_row = row_d0;

    while (true) {
        d7 = static_cast<std::int16_t>(d7 + 8);
        cur_row = static_cast<std::int16_t>(cur_row + 1);
        res = sample_tile_row_span(layer, cur_row, center_x, radius_d3);

        // 0x00969C: btst.b #4, d4; beq.w $96b6
        if ((res.accumulated_or & 0x10U) == 0U) {
            res.scan_extent_a4 = d7;
            return res;
        }

        // 0x0096A4: cmpi.w #$180, d7; bne.b $9694
        if (d7 == 0x0180) {
            d7 = 0;
            cur_row = static_cast<std::int16_t>(cur_row - 0x30);
            res = sample_tile_row_span(layer, cur_row, center_x, radius_d3);
            res.scan_extent_a4 = d7;
            return res;
        }
    }
}

VerticalWallProbeResult evaluate_vertical_solid_probe(
    const VerticalWallProbeParams& params) noexcept {
    VerticalWallProbeResult res{};

    // 0x0094D6 / 0x0095AE: btst.b #0, flags_38 -> intangible
    if ((params.flags_38 & 0x01U) != 0U) {
        return res;
    }

    // 0x0094E2 / 0x0095BA: tst.b $70(a6) -> bmi passable
    if (static_cast<std::int8_t>(params.cur_surface_type) < 0) {
        return res;
    }

    res.target_surface_type = lookup_vertical_terrain_surface_type(params.target_raw_terrain);

    // 0x009506 / 0x0095D4: bmi.w wall hit
    if (res.target_surface_type == 0xFFU || static_cast<std::int8_t>(res.target_surface_type) < 0) {
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    auto delta = static_cast<std::int16_t>(
        static_cast<std::int16_t>(res.target_surface_type) -
        static_cast<std::int16_t>(params.cur_surface_type));

    // 0x009512 / 0x0095E0: beq
    if (delta == 0) {
        if ((params.accumulated_or_d4 & 0x80U) != 0U) {
            if ((params.flags_38 & 0x40U) != 0U) {
                res.is_solid = true;
                res.wall_hit = true;
                res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
                return res;
            }
        }
        if ((params.accumulated_or_d4 & 0x10U) != 0U) {
            res.is_solid = true;
            res.wall_hit = true;
            res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
            return res;
        }
        return res;
    }

    // 0x009516 / 0x0095E4: cmpi.b #2, d0; bge
    if (delta >= 2) {
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    // 0x00951E / 0x0095EC: delta == 1
    if (delta == 1) {
        if ((params.flags_38 & 0x20U) != 0U) {
            res.is_solid = true;
            res.wall_hit = true;
            res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
            return res;
        }
        if ((params.accumulated_or_d4 & 0x10U) != 0U) {
            res.is_solid = true;
            res.wall_hit = true;
            res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
            return res;
        }
        return res;
    }

    // 0x009528 / 0x0095F6: cmpi.b #$fe, d0; ble
    if (delta <= -2) {
        bool can_drop = (params.flags_38 & 0x10U) != 0U;
        bool on_ground = (params.pos_z == 0);
        bool even_surface = ((params.cur_surface_type & 0x01U) == 0U);

        if (can_drop && on_ground && even_surface) {
            auto cur_idx = static_cast<std::size_t>(params.cur_surface_type);
            auto tgt_idx = static_cast<std::size_t>(res.target_surface_type);
            if (cur_idx < params.elevation_table.size() && tgt_idx < params.elevation_table.size()) {
                auto drop_h = static_cast<std::int16_t>(
                    params.elevation_table[cur_idx] - params.elevation_table[tgt_idx]);
                if (drop_h >= params.scan_extent_a4) {
                    res.is_solid = true;
                    res.ledge_drop_triggered = true;
                    res.flags_41 = VerticalWallProbeConfig::kCollisionLedgeDropMask;
                    res.ledge_drop_height = drop_h;
                    return res;
                }
            }
        }

        res.is_solid = true;
        res.wall_hit = true;
        res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    // delta == -1
    if ((params.accumulated_or_d4 & 0x10U) != 0U) {
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_41 = VerticalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    return res;
}

VerticalWallProbeResult step_vertical_wall_probe(
    std::span<std::uint8_t> slot,
    std::uint8_t target_raw_terrain,
    std::uint8_t accumulated_or_d4,
    std::int16_t scan_extent_a4,
    std::span<const std::int16_t> elevation_table,
    std::uint16_t* ledge_drop_ram) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    VerticalWallProbeParams params{};
    params.flags_38 = slot[VerticalWallProbeConfig::kFlags38Offset];
    params.cur_surface_type = slot[VerticalWallProbeConfig::kSurfaceTypeOffset];
    params.target_raw_terrain = target_raw_terrain;
    params.accumulated_or_d4 = accumulated_or_d4;
    params.pos_z = read_i32_be(slot, VerticalWallProbeConfig::kPosZOffset);
    params.scan_extent_a4 = scan_extent_a4;
    params.elevation_table = elevation_table;

    auto res = evaluate_vertical_solid_probe(params);
    slot[VerticalWallProbeConfig::kCollisionFlags41Offset] |= res.flags_41;

    if (res.ledge_drop_triggered) {
        write_i32_be(slot, VerticalWallProbeConfig::kVelYOffset, 0);
        if (ledge_drop_ram != nullptr) {
            *ledge_drop_ram = static_cast<std::uint16_t>(res.ledge_drop_height);
        }
    }

    return res;
}

VerticalWallProbeResult probe_vertical_wall_neg_y(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3,
    std::uint8_t flags_38,
    std::uint8_t cur_surface_type,
    std::int32_t pos_z,
    std::span<const std::int16_t> elevation_table) noexcept {
    TileRowScanResult scan = sample_tile_row_span(layer, row_d0, center_x, radius_d3);

    VerticalWallProbeParams params{};
    params.flags_38 = flags_38;
    params.cur_surface_type = cur_surface_type;
    params.target_raw_terrain = static_cast<std::uint8_t>(scan.accumulated_and & 0x0FU);
    params.accumulated_or_d4 = scan.accumulated_or;
    params.pos_z = pos_z;
    params.scan_extent_a4 = 0; // 0x0094F0: clr.w d7 -> a4 = 0
    params.elevation_table = elevation_table;

    return evaluate_vertical_solid_probe(params);
}

VerticalWallProbeResult probe_vertical_wall_pos_y(
    const VerticalLayerTilemapDescriptor& layer,
    std::int16_t row_d0,
    std::int16_t center_x,
    std::int16_t radius_d3,
    std::uint8_t flags_38,
    std::uint8_t cur_surface_type,
    std::int32_t pos_z,
    std::span<const std::int16_t> elevation_table) noexcept {
    TileRowScanResult scan = scan_tile_row_solidity(layer, row_d0, center_x, radius_d3);

    VerticalWallProbeParams params{};
    params.flags_38 = flags_38;
    params.cur_surface_type = cur_surface_type;
    params.target_raw_terrain = static_cast<std::uint8_t>(scan.accumulated_and & 0x0FU);
    params.accumulated_or_d4 = scan.accumulated_or;
    params.pos_z = pos_z;
    params.scan_extent_a4 = scan.scan_extent_a4;
    params.elevation_table = elevation_table;

    return evaluate_vertical_solid_probe(params);
}

VerticalWallProbeResult step_vertical_wall_probe_neg_y(
    std::span<std::uint8_t> slot,
    std::int16_t row_d0,
    std::int16_t radius_d3,
    const VerticalLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table,
    std::uint16_t* ledge_drop_ram) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    std::int16_t center_x = read_i16_be(slot, VerticalWallProbeConfig::kPosXOffset);
    std::uint8_t flags_38 = slot[VerticalWallProbeConfig::kFlags38Offset];
    std::uint8_t cur_surface_type = slot[VerticalWallProbeConfig::kSurfaceTypeOffset];
    std::int32_t pos_z = read_i32_be(slot, VerticalWallProbeConfig::kPosZOffset);

    auto res = probe_vertical_wall_neg_y(
        layer, row_d0, center_x, radius_d3, flags_38, cur_surface_type, pos_z, elevation_table);

    slot[VerticalWallProbeConfig::kCollisionFlags41Offset] |= res.flags_41;

    if (res.ledge_drop_triggered) {
        write_i32_be(slot, VerticalWallProbeConfig::kVelYOffset, 0);
        if (ledge_drop_ram != nullptr) {
            *ledge_drop_ram = static_cast<std::uint16_t>(res.ledge_drop_height);
        }
    }

    return res;
}

VerticalWallProbeResult step_vertical_wall_probe_pos_y(
    std::span<std::uint8_t> slot,
    std::int16_t row_d0,
    std::int16_t radius_d3,
    const VerticalLayerTilemapDescriptor& layer,
    std::span<const std::int16_t> elevation_table,
    std::uint16_t* ledge_drop_ram) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    std::int16_t center_x = read_i16_be(slot, VerticalWallProbeConfig::kPosXOffset);
    std::uint8_t flags_38 = slot[VerticalWallProbeConfig::kFlags38Offset];
    std::uint8_t cur_surface_type = slot[VerticalWallProbeConfig::kSurfaceTypeOffset];
    std::int32_t pos_z = read_i32_be(slot, VerticalWallProbeConfig::kPosZOffset);

    auto res = probe_vertical_wall_pos_y(
        layer, row_d0, center_x, radius_d3, flags_38, cur_surface_type, pos_z, elevation_table);

    slot[VerticalWallProbeConfig::kCollisionFlags41Offset] |= res.flags_41;

    if (res.ledge_drop_triggered) {
        write_i32_be(slot, VerticalWallProbeConfig::kVelYOffset, 0);
        if (ledge_drop_ram != nullptr) {
            *ledge_drop_ram = static_cast<std::uint16_t>(res.ledge_drop_height);
        }
    }

    return res;
}

} // namespace oasis::game::player
