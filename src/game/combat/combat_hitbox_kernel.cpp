#include "game/combat/combat_hitbox_kernel.hpp"

namespace oasis::game::combat {

std::uint16_t compute_ali_kill_rank_bonus(std::uint16_t kills) noexcept {
    std::uint16_t bonus = 0U;
    if (kills >= 10U)  bonus += 1U;
    if (kills >= 20U)  bonus += 1U;
    if (kills >= 30U)  bonus += 1U;
    if (kills >= 40U)  bonus += 1U;
    if (kills >= 50U)  bonus += 1U;
    if (kills >= 100U) bonus += 2U;
    if (kills >= 200U) bonus += 4U;
    return bonus;
}

bool is_shield_block(
    std::uint8_t guard_flag_6d,
    std::uint8_t attacker_facing,
    std::uint8_t defender_facing) noexcept {
    return guard_flag_6d != 0U &&
           ((attacker_facing ^ 2U) & 3U) == (defender_facing & 3U);
}

CombatHitResult resolve_combat_hit(const CombatHitInput& in) noexcept {
    CombatHitResult out{};
    out.resolved_hitbox_dir = canonical_hitbox_facing(in.attacker_facing);

    if (in.attacker_is_ali && !in.defender_is_spirit) {
        out.enemy_hp_hud_timer_ff184f = CombatHitboxConfig::kEnemyHpHudFrames; // 90 (0x5A)
    }

    if (is_shield_block(in.defender_guard_flag_6d, in.attacker_facing, in.defender_facing)) {
        out.shield_blocked = true;
        out.total_damage = 0U;
        return out;
    }

    const std::uint16_t bonus = in.attacker_is_ali ? compute_ali_kill_rank_bonus(in.ali_kills) : 0U;
    out.total_damage = static_cast<std::uint16_t>(in.base_damage + bonus);
    out.hit_flash_frames_83 = CombatHitboxConfig::kHitFlashFrames; // 6

    switch (in.reaction_code) {
        case 2U:  out.reaction = CombatReactionType::Stun2; break;
        case 4U:  out.reaction = CombatReactionType::Launch4; break;
        case 6U:  out.reaction = CombatReactionType::Heavy6; break;
        default:  out.reaction = CombatReactionType::Knockback0; break;
    }
    return out;
}

} // namespace oasis::game::combat
