#include "game/player/vertical_raycaster.hpp"

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

VerticalRaycastResult calculate_vertical_raycast(
    std::int16_t pos_x,
    std::int16_t start_y,
    std::uint16_t subpixel_y,
    std::int32_t vel_y_16_16,
    std::uint16_t radius,
    const VerticalRaycastEnvironment& env) noexcept {
    VerticalRaycastResult result{};
    result.final_y = start_y;
    result.final_subpixel_y = subpixel_y;

    if (vel_y_16_16 == 0) {
        return result;
    }

    auto cur_pos = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(start_y)) << 16U) |
                   static_cast<std::uint32_t>(subpixel_y);
    auto new_pos = cur_pos + static_cast<std::uint32_t>(vel_y_16_16);
    result.final_subpixel_y = static_cast<std::uint16_t>(new_pos & 0xFFFFU);
    auto new_int_y = static_cast<std::int16_t>(static_cast<std::uint16_t>(new_pos >> 16U));
    auto displacement = static_cast<std::int16_t>(new_int_y - start_y);

    if (displacement == 0) {
        return result;
    }

    bool cleared_stepped = false;

    if (displacement > 0) {
        // 0x00920C..0x0092B0: Positive Y (Down)
        if (env.clamp_boundary) {
            displacement = env.clamp_boundary(displacement);
        }
        if (env.sweep_solid_entities) {
            displacement = env.sweep_solid_entities(displacement);
        }

        std::int16_t probe_y = static_cast<std::int16_t>(start_y + radius);
        std::uint16_t d4 = compute_tile_row_distance_down(static_cast<std::uint16_t>(probe_y));
        std::int16_t rem_dist = displacement;

        while (true) {
            if (rem_dist <= static_cast<std::int16_t>(d4)) {
                if (rem_dist > 0) {
                    probe_y = static_cast<std::int16_t>(probe_y + rem_dist);
                    cleared_stepped = true;
                }
                break;
            }

            rem_dist = static_cast<std::int16_t>(rem_dist - d4);
            probe_y = static_cast<std::int16_t>(probe_y + d4);
            auto tile_row = static_cast<std::int16_t>(probe_y >> 3);

            if (env.is_tile_solid && env.is_tile_solid(pos_x, static_cast<std::int16_t>(tile_row + 1))) {
                result.hit_wall = true;
                break;
            }

            ++result.tiles_stepped;
            std::int16_t center_y = static_cast<std::int16_t>(probe_y - radius);
            std::int16_t sample_y = (rem_dist > 8) ? static_cast<std::int16_t>(center_y + 8)
                                                   : static_cast<std::int16_t>(center_y + rem_dist);
            if (env.probe_surface) {
                auto [h, raw_t] = env.probe_surface(pos_x, sample_y);
                result.surface_height = h;
                result.terrain_type = lookup_terrain_surface_type(raw_t);
            }
            d4 = 8U;
        }

        result.final_y = static_cast<std::int16_t>(probe_y - radius);
    } else {
        // 0x0092B0..0x00934E: Negative Y (Up)
        auto abs_disp = static_cast<std::int16_t>(-displacement);
        if (env.clamp_boundary) {
            abs_disp = env.clamp_boundary(abs_disp);
        }
        if (env.sweep_solid_entities) {
            abs_disp = env.sweep_solid_entities(abs_disp);
        }

        std::int16_t probe_y = static_cast<std::int16_t>(start_y - radius);
        std::uint16_t d4 = compute_tile_row_distance_up(static_cast<std::uint16_t>(probe_y));
        std::int16_t rem_dist = abs_disp;

        while (true) {
            if (rem_dist <= static_cast<std::int16_t>(d4)) {
                if (rem_dist > 0) {
                    probe_y = static_cast<std::int16_t>(probe_y - rem_dist);
                    cleared_stepped = true;
                }
                break;
            }

            rem_dist = static_cast<std::int16_t>(rem_dist - d4);
            probe_y = static_cast<std::int16_t>(probe_y - d4);
            auto tile_row = static_cast<std::int16_t>(probe_y >> 3);

            if (env.is_tile_solid && env.is_tile_solid(pos_x, static_cast<std::int16_t>(tile_row - 1))) {
                result.hit_wall = true;
                break;
            }

            ++result.tiles_stepped;
            std::int16_t center_y = static_cast<std::int16_t>(probe_y + radius);
            std::int16_t sample_y = (rem_dist > 8) ? static_cast<std::int16_t>(center_y - 8)
                                                   : static_cast<std::int16_t>(center_y - rem_dist);
            if (env.probe_surface) {
                auto [h, raw_t] = env.probe_surface(pos_x, sample_y);
                result.surface_height = h;
                result.terrain_type = lookup_terrain_surface_type(raw_t);
            }
            d4 = 8U;
        }

        result.final_y = static_cast<std::int16_t>(probe_y + radius);
    }

    // 0x009352..0x00937A: If no tiles stepped or partial final step cleared stepped count, probe resting pos
    if ((result.tiles_stepped == 0U || cleared_stepped) && env.probe_surface) {
        auto [h, raw_t] = env.probe_surface(pos_x, result.final_y);
        result.surface_height = h;
        result.terrain_type = lookup_terrain_surface_type(raw_t);
    }

    return result;
}

VerticalRaycastResult step_vertical_raycast(
    std::span<std::uint8_t> slot,
    std::uint32_t return_vector_ram,
    const VerticalRaycastEnvironment& env) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    // 0x0091F6: clr.l $72(a6)
    write_i32_be(slot, VerticalRaycasterConfig::kVelXOffset, 0);

    auto pos_x = static_cast<std::int16_t>(read_u16_be(slot, VerticalRaycasterConfig::kPosXOffset));
    auto start_y = static_cast<std::int16_t>(read_u16_be(slot, VerticalRaycasterConfig::kPosYOffset));
    std::uint16_t subpixel_y = read_u16_be(slot, VerticalRaycasterConfig::kSubpixelYOffset);
    std::int32_t vel_y = read_i32_be(slot, VerticalRaycasterConfig::kVelYOffset);
    std::uint16_t radius = read_u16_be(slot, VerticalRaycasterConfig::kRadiusOffset);

    if (vel_y > 0) {
        slot[VerticalRaycasterConfig::kDirCodeYOffset] = VerticalRaycasterConfig::kDirCodePosY;
    } else if (vel_y < 0) {
        slot[VerticalRaycasterConfig::kDirCodeYOffset] = VerticalRaycasterConfig::kDirCodeNegY;
    }

    auto res = calculate_vertical_raycast(pos_x, start_y, subpixel_y, vel_y, radius, env);

    write_u16_be(slot, VerticalRaycasterConfig::kPosYOffset, static_cast<std::uint16_t>(res.final_y));
    write_u16_be(slot, VerticalRaycasterConfig::kSubpixelYOffset, res.final_subpixel_y);

    if (res.surface_height != 0U || res.terrain_type != 0xFFU) {
        write_u16_be(slot, VerticalRaycasterConfig::kSurfaceHeightOffset, res.surface_height);
        slot[VerticalRaycasterConfig::kTerrainTypeOffset] = res.terrain_type;
    }

    // 0x00937E: clr.l $76(a6)
    write_i32_be(slot, VerticalRaycasterConfig::kVelYOffset, 0);

    // 0x009382..0x00938E: entity return dispatcher
    res.return_vector = dispatch_entity_return(return_vector_ram);

    return res;
}

} // namespace oasis::game::player
