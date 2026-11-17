#include "game/combat/weapon_hitbox_builder.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

using namespace oasis::game::combat;

void test_hitbox_mirroring_and_translation() {
    // Reference test vector from tests/rom_weapon_hitbox_builder_test.py
    const LocalHitbox local{
        .dz1 = 0,
        .dz2 = 24,
        .dx1 = 4,
        .dy1 = -8,
        .dx2 = 28,
        .dy2 = 8,
    };

    // Facing 1: Right
    const auto right = build_world_weapon_hitbox(100, 200, 10, 0, 1, local);
    assert(right.x1 == 104);
    assert(right.y1 == 192);
    assert(right.x2 == 128);
    assert(right.y2 == 208);
    assert(right.z1 == 10);
    assert(right.z2 == 34);

    // Facing 3: Left (mirrors X: dx1' = -28, dx2' = -4)
    const auto left = build_world_weapon_hitbox(100, 200, 10, 0, 3, local);
    assert(left.x1 == 72);
    assert(left.y1 == 192);
    assert(left.x2 == 96);
    assert(left.y2 == 208);
    assert(left.z1 == 10);
    assert(left.z2 == 34);

    // With non-zero elevation y_off = 15
    const auto elevated = build_world_weapon_hitbox(100, 200, 10, 15, 3, local);
    assert(elevated.x1 == 72);
    assert(elevated.y1 == 200 + 15 - 8);  // 207
    assert(elevated.x2 == 96);
    assert(elevated.y2 == 200 + 15 + 8);  // 223
    assert(elevated.z1 == 10 + 15 + 0);   // 25
    assert(elevated.z2 == 10 + 15 + 24);  // 49
}

void test_hitbox_table_lookup() {
    // ROM table values extracted from 0x0586E8 (Facing 0: Up, 1: Right, 2: Down)
    const std::array<LocalHitbox, 3> rom_table{{
        {.dz1 = -4, .dz2 = 10, .dx1 = -24, .dy1 = -40, .dx2 = 23, .dy2 = -9},
        {.dz1 = -4, .dz2 = 10, .dx1 = 8, .dy1 = -16, .dx2 = 47, .dy2 = 15},
        {.dz1 = -4, .dz2 = 10, .dx1 = -24, .dy1 = 8, .dx2 = 23, .dy2 = 39},
    }};

    // Facing 0: Up
    const auto up = build_world_weapon_hitbox_from_table(50, 60, 0, 0, 0, rom_table);
    assert(up.x1 == 26);
    assert(up.y1 == 20);
    assert(up.x2 == 73);
    assert(up.y2 == 51);

    // Facing 3: Left (uses index 1 with X mirroring: dx1' = -47, dx2' = -8)
    const auto left = build_world_weapon_hitbox_from_table(50, 60, 0, 0, 3, rom_table);
    assert(left.x1 == 50 - 47); // 3
    assert(left.y1 == 60 - 16); // 44
    assert(left.x2 == 50 - 8);  // 42
    assert(left.y2 == 60 + 15); // 75
}

void test_fx_pool_scanner() {
    std::vector<FxEntity> fx_pool(WeaponHitboxConfig::kFxPoolSlotCount);
    // Slot 0: inactive
    fx_pool[0].state = 0;

    // Slot 1: active at (100, 200, 10), rx=16, ry=16, hz=32
    fx_pool[1].state = 8;
    fx_pool[1].pos_x = 100;
    fx_pool[1].pos_y = 200;
    fx_pool[1].pos_z = 10;
    fx_pool[1].half_width_x = 16;
    fx_pool[1].half_width_y = 16;
    fx_pool[1].height_z = 32;

    // Slot 2: active at (100, 200, 80) -> out of Z
    fx_pool[2].state = 8;
    fx_pool[2].pos_x = 100;
    fx_pool[2].pos_y = 200;
    fx_pool[2].pos_z = 80;
    fx_pool[2].half_width_x = 16;
    fx_pool[2].half_width_y = 16;
    fx_pool[2].height_z = 32;

    // Slot 3: active at (110, 205, 12) -> in range
    fx_pool[3].state = 8;
    fx_pool[3].pos_x = 110;
    fx_pool[3].pos_y = 205;
    fx_pool[3].pos_z = 12;
    fx_pool[3].half_width_x = 10;
    fx_pool[3].half_width_y = 10;
    fx_pool[3].height_z = 20;

    WeaponCallerEntity caller{
        .pos_x = 100,
        .pos_y = 200,
        .pos_z = 10,
        .elevation = 0,
        .facing = 1,
    };

    FxIteratorState iter_state{};
    // Probe relative box: [-10, 10] in X, [-10, 10] in Y, rz = 5
    const auto hit1 = find_first_fx_pool_hit(caller, -10, -10, 10, 10, 5, fx_pool, iter_state);
    assert(hit1.hit);
    assert(hit1.slot_index == 1U);
    assert(hit1.ram_addr == WeaponHitboxConfig::kFxPoolRam + 1U * WeaponHitboxConfig::kFxPoolStrideBytes);

    // Resume to find next hit
    const auto hit2 = find_next_fx_pool_hit(fx_pool, iter_state);
    assert(hit2.hit);
    assert(hit2.slot_index == 3U);
    assert(hit2.ram_addr == WeaponHitboxConfig::kFxPoolRam + 3U * WeaponHitboxConfig::kFxPoolStrideBytes);

    // No further hits
    const auto hit3 = find_next_fx_pool_hit(fx_pool, iter_state);
    assert(!hit3.hit);
}

void test_enemy_pool_weapon_scan() {
    std::vector<WeaponTargetEntity> entities(WeaponHitboxConfig::kFullEntitySlotCount);

    // Slot 0 (Ali) is in range of weapon box, but enemy scanner should skip slots 0..3!
    entities[0].state = 1;
    entities[0].pos_x = 100;
    entities[0].pos_y = 100;
    entities[0].pos_z = 0;
    entities[0].half_width_x = 16;
    entities[0].half_width_y = 16;
    entities[0].height_z = 32;

    // Slot 4 (First enemy slot, $FF1CD8): active, matches weapon box
    entities[4].state = 2;
    entities[4].pos_x = 100;
    entities[4].pos_y = 100;
    entities[4].pos_z = 0;
    entities[4].half_width_x = 16;
    entities[4].half_width_y = 16;
    entities[4].height_z = 32;

    const std::array<LocalHitbox, 3> table{{
        {.dz1 = 0, .dz2 = 32, .dx1 = 0, .dy1 = 0, .dx2 = 16, .dy2 = 16},
        {.dz1 = 0, .dz2 = 32, .dx1 = 0, .dy1 = 0, .dx2 = 16, .dy2 = 16},
        {.dz1 = 0, .dz2 = 32, .dx1 = 0, .dy1 = 0, .dx2 = 16, .dy2 = 16},
    }};

    WeaponCallerEntity caller{
        .pos_x = 95,
        .pos_y = 95,
        .pos_z = 0,
        .elevation = 0,
        .facing = 1,
    };

    // Enemy scan should ignore slot 0 and hit slot 4!
    const auto enemy_hit = scan_weapon_hitbox_enemies(0U, caller, table, entities);
    assert(enemy_hit.hit);
    assert(enemy_hit.slot_index == 4U);
    assert(enemy_hit.ram_addr == WeaponHitboxConfig::kEnemyEntityPoolRam);

    // Inactive flags check: if bit 3 of flags_38 is set, enemy is skipped
    entities[4].flags_38 = 0x08U;
    const auto skipped_38 = scan_weapon_hitbox_enemies(0U, caller, table, entities);
    assert(!skipped_38.hit);
    entities[4].flags_38 = 0x00U;

    // Inactive flags check: if bit 0 of flags_3a is set, enemy is skipped
    entities[4].flags_3a = 0x01U;
    const auto skipped_3a = scan_weapon_hitbox_enemies(0U, caller, table, entities);
    assert(!skipped_3a.hit);
}

} // namespace

int main() {
    test_hitbox_mirroring_and_translation();
    test_hitbox_table_lookup();
    test_fx_pool_scanner();
    test_enemy_pool_weapon_scan();

    std::cout << "All M16.137 weapon hitbox builder tests passed successfully!\n";
    return 0;
}
