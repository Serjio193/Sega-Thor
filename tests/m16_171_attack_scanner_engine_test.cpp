#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "game/combat/attack_scanner_engine.hpp"

using namespace oasis::game::combat;

namespace {

void write_be16(std::vector<std::uint8_t>& mem, std::size_t offset, std::uint16_t val) {
    if (offset + 2 <= mem.size()) {
        mem[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
        mem[offset + 1] = static_cast<std::uint8_t>(val & 0xFF);
    }
}

void write_be16_signed(std::vector<std::uint8_t>& mem, std::size_t offset, std::int16_t val) {
    write_be16(mem, offset, static_cast<std::uint16_t>(val));
}

constexpr std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

// Builds a mock 44-byte attack frame record with 3 directional hitboxes (Down, Right, Up).
std::vector<std::uint8_t> create_mock_record_table(std::int16_t target_frame, std::uint8_t base_dmg, std::uint8_t element_raw) {
    std::vector<std::uint8_t> table(AttackScannerConfig::kRecordSize + 2, 0); // record + 0xFFFF terminator

    // Word 0: frame_id
    write_be16_signed(table, 0, target_frame);
    // Byte 2: base_dmg
    table[2] = base_dmg;
    // Byte 3: element_raw
    table[3] = element_raw;

    // Hitboxes at offset 8:
    // Dir 0 (Down, offset 8): z[0, 20], x[-8, 8], y[0, 24]
    const std::size_t h0 = 8;
    write_be16_signed(table, h0 + 0, 0);
    write_be16_signed(table, h0 + 2, 20);
    write_be16_signed(table, h0 + 4, -8);
    write_be16_signed(table, h0 + 6, 0);
    write_be16_signed(table, h0 + 8, 8);
    write_be16_signed(table, h0 + 10, 24);

    // Dir 1 (Right, offset 20): z[0, 20], x[0, 30], y[-10, 10]
    const std::size_t h1 = 20;
    write_be16_signed(table, h1 + 0, 0);
    write_be16_signed(table, h1 + 2, 20);
    write_be16_signed(table, h1 + 4, 0);
    write_be16_signed(table, h1 + 6, -10);
    write_be16_signed(table, h1 + 8, 30);
    write_be16_signed(table, h1 + 10, 10);

    // Dir 2 (Up, offset 32): z[0, 20], x[-8, 8], y[-24, 0]
    const std::size_t h2 = 32;
    write_be16_signed(table, h2 + 0, 0);
    write_be16_signed(table, h2 + 2, 20);
    write_be16_signed(table, h2 + 4, -8);
    write_be16_signed(table, h2 + 6, -24);
    write_be16_signed(table, h2 + 8, 8);
    write_be16_signed(table, h2 + 10, 0);

    // Terminator at offset 44: 0xFFFF
    write_be16_signed(table, AttackScannerConfig::kRecordSize, -1);

    return table;
}

void test_constants() {
    static_assert(AttackScannerConfig::kScanAllEntitiesAddr == 0x00BC8EU);
    static_assert(AttackScannerConfig::kScanGroup3AAddr == 0x00BCA4U);
    static_assert(AttackScannerConfig::kScanGroup3BAddr == 0x00BCBAU);
    static_assert(AttackScannerConfig::kScanSingleAddr == 0x00BCD8U);
    static_assert(AttackScannerConfig::kExitRtsAddr == 0x00BE96U);
    static_assert(AttackScannerConfig::kRangeEndAddr == 0x00BE98U);

    static_assert(AttackScannerConfig::kCallbackGroup3Addr == 0x00BE98U);
    static_assert(AttackScannerConfig::kCallbackAllAddr == 0x00BEBAU);
    static_assert(AttackScannerConfig::kDurabilityHookAddr == 0x008530U);

    static_assert(AttackScannerConfig::kCallbackPtrRam == 0x00FF1940U);
    static_assert(AttackScannerConfig::kModeFlag198BRam == 0x00FF198BU);
    static_assert(AttackScannerConfig::kAttackType198CRam == 0x00FF198CU);
    static_assert(AttackScannerConfig::kPlayerEntityRam == 0x00FF19E8U);
    static_assert(AttackScannerConfig::kGroup3EntityRam == 0x00FF1CD8U);

    static_assert(AttackScannerConfig::kAllEntitiesSlotCount == 21U);
    static_assert(AttackScannerConfig::kGroup3SlotCount == 17U);
    static_assert(AttackScannerConfig::kSingleSlotCount == 1U);
    static_assert(AttackScannerConfig::kSlotStride == 188U);
    static_assert(AttackScannerConfig::kRecordSize == 44U);

    std::cout << "[PASS] test_constants\n";
}

void test_ali_strength_bonus() {
    assert(calculate_ali_strength_bonus(0) == 0);
    assert(calculate_ali_strength_bonus(9) == 0);
    assert(calculate_ali_strength_bonus(10) == 1);
    assert(calculate_ali_strength_bonus(19) == 1);
    assert(calculate_ali_strength_bonus(20) == 2);
    assert(calculate_ali_strength_bonus(35) == 3);
    assert(calculate_ali_strength_bonus(49) == 4);
    assert(calculate_ali_strength_bonus(50) == 5);
    assert(calculate_ali_strength_bonus(99) == 5);
    assert(calculate_ali_strength_bonus(100) == 7);
    assert(calculate_ali_strength_bonus(199) == 7);
    assert(calculate_ali_strength_bonus(200) == 11);
    assert(calculate_ali_strength_bonus(255) == 11);

    std::cout << "[PASS] test_ali_strength_bonus\n";
}

void test_pack_attack_damage() {
    // Base damage 8, raw element 0x32 (element=0x3, stun=2)
    // stat_96 = 25 (bonus = 2), weapon_bonus = 10, is_ali = true
    // Total dmg = ((8 + 2) | 0x3000) + 10 = 0x3014
    // stun duration = (0x32 & 0xF) + 1 = 3
    // packed_a4 = (0x3014 << 16) | 3
    const AttackDamageDesc d1 = pack_attack_damage(8, 0x32, 25, 10, true);
    assert(d1.total_damage == 0x3014);
    assert(d1.stun_duration == 3);
    assert(d1.packed_a4 == ((0x3014U << 16U) | 3U));

    // Enemy test (is_ali = false): stat_96 bonus ignored
    const AttackDamageDesc d2 = pack_attack_damage(12, 0x01, 150, 0, false);
    assert(d2.total_damage == 12);
    assert(d2.stun_duration == 2);
    assert(d2.packed_a4 == ((12U << 16U) | 2U));

    std::cout << "[PASS] test_pack_attack_damage\n";
}

void test_early_phase_suppression() {
    ScannerAttackerState attacker{
        .address = 0x00FF19E8U,
        .flags_37 = 0x08U // Hit already registered this phase!
    };
    const std::vector<std::uint8_t> table = create_mock_record_table(5, 10, 0);
    std::vector<ScannerTargetState> targets(1);

    const AttackScanResult res = scan_attack_frame_pool(
        attacker, 0, table, targets, nullptr);
    assert(res.suppressed_early);
    assert(res.hit_count == 0);

    std::cout << "[PASS] test_early_phase_suppression\n";
}

void test_frame_search_and_mirroring() {
    const std::vector<std::uint8_t> table = create_mock_record_table(5, 10, 0);

    // Frame match with facing Right (dir 1)
    ScannerAttackerState atk_right{
        .address = 0x00FF19E8U,
        .world_x = 100,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .facing = 1, // Right
        .anim_frame = 5
    };
    std::vector<ScannerTargetState> targets;

    const AttackScanResult res_right = scan_attack_frame_pool(
        atk_right, 0, table, targets, nullptr);
    assert(res_right.frame_found);
    assert(res_right.projected_box.x_min == 100 && res_right.projected_box.x_max == 130);

    // Frame match with facing Left (dir 3 -> mirrored dir 1)
    ScannerAttackerState atk_left{
        .address = 0x00FF19E8U,
        .world_x = 100,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .facing = 3, // Left
        .anim_frame = 5
    };
    const AttackScanResult res_left = scan_attack_frame_pool(
        atk_left, 0, table, targets, nullptr);
    assert(res_left.frame_found);
    // Mirrored Right [0, 30] -> [-30, 0] + 100 -> [70, 100]
    assert(res_left.projected_box.x_min == 70 && res_left.projected_box.x_max == 100);

    // Frame mismatch
    atk_right.anim_frame = 99;
    const AttackScanResult res_miss = scan_attack_frame_pool(
        atk_right, 0, table, targets, nullptr);
    assert(!res_miss.frame_found);

    std::cout << "[PASS] test_frame_search_and_mirroring\n";
}

void test_target_filters_and_callbacks() {
    const std::vector<std::uint8_t> table = create_mock_record_table(3, 8, 0x12);

    ScannerAttackerState attacker{
        .address = 0x00FF19E8U, // Player Ali
        .world_x = 100,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .facing = 1, // Right: box x[100, 130], y[140, 160], z[0, 20]
        .anim_frame = 3,
        .strength_stat_96 = 20 // +2 bonus
    };

    std::vector<ScannerTargetState> targets(5);
    // Target 0: Inactive
    targets[0] = ScannerTargetState{.address = 0x00FF1CD8U, .type = 0, .world_x = 110, .world_y = 150};
    // Target 1: Self
    targets[1] = ScannerTargetState{.address = 0x00FF19E8U, .type = 1, .world_x = 110, .world_y = 150};
    // Target 2: Invulnerable (bit 3 of flags_38)
    targets[2] = ScannerTargetState{.address = 0x00FF1D94U, .type = 1, .world_x = 110, .world_y = 150, .flags_38 = 0x08};
    // Target 3: On cooldown
    targets[3] = ScannerTargetState{.address = 0x00FF1E50U, .type = 1, .world_x = 110, .world_y = 150, .cooldown_82 = 4};
    // Target 4: Valid candidate
    targets[4] = ScannerTargetState{
        .address = 0x00FF1F0CU,
        .type = 2,
        .world_x = 115,
        .world_y = 150,
        .elev_z = 0,
        .ground_h = 0,
        .rad_x = 10,
        .rad_y = 10,
        .height_z = 24
    };

    bool callback_called = false;
    std::uint32_t called_target_addr = 0;
    auto callback = [&](ScannerAttackerState& atk, ScannerTargetState& tgt, std::uint32_t a4, std::uint16_t type) {
        (void)a4;
        (void)type;
        callback_called = true;
        called_target_addr = tgt.address;
        atk.flags_37 |= 0x08U; // Register hit in phase
    };

    const AttackScanResult res = scan_attack_frame_pool(
        attacker, 6, table, targets, callback);
    assert(res.frame_found);
    assert(res.hit_count == 1);
    assert(callback_called);
    assert(called_target_addr == 0x00FF1F0CU);
    // Durability should be triggered: Ali, hit registered, attack_type 6
    assert(res.durability_triggered);

    std::cout << "[PASS] test_target_filters_and_callbacks\n";
}

void test_raw_ram_window_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0); // 64KB RAM window
    const std::vector<std::uint8_t> table = create_mock_record_table(2, 14, 0x01);

    // Setup Ali attacker at $FF19E8
    const std::size_t ali_off = to_ram_offset(0x00FF19E8U);
    write_be16_signed(ram, ali_off + AttackScannerConfig::kWorldXOffset, 100);
    write_be16_signed(ram, ali_off + AttackScannerConfig::kWorldYOffset, 150);
    write_be16(ram, ali_off + AttackScannerConfig::kFacingOffset, 1); // Right
    write_be16_signed(ram, ali_off + AttackScannerConfig::kAnimFrameOffset, 2);

    // Setup Enemy at $FF1CD8 (Group 3 slot 0)
    const std::size_t enm_off = to_ram_offset(0x00FF1CD8U);
    write_be16_signed(ram, enm_off + AttackScannerConfig::kTypeOffset, 5); // Active
    write_be16_signed(ram, enm_off + AttackScannerConfig::kWorldXOffset, 115);
    write_be16_signed(ram, enm_off + AttackScannerConfig::kWorldYOffset, 150);
    write_be16_signed(ram, enm_off + AttackScannerConfig::kRadiusXOffset, 10);
    write_be16_signed(ram, enm_off + AttackScannerConfig::kRadiusYOffset, 10);
    write_be16_signed(ram, enm_off + AttackScannerConfig::kHeightZOffset, 20);

    bool hit_registered = false;
    auto callback = [&](ScannerAttackerState& atk, ScannerTargetState& tgt, std::uint32_t a4, std::uint16_t type) {
        (void)tgt;
        (void)a4;
        (void)type;
        hit_registered = true;
        atk.flags_37 |= 0x08U;
    };

    // Test scan_group3_b_raw: should set $FF198B = 2
    const AttackScanResult res = scan_group3_b_raw(
        ram, 0x00FF19E8U, 10, table, callback);
    assert(res.frame_found);
    assert(res.hit_count == 1);
    assert(hit_registered);
    assert(res.durability_triggered); // attack_type 10 triggers durability
    assert(ram[to_ram_offset(AttackScannerConfig::kModeFlag198BRam)] == 2U);
    assert((ram[ali_off + AttackScannerConfig::kFlags37Offset] & 0x08U) != 0U);

    std::cout << "[PASS] test_raw_ram_window_integration\n";
}

} // namespace

int main() {
    test_constants();
    test_ali_strength_bonus();
    test_pack_attack_damage();
    test_early_phase_suppression();
    test_frame_search_and_mirroring();
    test_target_filters_and_callbacks();
    test_raw_ram_window_integration();

    std::cout << "\nAll M16.171 Attack Scanner Engine tests PASSED!\n";
    return 0;
}
