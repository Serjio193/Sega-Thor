#include "game/combat/combat_hit_apply.hpp"

namespace oasis::game::combat {

std::pair<bool, std::size_t> spawn_hit_spark(
    HitSparkPool& pool,
    const WorldHitbox3D& att_box,
    const CombatTargetEntity& target) noexcept {
    std::size_t free_idx = pool.slots.size();
    for (std::size_t i = 0; i < pool.slots.size(); ++i) {
        if (pool.slots[i].type == 0U) {
            free_idx = i;
            break;
        }
    }
    if (free_idx == pool.slots.size()) {
        return {false, 0U};
    }

    const auto tgt_z_min = static_cast<std::int16_t>(target.pos_z + target.elevation);
    const auto tgt_z_max = static_cast<std::int16_t>(tgt_z_min + target.height_z);
    const auto mid_z = clamp_midpoint(att_box.z1, att_box.z2, tgt_z_min, tgt_z_max);

    const auto tgt_x_min = static_cast<std::int16_t>(target.pos_x - target.half_width_x);
    const auto tgt_x_max = static_cast<std::int16_t>(target.pos_x + target.half_width_x);
    const auto mid_x = clamp_midpoint(att_box.x1, att_box.x2, tgt_x_min, tgt_x_max);

    const auto tgt_y_base = static_cast<std::int16_t>(target.pos_y + target.elevation);
    const auto tgt_y_min = static_cast<std::int16_t>(tgt_y_base - target.half_width_y);
    const auto tgt_y_max = static_cast<std::int16_t>(tgt_y_base + target.half_width_y);
    const auto mid_y = clamp_midpoint(att_box.y1, att_box.y2, tgt_y_min, tgt_y_max);

    auto& slot = pool.slots[free_idx];
    slot.type = CombatHitApplyConfig::kHitSparkType;
    slot.timer = CombatHitApplyConfig::kHitSparkTimerFrames;
    slot.pos_x = mid_x;
    slot.pos_y = static_cast<std::int16_t>(mid_y - mid_z);

    return {true, free_idx};
}

bool update_enemy_gauge_latch(
    EnemyGaugeState& gauge,
    const CombatTargetEntity& target) noexcept {
    if (target.ram_addr == CombatHitApplyConfig::kAliEntityRam) {
        return false;
    }
    if ((target.flags_3a & 0x01U) != 0U) {
        return false;
    }
    gauge.timer = CombatHitApplyConfig::kEnemyGaugeTimerDuration;
    gauge.target_ptr = target.ram_addr;
    return true;
}

std::tuple<KnockbackProfile, std::int32_t, std::int32_t, std::int32_t, std::uint16_t>
compute_knockback(
    std::uint16_t damage_word,
    bool is_airborne,
    std::uint16_t attacker_facing) noexcept {
    auto tier = static_cast<std::uint8_t>((damage_word >> 14) & 3U);
    bool airborne_fallback = false;
    std::uint16_t updated_damage = damage_word;

    if (tier == 0U && is_airborne) {
        updated_damage |= 0x8000U;
        tier = 2U;
        airborne_fallback = true;
    }

    KnockbackProfile profile{};
    profile.tier = tier;
    profile.airborne_fallback = airborne_fallback;

    std::int32_t vel_x = 0;
    std::int32_t vel_y = 0;
    std::int32_t vel_z = 0;

    switch (tier) {
    case 0U: {
        profile.vx_mag = 0x0300U;
        profile.vy_mag = 0x02C0U;
        profile.vz = 0x00000000;
        break;
    }
    case 1U: {
        profile.vx_mag = 0x0000U;
        profile.vy_mag = 0x0000U;
        profile.vz = 0x00070000;
        vel_x = 0;
        vel_y = 0;
        vel_z = profile.vz;
        return {profile, vel_x, vel_y, vel_z, updated_damage};
    }
    case 2U: {
        profile.vx_mag = 0x0400U;
        profile.vy_mag = 0x03C0U;
        profile.vz = 0x00028000;
        break;
    }
    case 3U:
    default: {
        profile.vx_mag = 0x0700U;
        profile.vy_mag = 0x0680U;
        profile.vz = 0x00030000;
        break;
    }
    }

    vel_z = profile.vz;

    const auto vx_scaled = static_cast<std::int32_t>(profile.vx_mag) << 8;
    const auto vy_scaled = static_cast<std::int32_t>(profile.vy_mag) << 8;

    switch (attacker_facing & 3U) {
    case 0U: // Up
        vel_x = 0;
        vel_y = -vy_scaled;
        break;
    case 1U: // Right
        vel_x = vx_scaled;
        vel_y = 0;
        break;
    case 2U: // Down
        vel_x = 0;
        vel_y = vy_scaled;
        break;
    case 3U: // Left
    default:
        vel_x = -vx_scaled;
        vel_y = 0;
        break;
    }

    return {profile, vel_x, vel_y, vel_z, updated_damage};
}

CombatHitApplyResult apply_combat_hit(
    CombatTargetEntity& target,
    CombatAttackerEntity& attacker,
    EnemyGaugeState& gauge,
    HitSparkPool& spark_pool,
    const CombatHitApplyParams& params,
    bool check_enemy_gauge) noexcept {
    CombatHitApplyResult result{};

    if (check_enemy_gauge) {
        result.gauge_latched = update_enemy_gauge_latch(gauge, target);
    }

    if (target.guard != 0U) {
        const auto opp_facing = static_cast<std::uint16_t>((attacker.facing ^ 2U) & 3U);
        if ((target.facing & 3U) == opp_facing) {
            result.guard_deflected = true;

            if ((attacker.is_ali || attacker.ram_addr == CombatHitApplyConfig::kAliEntityRam) &&
                (params.attack_index == 6U || params.attack_index == 10U)) {
                result.special_recoil_cancelled = true;
            }

            target.last_attacker = attacker.ram_addr;
            target.hitstop = params.hitstop;
            attacker.hitstop = params.hitstop;
            target.damage_word = params.damage_word;
            target.field_9d = 0U;
            target.hit_facing = attacker.facing;

            const auto [spark_ok, spark_idx] = spawn_hit_spark(spark_pool, params.world_box, target);
            result.spark_spawned = spark_ok;
            if (spark_ok) {
                result.spark_slot_index = spark_idx;
                result.spark_x = spark_pool.slots[spark_idx].pos_x;
                result.spark_y = spark_pool.slots[spark_idx].pos_y;
            }

            if ((attacker.flags_37 & 0x08U) == 0U) {
                attacker.flags_37 |= 0x08U;
                result.sfx_played = true;
                result.sfx_id = CombatHitApplyConfig::kBlockSfxId;
            }

            return result;
        }

        target.guard = 0U;
        result.guard_broken = true;
    }

    target.last_attacker = attacker.ram_addr;
    target.hitstop = params.hitstop;
    attacker.hitstop = params.hitstop;
    target.damage_word = params.damage_word;
    target.reaction_timer = CombatHitApplyConfig::kHitReactionTimerDuration;
    target.field_9d = 0U;
    target.hit_facing = attacker.facing;

    const auto [spark_ok, spark_idx] = spawn_hit_spark(spark_pool, params.world_box, target);
    result.spark_spawned = spark_ok;
    if (spark_ok) {
        result.spark_slot_index = spark_idx;
        result.spark_x = spark_pool.slots[spark_idx].pos_x;
        result.spark_y = spark_pool.slots[spark_idx].pos_y;
    }

    if ((attacker.flags_37 & 0x08U) == 0U) {
        attacker.flags_37 |= 0x08U;
        result.sfx_played = true;
        result.sfx_id = params.sfx_id;
    }

    const bool is_airborne = (target.pos_z != 0);
    const auto [prof, vx, vy, vz, updated_damage] = compute_knockback(
        params.damage_word, is_airborne, attacker.facing);

    result.knockback = prof;
    target.vel_x = vx;
    target.vel_y = vy;
    target.vel_z = vz;
    target.damage_word = updated_damage;

    return result;
}

} // namespace oasis::game::combat
