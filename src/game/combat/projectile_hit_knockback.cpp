#include "projectile_hit_knockback.hpp"

#include "game/entities/velocity_vector_math.hpp"
#include "game/entities/viewport_culler_landing.hpp"

namespace oasis::game::combat {

std::tuple<std::int32_t, std::int32_t, bool> step_projectile_z_gravity(
    std::int32_t z_16_16,
    std::int32_t vz_16_16) noexcept {
    vz_16_16 -= kProjectileGravity16_16;
    z_16_16 += vz_16_16;
    if (z_16_16 < 0) {
        return {0, 0, true};
    }
    return {z_16_16, vz_16_16, false};
}

TargetKnockbackResult apply_projectile_target_knockback(
    std::uint16_t damage_attr_a8,
    std::uint8_t proj_dir,
    bool is_light_knockback,
    std::int32_t target_z,
    std::uint32_t attacker_payload_aa) noexcept {
    TargetKnockbackResult res{};

    res.damage_attr_86 = damage_attr_a8;
    res.hit_stun_83 = kTargetHitStunFrames;
    res.status_9d = 0U;
    res.dir_84 = static_cast<std::uint16_t>(proj_dir);

    const auto speed_x = is_light_knockback ? kKbLightSpeedX : kKbNormalSpeedX;
    const auto speed_y = is_light_knockback ? kKbLightSpeedY : kKbNormalSpeedY;
    const auto vel = oasis::game::entities::compute_8dir_velocity(speed_x, speed_y, proj_dir);

    res.vx_16_16 = vel.vx;
    res.vy_16_16 = vel.vy;
    res.vz_16_16 = kKbZVel16_16;

    // ROM 0x00EB82..0x00EB88: target airborne hit bit 15
    if (target_z > 0) {
        res.damage_attr_86 |= 0x8000U;
        res.airborne_flag_set = true;
    }

    // ROM 0x00EB8E..0x00EBA0: Ali attacker HUD gauge
    if (attacker_payload_aa == kAliAttackerPtr) {
        res.enemy_gauge_triggered = true;
        res.enemy_gauge_timer = kEnemyGaugeTimerFrames;
    }

    res.hit_sfx = kProjectileHitSfx;
    return res;
}

ProjectileHitLatchResult attach_projectile_latch(
    std::int16_t proj_x, std::int16_t proj_y, std::int16_t proj_z,
    std::int16_t target_x, std::int16_t target_y, std::int16_t target_z,
    bool is_light_knockback) noexcept {
    ProjectileHitLatchResult res{};

    // ROM 0x00EBB0: light knockback skips latching
    if (is_light_knockback) {
        return res;
    }

    // ROM 0x00EC0C..0x00EC40: local relative XYZ and 8f latch timer
    res.latched = true;
    res.local_x_ae = static_cast<std::int16_t>(proj_x - target_x);
    res.local_y_b0 = static_cast<std::int16_t>(proj_y - target_y);
    res.local_z_b2 = static_cast<std::int16_t>(proj_z - target_z);
    res.state_2a = kLatchedCarryState;
    res.latch_timer_b8 = kLatchHitTimerFrames;

    return res;
}

ProjectileFlightGravityResult step_projectile_flight_gravity(
    std::int32_t z_16_16,
    std::int32_t vz_16_16,
    std::uint8_t flags_6e,
    std::int16_t px,
    std::int16_t py,
    std::int16_t cam_x,
    std::int16_t cam_y) noexcept {
    ProjectileFlightGravityResult res{};

    const auto [nz, nvz, grounded] = step_projectile_z_gravity(z_16_16, vz_16_16);
    res.z_16_16 = nz;
    res.vz_16_16 = nvz;
    res.grounded = grounded;

    if (grounded) {
        // ROM 0x00EBF4..0x00EBFC: ground impact
        if ((flags_6e & 0x80U) != 0U) {
            res.despawned = true;
        } else {
            res.transitioned_to_drop = true;
            res.state_2a = kLatchedCarryState;
            res.drop_timer_b8 = kDropTimerFrames;
        }
    } else {
        // ROM 0x00EC00..0x00EC06: offscreen culling with 10px margin
        const auto z_px = static_cast<std::int16_t>(nz >> 16);
        if (oasis::game::entities::is_offscreen_with_z(
                px, py, z_px, cam_x, cam_y, kProjectileCullMarginPx)) {
            res.despawned = true;
        }
    }

    return res;
}

} // namespace oasis::game::combat
