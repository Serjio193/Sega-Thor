#include "game/combat/combat_overlap_engine.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::combat;

static void test_constants() {
    static_assert(CombatOverlapConfig::kDirectionalHitboxScannerAddr == 0x00B79AU);
    static_assert(CombatOverlapConfig::kDirectionalScannerExitRtsAddr == 0x00B850U);
    static_assert(CombatOverlapConfig::kGroup2AabbScannerAddr == 0x00B856U);
    static_assert(CombatOverlapConfig::kGroup2AabbHitExitRtsAddr == 0x00B910U);
    static_assert(CombatOverlapConfig::kGroup2AabbResumeAddr == 0x00B912U);
    static_assert(CombatOverlapConfig::kGroup2IntervalScannerAddr == 0x00B922U);
    static_assert(CombatOverlapConfig::kGroup2IntervalHitExitRtsAddr == 0x00B9DCU);
    static_assert(CombatOverlapConfig::kGroup2IntervalResumeAddr == 0x00B9DEU);
    static_assert(CombatOverlapConfig::kGroup4ScannerAddr == 0x00B9ECU);
    static_assert(CombatOverlapConfig::kGroup4HitExitRtsAddr == 0x00BA9AU);
    static_assert(CombatOverlapConfig::kGroup4ResumeAddr == 0x00BA9CU);
    static_assert(CombatOverlapConfig::kCombatOverlapRangeEnd == 0x00BAACU);

    static_assert(CombatOverlapConfig::kGroup2PoolRam == 0x00FF19E8U);
    static_assert(CombatOverlapConfig::kGroup2SlotCount == 21U);
    static_assert(CombatOverlapConfig::kGroup2SlotStride == 188U);

    static_assert(CombatOverlapConfig::kGroup4PoolRam == 0x00FF2ABCU);
    static_assert(CombatOverlapConfig::kGroup4SlotCount == 8U);
    static_assert(CombatOverlapConfig::kGroup4SlotStride == 90U);
}

static void test_directional_hitbox_calculation() {
    const auto box_down = compute_directional_hitbox(16, 20, 0); // Down
    assert(box_down.x_min == -16 && box_down.x_max == 16);
    assert(box_down.y_min == 0 && box_down.y_max == 20);

    const auto box_left = compute_directional_hitbox(16, 20, 1); // Left
    assert(box_left.x_min == -16 && box_left.x_max == 0);
    assert(box_left.y_min == -20 && box_left.y_max == 20);

    const auto box_up = compute_directional_hitbox(16, 20, 2); // Up
    assert(box_up.x_min == -16 && box_up.x_max == 16);
    assert(box_up.y_min == -20 && box_up.y_max == 0);

    const auto box_right = compute_directional_hitbox(16, 20, 3); // Right
    assert(box_right.x_min == 0 && box_right.x_max == 16);
    assert(box_right.y_min == -20 && box_right.y_max == 20);
}

static void test_vulnerability_filters() {
    // Normal vulnerable target
    assert(is_target_vulnerable(0x00U, 0x00U, 0U, 0x1000U, 0x2000U));
    assert(is_target_vulnerable(0x00U, 0x00U, 0U, 0U, 0U));

    // Intangible / invulnerable bitmask 0x0C on +0x38
    assert(!is_target_vulnerable(0x04U, 0x00U, 0U, 0x1000U, 0x2000U));
    assert(!is_target_vulnerable(0x08U, 0x00U, 0U, 0x1000U, 0x2000U));

    // Status protected bit 0 on +0x3A
    assert(!is_target_vulnerable(0x00U, 0x01U, 0U, 0x1000U, 0x2000U));

    // Hit cooldown active
    assert(!is_target_vulnerable(0x00U, 0x00U, 5U, 0x1000U, 0x2000U));

    // Self damage prevention (owner == target)
    assert(!is_target_vulnerable(0x00U, 0x00U, 0U, 0x1000U, 0x1000U));
}

static void test_hit_registration() {
    CombatEntityState self{};
    self.damage_84 = 25;
    self.owner_7e = 0x00FF19E8U;
    self.hit_cooldown_82 = 0U;

    CombatEntityState target{};
    target.hit_cooldown_82 = 0U;

    register_combat_hit(self, target);

    assert(self.hit_cooldown_82 == 1U);
    assert(target.hit_cooldown_82 == 1U);
    assert(target.owner_7e == 0x00FF19E8U);
    assert(target.damage_84 == 25);
    assert(target.invuln_timer_83 == 0x0CU);
    assert(target.hit_status_86 == 0x8001U);
    assert(target.hit_stun_9d == 0U);
    assert(target.vel_x == 0x0200);
    assert(target.vel_y == 0x01C0);
    assert(target.vel_z == 0x00020000);
}

static void test_group2_aabb_point_scan() {
    CombatEntityState self{};
    self.world_x = 100;
    self.world_y = 100;
    self.elev_z = 0;
    self.ground_h = 0;
    self.facing = 3U; // Right: [0, 16, -16, 16]
    self.rad_x = 16;
    self.rad_y = 16;
    self.ram_addr = 0x00FF1000U;

    const auto rel_hitbox = compute_directional_hitbox(self.rad_x, self.rad_y, self.facing);

    std::vector<CombatEntityState> pool(3);
    // Target 0: In range (x = 110, y = 100, z = 0..20)
    pool[0].type = 1;
    pool[0].world_x = 110;
    pool[0].world_y = 100;
    pool[0].elev_z = 0;
    pool[0].ground_h = 0;
    pool[0].rad_x = 8;
    pool[0].rad_y = 8;
    pool[0].height_z = 20;
    pool[0].ram_addr = 0x00FF19E8U;

    // Target 1: Out of range X (x = 150)
    pool[1].type = 1;
    pool[1].world_x = 150;
    pool[1].world_y = 100;
    pool[1].rad_x = 8;
    pool[1].rad_y = 8;
    pool[1].ram_addr = 0x00FF1AA4U;

    // Target 2: In range (x = 112, y = 105, z = 0..20)
    pool[2].type = 1;
    pool[2].world_x = 112;
    pool[2].world_y = 105;
    pool[2].rad_x = 8;
    pool[2].rad_y = 8;
    pool[2].height_z = 20;
    pool[2].ram_addr = 0x00FF1B60U;

    CombatOverlapIterator iter{0xFFFFFFFFU, 0U, true};

    // First scan matches target 0
    auto match1 = scan_group2_aabb(self, rel_hitbox, pool, iter);
    assert(match1.hit);
    assert(match1.slot_index == 0U);
    assert(match1.target_addr == 0x00FF19E8U);

    // Second scan skips target 1 and matches target 2
    auto match2 = scan_group2_aabb(self, rel_hitbox, pool, iter);
    assert(match2.hit);
    assert(match2.slot_index == 2U);
    assert(match2.target_addr == 0x00FF1B60U);

    // Third scan finishes with no match
    auto match3 = scan_group2_aabb(self, rel_hitbox, pool, iter);
    assert(!match3.hit);
}

static void test_group2_interval_scan() {
    CombatEntityState self{};
    self.world_x = 50;
    self.world_y = 50;
    self.elev_z = 10;
    self.ground_h = 0;
    self.rad_x = 10;
    self.rad_y = 10;

    DirectionalHitbox hitbox{-10, 10, -10, 10};

    std::vector<CombatEntityState> pool(1);
    pool[0].type = 1;
    pool[0].world_x = 50;
    pool[0].world_y = 50;
    pool[0].elev_z = 30; // Z base = 30, height = 20 -> [30..50]
    pool[0].ground_h = 0;
    pool[0].rad_x = 10;
    pool[0].rad_y = 10;
    pool[0].height_z = 20;

    // Interval [0..10] relative to self.z (10) -> [10..20] -> Miss [30..50]
    CombatOverlapIterator iter1{0xFFFFFFFFU, 0U, true};
    auto m1 = scan_group2_interval(self, hitbox, 0, 10, pool, iter1);
    assert(!m1.hit);

    // Interval [15..35] relative to self.z (10) -> [25..45] -> Hits [30..50]
    CombatOverlapIterator iter2{0xFFFFFFFFU, 0U, true};
    auto m2 = scan_group2_interval(self, hitbox, 15, 35, pool, iter2);
    assert(m2.hit);
}

static void test_group4_breakables() {
    CombatEntityState self{};
    self.world_x = 80;
    self.world_y = 80;
    self.elev_z = 0;
    self.ground_h = 0;
    DirectionalHitbox hitbox{-10, 10, -10, 10};

    std::vector<CombatEntityState> pool(2);
    pool[0].type = 0x56; // Breakable pot
    pool[0].world_x = 85;
    pool[0].world_y = 85;
    pool[0].rad_x = 8;
    pool[0].rad_y = 8;
    pool[0].height_z = 16;

    CombatOverlapIterator iter{0xFFFFFFFFU, 0U, true};
    auto m = scan_group4_breakables(self, hitbox, pool, iter);
    assert(m.hit);
    assert(m.slot_index == 0U);
}

static void test_directional_attack_execution() {
    CombatEntityState self{};
    self.world_x = 100;
    self.world_y = 100;
    self.facing = 3U; // Right
    self.rad_x = 20;
    self.rad_y = 20;
    self.damage_84 = 50;
    self.owner_7e = 0x00FF1000U;
    self.ram_addr = 0x00FF1000U;

    std::vector<CombatEntityState> targets(2);
    // Target 0: In range & vulnerable
    targets[0].type = 1;
    targets[0].world_x = 110;
    targets[0].world_y = 100;
    targets[0].rad_x = 10;
    targets[0].rad_y = 10;
    targets[0].height_z = 20;
    targets[0].ram_addr = 0x00FF19E8U;

    // Target 1: In range but intangible
    targets[1].type = 1;
    targets[1].world_x = 115;
    targets[1].world_y = 100;
    targets[1].rad_x = 10;
    targets[1].rad_y = 10;
    targets[1].height_z = 20;
    targets[1].flags_38 = 0x04U; // Intangible
    targets[1].ram_addr = 0x00FF1AA4U;

    const std::size_t hits = execute_directional_combat_attack(self, targets);
    assert(hits == 1U);
    assert(targets[0].hit_cooldown_82 == 1U);
    assert(targets[0].damage_84 == 50);
    assert(targets[1].hit_cooldown_82 == 0U); // Skipped
}

static void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0U); // 64KB RAM window at 0x00FF0000
    const std::uint32_t self_addr = 0x00FF1800U;
    const std::size_t self_pos = self_addr - 0x00FF0000U;

    // Setup self: x=100, y=100, facing=3 (Right), rx=20, ry=20, dmg=30
    ram[self_pos + CombatOverlapConfig::kTypeOffset + 1] = 1U;
    ram[self_pos + CombatOverlapConfig::kWorldXOffset + 1] = 100U;
    ram[self_pos + CombatOverlapConfig::kWorldYOffset + 1] = 100U;
    ram[self_pos + CombatOverlapConfig::kFacingOffset + 1] = 3U;
    ram[self_pos + CombatOverlapConfig::kRadiusXOffset + 1] = 20U;
    ram[self_pos + CombatOverlapConfig::kRadiusYOffset + 1] = 20U;
    ram[self_pos + CombatOverlapConfig::kDamageOffset + 1] = 30U;

    // Setup target at Group 2 slot 0 (0x00FF19E8): x=110, y=100, rx=10, ry=10, hz=20
    const std::size_t t_pos = CombatOverlapConfig::kGroup2PoolRam - 0x00FF0000U;
    ram[t_pos + CombatOverlapConfig::kTypeOffset + 1] = 2U;
    ram[t_pos + CombatOverlapConfig::kWorldXOffset + 1] = 110U;
    ram[t_pos + CombatOverlapConfig::kWorldYOffset + 1] = 100U;
    ram[t_pos + CombatOverlapConfig::kRadiusXOffset + 1] = 10U;
    ram[t_pos + CombatOverlapConfig::kRadiusYOffset + 1] = 10U;
    ram[t_pos + CombatOverlapConfig::kHeightZOffset + 1] = 20U;

    const std::size_t hits = scan_combat_overlap_raw(ram, self_addr, 0x00FF0000U);
    assert(hits == 1U);

    // Verify self cooldown updated
    assert(ram[self_pos + CombatOverlapConfig::kHitCooldownOffset] == 1U);

    // Verify target cooldown, invuln timer, status, damage, vel
    assert(ram[t_pos + CombatOverlapConfig::kHitCooldownOffset] == 1U);
    assert(ram[t_pos + CombatOverlapConfig::kInvulnTimerOffset] == 0x0CU);
    assert(ram[t_pos + CombatOverlapConfig::kDamageOffset + 1] == 30U);
    assert(ram[t_pos + CombatOverlapConfig::kHitStatusOffset] == 0x80U);
    assert(ram[t_pos + CombatOverlapConfig::kHitStatusOffset + 1] == 0x01U);
}

int main() {
    test_constants();
    test_directional_hitbox_calculation();
    test_vulnerability_filters();
    test_hit_registration();
    test_group2_aabb_point_scan();
    test_group2_interval_scan();
    test_group4_breakables();
    test_directional_attack_execution();
    test_raw_ram_integration();

    std::cout << "All M16.169 combat overlap engine tests passed successfully!\n";
    return 0;
}
