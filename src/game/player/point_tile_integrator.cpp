#include "game/player/point_tile_integrator.hpp"

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

} // namespace

std::uint8_t read_point_tile_raw(
    const PointLayerTilemapDescriptor& layer,
    std::int16_t x,
    std::int16_t y) noexcept {
    if (layer.base == nullptr || layer.base_size == 0U) {
        return 0U;
    }

    auto col = static_cast<std::int16_t>(x >> 3);
    auto row = static_cast<std::int16_t>(y >> 3);

    std::size_t row_offset = (layer.row_shift != 0U)
        ? (static_cast<std::size_t>(static_cast<std::uint16_t>(row)) << layer.row_shift)
        : (static_cast<std::size_t>(static_cast<std::uint16_t>(row)) *
           static_cast<std::size_t>(layer.row_stride));

    std::size_t offset = row_offset + static_cast<std::size_t>(static_cast<std::uint16_t>(col));
    if (offset < layer.base_size) {
        return layer.base[offset];
    }
    return 0U;
}

std::uint8_t sample_point_tile_attr(
    const PointLayerTilemapDescriptor& layer,
    std::int16_t x,
    std::int16_t y) noexcept {
    return read_point_tile_raw(layer, x, y) & PointTileIntegratorConfig::kTileAttrMask;
}

PointTileProbeResult evaluate_point_tile_solidity(
    std::int16_t x,
    std::int16_t y,
    std::int16_t cur_ground_height,
    std::int16_t cur_air_height,
    const PointLayerTilemapDescriptor& layer,
    const PointTileEnvironment& env) noexcept {
    PointTileProbeResult res{};
    res.final_y = y;
    res.final_air_height = cur_air_height;
    res.final_ground_height = cur_ground_height;

    std::int16_t target_h = 0;
    if (env.probe_elevation) {
        target_h = env.probe_elevation(x, y);
    }

    // 0x0098FE: tst.w d4; bmi.b $9920
    if (target_h < 0) {
        auto raw = read_point_tile_raw(layer, x, y);
        if ((raw & PointTileIntegratorConfig::kRampFlagMask) ==
            PointTileIntegratorConfig::kRampFlagMask) {
            res.is_ramp = true;
            if (env.handle_ramp) {
                res.is_solid = env.handle_ramp(x, y);
            } else {
                res.is_solid = false;
            }
            return res;
        }

        // 0x009918: ori.b #1, ccr (blocked wall)
        res.is_solid = true;
        return res;
    }

    // 0x009904: tst.w d0; bmi.w $99aa
    if (cur_ground_height < 0) {
        res.is_solid = false;
        res.final_ground_height = target_h;
        return res;
    }

    // 0x00990A: sub.w d4, d0 (delta = cur_h - target_h)
    auto delta = static_cast<std::int16_t>(cur_ground_height - target_h);

    // 0x00990C: beq.w $9932
    if (delta == 0) {
        auto raw = read_point_tile_raw(layer, x, y);
        if ((raw & PointTileIntegratorConfig::kRampFlagMask) ==
            PointTileIntegratorConfig::kRampFlagMask) {
            res.is_ramp = true;
            if (env.handle_ramp) {
                res.is_solid = env.handle_ramp(x, y);
            }
        }
        res.final_ground_height = target_h;
        return res;
    }

    // 0x009910: clearance check: air_height + delta >= 0
    auto clearance = static_cast<std::int16_t>(cur_air_height + delta);
    if (clearance < 0) {
        // 0x009918: ori.b #1, ccr (not enough clearance: wall blocked)
        res.is_solid = true;
        return res;
    }

    // 0x009944: sample initial tile attribute nibble
    auto initial_attr = sample_point_tile_attr(layer, x, y);
    auto cur_y = y;
    auto air_h = cur_air_height;

    if (delta > 0) {
        // Descending slope / drop
        while (delta > 0) {
            cur_y = static_cast<std::int16_t>(cur_y + 1);
            if ((cur_y & 7) == 0) {
                auto attr = sample_point_tile_attr(layer, x, cur_y);
                if (attr != initial_attr) {
                    cur_y = static_cast<std::int16_t>(cur_y - 1);
                    break;
                }
            }
            air_h = static_cast<std::int16_t>(air_h + 1);
            delta = static_cast<std::int16_t>(delta - 1);
        }
    } else {
        // Ascending slope / climb
        while (delta < 0) {
            cur_y = static_cast<std::int16_t>(cur_y - 1);
            if ((cur_y & 7) == 7) {
                auto attr = sample_point_tile_attr(layer, x, cur_y);
                if (attr != initial_attr) {
                    cur_y = static_cast<std::int16_t>(cur_y + 1);
                    break;
                }
            }
            air_h = static_cast<std::int16_t>(air_h - 1);
            delta = static_cast<std::int16_t>(delta + 1);
        }
    }

    // 0x00999C: re-query elevation at final (x, cur_y)
    if (env.probe_elevation) {
        target_h = env.probe_elevation(x, cur_y);
    }

    res.is_solid = false;
    res.final_y = cur_y;
    res.final_air_height = air_h;
    res.final_ground_height = target_h;
    return res;
}

PointTileProbeResult step_point_tile_solidity_probe(
    std::span<std::uint8_t> slot,
    std::int16_t x,
    std::int16_t y,
    const PointLayerTilemapDescriptor& layer,
    const PointTileEnvironment& env) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    auto cur_ground_h = read_i16_be(slot, PointTileIntegratorConfig::kGroundHeightOffset);
    auto cur_air_h = read_i16_be(slot, PointTileIntegratorConfig::kAirHeightOffset);

    auto res = evaluate_point_tile_solidity(
        x, y, cur_ground_h, cur_air_h, layer, env);

    if (!res.is_solid) {
        write_i16_be(slot, PointTileIntegratorConfig::kYPosOffset, res.final_y);
        write_i16_be(slot, PointTileIntegratorConfig::kAirHeightOffset, res.final_air_height);
        write_i16_be(slot, PointTileIntegratorConfig::kGroundHeightOffset, res.final_ground_height);
    }

    return res;
}

} // namespace oasis::game::player
