#include "game/combat/combat_hitbox_kernel.hpp"

#include <cassert>

int main() {
    using oasis::game::combat::canonical_hitbox_facing;
    using oasis::game::combat::CombatHitInput;
    using oasis::game::combat::CombatReactionType;
    using oasis::game::combat::compute_ali_kill_rank_bonus;
    using oasis::game::combat::is_shield_block;
    using oasis::game::combat::resolve_combat_hit;

    // 1. 7-tier kill-rank bonus (0x00BD40).
    assert(compute_ali_kill_rank_bonus(0U) == 0U);
    assert(compute_ali_kill_rank_bonus(9U) == 0U);
    assert(compute_ali_kill_rank_bonus(10U) == 1U);
    assert(compute_ali_kill_rank_bonus(20U) == 2U);
    assert(compute_ali_kill_rank_bonus(30U) == 3U);
    assert(compute_ali_kill_rank_bonus(40U) == 4U);
    assert(compute_ali_kill_rank_bonus(50U) == 5U);
    assert(compute_ali_kill_rank_bonus(100U) == 7U);
    assert(compute_ali_kill_rank_bonus(200U) == 11U);
    assert(compute_ali_kill_rank_bonus(999U) == 11U);

    // 2. Dir 3 -> Dir 1 mirror & shield block opposite-facing check (0x00BFAE).
    assert(canonical_hitbox_facing(3U) == 1U);
    assert(canonical_hitbox_facing(2U) == 2U);
    assert(is_shield_block(1U, 1U, 3U));  // 1 ^ 2 == 3 -> blocked
    assert(is_shield_block(1U, 0U, 2U));  // 0 ^ 2 == 2 -> blocked
    assert(!is_shield_block(0U, 1U, 3U)); // guard_flag == 0 -> not blocked
    assert(!is_shield_block(1U, 1U, 1U)); // same facing -> not blocked

    // 3. Unblocked Ali hit on non-spirit enemy: 90-frame HP HUD, 6-frame flash, kill-rank bonus, reaction.
    const auto hit = resolve_combat_hit(CombatHitInput{
        .attacker_is_ali = true,
        .base_damage = 10U,
        .ali_kills = 100U,
        .attacker_facing = 3U,
        .defender_facing = 3U,
        .defender_guard_flag_6d = 1U,
        .defender_is_spirit = false,
        .reaction_code = 4U,
    });
    assert(hit.resolved_hitbox_dir == 1U);
    assert(!hit.shield_blocked);
    assert(hit.total_damage == 17U); // 10 + 7
    assert(hit.enemy_hp_hud_timer_ff184f == 90U);
    assert(hit.hit_flash_frames_83 == 6U);
    assert(hit.reaction == CombatReactionType::Launch4);

    // 4. Shield-blocked hit: 0 damage, 0 flash, 90-frame HUD still set on non-spirit enemy.
    const auto blocked = resolve_combat_hit(CombatHitInput{
        .attacker_is_ali = true,
        .base_damage = 10U,
        .ali_kills = 200U,
        .attacker_facing = 3U,
        .defender_facing = 1U,
        .defender_guard_flag_6d = 1U,
        .defender_is_spirit = false,
        .reaction_code = 6U,
    });
    assert(blocked.shield_blocked);
    assert(blocked.total_damage == 0U);
    assert(blocked.hit_flash_frames_83 == 0U);
    assert(blocked.enemy_hp_hud_timer_ff184f == 90U);

    return 0;
}
