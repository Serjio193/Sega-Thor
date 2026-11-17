#include "game/player/horizontal_wall_probe.hpp"

namespace oasis::game::player {

namespace {

inline void write_i32_be(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) noexcept {
    if (offset + 4U <= buf.size()) {
        auto u = static_cast<std::uint32_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

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

} // namespace

WallProbeResult evaluate_tile_solid_probe(const WallProbeParams& params) noexcept {
    WallProbeResult res{};

    // 0x00938E..0x009396: btst.b #0, $38(a6); bne.w $945a -> intangible is passable
    if ((params.flags_38 & (1U << HorizontalWallProbeConfig::kFlagIntangibleBit)) != 0U) {
        return res;
    }

    // 0x00939E: tst.b $70(a6); bmi.w $9456 -> negative current surface is passable
    if (static_cast<std::int8_t>(params.cur_surface_type) < 0) {
        return res;
    }

    // 0x0093AA..0x0093B4: map target surface type via 16-byte LUT
    res.target_surface_type = lookup_terrain_surface_type(params.target_raw_terrain);

    // 0x0093B8: bmi.w $9446 -> negative target surface is void/pit barrier
    if (static_cast<std::int8_t>(res.target_surface_type) < 0) {
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    auto cur_s = static_cast<std::int8_t>(params.cur_surface_type);
    auto tgt_s = static_cast<std::int8_t>(res.target_surface_type);
    auto delta = static_cast<std::int16_t>(tgt_s - cur_s);

    if (delta == 0) {
        // 0x0093EE: btst.b #7, d4 (steep slope/edge bit)
        if ((params.accumulated_or_d4 & 0x80U) != 0U) {
            // 0x0093F6: btst.b #6, $38(a6)
            if ((params.flags_38 & (1U << HorizontalWallProbeConfig::kFlagSlopeCheckBit)) != 0U) {
                res.is_solid = true;
                res.wall_hit = true;
                res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
                return res;
            }
        }
        // 0x0093E2: btst.b #4, d4 (wall/barrier solid bit)
        if ((params.accumulated_or_d4 & 0x10U) != 0U) {
            res.is_solid = true;
            res.wall_hit = true;
            res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
            return res;
        }
        return res; // passable
    }

    if (delta >= 2) {
        // 0x0093C8..0x0093CC: delta >= 2 -> too high, solid wall
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    if (delta == 1) {
        // 0x0093D0: btst.b #5, $38(a6) -> climb disabled
        if ((params.flags_38 & (1U << HorizontalWallProbeConfig::kFlagNoClimbBit)) != 0U) {
            res.is_solid = true;
            res.wall_hit = true;
            res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
            return res;
        }
        // 0x0093E2: btst.b #4, d4
        if ((params.accumulated_or_d4 & 0x10U) != 0U) {
            res.is_solid = true;
            res.wall_hit = true;
            res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
            return res;
        }
        return res; // passable 1-step incline
    }

    // delta <= -2 (0x0093DA..0x009402: signed 8-bit, 0xFE is -2)
    if (delta <= -2) {
        // 0x009402: btst.b #4, $38(a6) (ledge drop capable)
        bool can_drop = (params.flags_38 & (1U << HorizontalWallProbeConfig::kFlagLedgeDropCapableBit)) != 0U;
        // 0x00940C: tst.l $10(a6) (grounded: air height == 0)
        bool on_ground = (params.pos_z == 0);
        // 0x009414: btst.b #0, d7 (even surface type / flat floor)
        bool even_surface = ((params.cur_surface_type & 1U) == 0U);

        if (can_drop && on_ground && even_surface && !params.elevation_table.empty()) {
            std::size_t cur_idx = params.cur_surface_type;
            std::size_t tgt_idx = res.target_surface_type;
            if (cur_idx < params.elevation_table.size() && tgt_idx < params.elevation_table.size()) {
                std::int16_t drop_h = static_cast<std::int16_t>(
                    params.elevation_table[cur_idx] - params.elevation_table[tgt_idx]);
                // 0x009430: cmp.w a4, d7; blt.b $9446
                if (drop_h >= params.scan_extent_a4) {
                    res.is_solid = true; // sets Carry (halts horizontal step)
                    res.ledge_drop_triggered = true;
                    res.flags_40 = HorizontalWallProbeConfig::kCollisionLedgeDropMask; // bit 5
                    res.ledge_drop_height = drop_h;
                    return res;
                }
            }
        }

        // Otherwise treated as solid wall/barrier
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    // delta == -1: step down 1 level
    if ((params.accumulated_or_d4 & 0x10U) != 0U) {
        res.is_solid = true;
        res.wall_hit = true;
        res.flags_40 = HorizontalWallProbeConfig::kCollisionWallMask;
        return res;
    }

    return res;
}

WallProbeResult step_horizontal_wall_probe(
    std::span<std::uint8_t> slot,
    std::uint8_t target_raw_terrain,
    std::uint8_t accumulated_or_d4,
    std::int16_t scan_extent_a4,
    std::span<const std::int16_t> elevation_table,
    std::uint16_t* ledge_drop_ram) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    WallProbeParams params{};
    params.flags_38 = slot[HorizontalWallProbeConfig::kFlags38Offset];
    params.cur_surface_type = slot[HorizontalWallProbeConfig::kSurfaceTypeOffset];
    params.target_raw_terrain = target_raw_terrain;
    params.accumulated_or_d4 = accumulated_or_d4;
    params.pos_z = read_i32_be(slot, HorizontalWallProbeConfig::kPosZOffset);
    params.scan_extent_a4 = scan_extent_a4;
    params.elevation_table = elevation_table;

    auto res = evaluate_tile_solid_probe(params);

    slot[HorizontalWallProbeConfig::kCollisionFlags40Offset] |= res.flags_40;

    if (res.ledge_drop_triggered) {
        // 0x00943A: clr.l $76(a6)
        write_i32_be(slot, HorizontalWallProbeConfig::kVelYOffset, 0);
        if (ledge_drop_ram != nullptr) {
            *ledge_drop_ram = static_cast<std::uint16_t>(res.ledge_drop_height);
        }
    }

    return res;
}

TileColumnScanResult sample_tile_column_span(
    const LayerTilemapDescriptor& layer,
    std::int16_t col_d0,
    std::int16_t center_y,
    std::int16_t radius_d3) noexcept {
    TileColumnScanResult res{};
    if (layer.base == nullptr || layer.base_size == 0U) {
        return res;
    }

    std::int16_t start_row = static_cast<std::int16_t>((center_y - radius_d3) >> 3);
    std::int16_t end_row = static_cast<std::int16_t>((center_y + radius_d3) >> 3);
    std::int16_t count = static_cast<std::int16_t>(end_row - start_row);
    if (count < 0) {
        return res;
    }

    std::size_t offset = static_cast<std::size_t>(
        static_cast<std::uint16_t>(col_d0) + (static_cast<std::uint16_t>(start_row) << layer.col_shift));

    for (std::int16_t i = 0; i <= count; ++i) {
        if (offset < layer.base_size) {
            std::uint8_t byte_val = layer.base[offset];
            res.accumulated_or |= byte_val;
            res.accumulated_and &= byte_val;
        }
        offset += static_cast<std::size_t>(layer.row_stride);
    }

    return res;
}

} // namespace oasis::game::player
