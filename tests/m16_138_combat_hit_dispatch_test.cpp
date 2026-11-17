#include "game/combat/combat_hit_dispatch.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

using namespace oasis::game::combat;

void test_damage_pack_scaling_and_hitstop() {
    // Exact verification vectors from tests/rom_combat_hit_dispatch_test.py
    const auto pack1 = compute_damage_pack(5, 0x23, true, 0, 2);
    assert(pack1.damage_word == 0x2007);
    assert(pack1.hitstop == 4);
    assert(pack1.raw_damage == 5);

    const auto pack2 = compute_damage_pack(5, 0x23, true, 10, 0);
    assert(pack2.damage_word == 0x2006);
    assert(pack2.hitstop == 4);
    assert(pack2.raw_damage == 6);

    const auto pack3 = compute_damage_pack(5, 0x23, true, 50, 0);
    assert(pack3.damage_word == 0x200A);
    assert(pack3.hitstop == 4);
    assert(pack3.raw_damage == 10);

    const auto pack4 = compute_damage_pack(5, 0x23, true, 200, 3);
    assert(pack4.damage_word == 0x2013);
    assert(pack4.hitstop == 4);
    assert(pack4.raw_damage == 16);

    // When attacker is not Ali, kill count bonuses are not applied
    const auto pack5 = compute_damage_pack(5, 0x23, false, 200, 3);
    assert(pack5.damage_word == 0x2008);
    assert(pack5.hitstop == 4);
    assert(pack5.raw_damage == 5);
}

void test_early_exit_and_frame_search() {
    std::vector<CombatFrameRecord> records{
        CombatFrameRecord{
            .frame_id = 12,
            .base_damage = 8,
            .hit_flags = 0x11,
            .reserved = 0,
            .local_boxes = {{
                {.dz1 = 0, .dz2 = 20, .dx1 = -10, .dy1 = -10, .dx2 = 10, .dy2 = 10},
                {.dz1 = 0, .dz2 = 20, .dx1 = 5, .dy1 = -5, .dx2 = 25, .dy2 = 5},
                {.dz1 = 0, .dz2 = 20, .dx1 = -10, .dy1 = 5, .dx2 = 10, .dy2 = 25},
            }},
        },
        CombatFrameRecord{.frame_id = -1}, // Terminator
    };

    std::vector<TargetEntity> targets(CombatHitDispatchConfig::kFullEntitySlotCount);

    AttackerEntity attacker{
        .slot_index = 0,
        .is_ali = true,
        .pos_x = 100,
        .pos_y = 100,
        .pos_z = 0,
        .elevation = 0,
        .facing = 1,
        .flags_37 = 0x08U, // Bit 3 set -> early abort!
        .frame_id = 12,
        .power_bonus = 0,
        .kill_count = 0,
    };

    // Should abort early because bit 3 of flags_37 is set
    const auto early_result = dispatch_combat_hit_primary(attacker, 0, records, targets);
    assert(early_result.early_exit_flag_37);
    assert(!early_result.record_found);

    // Clear abort bit, but request non-existent frame ID
    attacker.flags_37 = 0x00U;
    attacker.frame_id = 99;
    const auto not_found_result = dispatch_combat_hit_primary(attacker, 0, records, targets);
    assert(!not_found_result.early_exit_flag_37);
    assert(!not_found_result.record_found);

    // Matching frame ID
    attacker.frame_id = 12;
    const auto found_result = dispatch_combat_hit_primary(attacker, 0, records, targets);
    assert(!found_result.early_exit_flag_37);
    assert(found_result.record_found);
    assert(found_result.damage_pack.raw_damage == 8);
    assert(found_result.damage_pack.hitstop == 2);
}

void test_four_entrypoints_and_pool_filtering() {
    std::vector<CombatFrameRecord> records{
        CombatFrameRecord{
            .frame_id = 5,
            .base_damage = 10,
            .hit_flags = 0x32,
            .reserved = 0,
            .local_boxes = {{
                {.dz1 = 0, .dz2 = 30, .dx1 = -10, .dy1 = -10, .dx2 = 10, .dy2 = 10},
                {.dz1 = 0, .dz2 = 30, .dx1 = 0, .dy1 = -10, .dx2 = 30, .dy2 = 10},
                {.dz1 = 0, .dz2 = 30, .dx1 = -10, .dy1 = 0, .dx2 = 10, .dy2 = 30},
            }},
        },
        CombatFrameRecord{.frame_id = -1},
    };

    std::vector<TargetEntity> targets(CombatHitDispatchConfig::kFullEntitySlotCount);
    // Slot 0 (Ali)
    targets[0].state = 1;
    targets[0].pos_x = 110;
    targets[0].pos_y = 100;
    targets[0].pos_z = 0;
    targets[0].half_width_x = 16;
    targets[0].half_width_y = 16;
    targets[0].height_z = 32;

    // Slot 4 (First enemy slot)
    targets[4].state = 2;
    targets[4].pos_x = 110;
    targets[4].pos_y = 100;
    targets[4].pos_z = 0;
    targets[4].half_width_x = 16;
    targets[4].half_width_y = 16;
    targets[4].height_z = 32;

    // Slot 5 (Second enemy slot) - inactive flags_38 bit 3 set
    targets[5].state = 2;
    targets[5].pos_x = 110;
    targets[5].pos_y = 100;
    targets[5].pos_z = 0;
    targets[5].half_width_x = 16;
    targets[5].half_width_y = 16;
    targets[5].height_z = 32;
    targets[5].flags_38 = 0x08U;

    // Slot 6 (Third enemy slot) - already in hitstop
    targets[6].state = 2;
    targets[6].pos_x = 110;
    targets[6].pos_y = 100;
    targets[6].pos_z = 0;
    targets[6].half_width_x = 16;
    targets[6].half_width_y = 16;
    targets[6].height_z = 32;
    targets[6].hitstop = 3U;

    AttackerEntity ali_attacker{
        .slot_index = 0,
        .is_ali = true,
        .pos_x = 100,
        .pos_y = 100,
        .pos_z = 0,
        .elevation = 0,
        .facing = 1,
        .flags_37 = 0,
        .frame_id = 5,
        .power_bonus = 0,
        .kill_count = 0,
    };

    // 1. scan_fx_pool (0x00BCA4): scans slots 4..20, skips slots 0..3 (Ali)
    const auto fx_result = dispatch_combat_hit_fx(ali_attacker, 0, records, targets);
    assert(fx_result.record_found);
    assert(fx_result.callback_addr == CombatHitDispatchConfig::kCallbackDefaultBe98);
    assert(fx_result.hit_marker == 0U);
    assert(fx_result.hits.size() == 1U);
    assert(fx_result.hits[0].target_slot == 4U);
    assert(fx_result.hits[0].target_ram_addr == CombatHitDispatchConfig::kEnemyEntityPoolRam);

    // 2. scan_fx_pool_marked (0x00BCBA): same as fx_pool, but hit_marker == 2
    const auto marked_result = dispatch_combat_hit_fx_marked(ali_attacker, 0, records, targets);
    assert(marked_result.hit_marker == 2U);
    assert(marked_result.hits.size() == 1U);
    assert(marked_result.hits[0].target_slot == 4U);

    // 3. scan_ali_only (0x00BCD8): scans slot 0 only (e.g. enemy attack)
    AttackerEntity enemy_attacker{
        .slot_index = 4,
        .is_ali = false,
        .pos_x = 100,
        .pos_y = 100,
        .pos_z = 0,
        .elevation = 0,
        .facing = 1,
        .flags_37 = 0,
        .frame_id = 5,
        .power_bonus = 0,
        .kill_count = 0,
    };
    const auto ali_only_result = dispatch_combat_hit_ali_only(enemy_attacker, 0, records, targets);
    assert(ali_only_result.callback_addr == CombatHitDispatchConfig::kCallbackDefaultBeba);
    assert(ali_only_result.hits.size() == 1U);
    assert(ali_only_result.hits[0].target_slot == 0U);

    // 4. scan_primary_pool (0x00BC8E): scans all 21 slots
    const auto primary_result = dispatch_combat_hit_primary(enemy_attacker, 0, records, targets);
    assert(primary_result.callback_addr == CombatHitDispatchConfig::kCallbackDefaultBeba);
    // Hits Ali (slot 0), but skips self (slot 4), skips slot 5 (flags_38), skips slot 6 (hitstop)
    assert(primary_result.hits.size() == 1U);
    assert(primary_result.hits[0].target_slot == 0U);
}

void test_post_loop_special_cancel() {
    std::vector<CombatFrameRecord> records{
        CombatFrameRecord{
            .frame_id = 1,
            .base_damage = 5,
            .hit_flags = 0x01,
            .reserved = 0,
            .local_boxes = {{
                {.dz1 = 0, .dz2 = 20, .dx1 = 0, .dy1 = 0, .dx2 = 20, .dy2 = 20},
                {.dz1 = 0, .dz2 = 20, .dx1 = 0, .dy1 = 0, .dx2 = 20, .dy2 = 20},
                {.dz1 = 0, .dz2 = 20, .dx1 = 0, .dy1 = 0, .dx2 = 20, .dy2 = 20},
            }},
        },
        CombatFrameRecord{.frame_id = -1},
    };

    std::vector<TargetEntity> targets(CombatHitDispatchConfig::kFullEntitySlotCount);
    targets[4].state = 1;
    targets[4].pos_x = 105;
    targets[4].pos_y = 105;
    targets[4].pos_z = 0;
    targets[4].half_width_x = 10;
    targets[4].half_width_y = 10;
    targets[4].height_z = 20;

    AttackerEntity ali_attacker{
        .slot_index = 0,
        .is_ali = true,
        .pos_x = 100,
        .pos_y = 100,
        .pos_z = 0,
        .elevation = 0,
        .facing = 1,
        .flags_37 = 0,
        .frame_id = 1,
        .power_bonus = 0,
        .kill_count = 0,
    };

    // Callback simulates target shield block setting bit 3 of attacker.flags_37
    auto shield_block_cb = [](TargetEntity&, const AttackerEntity&, const DamagePack&) {
        // Callback can notify attacker state or attacker flags change
    };

    // When attack_index == 6 and flags_37 bit 3 became set
    ali_attacker.flags_37 = 0x08U; // simulate flags_37 set by hit
    // Wait: if flags_37 is set initially, dispatch aborts early!
    // So let's have the callback set flags_37!
    ali_attacker.flags_37 = 0x00U;
    auto deflecting_cb = [&](TargetEntity&, const AttackerEntity&, const DamagePack&) {
        ali_attacker.flags_37 = 0x08U; // Set clang / shield deflection flag
    };

    const auto cancel_res_6 = dispatch_combat_hit_fx(ali_attacker, 6, records, targets, deflecting_cb);
    assert(cancel_res_6.special_recoil_cancel);

    ali_attacker.flags_37 = 0x00U;
    const auto cancel_res_10 = dispatch_combat_hit_fx(ali_attacker, 10, records, targets, deflecting_cb);
    assert(cancel_res_10.special_recoil_cancel);

    // Other attack index e.g. 7 should NOT trigger special recoil cancel
    ali_attacker.flags_37 = 0x00U;
    const auto no_cancel_res = dispatch_combat_hit_fx(ali_attacker, 7, records, targets, deflecting_cb);
    assert(!no_cancel_res.special_recoil_cancel);
}

} // namespace

int main() {
    test_damage_pack_scaling_and_hitstop();
    test_early_exit_and_frame_search();
    test_four_entrypoints_and_pool_filtering();
    test_post_loop_special_cancel();

    std::cout << "All M16.138 combat hitbox dispatch tests passed successfully!\n";
    return 0;
}
