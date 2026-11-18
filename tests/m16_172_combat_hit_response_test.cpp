#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "game/combat/combat_hit_response.hpp"

using namespace oasis::game::combat;

namespace {

void write_be16(std::vector<std::uint8_t>& mem, std::size_t offset, std::uint16_t val) {
    if (offset + 2 <= mem.size()) {
        mem[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
        mem[offset + 1] = static_cast<std::uint8_t>(val & 0xFF);
    }
}

void write_be32(std::vector<std::uint8_t>& mem, std::size_t offset, std::uint32_t val) {
    if (offset + 4 <= mem.size()) {
        mem[offset] = static_cast<std::uint8_t>((val >> 24) & 0xFF);
        mem[offset + 1] = static_cast<std::uint8_t>((val >> 16) & 0xFF);
        mem[offset + 2] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
        mem[offset + 3] = static_cast<std::uint8_t>(val & 0xFF);
    }
}

constexpr std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

void test_constants() {
    static_assert(CombatHitResponseConfig::kCombatHitResponseTrackedAddr == 0x00BE98U);
    static_assert(CombatHitResponseConfig::kCombatHitResponseDirectAddr == 0x00BEBAU);
    static_assert(CombatHitResponseConfig::kUnblockedHitAddr == 0x00BED4U);
    static_assert(CombatHitResponseConfig::kKnockbackDispatcherAddr == 0x00BF14U);
    static_assert(CombatHitResponseConfig::kBlockedHitAddr == 0x00BFAEU);
    static_assert(CombatHitResponseConfig::kExitRtsAddr == 0x00C010U);
    static_assert(CombatHitResponseConfig::kHitSparkHookAddr == 0x00C012U);

    static_assert(CombatHitResponseConfig::kShieldBlockSfx == 0x00900202U);
    static_assert(CombatHitResponseConfig::kDefaultHudTrackingTimer == 0x5AU);

    static_assert(CombatHitResponseConfig::kHudTrackingTimerRam == 0x00FF184FU);
    static_assert(CombatHitResponseConfig::kHudTrackingTargetRam == 0x00FF1850U);
    static_assert(CombatHitResponseConfig::kPlayerEntityRam == 0x00FF19E8U);

    std::cout << "[PASS] test_constants\n";
}

void test_shield_facing() {
    // 0=Down, 1=Right, 2=Up, 3=Left
    // Attacker facing Down (0) -> Target facing Up (2) blocks
    assert(is_shield_facing(0, 2));
    // Attacker facing Up (2) -> Target facing Down (0) blocks
    assert(is_shield_facing(2, 0));
    // Attacker facing Right (1) -> Target facing Left (3) blocks
    assert(is_shield_facing(1, 3));
    // Attacker facing Left (3) -> Target facing Right (1) blocks
    assert(is_shield_facing(3, 1));

    // Flank / back attacks
    assert(!is_shield_facing(0, 0));
    assert(!is_shield_facing(1, 1));
    assert(!is_shield_facing(2, 1));
    assert(!is_shield_facing(3, 2));

    std::cout << "[PASS] test_shield_facing\n";
}

void test_knockback_mode_resolution() {
    // damage_word:
    // 0x0014 -> bits 14..15 = 00 -> Mode 0 (Light)
    // 0x4014 -> bits 14..15 = 01 -> Mode 1 (VerticalPopup)
    // 0x8014 -> bits 14..15 = 10 -> Mode 2 (Heavy)
    // 0xC014 -> bits 14..15 = 11 -> Mode 3 (Severe)
    assert(resolve_knockback_mode(0x0014) == KnockbackMode::Light);
    assert(resolve_knockback_mode(0x4014) == KnockbackMode::VerticalPopup);
    assert(resolve_knockback_mode(0x8014) == KnockbackMode::Heavy);
    assert(resolve_knockback_mode(0xC014) == KnockbackMode::Severe);

    std::cout << "[PASS] test_knockback_mode_resolution\n";
}

void test_project_velocity_8way() {
    // Speed (0x0400, 0x03C0) -> 32-bit (0x00040000, 0x0003C000)
    // Dir 0 (Up): {0, -0x0003C000}
    const auto [vx0, vy0] = project_velocity_8way(0x0400, 0x03C0, 0);
    assert(vx0 == 0);
    assert(vy0 == -0x0003C000);

    // Dir 1 (Right): {+0x00040000, 0}
    const auto [vx1, vy1] = project_velocity_8way(0x0400, 0x03C0, 1);
    assert(vx1 == 0x00040000);
    assert(vy1 == 0);

    // Dir 2 (Down): {0, +0x0003C000}
    const auto [vx2, vy2] = project_velocity_8way(0x0400, 0x03C0, 2);
    assert(vx2 == 0);
    assert(vy2 == 0x0003C000);

    // Dir 3 (Left): {-0x00040000, 0}
    const auto [vx3, vy3] = project_velocity_8way(0x0400, 0x03C0, 3);
    assert(vx3 == -0x00040000);
    assert(vy3 == 0);

    // Dir 5 (Down-Right): scaled by 3/4 -> 0x00040000 * 3 / 4 = 0x00030000
    // 0x0003C000 * 3 / 4 = 0x0002D000
    const auto [vx5, vy5] = project_velocity_8way(0x0400, 0x03C0, 5);
    assert(vx5 == 0x00030000);
    assert(vy5 == 0x0002D000);

    std::cout << "[PASS] test_project_velocity_8way\n";
}

void test_knockback_trajectories() {
    // Mode 0 Grounded (elev == 0): speed (0x300, 0x2C0), impulse = 0
    const KnockbackVelocity v0_g = calculate_knockback(KnockbackMode::Light, 1, false);
    assert(v0_g.vel_x == 0x00030000);
    assert(v0_g.vel_y == 0);
    assert(v0_g.vel_z == 0);
    assert(!v0_g.air_juggle);

    // Mode 0 Airborne (elev != 0): speed (0x400, 0x3C0), impulse = 0x28000, air_juggle = true
    const KnockbackVelocity v0_a = calculate_knockback(KnockbackMode::Light, 1, true);
    assert(v0_a.vel_x == 0x00040000);
    assert(v0_a.vel_y == 0);
    assert(v0_a.vel_z == 0x00028000);
    assert(v0_a.air_juggle);

    // Mode 1 VerticalPopup: (0, 0, 0x70000)
    const KnockbackVelocity v1 = calculate_knockback(KnockbackMode::VerticalPopup, 1, false);
    assert(v1.vel_x == 0);
    assert(v1.vel_y == 0);
    assert(v1.vel_z == 0x00070000);

    // Mode 2 Heavy: (0x400, 0x3C0, 0x28000)
    const KnockbackVelocity v2 = calculate_knockback(KnockbackMode::Heavy, 1, false);
    assert(v2.vel_x == 0x00040000);
    assert(v2.vel_z == 0x00028000);

    // Mode 3 Severe: (0x700, 0x680, 0x30000)
    const KnockbackVelocity v3 = calculate_knockback(KnockbackMode::Severe, 1, false);
    assert(v3.vel_x == 0x00070000);
    assert(v3.vel_z == 0x00030000);

    std::cout << "[PASS] test_knockback_trajectories\n";
}

void test_hit_response_unblocked_and_blocked() {
    HitEntityState attacker{
        .address = 0x00FF19E8U, // Ali
        .facing = 1,            // Right
        .flags_37 = 0
    };
    HitEntityState target{
        .address = 0x00FF1CD8U, // Enemy
        .facing = 3,            // Facing Left (opposite of Right)
        .guard_6d = 1           // Guarding with shield!
    };

    const std::uint32_t packed_a4 = (0x0014U << 16U) | 4U; // damage=0x0014 (Light), stun=4
    const std::uint32_t hit_sfx = 0x00020101U;

    // 1. Blocked hit: target is facing attacker with shield
    const HitResponseResult res_blocked = apply_combat_hit_response(
        target, attacker, packed_a4, 6, hit_sfx, true);
    assert(res_blocked.blocked);
    assert(!res_blocked.guard_broken);
    assert(res_blocked.durability_reduced); // Ali attack type 6
    assert(res_blocked.sfx_played == CombatHitResponseConfig::kShieldBlockSfx);
    assert(target.owner_7e == 0x00FF19E8U);
    assert(target.cooldown_82 == 4);
    assert(attacker.cooldown_82 == 4);
    assert(target.damage_86 == 0x0014);
    assert(target.reaction_83 == 0); // Not set to 6 on blocked hit
    assert(target.vel_x == 0 && target.vel_y == 0 && target.vel_z == 0);

    // 2. Guard break on flank: attacker attacks from rear/flank (target facing 1)
    target.facing = 1; // Facing same direction as attacker
    target.guard_6d = 1;
    attacker.flags_37 = 0;

    const HitResponseResult res_broken = apply_combat_hit_response(
        target, attacker, packed_a4, 6, hit_sfx, true);
    assert(!res_broken.blocked);
    assert(res_broken.guard_broken);
    assert(target.guard_6d == 0); // Guard broken!
    assert(target.reaction_83 == 6); // Entered reaction mode 6
    assert(res_broken.sfx_played == hit_sfx);
    assert(target.vel_x == 0x00030000); // Light grounded knockback rightwards

    std::cout << "[PASS] test_hit_response_unblocked_and_blocked\n";
}

void test_hud_tracking_evaluation() {
    HitEntityState attacker{.address = 0x00FF19E8U, .facing = 1};
    HitEntityState enemy_target{.address = 0x00FF1CD8U, .facing = 0};
    HitEntityState player_target{.address = 0x00FF19E8U, .facing = 0};

    const std::uint32_t packed_a4 = (0x0010U << 16U) | 2U;

    // Enemy target -> tracked
    const HitResponseResult res_enm = apply_combat_hit_response(
        enemy_target, attacker, packed_a4, 0, 0, true);
    assert(res_enm.hud_tracked);

    // Player target -> not tracked
    const HitResponseResult res_ply = apply_combat_hit_response(
        player_target, attacker, packed_a4, 0, 0, true);
    assert(!res_ply.hud_tracked);

    // Protected enemy (flags_3a bit 0 set) -> not tracked
    enemy_target.flags_3a = 0x01U;
    const HitResponseResult res_prot = apply_combat_hit_response(
        enemy_target, attacker, packed_a4, 0, 0, true);
    assert(!res_prot.hud_tracked);

    std::cout << "[PASS] test_hud_tracking_evaluation\n";
}

void test_raw_ram_window_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0); // 64KB RAM window

    // Setup Ali attacker at $FF19E8
    const std::size_t ali_off = to_ram_offset(0x00FF19E8U);
    write_be16(ram, ali_off + CombatHitResponseConfig::kFacingOffset, 1); // Right

    // Setup Enemy at $FF1CD8
    const std::size_t enm_off = to_ram_offset(0x00FF1CD8U);
    write_be16(ram, enm_off + CombatHitResponseConfig::kFacingOffset, 3); // Left (facing Ali)
    ram[enm_off + CombatHitResponseConfig::kGuard6DOffset] = 1;           // Shield guard active

    const std::uint32_t packed_a4 = (0x8010U << 16U) | 5U; // Heavy knockback mode
    const std::uint32_t hit_sfx = 0x00010101U;

    // Apply hit response tracked: shield blocks attack
    const HitResponseResult res = apply_combat_hit_response_tracked_raw(
        ram, 0x00FF1CD8U, 0x00FF19E8U, packed_a4, 10, hit_sfx);
    assert(res.blocked);
    assert(res.durability_reduced);
    assert(res.hud_tracked);

    // Check HUD tracking written into RAM
    assert(ram[to_ram_offset(CombatHitResponseConfig::kHudTrackingTimerRam)] == 0x5AU);
    const std::size_t target_ptr_off = to_ram_offset(CombatHitResponseConfig::kHudTrackingTargetRam);
    assert(ram[target_ptr_off + 0] == 0x00);
    assert(ram[target_ptr_off + 1] == 0xFF);
    assert(ram[target_ptr_off + 2] == 0x1C);
    assert(ram[target_ptr_off + 3] == 0xD8);

    std::cout << "[PASS] test_raw_ram_window_integration\n";
}

} // namespace

int main() {
    test_constants();
    test_shield_facing();
    test_knockback_mode_resolution();
    test_project_velocity_8way();
    test_knockback_trajectories();
    test_hit_response_unblocked_and_blocked();
    test_hud_tracking_evaluation();
    test_raw_ram_window_integration();

    std::cout << "\nAll M16.172 Combat Hit Response tests PASSED!\n";
    return 0;
}
