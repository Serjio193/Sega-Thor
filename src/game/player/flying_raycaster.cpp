#include "game/player/flying_raycaster.hpp"

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

inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(buf[offset]) << 8U) |
            static_cast<std::uint16_t>(buf[offset + 1U]));
    }
    return 0U;
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

inline void write_i16_be(std::span<std::uint8_t> buf, std::size_t offset, std::int16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        auto u = static_cast<std::uint16_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

} // namespace

FlyingRaycastResult calculate_flying_horizontal_raycast(
    std::int16_t start_x,
    std::uint16_t subpixel_x,
    std::int32_t vel_x_16_16,
    std::int16_t cur_y,
    const FlyingRaycastEnvironment& env) noexcept {
    FlyingRaycastResult result{};
    result.final_x = start_x;
    result.final_subpixel_x = subpixel_x;

    if (vel_x_16_16 == 0) {
        return result;
    }

    auto cur_pos = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(start_x)) << 16U) |
                   static_cast<std::uint32_t>(subpixel_x);
    auto new_pos = cur_pos + static_cast<std::uint32_t>(vel_x_16_16);
    result.final_subpixel_x = static_cast<std::uint16_t>(new_pos & 0xFFFFU);
    auto new_int_x = static_cast<std::int16_t>(static_cast<std::uint16_t>(new_pos >> 16U));
    auto displacement = static_cast<std::int16_t>(new_int_x - start_x);

    if (displacement == 0) {
        return result;
    }

    result.step_count_a3 += 1U;

    if (displacement > 0) {
        // 0x00971A: +X Direction code 0x11
        result.dir_code_x = FlyingRaycasterConfig::kDirPosX;
        if (env.clamp_boundary_x) {
            displacement = env.clamp_boundary_x(displacement);
        }
        if (env.sweep_entities_x) {
            displacement = env.sweep_entities_x(displacement);
        }

        // 0x00973E: d4 = -( (start_x & 7) - 7 )
        auto d4 = static_cast<std::int16_t>(7 - (start_x & 7));
        auto rem = displacement;
        auto cur_x = start_x;

        while (rem > d4) {
            rem = static_cast<std::int16_t>(rem - d4);
            cur_x = static_cast<std::int16_t>(cur_x + d4);
            auto probe_x = static_cast<std::int16_t>(cur_x + 1);
            if (env.point_probe && env.point_probe(probe_x, cur_y)) {
                result.hit_wall_x = true;
                break;
            }
            d4 = 8;
        }

        if (!result.hit_wall_x && rem > 0) {
            cur_x = static_cast<std::int16_t>(cur_x + rem);
        }
        result.final_x = cur_x;
    } else {
        // 0x009780: -X Direction code 0x13
        result.dir_code_x = FlyingRaycasterConfig::kDirNegX;
        auto abs_disp = static_cast<std::int16_t>(-displacement);
        if (env.clamp_boundary_x) {
            abs_disp = env.clamp_boundary_x(abs_disp);
        }
        if (env.sweep_entities_x) {
            abs_disp = env.sweep_entities_x(abs_disp);
        }

        // 0x0097A6: d4 = start_x & 7
        auto d4 = static_cast<std::int16_t>(start_x & 7);
        auto rem = abs_disp;
        auto cur_x = start_x;

        while (rem > d4) {
            rem = static_cast<std::int16_t>(rem - d4);
            cur_x = static_cast<std::int16_t>(cur_x - d4);
            auto probe_x = static_cast<std::int16_t>(cur_x - 1);
            if (env.point_probe && env.point_probe(probe_x, cur_y)) {
                result.hit_wall_x = true;
                break;
            }
            d4 = 8;
        }

        if (!result.hit_wall_x && rem > 0) {
            cur_x = static_cast<std::int16_t>(cur_x - rem);
        }
        result.final_x = cur_x;
    }

    return result;
}

FlyingRaycastResult calculate_flying_vertical_raycast(
    std::int16_t cur_x,
    std::int16_t start_y,
    std::uint16_t subpixel_y,
    std::int32_t vel_y_16_16,
    const FlyingRaycastEnvironment& env) noexcept {
    FlyingRaycastResult result{};
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

    result.step_count_a3 += 1U;

    if (displacement > 0) {
        // 0x0097F6: +Y Direction code 0x12
        result.dir_code_y = FlyingRaycasterConfig::kDirPosY;
        if (env.clamp_boundary_y) {
            displacement = env.clamp_boundary_y(displacement);
        }
        if (env.sweep_entities_y) {
            displacement = env.sweep_entities_y(displacement);
        }

        // 0x00981A: d4 = -( (start_y & 7) - 7 )
        auto d4 = static_cast<std::int16_t>(7 - (start_y & 7));
        auto rem = displacement;
        auto cur_y = start_y;

        while (rem > d4) {
            rem = static_cast<std::int16_t>(rem - d4);
            cur_y = static_cast<std::int16_t>(cur_y + d4);
            auto probe_y = static_cast<std::int16_t>(cur_y + 1);
            if (env.point_probe && env.point_probe(cur_x, probe_y)) {
                result.hit_wall_y = true;
                break;
            }
            d4 = 8;
        }

        if (!result.hit_wall_y && rem > 0) {
            cur_y = static_cast<std::int16_t>(cur_y + rem);
        }
        result.final_y = cur_y;
    } else {
        // 0x00985C: -Y Direction code 0x10
        result.dir_code_y = FlyingRaycasterConfig::kDirNegY;
        auto abs_disp = static_cast<std::int16_t>(-displacement);
        if (env.clamp_boundary_y) {
            abs_disp = env.clamp_boundary_y(abs_disp);
        }
        if (env.sweep_entities_y) {
            abs_disp = env.sweep_entities_y(abs_disp);
        }

        // 0x009882: d4 = start_y & 7
        auto d4 = static_cast<std::int16_t>(start_y & 7);
        auto rem = abs_disp;
        auto cur_y = start_y;

        while (rem > d4) {
            rem = static_cast<std::int16_t>(rem - d4);
            cur_y = static_cast<std::int16_t>(cur_y - d4);
            auto probe_y = static_cast<std::int16_t>(cur_y - 1);
            if (env.point_probe && env.point_probe(cur_x, probe_y)) {
                result.hit_wall_y = true;
                break;
            }
            d4 = 8;
        }

        if (!result.hit_wall_y && rem > 0) {
            cur_y = static_cast<std::int16_t>(cur_y - rem);
        }
        result.final_y = cur_y;
    }

    return result;
}

FlyingRaycastResult calculate_flying_dual_axis_raycast(
    std::int16_t start_x,
    std::uint16_t subpixel_x,
    std::int32_t vel_x_16_16,
    std::int16_t start_y,
    std::uint16_t subpixel_y,
    std::int32_t vel_y_16_16,
    std::int16_t radius_46,
    const FlyingRaycastEnvironment& env) noexcept {
    auto res_x = calculate_flying_horizontal_raycast(
        start_x, subpixel_x, vel_x_16_16, start_y, env);

    auto res_y = calculate_flying_vertical_raycast(
        res_x.final_x, start_y, subpixel_y, vel_y_16_16, env);

    FlyingRaycastResult combined{};
    combined.final_x = res_x.final_x;
    combined.final_subpixel_x = res_x.final_subpixel_x;
    combined.final_y = res_y.final_y;
    combined.final_subpixel_y = res_y.final_subpixel_y;
    combined.hit_wall_x = res_x.hit_wall_x;
    combined.hit_wall_y = res_y.hit_wall_y;
    combined.dir_code_x = res_x.dir_code_x;
    combined.dir_code_y = res_y.dir_code_y;
    combined.step_count_a3 = res_x.step_count_a3 + res_y.step_count_a3;

    // 0x0098C2: if a3 != 0, sample surface height
    if (combined.step_count_a3 != 0U && env.probe_surface_height) {
        combined.ground_surface_height = env.probe_surface_height(
            combined.final_x, combined.final_y, radius_46);
    }

    return combined;
}

FlyingRaycastResult step_flying_entity_simulation(
    std::span<std::uint8_t> slot,
    const FlyingRaycastEnvironment& env) noexcept {
    if (slot.size() < 0x7AU) {
        return {};
    }

    auto start_x = read_i16_be(slot, FlyingRaycasterConfig::kPosXOffset);
    auto subpixel_x = read_u16_be(slot, FlyingRaycasterConfig::kSubpixelXOffset);
    auto vel_x = read_i32_be(slot, FlyingRaycasterConfig::kVelXOffset);

    auto start_y = read_i16_be(slot, FlyingRaycasterConfig::kPosYOffset);
    auto subpixel_y = read_u16_be(slot, FlyingRaycasterConfig::kSubpixelYOffset);
    auto vel_y = read_i32_be(slot, FlyingRaycasterConfig::kVelYOffset);

    auto radius_46 = read_i16_be(slot, FlyingRaycasterConfig::kRadiusOffset);

    auto res = calculate_flying_dual_axis_raycast(
        start_x, subpixel_x, vel_x, start_y, subpixel_y, vel_y, radius_46, env);

    // Apply horizontal results
    if (vel_x != 0) {
        slot[FlyingRaycasterConfig::kCollisionFlags40Offset] = res.dir_code_x;
        if (res.hit_wall_x) {
            slot[FlyingRaycasterConfig::kCollisionFlags40Offset] |= FlyingRaycasterConfig::kCollisionWallMask;
        }
        write_i16_be(slot, FlyingRaycasterConfig::kPosXOffset, res.final_x);
        write_u16_be(slot, FlyingRaycasterConfig::kSubpixelXOffset, res.final_subpixel_x);
        write_i32_be(slot, FlyingRaycasterConfig::kVelXOffset, 0);
    }

    // Apply vertical results
    if (vel_y != 0) {
        slot[FlyingRaycasterConfig::kCollisionFlags41Offset] = res.dir_code_y;
        if (res.hit_wall_y) {
            slot[FlyingRaycasterConfig::kCollisionFlags41Offset] |= FlyingRaycasterConfig::kCollisionWallMask;
        }
        write_i16_be(slot, FlyingRaycasterConfig::kPosYOffset, res.final_y);
        write_u16_be(slot, FlyingRaycasterConfig::kSubpixelYOffset, res.final_subpixel_y);
        write_i32_be(slot, FlyingRaycasterConfig::kVelYOffset, 0);
    }

    // Apply surface height if step count != 0
    if (res.step_count_a3 != 0U) {
        write_i16_be(slot, FlyingRaycasterConfig::kSurfaceHeightOffset, res.ground_surface_height);
    }

    return res;
}

} // namespace oasis::game::player
