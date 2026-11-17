#include "game/player/horizontal_raycaster.hpp"

namespace oasis::game::player {

namespace {

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
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

inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(buf[offset]) << 8U) |
            static_cast<std::uint16_t>(buf[offset + 1U]));
    }
    return 0U;
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

HorizontalRaycastResult calculate_horizontal_raycast(
    std::int16_t start_x,
    std::uint16_t subpixel_x,
    std::int32_t vel_x_16_16,
    std::int16_t pos_y,
    std::uint16_t radius_x,
    const RaycastEnvironment& env) noexcept {
    HorizontalRaycastResult result{};
    result.final_x = start_x;
    result.final_subpixel = subpixel_x;

    if (vel_x_16_16 == 0) {
        if (env.probe_surface) {
            auto [h, raw_t] = env.probe_surface(start_x, pos_y);
            result.surface_height = h;
            result.terrain_type = lookup_terrain_surface_type(raw_t);
        }
        return result;
    }

    auto cur_pos = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(start_x)) << 16U) |
                   static_cast<std::uint32_t>(subpixel_x);
    auto new_pos = cur_pos + static_cast<std::uint32_t>(vel_x_16_16);
    result.final_subpixel = static_cast<std::uint16_t>(new_pos & 0xFFFFU);
    auto new_int_x = static_cast<std::int16_t>(static_cast<std::uint16_t>(new_pos >> 16U));
    auto displacement = static_cast<std::int16_t>(new_int_x - start_x);

    if (displacement == 0) {
        if (env.probe_surface) {
            auto [h, raw_t] = env.probe_surface(start_x, pos_y);
            result.surface_height = h;
            result.terrain_type = lookup_terrain_surface_type(raw_t);
        }
        return result;
    }

    if (displacement > 0) {
        // 0x009084..0x009128: Positive X
        if (env.clamp_boundary) {
            displacement = env.clamp_boundary(displacement);
        }
        if (env.sweep_solid_entities) {
            displacement = env.sweep_solid_entities(displacement);
        }

        std::int16_t probe_x = static_cast<std::int16_t>(start_x + radius_x);
        std::uint16_t d4 = compute_tile_distance_right(static_cast<std::uint16_t>(probe_x));
        std::int16_t rem_dist = displacement;

        while (true) {
            if (rem_dist <= static_cast<std::int16_t>(d4)) {
                if (rem_dist > 0) {
                    probe_x = static_cast<std::int16_t>(probe_x + rem_dist);
                }
                break;
            }

            rem_dist = static_cast<std::int16_t>(rem_dist - d4);
            probe_x = static_cast<std::int16_t>(probe_x + d4);
            auto tile_col = static_cast<std::int16_t>(probe_x >> 3);

            if (env.is_tile_solid && env.is_tile_solid(static_cast<std::int16_t>(tile_col + 1), pos_y)) {
                result.hit_wall = true;
                break;
            }

            ++result.tiles_stepped;
            std::int16_t center_x = static_cast<std::int16_t>(probe_x - radius_x);
            std::int16_t sample_x = (rem_dist > 8) ? static_cast<std::int16_t>(center_x + 8)
                                                   : static_cast<std::int16_t>(center_x + rem_dist);
            if (env.probe_surface) {
                auto [h, raw_t] = env.probe_surface(sample_x, pos_y);
                result.surface_height = h;
                result.terrain_type = lookup_terrain_surface_type(raw_t);
            }
            d4 = 8U;
        }

        result.final_x = static_cast<std::int16_t>(probe_x - radius_x);
    } else {
        // 0x009128..0x0091C6: Negative X
        auto abs_disp = static_cast<std::int16_t>(-displacement);
        if (env.clamp_boundary) {
            abs_disp = env.clamp_boundary(abs_disp);
        }
        if (env.sweep_solid_entities) {
            abs_disp = env.sweep_solid_entities(abs_disp);
        }

        std::int16_t probe_x = static_cast<std::int16_t>(start_x - radius_x);
        std::uint16_t d4 = compute_tile_distance_left(static_cast<std::uint16_t>(probe_x));
        std::int16_t rem_dist = abs_disp;

        while (true) {
            if (rem_dist <= static_cast<std::int16_t>(d4)) {
                if (rem_dist > 0) {
                    probe_x = static_cast<std::int16_t>(probe_x - rem_dist);
                }
                break;
            }

            rem_dist = static_cast<std::int16_t>(rem_dist - d4);
            probe_x = static_cast<std::int16_t>(probe_x - d4);
            auto tile_col = static_cast<std::int16_t>(probe_x >> 3);

            if (env.is_tile_solid && env.is_tile_solid(static_cast<std::int16_t>(tile_col - 1), pos_y)) {
                result.hit_wall = true;
                break;
            }

            ++result.tiles_stepped;
            std::int16_t center_x = static_cast<std::int16_t>(probe_x + radius_x);
            std::int16_t sample_x = (rem_dist > 8) ? static_cast<std::int16_t>(center_x - 8)
                                                   : static_cast<std::int16_t>(center_x - rem_dist);
            if (env.probe_surface) {
                auto [h, raw_t] = env.probe_surface(sample_x, pos_y);
                result.surface_height = h;
                result.terrain_type = lookup_terrain_surface_type(raw_t);
            }
            d4 = 8U;
        }

        result.final_x = static_cast<std::int16_t>(probe_x + radius_x);
    }

    // 0x0091CA..0x0091F2: If no tile was stepped, probe resting position
    if (result.tiles_stepped == 0U && env.probe_surface) {
        auto [h, raw_t] = env.probe_surface(result.final_x, pos_y);
        result.surface_height = h;
        result.terrain_type = lookup_terrain_surface_type(raw_t);
    }

    return result;
}

HorizontalRaycastResult step_horizontal_raycast(
    std::span<std::uint8_t> slot,
    const RaycastEnvironment& env) noexcept {
    if (slot.size() < 0x76U) {
        return {};
    }

    auto start_x = static_cast<std::int16_t>(read_u16_be(slot, HorizontalRaycasterConfig::kPosXOffset));
    std::uint16_t subpixel_x = read_u16_be(slot, HorizontalRaycasterConfig::kSubpixelXOffset);
    auto pos_y = static_cast<std::int16_t>(read_u16_be(slot, HorizontalRaycasterConfig::kPosYOffset));
    std::int32_t vel_x = read_i32_be(slot, HorizontalRaycasterConfig::kVelXOffset);
    std::uint16_t radius_x = read_u16_be(slot, HorizontalRaycasterConfig::kRadiusXOffset);

    if (vel_x > 0) {
        slot[HorizontalRaycasterConfig::kDirCodeOffset] = HorizontalRaycasterConfig::kDirCodePosX;
    } else if (vel_x < 0) {
        slot[HorizontalRaycasterConfig::kDirCodeOffset] = HorizontalRaycasterConfig::kDirCodeNegX;
    }

    auto res = calculate_horizontal_raycast(start_x, subpixel_x, vel_x, pos_y, radius_x, env);

    write_u16_be(slot, HorizontalRaycasterConfig::kPosXOffset, static_cast<std::uint16_t>(res.final_x));
    write_u16_be(slot, HorizontalRaycasterConfig::kSubpixelXOffset, res.final_subpixel);
    write_u16_be(slot, HorizontalRaycasterConfig::kSurfaceHeightOffset, res.surface_height);
    slot[HorizontalRaycasterConfig::kTerrainTypeOffset] = res.terrain_type;

    // 0x0091F6: clr.l $72(a6)
    write_i32_be(slot, HorizontalRaycasterConfig::kVelXOffset, 0);

    return res;
}

} // namespace oasis::game::player
