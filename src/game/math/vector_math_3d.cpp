#include "vector_math_3d.hpp"

#include <algorithm>
#include <cstdint>

namespace thor::math {
namespace {

[[nodiscard]] std::int16_t wrap_sub_i16(std::int16_t lhs, std::int16_t rhs) noexcept {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(lhs) - static_cast<std::uint16_t>(rhs));
}

[[nodiscard]] std::int16_t wrap_add_i16(std::int16_t lhs, std::int16_t rhs) noexcept {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(lhs) + static_cast<std::uint16_t>(rhs));
}

[[nodiscard]] std::int32_t wrap_sub_i32(std::int32_t lhs, std::int32_t rhs) noexcept {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(lhs) - static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] std::uint32_t abs_u32(std::int32_t value) noexcept {
    if (value < 0) {
        return 0U - static_cast<std::uint32_t>(value);
    }
    return static_cast<std::uint32_t>(value);
}

[[nodiscard]] std::uint32_t square_i16_u32(std::int16_t delta) noexcept {
    const std::int32_t v = static_cast<std::int32_t>(delta);
    return static_cast<std::uint32_t>(v * v);
}

/// Exact Motorola 68000 `divs.w` semantics: 32-bit signed / 16-bit signed -> 16-bit quotient.
/// Guard: returns 0 if divisor == 0, and preserves low 16 bits of dividend if quotient overflows s16 (V=1).
[[nodiscard]] std::int16_t m68k_divs_w_quotient(std::int32_t dividend, std::int16_t divisor) noexcept {
    if (divisor == 0) {
        return 0;
    }
    const std::int64_t q = static_cast<std::int64_t>(dividend) / static_cast<std::int64_t>(divisor);
    if (q < -32768 || q > 32767) {
        return static_cast<std::int16_t>(static_cast<std::uint32_t>(dividend) & 0xFFFFU);
    }
    return static_cast<std::int16_t>(q);
}

[[nodiscard]] std::int32_t scale_quotient_shift12(std::int16_t quotient) noexcept {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int32_t>(quotient)) << kVelocityNormalizeShift);
}

/// Local equivalent of 0x00E338 (`ResolveCardinalDirection4Way`: 0=N, 1=E, 2=S, 3=W).
[[nodiscard]] std::uint16_t resolve_cardinal_direction_4way_00e338(std::int32_t dx, std::int32_t dy) noexcept {
    const std::uint32_t abs_dx = abs_u32(dx);
    const std::uint32_t abs_dy = abs_u32(dy);
    if (abs_dx > abs_dy) {
        return (dx < 0) ? 3U : 1U;
    }
    return (dy < 0) ? 0U : 2U;
}

/// Local equivalent of 0x00E2D4 (`ProjectVelocity8Way`) restricted to cardinal dirs 0..3 from 0x00E338.
void project_cardinal_velocity_00e2d4(
    std::uint16_t speed_8_8,
    std::uint16_t dir_4way,
    std::int32_t& out_vx,
    std::int32_t& out_vy) noexcept {
    const std::int32_t speed_fp = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_8_8) << 8U);
    switch (dir_4way & 3U) {
        case 0U: // North
            out_vx = 0;
            out_vy = wrap_sub_i32(0, speed_fp);
            break;
        case 1U: // East
            out_vx = speed_fp;
            out_vy = 0;
            break;
        case 2U: // South
            out_vx = 0;
            out_vy = speed_fp;
            break;
        default: // West (3)
            out_vx = wrap_sub_i32(0, speed_fp);
            out_vy = 0;
            break;
    }
}

[[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::int16_t read_be_s16(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(data, offset));
}

[[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

[[nodiscard]] std::int32_t read_be_s32(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return static_cast<std::int32_t>(read_be32(data, offset));
}

void write_be16(std::span<std::uint8_t> data, std::size_t offset, std::uint16_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(std::span<std::uint8_t> data, std::size_t offset, std::uint32_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] bool has_entity_pos_bounds(std::span<const std::uint8_t> wram, std::size_t offset) noexcept {
    return offset <= wram.size() && (wram.size() - offset) >= (kEntityPosZFpOffset + 2U);
}

[[nodiscard]] bool has_spawner_bounds(std::span<const std::uint8_t> wram, std::size_t offset) noexcept {
    return offset <= wram.size() && (wram.size() - offset) >= (kSpawnerProjectileParamOffset + 4U);
}

[[nodiscard]] bool has_projectile_bounds(std::span<const std::uint8_t> wram, std::size_t offset) noexcept {
    return offset <= wram.size() && (wram.size() - offset) >= (kEntityFacingDir4WayOffset + 2U);
}

} // namespace

std::uint16_t distance_2d(
    std::int16_t x0,
    std::int16_t y0,
    std::int16_t x1,
    std::int16_t y1) noexcept {
    const std::int16_t dx = wrap_sub_i16(x1, x0);
    const std::int16_t dy = wrap_sub_i16(y1, y0);
    const std::uint32_t sum_sq = square_i16_u32(dx) + square_i16_u32(dy);
    return oasis::game::math::isqrt_u32(sum_sq);
}

std::uint16_t distance_3d(Vec3i16 p0, Vec3i16 p1) noexcept {
    const std::int16_t dx = wrap_sub_i16(p1.x, p0.x);
    const std::int16_t dy = wrap_sub_i16(p1.y, p0.y);
    const std::int16_t dz = wrap_sub_i16(p1.z, p0.z);
    const std::uint32_t sum_sq = square_i16_u32(dx) + square_i16_u32(dy) + square_i16_u32(dz);
    return oasis::game::math::isqrt_u32(sum_sq);
}

Vec3Fixed normalize_velocity_3d(
    Vec3i16 p0,
    Vec3i16 p1,
    std::int16_t speed) noexcept {
    const std::int16_t dx = wrap_sub_i16(p1.x, p0.x);
    const std::int16_t dy = wrap_sub_i16(p1.y, p0.y);
    const std::int16_t dz = wrap_sub_i16(p1.z, p0.z);
    const std::uint32_t sum_sq = square_i16_u32(dx) + square_i16_u32(dy) + square_i16_u32(dz);
    const std::uint16_t dist = oasis::game::math::isqrt_u32(sum_sq);
    if (dist == 0U) {
        return {};
    }

    const std::int16_t signed_dist = static_cast<std::int16_t>(dist);
    const std::int32_t s = static_cast<std::int32_t>(speed);
    const std::int16_t qx = m68k_divs_w_quotient(static_cast<std::int32_t>(dx) * s, signed_dist);
    const std::int16_t qy = m68k_divs_w_quotient(static_cast<std::int32_t>(dy) * s, signed_dist);
    const std::int16_t qz = m68k_divs_w_quotient(static_cast<std::int32_t>(dz) * s, signed_dist);
    return {
        scale_quotient_shift12(qx),
        scale_quotient_shift12(qy),
        scale_quotient_shift12(qz),
    };
}

SphereClampResult clamp_to_sphere_3d(
    Vec3i16 origin,
    Vec3i16 target,
    std::uint16_t radius) noexcept {
    const std::int16_t dx = wrap_sub_i16(target.x, origin.x);
    const std::int16_t dy = wrap_sub_i16(target.y, origin.y);
    const std::int16_t dz = wrap_sub_i16(target.z, origin.z);
    const std::uint32_t sum_sq = square_i16_u32(dx) + square_i16_u32(dy) + square_i16_u32(dz);
    const std::uint16_t dist = oasis::game::math::isqrt_u32(sum_sq);

    // 0x00F17A..0x00F17C: cmp.w d4, d0; bls.w $f1a8 (unsigned dist <= radius exits with C=0).
    if (dist <= radius) {
        return {target, dist, false};
    }

    const std::int32_t signed_radius = static_cast<std::int32_t>(static_cast<std::int16_t>(radius));
    const std::int16_t signed_dist = static_cast<std::int16_t>(dist);
    const std::int16_t qx = m68k_divs_w_quotient(static_cast<std::int32_t>(dx) * signed_radius, signed_dist);
    const std::int16_t qy = m68k_divs_w_quotient(static_cast<std::int32_t>(dy) * signed_radius, signed_dist);
    const std::int16_t qz = m68k_divs_w_quotient(static_cast<std::int32_t>(dz) * signed_radius, signed_dist);

    return {
        {
            wrap_add_i16(origin.x, qx),
            wrap_add_i16(origin.y, qy),
            wrap_add_i16(origin.z, qz),
        },
        dist,
        true,
    };
}

Boss3ARockVelocityEntry select_boss3a_rock_velocity_entry(std::uint16_t index) noexcept {
    const std::size_t clamped_index = std::min<std::size_t>(
        static_cast<std::size_t>(index),
        kBoss3ARockVelocityTable.size() - 1U);
    return kBoss3ARockVelocityTable[clamped_index];
}

Boss3ALaunchResult setup_boss3a_rock_launch(
    const Boss3ASpawnerState& spawner,
    Boss3AProjectileState& projectile) noexcept {
    Boss3ALaunchResult result{};

    // 0x00F054 -> bsr.w $be98: latch enemy HUD tracker if not player slot ($FF19E8) and bit 0 clear.
    result.hud_tracker_updated =
        (projectile.target_ram_addr != kPlayerEntityRamAddr) &&
        ((projectile.flags_3a & 0x01U) == 0U);

    // 0x00F05C: move.l $aa(a6), $7e(a0)
    projectile.param_7e = spawner.param_aa;

    // 0x00F062..0x00F076: 32-bit delta + 4-way direction via 0x00E338 -> $84(a0).
    const std::int32_t dx = wrap_sub_i32(projectile.x_fp, spawner.x_fp);
    const std::int32_t dy = wrap_sub_i32(projectile.y_fp, spawner.y_fp);
    const std::uint16_t dir = resolve_cardinal_direction_4way_00e338(dx, dy);
    projectile.facing_dir_4way = dir;
    result.facing_dir_4way = dir;

    // 0x00F07A..0x00F09A: unsigned max(|dx|, |dy|); override table index to 3 when <= 0x80000 (8 px).
    const std::uint32_t max_axis = std::max(abs_u32(dx), abs_u32(dy));
    result.max_axis_abs_fp = max_axis;
    std::uint16_t requested_index = spawner.attack_phase_index;
    if (max_axis <= kBoss3ACloseRangeThresholdFp) {
        requested_index = kBoss3ACloseRangeTableIndex;
        result.close_range_override = true;
    }
    result.requested_table_index = requested_index;

    const std::size_t table_limit = kBoss3ARockVelocityTable.size() - 1U;
    if (static_cast<std::size_t>(requested_index) > table_limit) {
        result.effective_table_index = static_cast<std::uint16_t>(table_limit);
        result.table_index_clamped = true;
    } else {
        result.effective_table_index = requested_index;
    }

    // 0x00F09C..0x00F0AA: lookup speed_xy / launch_z; ext.l + asl.l #8 for vel_z -> $56(a0).
    const Boss3ARockVelocityEntry entry = select_boss3a_rock_velocity_entry(result.effective_table_index);
    const std::int32_t vel_z = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int32_t>(entry.launch_z_8_8)) << 8U);

    // 0x00F0AE..0x00F0B8: pass speed_xy in both d1/d2 + facing_dir in d3 to 0x00E2D4 -> $4E(a0), $52(a0).
    std::int32_t vel_x = 0;
    std::int32_t vel_y = 0;
    project_cardinal_velocity_00e2d4(entry.speed_xy_8_8, dir, vel_x, vel_y);

    projectile.vel_x_fp = vel_x;
    projectile.vel_y_fp = vel_y;
    projectile.vel_z_fp = vel_z;

    result.vel_x_fp = vel_x;
    result.vel_y_fp = vel_y;
    result.vel_z_fp = vel_z;
    return result;
}

Vec3i16 read_entity_pos_i16_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t entity_offset) noexcept {
    if (!has_entity_pos_bounds(wram, entity_offset)) {
        return {};
    }
    return {
        read_be_s16(wram, entity_offset + kEntityPosXFpOffset),
        read_be_s16(wram, entity_offset + kEntityPosYFpOffset),
        read_be_s16(wram, entity_offset + kEntityPosZFpOffset),
    };
}

void write_entity_pos_i16_to_ram(
    std::span<std::uint8_t> wram,
    std::size_t entity_offset,
    Vec3i16 pos) noexcept {
    if (!has_entity_pos_bounds(wram, entity_offset)) {
        return;
    }
    write_be16(wram, entity_offset + kEntityPosXFpOffset, static_cast<std::uint16_t>(pos.x));
    write_be16(wram, entity_offset + kEntityPosYFpOffset, static_cast<std::uint16_t>(pos.y));
    write_be16(wram, entity_offset + kEntityPosZFpOffset, static_cast<std::uint16_t>(pos.z));
}

std::uint16_t distance_2d_entities_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t entity0_offset,
    std::size_t entity1_offset) noexcept {
    if (!has_entity_pos_bounds(wram, entity0_offset) ||
        !has_entity_pos_bounds(wram, entity1_offset)) {
        return 0U;
    }
    const Vec3i16 p0 = read_entity_pos_i16_from_ram(wram, entity0_offset);
    const Vec3i16 p1 = read_entity_pos_i16_from_ram(wram, entity1_offset);
    return distance_2d(p0.x, p0.y, p1.x, p1.y);
}

std::uint16_t distance_3d_entities_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t entity0_offset,
    std::size_t entity1_offset) noexcept {
    if (!has_entity_pos_bounds(wram, entity0_offset) ||
        !has_entity_pos_bounds(wram, entity1_offset)) {
        return 0U;
    }
    const Vec3i16 p0 = read_entity_pos_i16_from_ram(wram, entity0_offset);
    const Vec3i16 p1 = read_entity_pos_i16_from_ram(wram, entity1_offset);
    return distance_3d(p0, p1);
}

Vec3Fixed normalize_entity_velocity_3d_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t origin_entity_offset,
    std::size_t target_entity_offset,
    std::int16_t speed) noexcept {
    if (!has_entity_pos_bounds(wram, origin_entity_offset) ||
        !has_entity_pos_bounds(wram, target_entity_offset)) {
        return {};
    }
    const Vec3i16 p0 = read_entity_pos_i16_from_ram(wram, origin_entity_offset);
    const Vec3i16 p1 = read_entity_pos_i16_from_ram(wram, target_entity_offset);
    return normalize_velocity_3d(p0, p1, speed);
}

SphereClampResult clamp_entity_pos_to_sphere_from_ram(
    std::span<std::uint8_t> wram,
    std::size_t origin_entity_offset,
    std::size_t target_entity_offset,
    std::uint16_t radius) noexcept {
    if (!has_entity_pos_bounds(wram, origin_entity_offset) ||
        !has_entity_pos_bounds(wram, target_entity_offset)) {
        return {};
    }
    const Vec3i16 origin = read_entity_pos_i16_from_ram(wram, origin_entity_offset);
    const Vec3i16 target = read_entity_pos_i16_from_ram(wram, target_entity_offset);
    const SphereClampResult result = clamp_to_sphere_3d(origin, target, radius);
    if (result.clamped) {
        write_entity_pos_i16_to_ram(wram, target_entity_offset, result.position);
    }
    return result;
}

Boss3ALaunchResult setup_boss3a_rock_launch_from_ram(
    std::span<std::uint8_t> wram,
    std::size_t spawner_offset,
    std::size_t projectile_offset) noexcept {
    if (!has_spawner_bounds(wram, spawner_offset) ||
        !has_projectile_bounds(wram, projectile_offset)) {
        return {};
    }

    const Boss3ASpawnerState spawner{
        read_be_s32(wram, spawner_offset + kEntityPosXFpOffset),
        read_be_s32(wram, spawner_offset + kEntityPosYFpOffset),
        read_be16(wram, spawner_offset + kSpawnerAttackPhaseOffset),
        read_be32(wram, spawner_offset + kSpawnerProjectileParamOffset),
    };

    const std::uint32_t projectile_ram_addr =
        kWramBaseAddr + static_cast<std::uint32_t>(projectile_offset & 0xFFFFU);

    Boss3AProjectileState projectile{
        read_be_s32(wram, projectile_offset + kEntityPosXFpOffset),
        read_be_s32(wram, projectile_offset + kEntityPosYFpOffset),
        wram[projectile_offset + kEntityHudFlagsOffset],
        projectile_ram_addr,
        read_be_s32(wram, projectile_offset + kEntityVelXFpOffset),
        read_be_s32(wram, projectile_offset + kEntityVelYFpOffset),
        read_be_s32(wram, projectile_offset + kEntityVelZFpOffset),
        read_be32(wram, projectile_offset + kEntityInheritedParamOffset),
        read_be16(wram, projectile_offset + kEntityFacingDir4WayOffset),
    };

    const Boss3ALaunchResult result = setup_boss3a_rock_launch(spawner, projectile);

    write_be32(wram, projectile_offset + kEntityInheritedParamOffset, projectile.param_7e);
    write_be16(wram, projectile_offset + kEntityFacingDir4WayOffset, projectile.facing_dir_4way);
    write_be32(
        wram,
        projectile_offset + kEntityVelXFpOffset,
        static_cast<std::uint32_t>(projectile.vel_x_fp));
    write_be32(
        wram,
        projectile_offset + kEntityVelYFpOffset,
        static_cast<std::uint32_t>(projectile.vel_y_fp));
    write_be32(
        wram,
        projectile_offset + kEntityVelZFpOffset,
        static_cast<std::uint32_t>(projectile.vel_z_fp));

    if (result.hud_tracker_updated &&
        wram.size() >= (kEnemyHudTargetPtrRamOffset + 4U)) {
        wram[kEnemyHudTimerRamOffset] = kEnemyHudLockDurationFrames;
        write_be32(wram, kEnemyHudTargetPtrRamOffset, projectile_ram_addr);
    }

    return result;
}

} // namespace thor::math
