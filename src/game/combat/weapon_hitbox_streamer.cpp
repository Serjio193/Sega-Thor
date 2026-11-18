#include "game/combat/weapon_hitbox_streamer.hpp"

namespace oasis::game::combat {

namespace {

[[nodiscard]] inline std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    if (offset + 2U > data.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] inline std::int16_t read_be16_signed(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(data, offset));
}

[[nodiscard]] inline std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

} // namespace

RawHitboxRecord unpack_hitbox_record(std::span<const std::uint8_t> stream, std::size_t byte_offset) {
    if (byte_offset + WeaponHitboxConfig::kHitboxEntryBytes > stream.size()) {
        return RawHitboxRecord{};
    }
    return RawHitboxRecord{
        read_be16_signed(stream, byte_offset + 0U),
        read_be16_signed(stream, byte_offset + 2U),
        read_be16_signed(stream, byte_offset + 4U),
        read_be16_signed(stream, byte_offset + 6U),
        read_be16_signed(stream, byte_offset + 8U),
        read_be16_signed(stream, byte_offset + 10U)
    };
}

DirectionalHitbox resolve_directional_hitbox(std::uint16_t facing, std::span<const std::uint8_t> stream) {
    DirectionalHitbox result{};
    std::uint16_t dir = static_cast<std::uint16_t>(facing & 0x03U);

    if (dir == 3U) {
        // Facing Left: mirror direction 1 (Right)
        result.mirrored = true;
        dir = 1U;
    }

    result.stream_offset = static_cast<std::size_t>(dir) * WeaponHitboxConfig::kHitboxEntryBytes;
    result.local_box = unpack_hitbox_record(stream, result.stream_offset);

    if (result.mirrored) {
        result.local_box = result.local_box.mirror_horizontal();
    }

    return result;
}

ProjectedHitbox project_to_world(
    const RawHitboxRecord& local_box,
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::int16_t ground_h) noexcept {
    const auto z_base = static_cast<std::int16_t>(elev_z + ground_h);
    const auto y_base = static_cast<std::int16_t>(world_y + ground_h);

    return ProjectedHitbox{
        static_cast<std::int16_t>(local_box.z_min + z_base),
        static_cast<std::int16_t>(local_box.z_max + z_base),
        static_cast<std::int16_t>(local_box.x_min + world_x),
        static_cast<std::int16_t>(local_box.y_min + y_base),
        static_cast<std::int16_t>(local_box.x_max + world_x),
        static_cast<std::int16_t>(local_box.y_max + y_base)
    };
}

bool is_target_eligible(
    const TargetEntityState& target,
    std::uint32_t attacker_addr,
    bool ignore_protection) noexcept {
    if (target.type <= 0) {
        return false;
    }
    if (target.address != 0U && attacker_addr != 0U && target.address == attacker_addr) {
        return false;
    }
    if ((target.flags_38 & WeaponHitboxConfig::kInvulnerableMask) != 0U) {
        return false;
    }
    if (!ignore_protection && (target.flags_3a & WeaponHitboxConfig::kProtectedMask) != 0U) {
        return false;
    }
    return true;
}

bool test_aabb_overlap(
    const ProjectedHitbox& box,
    const TargetEntityState& target) noexcept {
    // 3D Z bounds check
    const auto tz_base = static_cast<std::int16_t>(target.elev_z + target.ground_h);
    const auto tz_top = static_cast<std::int16_t>(tz_base + target.height_z);
    if (box.z_max < tz_base) {
        return false;
    }
    if (box.z_min > tz_top) {
        return false;
    }

    // 2D X bounds check
    const auto tx_min = static_cast<std::int16_t>(target.world_x - target.rad_x);
    const auto tx_max = static_cast<std::int16_t>(target.world_x + target.rad_x);
    if (box.x_min > tx_max) {
        return false;
    }
    if (box.x_max < tx_min) {
        return false;
    }

    // 2D Y bounds check
    const auto ty_adj = static_cast<std::int16_t>(target.world_y + target.ground_h);
    const auto ty_min = static_cast<std::int16_t>(ty_adj - target.rad_y);
    const auto ty_max = static_cast<std::int16_t>(ty_adj + target.rad_y);
    if (box.y_min > ty_max) {
        return false;
    }
    if (box.y_max < ty_min) {
        return false;
    }

    return true;
}

WeaponHitResult scan_weapon_hitbox_pool(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    std::span<const TargetEntityState> targets,
    bool ignore_protection) {
    const DirectionalHitbox resolved = resolve_directional_hitbox(attacker.facing, hitbox_stream);
    const ProjectedHitbox box = project_to_world(
        resolved.local_box,
        attacker.world_x,
        attacker.world_y,
        attacker.elev_z,
        attacker.ground_h);

    for (std::size_t i = 0; i < targets.size(); ++i) {
        const auto& candidate = targets[i];
        if (!is_target_eligible(candidate, attacker.address, ignore_protection)) {
            continue;
        }
        if (test_aabb_overlap(box, candidate)) {
            return WeaponHitResult{
                .carry = true,
                .target_addr = candidate.address,
                .slot_index = i,
                .projected_box = box
            };
        }
    }

    return WeaponHitResult{
        .carry = false,
        .target_addr = 0U,
        .slot_index = targets.size(),
        .projected_box = box
    };
}

WeaponHitResult scan_player_weapon_hitbox(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    const TargetEntityState& player_slot) {
    const std::array<TargetEntityState, 1> slots{player_slot};
    return scan_weapon_hitbox_pool(attacker, hitbox_stream, slots, false);
}

WeaponHitResult scan_enemy_weapon_hitbox(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    std::span<const TargetEntityState> enemy_slots) {
    return scan_weapon_hitbox_pool(attacker, hitbox_stream, enemy_slots, false);
}

WeaponHitResult scan_enemy_weapon_hitbox_unprotected(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    std::span<const TargetEntityState> enemy_slots) {
    return scan_weapon_hitbox_pool(attacker, hitbox_stream, enemy_slots, true);
}

TargetEntityState read_target_entity_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr) {
    const std::size_t offset = to_ram_offset(slot_addr);
    if (offset + WeaponHitboxConfig::kSlotStride > ram.size()) {
        return TargetEntityState{.address = slot_addr};
    }

    return TargetEntityState{
        .address = slot_addr,
        .type = read_be16_signed(ram, offset + WeaponHitboxConfig::kTypeOffset),
        .world_x = read_be16_signed(ram, offset + WeaponHitboxConfig::kWorldXOffset),
        .world_y = read_be16_signed(ram, offset + WeaponHitboxConfig::kWorldYOffset),
        .elev_z = read_be16_signed(ram, offset + WeaponHitboxConfig::kElevZOffset),
        .ground_h = read_be16_signed(ram, offset + WeaponHitboxConfig::kGroundHOffset),
        .flags_38 = ram[offset + WeaponHitboxConfig::kFlags38Offset],
        .flags_3a = ram[offset + WeaponHitboxConfig::kFlags3AOffset],
        .rad_x = read_be16_signed(ram, offset + WeaponHitboxConfig::kRadiusXOffset),
        .rad_y = read_be16_signed(ram, offset + WeaponHitboxConfig::kRadiusYOffset),
        .height_z = read_be16_signed(ram, offset + WeaponHitboxConfig::kHeightZOffset)
    };
}

AttackerEntityState read_attacker_entity_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr) {
    const std::size_t offset = to_ram_offset(slot_addr);
    if (offset + WeaponHitboxConfig::kSlotStride > ram.size()) {
        return AttackerEntityState{.address = slot_addr};
    }

    return AttackerEntityState{
        .address = slot_addr,
        .world_x = read_be16_signed(ram, offset + WeaponHitboxConfig::kWorldXOffset),
        .world_y = read_be16_signed(ram, offset + WeaponHitboxConfig::kWorldYOffset),
        .elev_z = read_be16_signed(ram, offset + WeaponHitboxConfig::kElevZOffset),
        .ground_h = read_be16_signed(ram, offset + WeaponHitboxConfig::kGroundHOffset),
        .facing = read_be16(ram, offset + WeaponHitboxConfig::kFacingOffset)
    };
}

WeaponHitResult scan_player_weapon_hitbox_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream) {
    const AttackerEntityState attacker = read_attacker_entity_raw(ram, attacker_addr);
    const TargetEntityState player = read_target_entity_raw(ram, WeaponHitboxConfig::kGroup2PoolRam);
    return scan_player_weapon_hitbox(attacker, hitbox_stream, player);
}

WeaponHitResult scan_enemy_weapon_hitbox_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream) {
    const AttackerEntityState attacker = read_attacker_entity_raw(ram, attacker_addr);
    std::vector<TargetEntityState> enemies;
    enemies.reserve(WeaponHitboxConfig::kEnemySlotCount);

    for (std::size_t i = 0; i < WeaponHitboxConfig::kEnemySlotCount; ++i) {
        const std::uint32_t addr = WeaponHitboxConfig::kEnemyPoolRam +
            static_cast<std::uint32_t>(i * WeaponHitboxConfig::kSlotStride);
        enemies.push_back(read_target_entity_raw(ram, addr));
    }

    return scan_enemy_weapon_hitbox(attacker, hitbox_stream, enemies);
}

WeaponHitResult scan_enemy_weapon_hitbox_unprotected_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream) {
    const AttackerEntityState attacker = read_attacker_entity_raw(ram, attacker_addr);
    std::vector<TargetEntityState> enemies;
    enemies.reserve(WeaponHitboxConfig::kEnemySlotCount);

    for (std::size_t i = 0; i < WeaponHitboxConfig::kEnemySlotCount; ++i) {
        const std::uint32_t addr = WeaponHitboxConfig::kEnemyPoolRam +
            static_cast<std::uint32_t>(i * WeaponHitboxConfig::kSlotStride);
        enemies.push_back(read_target_entity_raw(ram, addr));
    }

    return scan_enemy_weapon_hitbox_unprotected(attacker, hitbox_stream, enemies);
}

WeaponHitResult scan_all_group2_weapon_hitbox_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream) {
    const AttackerEntityState attacker = read_attacker_entity_raw(ram, attacker_addr);
    std::vector<TargetEntityState> slots;
    slots.reserve(WeaponHitboxConfig::kAllGroup2SlotCount);

    for (std::size_t i = 0; i < WeaponHitboxConfig::kAllGroup2SlotCount; ++i) {
        const std::uint32_t addr = WeaponHitboxConfig::kGroup2PoolRam +
            static_cast<std::uint32_t>(i * WeaponHitboxConfig::kSlotStride);
        slots.push_back(read_target_entity_raw(ram, addr));
    }

    return scan_weapon_hitbox_pool(attacker, hitbox_stream, slots, false);
}

} // namespace oasis::game::combat
