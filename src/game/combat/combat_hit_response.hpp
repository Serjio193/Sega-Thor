#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace oasis::game::combat {

/// Configuration and constants for ROM Entity Combat Hit Response,
/// Knockback & Trajectory Physics Calculator (0x00BE98..0x00C012).
struct CombatHitResponseConfig {
    static constexpr std::uint32_t kCombatHitResponseTrackedAddr = 0x00BE98U;
    static constexpr std::uint32_t kCombatHitResponseDirectAddr = 0x00BEBAU;
    static constexpr std::uint32_t kUnblockedHitAddr = 0x00BED4U;
    static constexpr std::uint32_t kKnockbackDispatcherAddr = 0x00BF14U;
    static constexpr std::uint32_t kBlockedHitAddr = 0x00BFAEU;
    static constexpr std::uint32_t kExitRtsAddr = 0x00C010U;
    static constexpr std::uint32_t kHitSparkHookAddr = 0x00C012U;
    static constexpr std::uint32_t kProjectVelocityAddr = 0x00E2D4U;
    static constexpr std::uint32_t kPlaySfxAddr = 0x00E7FCU;
    static constexpr std::uint32_t kDurabilityHookAddr = 0x008530U;

    // RAM locations
    static constexpr std::uint32_t kHudTrackingTimerRam = 0x00FF184FU;
    static constexpr std::uint32_t kHudTrackingTargetRam = 0x00FF1850U;
    static constexpr std::uint32_t kAttackType198CRam = 0x00FF198CU;
    static constexpr std::uint32_t kPlayerEntityRam = 0x00FF19E8U;

    static constexpr std::uint8_t kDefaultHudTrackingTimer = 0x5AU; // 90 frames
    static constexpr std::uint32_t kShieldBlockSfx = 0x00900202U;

    // Slot field offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kFacingOffset = 0x16U;
    static constexpr std::size_t kFlags37Offset = 0x37U;
    static constexpr std::size_t kFlags38Offset = 0x38U;
    static constexpr std::size_t kFlags3AOffset = 0x3AU;
    static constexpr std::size_t kVelXOffset = 0x4EU;
    static constexpr std::size_t kVelYOffset = 0x52U;
    static constexpr std::size_t kVelZOffset = 0x56U;
    static constexpr std::size_t kGuard6DOffset = 0x6DU;
    static constexpr std::size_t kOwner7EOffset = 0x7EU;
    static constexpr std::size_t kCooldown82Offset = 0x82U;
    static constexpr std::size_t kReaction83Offset = 0x83U;
    static constexpr std::size_t kHitFacing84Offset = 0x84U;
    static constexpr std::size_t kDamage86Offset = 0x86U;
    static constexpr std::size_t kStun9DOffset = 0x9DU;
};

/// 4 knockback trajectory modes resolved from bits 14..15 of damage word.
enum class KnockbackMode : std::uint8_t {
    Light = 0,          // 0x0000 -> grounded (300, 2C0, 0) / airborne (400, 3C0, 28000)
    VerticalPopup = 1,  // 0x4000 -> (0, 0, 70000)
    Heavy = 2,          // 0x8000 -> (400, 3C0, 28000)
    Severe = 3          // 0xC000 -> (700, 680, 30000)
};

/// Knockback velocity and impulse vectors.
struct KnockbackVelocity {
    std::int32_t vel_x{0};
    std::int32_t vel_y{0};
    std::int32_t vel_z{0};
    bool air_juggle{false};
};

/// Entity state representation for combat hit response.
struct HitEntityState {
    std::uint32_t address{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint16_t facing{0};
    std::uint8_t flags_37{0};
    std::uint8_t flags_38{0};
    std::uint8_t flags_3a{0};
    std::uint8_t guard_6d{0};
    std::uint32_t owner_7e{0};
    std::uint8_t cooldown_82{0};
    std::uint8_t reaction_83{0};
    std::uint16_t hit_facing_84{0};
    std::uint16_t damage_86{0};
    std::uint8_t stun_9d{0};
    std::int32_t vel_x{0};
    std::int32_t vel_y{0};
    std::int32_t vel_z{0};
};

/// Result summary of hit response application.
struct HitResponseResult {
    bool blocked{false};
    bool guard_broken{false};
    bool hud_tracked{false};
    bool durability_reduced{false};
    std::uint32_t sfx_played{0};
    KnockbackMode knockback_mode{KnockbackMode::Light};
    KnockbackVelocity knockback{};
};

/// Checks if target is facing attacker with shield: attacker_facing ^ 2 == target_facing.
[[nodiscard]] constexpr bool is_shield_facing(std::uint16_t attacker_facing, std::uint16_t target_facing) noexcept {
    return (attacker_facing ^ 2U) == target_facing;
}

/// Resolves knockback mode from damage word: (rol16(damage_word, 3) & 6) >> 1.
[[nodiscard]] constexpr KnockbackMode resolve_knockback_mode(std::uint16_t damage_word) noexcept {
    const std::uint16_t rol3 = static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(damage_word) << 3U) |
        (static_cast<std::uint32_t>(damage_word) >> 13U));
    const std::uint16_t mode_idx = static_cast<std::uint16_t>((rol3 & 6U) >> 1U);
    return static_cast<KnockbackMode>(mode_idx);
}

/// Projects horizontal speed vector in 8-way direction matching ROM 0x00E2D4.
[[nodiscard]] std::pair<std::int32_t, std::int32_t> project_velocity_8way(
    std::uint16_t h_speed_x,
    std::uint16_t h_speed_y,
    std::uint16_t direction) noexcept;

/// Calculates 3D knockback physics based on mode, attacker facing, and airborne status.
[[nodiscard]] KnockbackVelocity calculate_knockback(
    KnockbackMode mode,
    std::uint16_t attacker_facing,
    bool is_airborne) noexcept;

/// Applies combat hit response to target and attacker entities.
HitResponseResult apply_combat_hit_response(
    HitEntityState& target,
    HitEntityState& attacker,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx,
    bool enable_hud_tracking = true);

/// Ingress 1: combat_hit_response_tracked (0x00BE98).
inline HitResponseResult combat_hit_response_tracked(
    HitEntityState& target,
    HitEntityState& attacker,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx) {
    return apply_combat_hit_response(target, attacker, packed_a4, attack_type, hit_sfx, true);
}

/// Ingress 2: combat_hit_response_direct (0x00BEBA).
inline HitResponseResult combat_hit_response_direct(
    HitEntityState& target,
    HitEntityState& attacker,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx) {
    return apply_combat_hit_response(target, attacker, packed_a4, attack_type, hit_sfx, false);
}

/// Raw 64KB RAM window integration helpers.
HitEntityState read_hit_entity_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr);

void write_hit_entity_raw(
    std::span<std::uint8_t> ram,
    const HitEntityState& entity);

HitResponseResult apply_combat_hit_response_tracked_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t target_addr,
    std::uint32_t attacker_addr,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx);

HitResponseResult apply_combat_hit_response_direct_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t target_addr,
    std::uint32_t attacker_addr,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx);

} // namespace oasis::game::combat
