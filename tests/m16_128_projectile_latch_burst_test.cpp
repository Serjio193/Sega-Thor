#include "game/combat/projectile_latch_burst.hpp"

#include <cassert>
#include <iostream>

using namespace oasis::game::combat;

static void test_death_burst_jitter_and_expiry() {
    DeathBurstState state{
        .burst_timer_9e = 5U,
        .flags_37 = 0x02U,
        .base_x_4e = 100,
        .base_y_52 = 200,
        .current_x = 100,
        .current_y = 200,
        .active = true
    };

    // Flags_37 bit 1 not set -> inactive
    state.flags_37 = 0x00U;
    auto noop_res = step_death_explosion_burst(state, 0x00, 0x1F);
    assert(!noop_res.sfx_triggered);
    assert(!noop_res.destroyed);

    // Flags_37 bit 1 set -> step 5 to 4 with exact jitter matching python reference:
    // jx = (0 & 0x1F) - 16 = -16 -> 84
    // jy = (31 & 0x1F) - 16 = +15 -> 215
    state.flags_37 = 0x02U;
    auto step_res = step_death_explosion_burst(state, 0x00, 0x1F);
    assert(!step_res.destroyed);
    assert(step_res.sfx_triggered);
    assert(step_res.sfx_1 == 0x0090021BU);
    assert(step_res.sfx_2 == 0x00600382U);
    assert(step_res.next_state.burst_timer_9e == 4U);
    assert(step_res.next_state.current_x == 84);
    assert(step_res.next_state.current_y == 215);

    // Countdown reaches 0 (from 1) -> destroyed & inactive, no SFX
    state.burst_timer_9e = 1U;
    auto exp_res = step_death_explosion_burst(state, 0x10, 0x10);
    assert(exp_res.destroyed);
    assert(!exp_res.next_state.active);
    assert(!exp_res.sfx_triggered);
}

static void test_latch_follow_binding() {
    LatchFollowState state{
        .latch_timer_b8 = 8U,
        .local_x_ae = -4,
        .local_y_b0 = 6,
        .local_z_b2 = 2,
        .pos_x = 0,
        .pos_y = 0,
        .pos_z = 0
    };

    // Step 8 -> 7 with parent (120, 80, 10)
    auto res = step_carried_latch_follow(state, true, 120, 80, 10);
    assert(res.action == LatchFollowAction::Following);
    assert(res.next_state.latch_timer_b8 == 7U);
    assert(res.next_state.pos_x == 116);
    assert(res.next_state.pos_y == 86);
    assert(res.next_state.pos_z == 12);

    // Timer == 1 expires
    state.latch_timer_b8 = 1U;
    auto exp_res = step_carried_latch_follow(state, true, 120, 80, 10);
    assert(exp_res.action == LatchFollowAction::DespawnExpired);

    // Timer == 0 transitions to probe
    state.latch_timer_b8 = 0U;
    auto probe_res = step_carried_latch_follow(state, true, 120, 80, 10);
    assert(probe_res.action == LatchFollowAction::TransitionToProbe);

    // Parent invalid
    state.latch_timer_b8 = 5U;
    auto detach_res = step_carried_latch_follow(state, false, 0, 0, 0);
    assert(detach_res.action == LatchFollowAction::DetachedOrNoParent);
}

static void test_probe_box_extent() {
    assert(is_in_projectile_probe_box(100, 100, 10, 106, 94, 12));  // dx=6, dy=6, dz=2 -> in
    assert(!is_in_projectile_probe_box(100, 100, 10, 107, 100, 10)); // dx=7 -> out
    assert(!is_in_projectile_probe_box(100, 100, 10, 100, 107, 10)); // dy=7 -> out
    assert(!is_in_projectile_probe_box(100, 100, 10, 100, 100, 13)); // dz=3 -> out
    assert(!is_in_projectile_probe_box(100, 100, 10, 100, 100, 7));  // dz=-3 -> out
}

static void test_probe_wall_and_hit_outcomes() {
    ProjectileProbeInput input{
        .impact_timer_86 = 0U,
        .terrain_flags_40 = 0x00U,
        .terrain_flags_41 = 0x00U,
        .anim_timer_16 = 0U,
        .payload_aa = 0x12345678U,
        .damage_attr_a8 = 0x000CU
    };

    // 1. Already impacted
    input.impact_timer_86 = 5U;
    assert(evaluate_projectile_probe(input, std::nullopt).outcome ==
           ProjectileProbeOutcome::AlreadyImpacted);
    input.impact_timer_86 = 0U;

    // 2. Wall bounce (bit 6 set) -> SFX 0x00700378
    input.terrain_flags_40 = 0x40U;
    auto wall_res = evaluate_projectile_probe(input, std::nullopt);
    assert(wall_res.outcome == ProjectileProbeOutcome::WallBounce);
    assert(wall_res.sfx_triggered);
    assert(wall_res.sfx == 0x00700378U);

    // 3. Wall despawn (bit 6 + bit 3 set)
    input.terrain_flags_40 = 0x48U;
    assert(evaluate_projectile_probe(input, std::nullopt).outcome ==
           ProjectileProbeOutcome::TerrainDespawn);
    input.terrain_flags_40 = 0x00U;

    // 4. No target
    assert(evaluate_projectile_probe(input, std::nullopt).outcome ==
           ProjectileProbeOutcome::NoTarget);

    ProbeTargetEntity target{};

    // 5. Shield bounce (bit 1 of 0x3A) -> SFX 0x00700378
    target.trait_flags_3a = 0x02U;
    auto shield_res = evaluate_projectile_probe(input, target);
    assert(shield_res.outcome == ProjectileProbeOutcome::ShieldBounce);
    assert(shield_res.sfx_triggered);
    assert(shield_res.sfx == 0x00700378U);
    target.trait_flags_3a = 0x00U;

    // 6. Intangible pass (bit 2 of 0x38)
    target.render_flags_38 = 0x04U;
    assert(evaluate_projectile_probe(input, target).outcome ==
           ProjectileProbeOutcome::IntangiblePass);

    // 7. Invulnerable spark (bit 3 of 0x38 or hitstun 0x82 != 0)
    target.render_flags_38 = 0x08U;
    assert(evaluate_projectile_probe(input, target).outcome ==
           ProjectileProbeOutcome::InvulnerableSpark);
    target.render_flags_38 = 0x00U;
    target.hitstun_flag_82 = 1U;
    assert(evaluate_projectile_probe(input, target).outcome ==
           ProjectileProbeOutcome::InvulnerableSpark);
    target.hitstun_flag_82 = 0U;

    // 8. Hit applied!
    auto hit_res = evaluate_projectile_probe(input, target);
    assert(hit_res.outcome == ProjectileProbeOutcome::HitApplied);
    assert(hit_res.target_modified);
    assert(hit_res.modified_target.damage_mailbox_7e == 0x12345678U);
    assert(hit_res.modified_target.damage_attr_86 == 0x000CU);
    assert(hit_res.modified_target.hitstun_timer_83 == 10U);
    assert(hit_res.modified_target.status_flag_9d == 0U);
}

int main() {
    test_death_burst_jitter_and_expiry();
    test_latch_follow_binding();
    test_probe_box_extent();
    test_probe_wall_and_hit_outcomes();
    std::cout << "oasis_m16_128_projectile_latch_burst_test passed\n";
    return 0;
}
