#include "game/physics/player_targeting.hpp"

#include <optional>

namespace thor::physics {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8U) |
        static_cast<std::uint16_t>(buf[offset + 1U]));
}

[[nodiscard]] std::int16_t read_be16_signed(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(buf, offset));
}

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    const std::uint32_t raw =
        (static_cast<std::uint32_t>(buf[offset]) << 24U) |
        (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(buf[offset + 3U]);
    return static_cast<std::int32_t>(raw);
}

void write_be16(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::uint16_t value) noexcept {
    buf[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32_signed(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::int32_t value) noexcept {
    const auto u = static_cast<std::uint32_t>(value);
    buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
}

[[nodiscard]] std::optional<std::size_t> normalize_ram_offset(
    std::uint32_t addr,
    std::size_t required_bytes,
    std::size_t ram_size) noexcept {
    std::size_t offset = 0U;
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        offset = static_cast<std::size_t>(addr - 0x00FF0000U);
    } else if (addr < 0x10000U) {
        offset = static_cast<std::size_t>(addr);
    } else {
        return std::nullopt;
    }

    if (offset > ram_size || (ram_size - offset) < required_bytes) {
        return std::nullopt;
    }
    return offset;
}

[[nodiscard]] constexpr std::int32_t scale_diagonal_075(std::int32_t value) noexcept {
    return (value >> 1) + (value >> 2);
}

/// Self-contained 8-way velocity projection matching ROM 0x00E2D4..0x00E336.
[[nodiscard]] constexpr CardinalMotionEntry project_8way_velocity_00e2d4(
    std::uint16_t speed_x,
    std::uint16_t speed_y,
    std::uint16_t direction) noexcept {
    const std::int32_t sx = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_x) << 8U);
    const std::int32_t sy = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_y) << 8U);

    switch (direction & 7U) {
    case 0U: // N
        return CardinalMotionEntry{.vel_x_fp = 0, .vel_y_fp = -sy};
    case 1U: // E
        return CardinalMotionEntry{.vel_x_fp = sx, .vel_y_fp = 0};
    case 2U: // S
        return CardinalMotionEntry{.vel_x_fp = 0, .vel_y_fp = sy};
    case 3U: // W
        return CardinalMotionEntry{.vel_x_fp = -sx, .vel_y_fp = 0};
    case 4U: // NE
        return CardinalMotionEntry{
            .vel_x_fp = scale_diagonal_075(sx),
            .vel_y_fp = -scale_diagonal_075(sy),
        };
    case 5U: // SE
        return CardinalMotionEntry{
            .vel_x_fp = scale_diagonal_075(sx),
            .vel_y_fp = scale_diagonal_075(sy),
        };
    case 6U: // SW
        return CardinalMotionEntry{
            .vel_x_fp = -scale_diagonal_075(sx),
            .vel_y_fp = scale_diagonal_075(sy),
        };
    case 7U: // NW
        return CardinalMotionEntry{
            .vel_x_fp = -scale_diagonal_075(sx),
            .vel_y_fp = -scale_diagonal_075(sy),
        };
    default:
        return CardinalMotionEntry{};
    }
}

} // namespace

CardinalMotionTable decode_cardinal_motion_table(
    std::span<const std::uint8_t> table_bytes) noexcept {
    CardinalMotionTable table{};
    if (table_bytes.size() < kTargetMotionTableEntries * kTargetMotionTableEntryBytes) {
        return table;
    }

    for (std::size_t i = 0U; i < kTargetMotionTableEntries; ++i) {
        const std::size_t off = i * kTargetMotionTableEntryBytes;
        table[i].vel_x_fp = read_be32_signed(table_bytes, off);
        table[i].vel_y_fp = read_be32_signed(table_bytes, off + 4U);
    }
    return table;
}

MotionTableLoadResult target_player_ali_and_load_motion_table(
    std::int16_t player_x,
    std::int16_t player_y,
    TargetingEntityState& entity,
    const CardinalMotionTable& motion_table) noexcept {
    const auto dx = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(player_x) - static_cast<std::uint16_t>(entity.pos_x));
    const auto dy = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(player_y) - static_cast<std::uint16_t>(entity.pos_y));
    const auto abs_x = (dx < 0)
        ? static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(dx))
        : static_cast<std::uint16_t>(dx);
    const auto abs_y = (dy < 0)
        ? static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(dy))
        : static_cast<std::uint16_t>(dy);

    std::uint8_t dir = kTargetDirUp;
    std::uint16_t dom_dist = 0U;
    // 0x00E40C..0x00E40E: cmp.w d4, d3; bls.w $e428 -> ties resolve to Y-dominant.
    if (abs_x > abs_y) {
        dir = (dx < 0) ? kTargetDirLeft : kTargetDirRight;
        dom_dist = abs_x;
    } else {
        dir = (dy < 0) ? kTargetDirUp : kTargetDirDown;
        dom_dist = abs_y;
    }

    const CardinalMotionEntry& chosen = motion_table[dir];
    const std::uint16_t clamped_timer = (dom_dist < kTargetEntityMinMotionTimer)
        ? kTargetEntityMinMotionTimer
        : dom_dist;

    entity.facing_16 = dir;
    entity.vel_x_fp = chosen.vel_x_fp;
    entity.vel_y_fp = chosen.vel_y_fp;
    entity.timer_02 = clamped_timer;

    return MotionTableLoadResult{
        .direction = dir,
        .table_byte_offset = static_cast<std::uint16_t>(static_cast<std::uint16_t>(dir) << 3U),
        .motion_timer = clamped_timer,
        .vel_x_fp = chosen.vel_x_fp,
        .vel_y_fp = chosen.vel_y_fp,
    };
}

ProjectileLaunchResult launch_directional_projectile_physics(
    TargetingEntityState& entity,
    std::uint16_t status_mask_d7) noexcept {
    if ((status_mask_d7 & kLaunchSuppressMask) != 0U) {
        return ProjectileLaunchResult{};
    }

    const CardinalMotionEntry vel = project_8way_velocity_00e2d4(
        kLaunchBaseSpeedX,
        kLaunchBaseSpeedY,
        entity.launch_dir_84);

    entity.vel_x_fp = vel.vel_x_fp;
    entity.vel_y_fp = vel.vel_y_fp;
    entity.vel_z_fp = kLaunchUpwardVelZFp;

    return ProjectileLaunchResult{
        .launched = true,
        .vel_x_fp = vel.vel_x_fp,
        .vel_y_fp = vel.vel_y_fp,
        .vel_z_fp = kLaunchUpwardVelZFp,
    };
}

TargetingEntityState load_targeting_entity_state_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept {
    TargetingEntityState state{};
    const auto off = normalize_ram_offset(
        entity_ram_addr, kTargetEntityMinStructBytes, ram.size());
    if (!off.has_value()) {
        return state;
    }

    state.timer_02 = read_be16(ram, *off + kTargetEntityTimerOffset);
    state.pos_x = read_be16_signed(ram, *off + kTargetEntityPosXOffset);
    state.pos_y = read_be16_signed(ram, *off + kTargetEntityPosYOffset);
    state.facing_16 = read_be16(ram, *off + kTargetEntityFacingOffset);
    state.vel_x_fp = read_be32_signed(ram, *off + kTargetEntityVelXFpOffset);
    state.vel_y_fp = read_be32_signed(ram, *off + kTargetEntityVelYFpOffset);
    state.vel_z_fp = read_be32_signed(ram, *off + kTargetEntityVelZFpOffset);
    state.launch_dir_84 = read_be16(ram, *off + kTargetEntityLaunchDirOffset);
    return state;
}

void store_targeting_entity_state_to_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    const TargetingEntityState& state) noexcept {
    const auto off = normalize_ram_offset(
        entity_ram_addr, kTargetEntityMinStructBytes, ram.size());
    if (!off.has_value()) {
        return;
    }

    write_be16(ram, *off + kTargetEntityTimerOffset, state.timer_02);
    write_be16(ram, *off + kTargetEntityPosXOffset, static_cast<std::uint16_t>(state.pos_x));
    write_be16(ram, *off + kTargetEntityPosYOffset, static_cast<std::uint16_t>(state.pos_y));
    write_be16(ram, *off + kTargetEntityFacingOffset, state.facing_16);
    write_be32_signed(ram, *off + kTargetEntityVelXFpOffset, state.vel_x_fp);
    write_be32_signed(ram, *off + kTargetEntityVelYFpOffset, state.vel_y_fp);
    write_be32_signed(ram, *off + kTargetEntityVelZFpOffset, state.vel_z_fp);
    write_be16(ram, *off + kTargetEntityLaunchDirOffset, state.launch_dir_84);
}

TargetResult target_player_ali_quadrant_angle_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept {
    const auto ent_off = normalize_ram_offset(
        entity_ram_addr, kTargetEntityPosYOffset + 2U, ram.size());
    if (!ent_off.has_value() || ram.size() < kPlayerAliPosYRamOffset + 2U) {
        return TargetResult{};
    }

    const std::int16_t player_x = read_be16_signed(ram, kPlayerAliPosXRamOffset);
    const std::int16_t player_y = read_be16_signed(ram, kPlayerAliPosYRamOffset);
    const std::int16_t entity_x = read_be16_signed(ram, *ent_off + kTargetEntityPosXOffset);
    const std::int16_t entity_y = read_be16_signed(ram, *ent_off + kTargetEntityPosYOffset);
    return target_player_ali_quadrant_angle(player_x, player_y, entity_x, entity_y);
}

MotionTableLoadResult target_player_ali_and_load_motion_table_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    const CardinalMotionTable& motion_table) noexcept {
    const auto ent_off = normalize_ram_offset(
        entity_ram_addr, kTargetEntityVelYFpOffset + 4U, ram.size());
    if (!ent_off.has_value() || ram.size() < kPlayerAliPosYRamOffset + 2U) {
        return MotionTableLoadResult{};
    }

    const std::int16_t player_x = read_be16_signed(ram, kPlayerAliPosXRamOffset);
    const std::int16_t player_y = read_be16_signed(ram, kPlayerAliPosYRamOffset);

    TargetingEntityState entity{};
    entity.pos_x = read_be16_signed(ram, *ent_off + kTargetEntityPosXOffset);
    entity.pos_y = read_be16_signed(ram, *ent_off + kTargetEntityPosYOffset);

    const MotionTableLoadResult res = target_player_ali_and_load_motion_table(
        player_x, player_y, entity, motion_table);

    write_be16(ram, *ent_off + kTargetEntityFacingOffset, entity.facing_16);
    write_be32_signed(ram, *ent_off + kTargetEntityVelXFpOffset, entity.vel_x_fp);
    write_be32_signed(ram, *ent_off + kTargetEntityVelYFpOffset, entity.vel_y_fp);
    write_be16(ram, *ent_off + kTargetEntityTimerOffset, entity.timer_02);
    return res;
}

NormalizedAimVector normalize_target_aim_vector_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    std::int16_t target_x,
    std::int16_t target_y) noexcept {
    const auto ent_off = normalize_ram_offset(
        entity_ram_addr, kTargetEntityFacingOffset + 2U, ram.size());
    if (!ent_off.has_value()) {
        return NormalizedAimVector{};
    }

    const std::int16_t entity_x = read_be16_signed(ram, *ent_off + kTargetEntityPosXOffset);
    const std::int16_t entity_y = read_be16_signed(ram, *ent_off + kTargetEntityPosYOffset);
    const std::uint16_t facing = read_be16(ram, *ent_off + kTargetEntityFacingOffset);
    return normalize_target_aim_vector_divs(target_x, target_y, entity_x, entity_y, facing);
}

ProjectileLaunchResult launch_directional_projectile_physics_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    std::uint16_t status_mask_d7) noexcept {
    if ((status_mask_d7 & kLaunchSuppressMask) != 0U) {
        return ProjectileLaunchResult{};
    }
    const auto ent_off = normalize_ram_offset(
        entity_ram_addr, kTargetEntityMinStructBytes, ram.size());
    if (!ent_off.has_value()) {
        return ProjectileLaunchResult{};
    }

    TargetingEntityState entity{};
    entity.launch_dir_84 = read_be16(ram, *ent_off + kTargetEntityLaunchDirOffset);

    const ProjectileLaunchResult res = launch_directional_projectile_physics(
        entity, status_mask_d7);
    if (res.launched) {
        write_be32_signed(ram, *ent_off + kTargetEntityVelXFpOffset, entity.vel_x_fp);
        write_be32_signed(ram, *ent_off + kTargetEntityVelYFpOffset, entity.vel_y_fp);
        write_be32_signed(ram, *ent_off + kTargetEntityVelZFpOffset, entity.vel_z_fp);
    }
    return res;
}

} // namespace thor::physics
