#include "game/combat/projectile_hit_knockback.hpp"

#include <cassert>
#include <iostream>

using namespace oasis::game::combat;

static void test_projectile_z_gravity_exact() {
    // Exact match with Python reference test
    auto [z1, vz1, hit1] = step_projectile_z_gravity(0x00020000, 0x00004000);
    assert(!hit1);
    assert(z1 == 0x0001E000);
    assert(vz1 == -0x00002000);

    auto [z2, vz2, hit2] = step_projectile_z_gravity(0x00001000, 0x00000000);
    assert(hit2);
    assert(z2 == 0);
    assert(vz2 == 0);
}

static void test_target_knockback_launch() {
    // Normal knockback, dir 1 (East)
    auto res_norm = apply_projectile_target_knockback(
        0x0012U, 1U, false, 0, 0x00FF2000U);
    assert(res_norm.hit_stun_83 == 10U);
    assert(res_norm.status_9d == 0U);
    assert(res_norm.dir_84 == 1U);
    assert(res_norm.vx_16_16 == 0x00040000);
    assert(res_norm.vy_16_16 == 0);
    assert(res_norm.vz_16_16 == 0x00020000);
    assert(!res_norm.airborne_flag_set);
    assert(!res_norm.enemy_gauge_triggered);
    assert(res_norm.hit_sfx == 0x00800210U);

    // Light knockback, dir 1 (East)
    auto res_light = apply_projectile_target_knockback(
        0x0012U, 1U, true, 0, 0x00FF2000U);
    assert(res_light.vx_16_16 == 0x00020000);

    // Target airborne (z > 0) sets bit 15 on damage attribute 0x86
    auto res_air = apply_projectile_target_knockback(
        0x0005U, 0U, false, 0x00010000, 0x00FF2000U);
    assert(res_air.airborne_flag_set);
    assert(res_air.damage_attr_86 == 0x8005U);

    // Attacker is Ali ($FF19E8) triggers 90-frame HUD enemy gauge timer
    auto res_ali = apply_projectile_target_knockback(
        0x000AU, 2U, false, 0, kAliAttackerPtr);
    assert(res_ali.enemy_gauge_triggered);
    assert(res_ali.enemy_gauge_timer == 90U);
}

static void test_attach_projectile_latch() {
    // Normal knockback attaches latch (8 frames, state 6)
    auto res = attach_projectile_latch(
        100, 200, 15,
        95, 192, 10,
        false);
    assert(res.latched);
    assert(res.local_x_ae == 5);
    assert(res.local_y_b0 == 8);
    assert(res.local_z_b2 == 5);
    assert(res.state_2a == 6U);
    assert(res.latch_timer_b8 == 8U);

    // Light knockback does not latch
    auto res_light = attach_projectile_latch(
        100, 200, 15,
        95, 192, 10,
        true);
    assert(!res_light.latched);
}

static void test_flight_gravity_and_culling() {
    // Midair inside screen (cam 0,0, pos 160, 112)
    auto res_air = step_projectile_flight_gravity(
        0x00050000, 0x00010000, 0x00, 160, 112, 0, 0);
    assert(!res_air.grounded);
    assert(!res_air.despawned);
    assert(res_air.z_16_16 == (0x00050000 + (0x00010000 - 0x00006000)));

    // Midair offscreen with 10px margin (pos 350 > 320 + 10)
    auto res_off = step_projectile_flight_gravity(
        0x00050000, 0x00010000, 0x00, 350, 112, 0, 0);
    assert(res_off.despawned);

    // Ground hit with flags_6e bit 7 -> despawn
    auto res_g_despawn = step_projectile_flight_gravity(
        0x00001000, 0x00000000, 0x80, 160, 112, 0, 0);
    assert(res_g_despawn.grounded);
    assert(res_g_despawn.despawned);

    // Ground hit without flags_6e bit 7 -> drop timer 5 frames, state 6
    auto res_g_drop = step_projectile_flight_gravity(
        0x00001000, 0x00000000, 0x00, 160, 112, 0, 0);
    assert(res_g_drop.grounded);
    assert(!res_g_drop.despawned);
    assert(res_g_drop.transitioned_to_drop);
    assert(res_g_drop.state_2a == 6U);
    assert(res_g_drop.drop_timer_b8 == 5U);
}

int main() {
    test_projectile_z_gravity_exact();
    test_target_knockback_launch();
    test_attach_projectile_latch();
    test_flight_gravity_and_culling();
    std::cout << "oasis_m16_129_projectile_hit_knockback_test passed\n";
    return 0;
}
