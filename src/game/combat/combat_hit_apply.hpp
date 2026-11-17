#pragma once

#include "game/combat/weapon_hitbox_builder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>

namespace oasis::game::combat {

struct CombatHitApplyConfig {
    static constexpr std::uint32_t kEnemyGaugeLatchAddr = 0x00BE98U;
    static constexpr std::uint32_t kHitApplyDirectAddr = 0x00BEBAU;
    static constexpr std::uint32_t kGuardDeflectAddr = 0x00BFAEU;
    static constexpr std::uint32_t kHitSparkSpawnAddr = 0x00C012U;
    static constexpr std::uint32_t kSpecialRecoilCancelAddr = 0x008530U;
    static constexpr std::uint32_t kSoundEffectPlayAddr = 0x00E7FCU;

    static constexpr std::uint32_t kAliEntityRam = 0x00FF19E8U;
    static constexpr std::uint32_t kEnemyGaugeTimerRam = 0x00FF184FU;
    static constexpr std::uint32_t kEnemyGaugeTargetPtrRam = 0x00FF1850U;
    static constexpr std::uint32_t kAliAttackIndexRam = 0x00FF198CU;
    static constexpr std::uint32_t kHitSparkPoolRam = 0x00FF19C8U;

    static constexpr std::size_t kHitSparkSlotCount = 4U;
    static constexpr std::size_t kHitSparkSlotStrideBytes = 8U;
    static constexpr std::uint16_t kHitSparkType = 2U;
    static constexpr std::uint16_t kHitSparkTimerFrames = 5U;

    static constexpr std::uint32_t kBlockSfxId = 0x00900202U;
    static constexpr std::uint8_t kEnemyGaugeTimerDuration = 0x5AU; // 90 frames
    static constexpr std::uint8_t kHitReactionTimerDuration = 6U;
};

struct HitSparkSlot {
    std::uint16_t type{0U};  // +0x00: 0 = free, 2 = active spark
    std::int16_t pos_x{0};   // +0x02: screen X (mid_x)
    std::int16_t pos_y{0};   // +0x04: screen Y (mid_y - mid_z)
    std::uint16_t timer{0U}; // +0x06: duration (5 frames)
};

struct HitSparkPool {
    std::array<HitSparkSlot, CombatHitApplyConfig::kHitSparkSlotCount> slots{};
};

struct EnemyGaugeState {
    std::uint8_t timer{0U};      // $FF184F
    std::uint32_t target_ptr{0U}; // $FF1850
};

struct KnockbackProfile {
    std::uint8_t tier{0U};
    std::uint16_t vx_mag{0U};
    std::uint16_t vy_mag{0U};
    std::int32_t vz{0};
    bool airborne_fallback{false};
};

struct CombatTargetEntity {
    std::uint32_t ram_addr{0U};
    std::int16_t pos_x{0};        // +0x08
    std::int16_t pos_y{0};        // +0x0C
    std::int32_t pos_z{0};        // +0x10: 32-bit Z position
    std::int16_t elevation{0};    // +0x14: elevation bias
    std::uint16_t facing{0U};     // +0x16: 0..3 (Up, Right, Down, Left)
    std::uint8_t flags_3a{0U};    // +0x3A: bit 0 hides gauge
    std::int16_t half_width_x{0}; // +0x42: rx
    std::int16_t half_width_y{0}; // +0x44: ry
    std::int16_t height_z{0};     // +0x4A: hz
    std::int32_t vel_x{0};        // +0x4E: 16.16 fixed-point velocity X
    std::int32_t vel_y{0};        // +0x52: 16.16 fixed-point velocity Y
    std::int32_t vel_z{0};        // +0x56: 16.16 fixed-point velocity Z
    std::uint8_t guard{0U};       // +0x6D: shield / guard active if != 0
    std::uint32_t last_attacker{0U}; // +0x7E
    std::uint8_t hitstop{0U};     // +0x82
    std::uint8_t reaction_timer{0U}; // +0x83
    std::uint16_t hit_facing{0U}; // +0x84
    std::uint16_t damage_word{0U};// +0x86
    std::uint8_t field_9d{0U};    // +0x9D
};

struct CombatAttackerEntity {
    std::uint32_t ram_addr{0U};
    bool is_ali{false};
    std::uint16_t facing{0U};     // +0x16: 0: Up, 1: Right, 2: Down, 3: Left
    std::uint8_t flags_37{0U};    // +0x37: bit 3 audio play / block marker
    std::uint8_t hitstop{0U};     // +0x82
};

struct CombatHitApplyParams {
    std::uint16_t damage_word{0U};
    std::uint8_t hitstop{0U};
    std::uint32_t sfx_id{0U};
    std::uint16_t attack_index{0U}; // $FF198C
    WorldHitbox3D world_box{};
};

struct CombatHitApplyResult {
    bool gauge_latched{false};
    bool guard_deflected{false};
    bool guard_broken{false};
    bool spark_spawned{false};
    std::size_t spark_slot_index{0U};
    std::int16_t spark_x{0};
    std::int16_t spark_y{0};
    bool sfx_played{false};
    std::uint32_t sfx_id{0U};
    bool special_recoil_cancelled{false};
    KnockbackProfile knockback{};
};

/// ROM 0x00C054..0x00C0AC:
/// Clamped midpoint of two 1D intervals: lo + ((hi - lo) >> 1) where lo = max(a_min, b_min), hi = min(a_max, b_max).
[[nodiscard]] constexpr std::int16_t clamp_midpoint(
    std::int16_t a_min,
    std::int16_t a_max,
    std::int16_t b_min,
    std::int16_t b_max) noexcept {
    const auto lo = std::max(a_min, b_min);
    const auto hi = std::min(a_max, b_max);
    return static_cast<std::int16_t>(lo + ((hi - lo) >> 1));
}

/// ROM 0x00C012..0x00C0BC:
/// Finds first free slot in 4-slot pool ($FF19C8), calculates clamped 3D AABB midpoint,
/// and stores spark (type=2, timer=5, x=mid_x, y=mid_y - mid_z).
[[nodiscard]] std::pair<bool, std::size_t> spawn_hit_spark(
    HitSparkPool& pool,
    const WorldHitbox3D& att_box,
    const CombatTargetEntity& target) noexcept;

/// ROM 0x00BE98..0x00BEB8:
/// Latches enemy gauge if target is not Ali and flag bit 3A.0 is clear.
[[nodiscard]] bool update_enemy_gauge_latch(
    EnemyGaugeState& gauge,
    const CombatTargetEntity& target) noexcept;

/// ROM 0x00BF14..0x00BFAC:
/// Evaluates knockback tier (0..3), airborne fallback to tier 2 with bit 15 set,
/// and direction velocities via 0x00E2D4 cardinal mapping.
[[nodiscard]] std::tuple<KnockbackProfile, std::int32_t, std::int32_t, std::int32_t, std::uint16_t>
compute_knockback(
    std::uint16_t damage_word,
    bool is_airborne,
    std::uint16_t attacker_facing) noexcept;

/// ROM 0x00BE98..0x00C0BC:
/// Full hit-apply routine handling enemy gauge latch, guard deflection, special recoil cancel,
/// hit-spark spawning, SFX trigger, damage/hitstop recording, and 4-way knockback application.
[[nodiscard]] CombatHitApplyResult apply_combat_hit(
    CombatTargetEntity& target,
    CombatAttackerEntity& attacker,
    EnemyGaugeState& gauge,
    HitSparkPool& spark_pool,
    const CombatHitApplyParams& params,
    bool check_enemy_gauge = true) noexcept;

} // namespace oasis::game::combat
