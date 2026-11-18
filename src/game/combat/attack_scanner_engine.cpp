#include "game/combat/attack_scanner_engine.hpp"

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

bool is_scanner_target_eligible(
    const ScannerTargetState& target,
    std::uint32_t attacker_addr) noexcept {
    if (target.type <= 0) {
        return false;
    }
    if (target.address != 0U && attacker_addr != 0U && target.address == attacker_addr) {
        return false;
    }
    if ((target.flags_38 & AttackScannerConfig::kInvulnerableMask) != 0U) {
        return false;
    }
    if (target.cooldown_82 != 0U) {
        return false;
    }
    return true;
}

bool test_scanner_aabb_overlap(
    const AttackBox3D& box,
    const ScannerTargetState& target) noexcept {
    // 3D Z bounds
    const auto tz_base = static_cast<std::int16_t>(target.elev_z + target.ground_h);
    const auto tz_top = static_cast<std::int16_t>(tz_base + target.height_z);
    if (box.z_max < tz_base || box.z_min > tz_top) {
        return false;
    }

    // 2D X bounds
    const auto tx_min = static_cast<std::int16_t>(target.world_x - target.rad_x);
    const auto tx_max = static_cast<std::int16_t>(target.world_x + target.rad_x);
    if (box.x_min > tx_max || box.x_max < tx_min) {
        return false;
    }

    // 2D Y bounds
    const auto ty_adj = static_cast<std::int16_t>(target.world_y + target.ground_h);
    const auto ty_min = static_cast<std::int16_t>(ty_adj - target.rad_y);
    const auto ty_max = static_cast<std::int16_t>(ty_adj + target.rad_y);
    if (box.y_min > ty_max || box.y_max < ty_min) {
        return false;
    }

    return true;
}

AttackScanResult scan_attack_frame_pool(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets,
    const HitCallback& callback,
    std::uint32_t callback_addr) {
    (void)callback_addr;
    AttackScanResult result{};

    // Early Phase Suppression: bit 3 of +0x37(attacker)
    if ((attacker.flags_37 & AttackScannerConfig::kHitRegisteredMask) != 0U) {
        result.suppressed_early = true;
        return result;
    }

    // Resolve record list starting offset
    std::size_t rec_offset = 0U;
    if (attack_type + 2U <= descriptor_table.size()) {
        const auto disp = read_be16_signed(descriptor_table, attack_type);
        const auto candidate_offset = static_cast<std::size_t>(
            static_cast<std::int32_t>(attack_type) + disp);
        if (candidate_offset + AttackScannerConfig::kRecordSize <= descriptor_table.size()) {
            rec_offset = candidate_offset;
        }
    }

    // Search 44-byte records for matching anim_frame
    bool found = false;
    std::size_t match_offset = 0U;
    while (rec_offset + AttackScannerConfig::kRecordSize <= descriptor_table.size()) {
        const std::int16_t frame_id = read_be16_signed(descriptor_table, rec_offset);
        if (frame_id < 0) {
            break; // 0xFFFF terminator
        }
        if (frame_id == attacker.anim_frame) {
            found = true;
            match_offset = rec_offset;
            break;
        }
        rec_offset += AttackScannerConfig::kRecordSize;
    }

    if (!found) {
        result.frame_found = false;
        return result;
    }
    result.frame_found = true;

    // Read damage and element bytes
    const std::uint8_t base_dmg = descriptor_table[match_offset + 2U];
    const std::uint8_t element_raw = descriptor_table[match_offset + 3U];
    const bool is_ali = (attacker.address == AttackScannerConfig::kPlayerEntityRam);

    result.damage_desc = pack_attack_damage(
        base_dmg,
        element_raw,
        attacker.strength_stat_96,
        attacker.weapon_bonus_90,
        is_ali);

    // Directional hitbox & mirroring
    std::uint16_t dir = static_cast<std::uint16_t>(attacker.facing & 0x03U);
    bool flipped = false;
    if (dir == 3U) {
        flipped = true;
        dir = 1U;
    }

    const std::size_t box_offset = match_offset + AttackScannerConfig::kHitboxesOffset +
        (static_cast<std::size_t>(dir) * 12U);

    AttackBox3D local_box{
        .z_min = read_be16_signed(descriptor_table, box_offset + 0U),
        .z_max = read_be16_signed(descriptor_table, box_offset + 2U),
        .x_min = read_be16_signed(descriptor_table, box_offset + 4U),
        .y_min = read_be16_signed(descriptor_table, box_offset + 6U),
        .x_max = read_be16_signed(descriptor_table, box_offset + 8U),
        .y_max = read_be16_signed(descriptor_table, box_offset + 10U)
    };

    if (flipped) {
        local_box = local_box.mirror_horizontal();
    }

    // 3D world space coordinate projection
    const auto z_base = static_cast<std::int16_t>(attacker.elev_z + attacker.ground_h);
    const auto y_base = static_cast<std::int16_t>(attacker.world_y + attacker.ground_h);

    result.projected_box = AttackBox3D{
        .z_min = static_cast<std::int16_t>(local_box.z_min + z_base),
        .z_max = static_cast<std::int16_t>(local_box.z_max + z_base),
        .x_min = static_cast<std::int16_t>(local_box.x_min + attacker.world_x),
        .y_min = static_cast<std::int16_t>(local_box.y_min + y_base),
        .x_max = static_cast<std::int16_t>(local_box.x_max + attacker.world_x),
        .y_max = static_cast<std::int16_t>(local_box.y_max + y_base)
    };

    // Iterate over candidate targets
    for (auto& target : targets) {
        if (!is_scanner_target_eligible(target, attacker.address)) {
            continue;
        }
        if (test_scanner_aabb_overlap(result.projected_box, target)) {
            if (callback) {
                callback(attacker, target, result.damage_desc.packed_a4, attack_type);
            }
            result.hit_count++;
        }
    }

    // Post-loop weapon durability check for Ali
    if (is_ali && (attacker.flags_37 & AttackScannerConfig::kHitRegisteredMask) != 0U) {
        if (attack_type == 6U || attack_type == 10U) {
            result.durability_triggered = true;
        }
    }

    return result;
}

AttackScanResult scan_all_entities(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets_21,
    const HitCallback& callback) {
    return scan_attack_frame_pool(
        attacker,
        attack_type,
        descriptor_table,
        targets_21,
        callback,
        AttackScannerConfig::kCallbackAllAddr);
}

AttackScanResult scan_group3_a(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets_17,
    const HitCallback& callback) {
    return scan_attack_frame_pool(
        attacker,
        attack_type,
        descriptor_table,
        targets_17,
        callback,
        AttackScannerConfig::kCallbackGroup3Addr);
}

AttackScanResult scan_group3_b(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets_17,
    const HitCallback& callback,
    std::uint8_t* out_mode_flag_198b) {
    if (out_mode_flag_198b != nullptr) {
        *out_mode_flag_198b = 2U;
    }
    return scan_attack_frame_pool(
        attacker,
        attack_type,
        descriptor_table,
        targets_17,
        callback,
        AttackScannerConfig::kCallbackGroup3Addr);
}

AttackScanResult scan_single(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    ScannerTargetState& target_slot0,
    const HitCallback& callback) {
    const std::array<ScannerTargetState, 1> single_arr{target_slot0};
    std::span<ScannerTargetState> span_single{const_cast<ScannerTargetState*>(single_arr.data()), 1U};
    AttackScanResult res = scan_attack_frame_pool(
        attacker,
        attack_type,
        descriptor_table,
        span_single,
        callback,
        AttackScannerConfig::kCallbackAllAddr);
    target_slot0 = single_arr[0];
    return res;
}

ScannerAttackerState read_scanner_attacker_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr) {
    const std::size_t offset = to_ram_offset(slot_addr);
    if (offset + AttackScannerConfig::kSlotStride > ram.size()) {
        return ScannerAttackerState{.address = slot_addr};
    }

    return ScannerAttackerState{
        .address = slot_addr,
        .world_x = read_be16_signed(ram, offset + AttackScannerConfig::kWorldXOffset),
        .world_y = read_be16_signed(ram, offset + AttackScannerConfig::kWorldYOffset),
        .elev_z = read_be16_signed(ram, offset + AttackScannerConfig::kElevZOffset),
        .ground_h = read_be16_signed(ram, offset + AttackScannerConfig::kGroundHOffset),
        .facing = read_be16(ram, offset + AttackScannerConfig::kFacingOffset),
        .anim_frame = read_be16_signed(ram, offset + AttackScannerConfig::kAnimFrameOffset),
        .flags_37 = ram[offset + AttackScannerConfig::kFlags37Offset],
        .weapon_bonus_90 = read_be16(ram, offset + AttackScannerConfig::kWeaponBonus90Offset),
        .strength_stat_96 = read_be16(ram, offset + AttackScannerConfig::kStrengthStat96Offset)
    };
}

ScannerTargetState read_scanner_target_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr) {
    const std::size_t offset = to_ram_offset(slot_addr);
    if (offset + AttackScannerConfig::kSlotStride > ram.size()) {
        return ScannerTargetState{.address = slot_addr};
    }

    return ScannerTargetState{
        .address = slot_addr,
        .type = read_be16_signed(ram, offset + AttackScannerConfig::kTypeOffset),
        .world_x = read_be16_signed(ram, offset + AttackScannerConfig::kWorldXOffset),
        .world_y = read_be16_signed(ram, offset + AttackScannerConfig::kWorldYOffset),
        .elev_z = read_be16_signed(ram, offset + AttackScannerConfig::kElevZOffset),
        .ground_h = read_be16_signed(ram, offset + AttackScannerConfig::kGroundHOffset),
        .flags_38 = ram[offset + AttackScannerConfig::kFlags38Offset],
        .rad_x = read_be16_signed(ram, offset + AttackScannerConfig::kRadiusXOffset),
        .rad_y = read_be16_signed(ram, offset + AttackScannerConfig::kRadiusYOffset),
        .height_z = read_be16_signed(ram, offset + AttackScannerConfig::kHeightZOffset),
        .cooldown_82 = ram[offset + AttackScannerConfig::kHitCooldownOffset]
    };
}

void write_scanner_attacker_flags_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_addr,
    std::uint8_t flags_37) {
    const std::size_t offset = to_ram_offset(slot_addr);
    if (offset + AttackScannerConfig::kFlags37Offset < ram.size()) {
        ram[offset + AttackScannerConfig::kFlags37Offset] = flags_37;
    }
}

AttackScanResult scan_all_entities_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback) {
    ScannerAttackerState attacker = read_scanner_attacker_raw(ram, attacker_addr);
    std::vector<ScannerTargetState> targets;
    targets.reserve(AttackScannerConfig::kAllEntitiesSlotCount);

    for (std::size_t i = 0; i < AttackScannerConfig::kAllEntitiesSlotCount; ++i) {
        const std::uint32_t addr = AttackScannerConfig::kPlayerEntityRam +
            static_cast<std::uint32_t>(i * AttackScannerConfig::kSlotStride);
        targets.push_back(read_scanner_target_raw(ram, addr));
    }

    const AttackScanResult res = scan_all_entities(attacker, attack_type, descriptor_table, targets, callback);
    write_scanner_attacker_flags_raw(ram, attacker_addr, attacker.flags_37);
    return res;
}

AttackScanResult scan_group3_a_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback) {
    ScannerAttackerState attacker = read_scanner_attacker_raw(ram, attacker_addr);
    std::vector<ScannerTargetState> targets;
    targets.reserve(AttackScannerConfig::kGroup3SlotCount);

    for (std::size_t i = 0; i < AttackScannerConfig::kGroup3SlotCount; ++i) {
        const std::uint32_t addr = AttackScannerConfig::kGroup3EntityRam +
            static_cast<std::uint32_t>(i * AttackScannerConfig::kSlotStride);
        targets.push_back(read_scanner_target_raw(ram, addr));
    }

    const AttackScanResult res = scan_group3_a(attacker, attack_type, descriptor_table, targets, callback);
    write_scanner_attacker_flags_raw(ram, attacker_addr, attacker.flags_37);
    return res;
}

AttackScanResult scan_group3_b_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback) {
    const std::size_t flag_offset = to_ram_offset(AttackScannerConfig::kModeFlag198BRam);
    if (flag_offset < ram.size()) {
        ram[flag_offset] = 2U;
    }
    ScannerAttackerState attacker = read_scanner_attacker_raw(ram, attacker_addr);
    std::vector<ScannerTargetState> targets;
    targets.reserve(AttackScannerConfig::kGroup3SlotCount);

    for (std::size_t i = 0; i < AttackScannerConfig::kGroup3SlotCount; ++i) {
        const std::uint32_t addr = AttackScannerConfig::kGroup3EntityRam +
            static_cast<std::uint32_t>(i * AttackScannerConfig::kSlotStride);
        targets.push_back(read_scanner_target_raw(ram, addr));
    }

    const AttackScanResult res = scan_group3_b(attacker, attack_type, descriptor_table, targets, callback);
    write_scanner_attacker_flags_raw(ram, attacker_addr, attacker.flags_37);
    return res;
}

AttackScanResult scan_single_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback) {
    ScannerAttackerState attacker = read_scanner_attacker_raw(ram, attacker_addr);
    ScannerTargetState player = read_scanner_target_raw(ram, AttackScannerConfig::kPlayerEntityRam);
    const AttackScanResult res = scan_single(attacker, attack_type, descriptor_table, player, callback);
    write_scanner_attacker_flags_raw(ram, attacker_addr, attacker.flags_37);
    return res;
}

} // namespace oasis::game::combat
