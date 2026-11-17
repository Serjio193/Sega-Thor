#include "game/combat/combat_hit_apply.hpp"

#include <cassert>
#include <iostream>

using namespace oasis::game::combat;

static void test_clamp_midpoint() {
    // Exact test from python rom_hit_apply_spark_test.py
    const auto mid_z = clamp_midpoint(0, 16, 4, 20);
    const auto mid_x = clamp_midpoint(100, 130, 110, 140);
    const auto mid_y = clamp_midpoint(200, 220, 204, 240);

    assert(mid_z == 10);
    assert(mid_x == 120);
    assert(mid_y == 212);
    assert((mid_y - mid_z) == 202);

    // Completely disjoint intervals (clamping to edge)
    const auto mid_disjoint = clamp_midpoint(10, 20, 30, 40);
    assert(mid_disjoint >= 20 && mid_disjoint <= 30);
}

static void test_enemy_gauge_latch() {
    EnemyGaugeState gauge{};

    // Target is Ali: gauge should NOT be latched
    CombatTargetEntity ali_target{};
    ali_target.ram_addr = CombatHitApplyConfig::kAliEntityRam;
    ali_target.flags_3a = 0U;
    assert(!update_enemy_gauge_latch(gauge, ali_target));
    assert(gauge.timer == 0U);
    assert(gauge.target_ptr == 0U);

    // Target is enemy with hide flag bit 0 set: gauge should NOT be latched
    CombatTargetEntity boss_target{};
    boss_target.ram_addr = 0x00FF1CD8U;
    boss_target.flags_3a = 0x01U;
    assert(!update_enemy_gauge_latch(gauge, boss_target));
    assert(gauge.timer == 0U);
    assert(gauge.target_ptr == 0U);

    // Normal enemy: latches 0x5A frames and target ptr
    CombatTargetEntity normal_enemy{};
    normal_enemy.ram_addr = 0x00FF1CE0U;
    normal_enemy.flags_3a = 0x00U;
    assert(update_enemy_gauge_latch(gauge, normal_enemy));
    assert(gauge.timer == 0x5AU);
    assert(gauge.target_ptr == 0x00FF1CE0U);
}

static void test_hit_spark_pool() {
    HitSparkPool pool{};
    WorldHitbox3D att_box{
        .x1 = 100, .y1 = 200,
        .x2 = 130, .y2 = 220,
        .z1 = 0,   .z2 = 16,
    };
    CombatTargetEntity target{
        .pos_x = 125,
        .pos_y = 218,
        .pos_z = 0,
        .elevation = 4,
        .half_width_x = 15,
        .half_width_y = 18,
        .height_z = 16,
    };

    // Slot 0
    auto [ok0, idx0] = spawn_hit_spark(pool, att_box, target);
    assert(ok0 && idx0 == 0U);
    assert(pool.slots[0].type == CombatHitApplyConfig::kHitSparkType);
    assert(pool.slots[0].timer == CombatHitApplyConfig::kHitSparkTimerFrames);
    assert(pool.slots[0].pos_x == 120);
    assert(pool.slots[0].pos_y == 202);

    // Slot 1
    auto [ok1, idx1] = spawn_hit_spark(pool, att_box, target);
    assert(ok1 && idx1 == 1U);

    // Slot 2
    auto [ok2, idx2] = spawn_hit_spark(pool, att_box, target);
    assert(ok2 && idx2 == 2U);

    // Slot 3
    auto [ok3, idx3] = spawn_hit_spark(pool, att_box, target);
    assert(ok3 && idx3 == 3U);

    // Pool full: 5th spark should fail
    auto [ok4, idx4] = spawn_hit_spark(pool, att_box, target);
    assert(!ok4);
}

static void test_knockback_profiles_and_directions() {
    // Tier 0, Grounded
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x0010U, false, 0U); // Up
        assert(prof.tier == 0U);
        assert(!prof.airborne_fallback);
        assert(prof.vx_mag == 0x0300U);
        assert(prof.vy_mag == 0x02C0U);
        assert(vx == 0);
        assert(vy == -0x0002C000);
        assert(vz == 0);
        assert(dmg == 0x0010U);
    }
    // Tier 0, Grounded, Facing Right
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x0010U, false, 1U); // Right
        assert(vx == 0x00030000);
        assert(vy == 0);
        assert(vz == 0);
    }
    // Tier 0, Grounded, Facing Down
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x0010U, false, 2U); // Down
        assert(vx == 0);
        assert(vy == 0x0002C000);
        assert(vz == 0);
    }
    // Tier 0, Grounded, Facing Left
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x0010U, false, 3U); // Left
        assert(vx == -0x00030000);
        assert(vy == 0);
        assert(vz == 0);
    }

    // Tier 0, Airborne -> falls through to Tier 2 with bit 15 set!
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x0010U, true, 1U); // Right
        assert(prof.tier == 2U);
        assert(prof.airborne_fallback);
        assert(prof.vx_mag == 0x0400U);
        assert(prof.vy_mag == 0x03C0U);
        assert(vx == 0x00040000);
        assert(vy == 0);
        assert(vz == 0x00028000);
        assert((dmg & 0x8000U) != 0U);
    }

    // Tier 1 -> Pop-up (0, 0, vz = 0x70000)
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x4010U, false, 3U);
        assert(prof.tier == 1U);
        assert(vx == 0);
        assert(vy == 0);
        assert(vz == 0x00070000);
    }

    // Tier 2 -> (0x400, 0x3C0, vz = 0x28000)
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0x8010U, false, 0U); // Up
        assert(prof.tier == 2U);
        assert(vx == 0);
        assert(vy == -0x0003C000);
        assert(vz == 0x00028000);
    }

    // Tier 3 -> (0x700, 0x680, vz = 0x30000)
    {
        const auto [prof, vx, vy, vz, dmg] = compute_knockback(0xC010U, false, 2U); // Down
        assert(prof.tier == 3U);
        assert(vx == 0);
        assert(vy == 0x00068000);
        assert(vz == 0x00030000);
    }
}

static void test_guard_deflection_and_break() {
    HitSparkPool spark_pool{};
    EnemyGaugeState gauge{};

    // 1. Guard Deflect (Target facing 0 Up, Attacker facing 2 Down -> opposite = 0)
    CombatTargetEntity target{};
    target.ram_addr = 0x00FF1D00U;
    target.guard = 1U;
    target.facing = 0U; // Up
    target.pos_x = 100;
    target.pos_y = 100;
    target.half_width_x = 10;
    target.half_width_y = 10;
    target.height_z = 20;

    CombatAttackerEntity ali_attacker{};
    ali_attacker.ram_addr = CombatHitApplyConfig::kAliEntityRam;
    ali_attacker.is_ali = true;
    ali_attacker.facing = 2U; // Down
    ali_attacker.flags_37 = 0U;

    CombatHitApplyParams params{
        .damage_word = 0x0020U,
        .hitstop = 4U,
        .sfx_id = 0x00800210U,
        .attack_index = 6U, // Trigger special recoil cancel
        .world_box = {.x1 = 95, .y1 = 95, .x2 = 105, .y2 = 105, .z1 = 0, .z2 = 10},
    };

    auto res = apply_combat_hit(target, ali_attacker, gauge, spark_pool, params, true);
    assert(res.gauge_latched);
    assert(res.guard_deflected);
    assert(!res.guard_broken);
    assert(res.special_recoil_cancelled);
    assert(res.sfx_played);
    assert(res.sfx_id == CombatHitApplyConfig::kBlockSfxId);
    assert(ali_attacker.flags_37 & 0x08U);
    assert(target.guard == 1U); // Guard intact
    assert(target.reaction_timer == 0U); // No reaction stun timer
    assert(target.vel_x == 0 && target.vel_y == 0 && target.vel_z == 0); // No knockback
    assert(target.hitstop == 4U);
    assert(ali_attacker.hitstop == 4U);
    assert(target.damage_word == 0x0020U);
    assert(target.hit_facing == 2U);

    // 2. Guard Broken (Target facing 0 Up, Attacker facing 0 Up -> opposite = 2 != 0)
    target.guard = 1U;
    target.facing = 0U;
    ali_attacker.facing = 0U; // Attacking from behind
    ali_attacker.flags_37 = 0U; // Reset flag to test hit SFX
    params.attack_index = 0U;

    auto res2 = apply_combat_hit(target, ali_attacker, gauge, spark_pool, params, true);
    assert(!res2.guard_deflected);
    assert(res2.guard_broken);
    assert(target.guard == 0U); // Guard broken!
    assert(res2.sfx_played);
    assert(res2.sfx_id == params.sfx_id); // Hit SFX played
    assert(target.reaction_timer == CombatHitApplyConfig::kHitReactionTimerDuration);
    assert(target.vel_y == -0x0002C000); // Knockback applied along facing Up
}

static void test_normal_hit_apply_and_ali_bypass() {
    HitSparkPool spark_pool{};
    EnemyGaugeState gauge{};

    // Target is Ali (should NOT latch gauge)
    CombatTargetEntity ali_target{};
    ali_target.ram_addr = CombatHitApplyConfig::kAliEntityRam;
    ali_target.pos_x = 50;
    ali_target.pos_y = 50;
    ali_target.pos_z = 0;
    ali_target.half_width_x = 10;
    ali_target.half_width_y = 10;
    ali_target.height_z = 20;

    CombatAttackerEntity enemy_attacker{};
    enemy_attacker.ram_addr = 0x00FF1D00U;
    enemy_attacker.is_ali = false;
    enemy_attacker.facing = 1U; // Right

    CombatHitApplyParams params{
        .damage_word = 0xC030U, // Tier 3
        .hitstop = 8U,
        .sfx_id = 0x00800220U,
        .world_box = {.x1 = 45, .y1 = 45, .x2 = 55, .y2 = 55, .z1 = 0, .z2 = 10},
    };

    auto res = apply_combat_hit(ali_target, enemy_attacker, gauge, spark_pool, params, true);
    assert(!res.gauge_latched);
    assert(gauge.timer == 0U);
    assert(!res.guard_deflected);
    assert(res.sfx_played);
    assert(res.sfx_id == 0x00800220U);
    assert(ali_target.reaction_timer == 6U);
    assert(ali_target.hitstop == 8U);
    assert(enemy_attacker.hitstop == 8U);
    assert(ali_target.vel_x == 0x00070000); // Facing 1 Right: +0x700 << 8
    assert(ali_target.vel_y == 0);
    assert(ali_target.vel_z == 0x00030000); // Tier 3 Z velocity
}

int main() {
    std::cout << "[oasis_m16_139_combat_hit_apply_test] Starting..." << std::endl;
    test_clamp_midpoint();
    test_enemy_gauge_latch();
    test_hit_spark_pool();
    test_knockback_profiles_and_directions();
    test_guard_deflection_and_break();
    test_normal_hit_apply_and_ali_bypass();
    std::cout << "[oasis_m16_139_combat_hit_apply_test] All tests passed!" << std::endl;
    return 0;
}
