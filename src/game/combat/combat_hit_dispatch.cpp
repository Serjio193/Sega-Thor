#include "game/combat/combat_hit_dispatch.hpp"

#include <algorithm>

namespace oasis::game::combat {

DamagePack compute_damage_pack(
    std::uint8_t base_damage,
    std::uint8_t hit_flags,
    bool is_ali,
    std::uint16_t kill_count,
    std::int16_t power_bonus) noexcept
{
    std::uint16_t dmg = base_damage;
    if (is_ali) {
        if (kill_count >= 10U) dmg += 1U;
        if (kill_count >= 20U) dmg += 1U;
        if (kill_count >= 30U) dmg += 1U;
        if (kill_count >= 40U) dmg += 1U;
        if (kill_count >= 50U) dmg += 1U;
        if (kill_count >= 100U) dmg += 2U;
        if (kill_count >= 200U) dmg += 4U;
    }

    const auto hitstop = static_cast<std::uint8_t>((hit_flags & 0x0FU) + 1U);
    const auto upper_flags = static_cast<std::uint16_t>((hit_flags & 0xF0U) << 8);
    const auto damage_word = static_cast<std::uint16_t>((upper_flags | (dmg & 0xFFU)) + power_bonus);

    return DamagePack{
        .damage_word = damage_word,
        .hitstop = hitstop,
        .raw_damage = static_cast<std::uint8_t>(dmg & 0xFFU),
        .hit_flags = hit_flags,
    };
}

CombatHitDispatchResult dispatch_combat_hit(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    std::size_t pool_start_slot,
    std::size_t pool_slot_count,
    std::uint32_t callback_addr,
    std::uint8_t hit_marker,
    const HitCallback& callback) noexcept
{
    CombatHitDispatchResult result{
        .hit_marker = hit_marker,
        .attack_index = attack_index,
        .callback_addr = callback_addr,
    };

    if ((attacker.flags_37 & 0x08U) != 0U) {
        result.early_exit_flag_37 = true;
        return result;
    }

    const CombatFrameRecord* matched_record = nullptr;
    for (const auto& rec : records) {
        if (rec.frame_id < 0) {
            break;
        }
        if (rec.frame_id == attacker.frame_id) {
            matched_record = &rec;
            break;
        }
    }

    if (!matched_record) {
        return result;
    }
    result.record_found = true;

    std::size_t dir_idx = 0U;
    if (attacker.facing == WeaponHitboxConfig::kMirrorFacingId) {
        dir_idx = 1U;
    } else if (attacker.facing < matched_record->local_boxes.size()) {
        dir_idx = attacker.facing;
    }

    const auto& local_box = matched_record->local_boxes[dir_idx];
    result.world_box = build_world_weapon_hitbox(
        attacker.pos_x,
        attacker.pos_y,
        attacker.pos_z,
        attacker.elevation,
        attacker.facing,
        local_box);

    result.damage_pack = compute_damage_pack(
        matched_record->base_damage,
        matched_record->hit_flags,
        attacker.is_ali,
        attacker.kill_count,
        attacker.power_bonus);

    const std::size_t end_slot = std::min(targets.size(), pool_start_slot + pool_slot_count);
    for (std::size_t i = pool_start_slot; i < end_slot; ++i) {
        if (i == attacker.slot_index) {
            continue;
        }

        auto& target = targets[i];
        if (target.state <= 0) {
            continue;
        }
        if ((target.flags_38 & 0x08U) != 0U) {
            continue;
        }

        const auto target_z_min = static_cast<std::int16_t>(target.pos_z + target.elevation);
        const auto target_z_max = static_cast<std::int16_t>(target_z_min + target.height_z);
        if (result.world_box.z2 < target_z_min || result.world_box.z1 > target_z_max) {
            continue;
        }

        const auto rx = target.half_width_x;
        if (result.world_box.x1 > target.pos_x + rx || result.world_box.x2 < target.pos_x - rx) {
            continue;
        }

        const auto target_y_base = static_cast<std::int16_t>(target.pos_y + target.elevation);
        const auto ry = target.half_width_y;
        if (result.world_box.y1 > target_y_base + ry || result.world_box.y2 < target_y_base - ry) {
            continue;
        }

        if (target.hitstop != 0U) {
            continue;
        }

        const std::uint32_t ram_addr = CombatHitDispatchConfig::kFullEntityPoolRam +
            static_cast<std::uint32_t>(i * WeaponHitboxConfig::kEntityStrideBytes);

        result.hits.push_back(TargetHitEvent{
            .target_slot = i,
            .target_ram_addr = ram_addr,
            .damage_pack = result.damage_pack,
        });

        if (callback) {
            callback(target, attacker, result.damage_pack);
        }
    }

    if (attacker.is_ali && (attacker.flags_37 & 0x08U) != 0U) {
        if (attack_index == CombatHitDispatchConfig::kSpecialCancelIndex1 ||
            attack_index == CombatHitDispatchConfig::kSpecialCancelIndex2) {
            result.special_recoil_cancel = true;
        }
    }

    return result;
}

CombatHitDispatchResult dispatch_combat_hit_primary(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback)
{
    return dispatch_combat_hit(
        attacker,
        attack_index,
        records,
        targets,
        0U,
        CombatHitDispatchConfig::kFullEntitySlotCount,
        CombatHitDispatchConfig::kCallbackDefaultBeba,
        0U,
        callback);
}

CombatHitDispatchResult dispatch_combat_hit_fx(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback)
{
    return dispatch_combat_hit(
        attacker,
        attack_index,
        records,
        targets,
        CombatHitDispatchConfig::kEnemyPoolStartIndex,
        CombatHitDispatchConfig::kEnemyEntitySlotCount,
        CombatHitDispatchConfig::kCallbackDefaultBe98,
        0U,
        callback);
}

CombatHitDispatchResult dispatch_combat_hit_fx_marked(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback)
{
    return dispatch_combat_hit(
        attacker,
        attack_index,
        records,
        targets,
        CombatHitDispatchConfig::kEnemyPoolStartIndex,
        CombatHitDispatchConfig::kEnemyEntitySlotCount,
        CombatHitDispatchConfig::kCallbackDefaultBe98,
        CombatHitDispatchConfig::kHitMarkerValue,
        callback);
}

CombatHitDispatchResult dispatch_combat_hit_ali_only(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback)
{
    return dispatch_combat_hit(
        attacker,
        attack_index,
        records,
        targets,
        0U,
        1U,
        CombatHitDispatchConfig::kCallbackDefaultBeba,
        0U,
        callback);
}

} // namespace oasis::game::combat
