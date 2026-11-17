#include "projectile_latch_burst.hpp"

#include <cstdlib>

namespace oasis::game::combat {

DeathBurstResult step_death_explosion_burst(
    DeathBurstState state,
    std::uint16_t prng_x,
    std::uint16_t prng_y) noexcept {
    DeathBurstResult result{};

    // ROM 0x00EA16: btst.b #1, $37(a6)
    if ((state.flags_37 & 0x02U) == 0U) {
        result.next_state = state;
        return result;
    }

    // ROM 0x00EA20..0x00EA2E: timer countdown; 0 clears entity slot
    if (state.burst_timer_9e == 0U) {
        state.active = false;
        result.next_state = state;
        result.destroyed = true;
        return result;
    }

    state.burst_timer_9e -= 1U;
    if (state.burst_timer_9e == 0U) {
        state.active = false;
        result.next_state = state;
        result.destroyed = true;
        return result;
    }

    // ROM 0x00EA32..0x00EA58: (prng & 0x1F) - 16 jitter
    const auto jx = static_cast<std::int16_t>(
        static_cast<std::int16_t>(prng_x & kBurstJitterMask) + kBurstJitterBias);
    const auto jy = static_cast<std::int16_t>(
        static_cast<std::int16_t>(prng_y & kBurstJitterMask) + kBurstJitterBias);

    state.current_x = static_cast<std::int16_t>(state.base_x_4e + jx);
    state.current_y = static_cast<std::int16_t>(state.base_y_52 + jy);

    result.next_state = state;
    result.destroyed = false;
    result.sfx_triggered = true;
    result.sfx_1 = kBurstSfx1;
    result.sfx_2 = kBurstSfx2;

    return result;
}

LatchFollowResult step_carried_latch_follow(
    LatchFollowState state,
    bool parent_valid,
    std::int16_t parent_x,
    std::int16_t parent_y,
    std::int16_t parent_z) noexcept {
    LatchFollowResult result{};

    // ROM 0x00EA7E..0x00EA88: timer == 0 transitions to probe; timer == 1 expires
    if (state.latch_timer_b8 == 0U) {
        result.next_state = state;
        result.action = LatchFollowAction::TransitionToProbe;
        return result;
    }

    if (state.latch_timer_b8 == 1U) {
        state.latch_timer_b8 = 0U;
        result.next_state = state;
        result.action = LatchFollowAction::DespawnExpired;
        return result;
    }

    state.latch_timer_b8 -= 1U;

    // ROM 0x00EA92..0x00EABA: parent null check & coordinate binding
    if (!parent_valid) {
        result.next_state = state;
        result.action = LatchFollowAction::DetachedOrNoParent;
        return result;
    }

    state.pos_x = static_cast<std::int16_t>(parent_x + state.local_x_ae);
    state.pos_y = static_cast<std::int16_t>(parent_y + state.local_y_b0);
    state.pos_z = static_cast<std::int16_t>(parent_z + state.local_z_b2);

    result.next_state = state;
    result.action = LatchFollowAction::Following;
    return result;
}

bool is_in_projectile_probe_box(
    std::int16_t proj_x, std::int16_t proj_y, std::int16_t proj_z,
    std::int16_t target_x, std::int16_t target_y, std::int16_t target_z) noexcept {
    const auto dx = std::abs(static_cast<int>(target_x) - static_cast<int>(proj_x));
    const auto dy = std::abs(static_cast<int>(target_y) - static_cast<int>(proj_y));
    const auto dz = static_cast<int>(target_z) - static_cast<int>(proj_z);

    return (dx <= kProjProbeHalfExtentX) &&
           (dy <= kProjProbeHalfExtentY) &&
           (dz >= -kProjProbeHalfExtentZ && dz <= kProjProbeHalfExtentZ);
}

ProjectileProbeResult evaluate_projectile_probe(
    const ProjectileProbeInput& input,
    std::optional<ProbeTargetEntity> target) noexcept {
    ProjectileProbeResult result{};

    // ROM 0x00EAC0: already impacted / animating
    if (input.impact_timer_86 != 0U) {
        result.outcome = ProjectileProbeOutcome::AlreadyImpacted;
        return result;
    }

    // ROM 0x00EAC8..0x00EAF2: terrain wall collision
    const auto terrain = static_cast<std::uint8_t>(
        input.terrain_flags_40 | input.terrain_flags_41);
    if ((terrain & 0x40U) != 0U) {
        if ((terrain & 0x08U) != 0U || input.anim_timer_16 == 2U) {
            result.outcome = ProjectileProbeOutcome::TerrainDespawn;
            return result;
        }
        result.outcome = ProjectileProbeOutcome::WallBounce;
        result.sfx_triggered = true;
        result.sfx = kProjBounceSfx;
        return result;
    }

    if (!target.has_value()) {
        result.outcome = ProjectileProbeOutcome::NoTarget;
        return result;
    }

    auto tgt = *target;

    // ROM 0x00EB10: shield block (bit 1 of +0x3A)
    if ((tgt.trait_flags_3a & 0x02U) != 0U) {
        result.outcome = ProjectileProbeOutcome::ShieldBounce;
        result.sfx_triggered = true;
        result.sfx = kProjBounceSfx;
        return result;
    }

    // ROM 0x00EB1A: intangible / Shade phase pass (bit 2 of +0x38)
    if ((tgt.render_flags_38 & 0x04U) != 0U) {
        result.outcome = ProjectileProbeOutcome::IntangiblePass;
        return result;
    }

    // ROM 0x00EB24..0x00EB32: invulnerable (bit 3 of +0x38) or hitstun (+0x82 != 0)
    if ((tgt.render_flags_38 & 0x08U) != 0U || tgt.hitstun_flag_82 != 0U) {
        result.outcome = ProjectileProbeOutcome::InvulnerableSpark;
        return result;
    }

    // ROM 0x00EB36..0x00EB4C: payload transfer to target mailbox
    tgt.damage_mailbox_7e = input.payload_aa;
    tgt.hitstun_flag_82 = 0U;
    tgt.damage_attr_86 = input.damage_attr_a8;
    tgt.hitstun_timer_83 = kDefaultHitstunFrames;
    tgt.status_flag_9d = 0U;

    result.outcome = ProjectileProbeOutcome::HitApplied;
    result.target_modified = true;
    result.modified_target = tgt;

    return result;
}

} // namespace oasis::game::combat
