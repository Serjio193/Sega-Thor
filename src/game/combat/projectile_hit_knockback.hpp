#pragma once

#include <cstdint>
#include <tuple>

namespace oasis::game::combat {

inline constexpr std::uint32_t kProjectileHitKnockbackAddr = 0x00EB40U;
inline constexpr std::uint32_t kProjectileFlightGravityAddr = 0x00EBC8U;
inline constexpr std::uint32_t kProjectileLatchAttachAddr = 0x00EC0CU;
inline constexpr std::uint32_t kProjectileDropRicochetAddr = 0x00EC42U;
inline constexpr std::uint32_t kProjectileDespawnAddr = 0x00EC54U;

inline constexpr std::uint8_t kTargetHitStunFrames = 10U;
inline constexpr std::uint16_t kKbNormalSpeedX = 0x0400U;
inline constexpr std::uint16_t kKbNormalSpeedY = 0x03C0U;
inline constexpr std::uint16_t kKbLightSpeedX = 0x0200U;
inline constexpr std::uint16_t kKbLightSpeedY = 0x01E0U;
inline constexpr std::int32_t kKbZVel16_16 = 0x00020000; // +2.0 px/frame
inline constexpr std::uint8_t kEnemyGaugeTimerFrames = 90U; // 0x5A
inline constexpr std::uint32_t kAliAttackerPtr = 0x00FF19E8U;
inline constexpr std::uint32_t kProjectileHitSfx = 0x00800210U;
inline constexpr std::int32_t kProjectileGravity16_16 = 0x00006000; // 0.375 px/frame^2
inline constexpr std::int16_t kProjectileCullMarginPx = 10;
inline constexpr std::uint8_t kLatchHitTimerFrames = 8U;
inline constexpr std::uint8_t kDropTimerFrames = 5U;
inline constexpr std::uint16_t kLatchedCarryState = 6U;

struct TargetKnockbackResult {
    std::uint16_t damage_attr_86{0U};
    std::uint8_t hit_stun_83{kTargetHitStunFrames};
    std::uint8_t status_9d{0U};
    std::uint16_t dir_84{0U};
    std::int32_t vx_16_16{0};
    std::int32_t vy_16_16{0};
    std::int32_t vz_16_16{kKbZVel16_16};
    bool airborne_flag_set{false};
    bool enemy_gauge_triggered{false};
    std::uint8_t enemy_gauge_timer{0U};
    std::uint32_t hit_sfx{kProjectileHitSfx};
};

struct ProjectileHitLatchResult {
    bool latched{false};
    std::int16_t local_x_ae{0};
    std::int16_t local_y_b0{0};
    std::int16_t local_z_b2{0};
    std::uint16_t state_2a{0U};
    std::uint8_t latch_timer_b8{0U};
};

struct ProjectileFlightGravityResult {
    std::int32_t z_16_16{0};
    std::int32_t vz_16_16{0};
    bool grounded{false};
    bool despawned{false};
    bool transitioned_to_drop{false};
    std::uint16_t state_2a{0U};
    std::uint8_t drop_timer_b8{0U};
};

/// Models 0x00EBD8..0x00EBF2: steps projectile Z gravity (vz -= 0x00006000, clamps ground at 0).
[[nodiscard]] std::tuple<std::int32_t, std::int32_t, bool> step_projectile_z_gravity(
    std::int32_t z_16_16,
    std::int32_t vz_16_16) noexcept;

/// Models 0x00EB40..0x00EBAC: calculates knockback velocities, facing, 10f stun,
/// airborne bit 15 flag on +0x86 if target Z > 0, and 90f enemy gauge if attacker == Ali.
[[nodiscard]] TargetKnockbackResult apply_projectile_target_knockback(
    std::uint16_t damage_attr_a8,
    std::uint8_t proj_dir,
    bool is_light_knockback,
    std::int32_t target_z,
    std::uint32_t attacker_payload_aa) noexcept;

/// Models 0x00EBB0..0x00EC40: binds local relative XYZ offset and attaches latch (8f)
/// if not light knockback.
[[nodiscard]] ProjectileHitLatchResult attach_projectile_latch(
    std::int16_t proj_x, std::int16_t proj_y, std::int16_t proj_z,
    std::int16_t target_x, std::int16_t target_y, std::int16_t target_z,
    bool is_light_knockback) noexcept;

/// Models 0x00EBC8..0x00EC0A: full projectile Z gravity step, ground collision check
/// with flags_6e bit 7, and offscreen culling with 10px margin.
[[nodiscard]] ProjectileFlightGravityResult step_projectile_flight_gravity(
    std::int32_t z_16_16,
    std::int32_t vz_16_16,
    std::uint8_t flags_6e,
    std::int16_t px,
    std::int16_t py,
    std::int16_t cam_x,
    std::int16_t cam_y) noexcept;

} // namespace oasis::game::combat
