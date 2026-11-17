#pragma once

#include <cstdint>

namespace oasis::game::combat {

// ROM 0x00BF26: Reaction types dispatched on unblocked contact.
enum class CombatReactionType : std::uint8_t {
    Knockback0 = 0,
    Stun2 = 2,
    Launch4 = 4,
    Heavy6 = 6,
};

struct CombatHitboxConfig {
    static constexpr std::uint32_t kPlayerAttackEntry = 0x0000BCBAU;
    static constexpr std::uint32_t kEnemyAttackEntry = 0x0000BCD8U;
    static constexpr std::uint32_t kKillRankBonusAddr = 0x0000BD40U;
    static constexpr std::uint32_t kPlayerContactCb = 0x0000BE98U;
    static constexpr std::uint32_t kEnemyContactCb = 0x0000BEBAU;
    static constexpr std::uint32_t kShieldBlockTarget = 0x0000BFAEU;
    static constexpr std::uint32_t kReactionTableAddr = 0x0000BF26U;

    static constexpr std::uint8_t kEnemyHpHudFrames = 90U; // 0x5A ($FF184F)
    static constexpr std::uint8_t kHitFlashFrames = 6U;    // $83
};

// ROM 0x00BD40: 7-tier cumulative kill-rank bonus (+1 at 10/20/30/40/50, +2 at 100, +4 at 200; max +11).
[[nodiscard]] std::uint16_t compute_ali_kill_rank_bonus(std::uint16_t kills) noexcept;

// ROM 0x00BFAE: Shield block succeeds iff defender guard `$6D != 0` and `(attacker_facing ^ 2) == defender_facing`.
[[nodiscard]] bool is_shield_block(
    std::uint8_t guard_flag_6d,
    std::uint8_t attacker_facing,
    std::uint8_t defender_facing) noexcept;

// Mirrors dir 3 -> dir 1 for 44-byte (0x2C) hitbox record lookup.
[[nodiscard]] constexpr std::uint8_t canonical_hitbox_facing(std::uint8_t facing) noexcept {
    return (facing == 3U) ? 1U : (facing & 3U);
}

struct CombatHitInput {
    bool attacker_is_ali{true};
    std::uint16_t base_damage{0U};
    std::uint16_t ali_kills{0U};
    std::uint8_t attacker_facing{0U}; // 0..3 (dir 3 mirrors dir 1)
    std::uint8_t defender_facing{0U};
    std::uint8_t defender_guard_flag_6d{0U};
    bool defender_is_spirit{false};
    std::uint8_t reaction_code{0U}; // 0, 2, 4, 6
};

struct CombatHitResult {
    std::uint8_t resolved_hitbox_dir{0U};
    bool shield_blocked{false};
    std::uint16_t total_damage{0U};
    std::uint8_t enemy_hp_hud_timer_ff184f{0U}; // 90 (0x5A) for non-spirit enemy hit
    std::uint8_t hit_flash_frames_83{0U};       // 6 on unblocked hit
    CombatReactionType reaction{CombatReactionType::Knockback0};
};

// ROM 0x00BCBA..0x00BF26: Resolves hitbox dir mirror, kill-rank bonus, shield block,
// 90-frame enemy HP HUD timer, 6-frame hit-flash, and reaction dispatch.
[[nodiscard]] CombatHitResult resolve_combat_hit(const CombatHitInput& in) noexcept;

} // namespace oasis::game::combat
