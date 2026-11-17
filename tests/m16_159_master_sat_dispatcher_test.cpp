#include "game/render/master_sat_dispatcher.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

// Minimal test framework matching Sega-Thor test conventions
#include <iostream>
#include <string_view>

namespace {

int g_failures = 0;

void expect_true(bool cond, std::string_view msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << "\n";
        ++g_failures;
    }
}

void expect_eq_u32(std::uint32_t a, std::uint32_t b, std::string_view msg) {
    if (a != b) {
        std::cerr << "FAIL: " << msg << " (expected " << b << ", got " << a << ")\n";
        ++g_failures;
    }
}

void expect_eq_i32(std::int32_t a, std::int32_t b, std::string_view msg) {
    if (a != b) {
        std::cerr << "FAIL: " << msg << " (expected " << b << ", got " << a << ")\n";
        ++g_failures;
    }
}

} // namespace

using namespace oasis::game::render;

static void test_rom_constants() {
    expect_eq_u32(MasterSatDispatcherConfig::kMasterSatDispatcherAddr, 0x00A196U, "kMasterSatDispatcherAddr");
    expect_eq_u32(MasterSatDispatcherConfig::kMasterSatDispatcherEnd, 0x00A2EEU, "kMasterSatDispatcherEnd");
    expect_eq_u32(MasterSatDispatcherConfig::kCompositeFlagRam, 0x00FF1651U, "kCompositeFlagRam");
    expect_eq_u32(MasterSatDispatcherConfig::kDemoActiveFlagRam, 0x00FF1996U, "kDemoActiveFlagRam");
    expect_eq_u32(MasterSatDispatcherConfig::kForegroundPlayerFlagRam, 0x00FF1995U, "kForegroundPlayerFlagRam");
    expect_eq_u32(MasterSatDispatcherConfig::kVdpSatBufferRam, 0x00FF13CCU, "kVdpSatBufferRam");
    expect_eq_u32(MasterSatDispatcherConfig::kSatByteOffsetRam, 0x00FF188CU, "kSatByteOffsetRam");
    expect_eq_u32(MasterSatDispatcherConfig::kEntityDoneMaskRam, 0x00FF188EU, "kEntityDoneMaskRam");
    expect_eq_u32(MasterSatDispatcherConfig::kVdpSyncFlagsRam, 0x00FF164EU, "kVdpSyncFlagsRam");
    expect_eq_u32(MasterSatDispatcherConfig::kVdpSyncMask, 0x03U, "kVdpSyncMask");

    expect_eq_u32(MasterSatDispatcherConfig::kGroup1PoolRam, 0x00FF2954U, "kGroup1PoolRam");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kGroup1SlotCount), 4U, "kGroup1SlotCount");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kGroup1SlotStride), 90U, "kGroup1SlotStride");

    expect_eq_u32(MasterSatDispatcherConfig::kSpecialPoolRam, 0x00FF19C8U, "kSpecialPoolRam");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kSpecialSlotCount), 4U, "kSpecialSlotCount");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kSpecialSlotStride), 8U, "kSpecialSlotStride");

    expect_eq_u32(MasterSatDispatcherConfig::kGroup2PoolRam, 0x00FF19E8U, "kGroup2PoolRam");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kGroup2SlotCount), 21U, "kGroup2SlotCount");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kGroup2SlotStride), 188U, "kGroup2SlotStride");

    expect_eq_u32(MasterSatDispatcherConfig::kGroup4PoolRam, 0x00FF2ABCU, "kGroup4PoolRam");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kGroup4SlotCount), 8U, "kGroup4SlotCount");

    expect_eq_u32(MasterSatDispatcherConfig::kGroup3PoolRam, 0x00FF2D8CU, "kGroup3PoolRam");
    expect_eq_u32(static_cast<std::uint32_t>(MasterSatDispatcherConfig::kGroup3SlotCount), 6U, "kGroup3SlotCount");

    expect_eq_u32(static_cast<std::uint32_t>(SatDispatchTarget::NormalSat), 0x00AD76U, "NormalSat");
    expect_eq_u32(static_cast<std::uint32_t>(SatDispatchTarget::CustomSat), 0x00A9A0U, "CustomSat");
    expect_eq_u32(static_cast<std::uint32_t>(SatDispatchTarget::NormalSatForeground), 0x00AD80U, "NormalSatForeground");
    expect_eq_u32(static_cast<std::uint32_t>(SatDispatchTarget::CustomSatForeground), 0x00A9AAU, "CustomSatForeground");
    expect_eq_u32(static_cast<std::uint32_t>(SatDispatchTarget::SpecialJumpTable), 0x00A33CU, "SpecialJumpTable");

    expect_eq_i32(MasterSatDispatcherConfig::kInitialMaxYSentinel, -1, "kInitialMaxYSentinel");
}

static void test_group2_inactive_mask() {
    std::vector<SatEntitySlot> pool(21);
    // All inactive -> all 21 bits set
    std::uint32_t mask_all_inactive = build_group2_inactive_mask(pool);
    expect_eq_u32(mask_all_inactive, 0x001FFFFFU, "all inactive mask");

    // Slot 5 active
    pool[5].type = 1;
    std::uint32_t mask_5_active = build_group2_inactive_mask(pool);
    expect_eq_u32(mask_5_active, 0x001FFFFFU ^ (1U << 5), "slot 5 active mask");

    // All active
    for (auto& s : pool) {
        s.type = 2;
    }
    std::uint32_t mask_all_active = build_group2_inactive_mask(pool);
    expect_eq_u32(mask_all_active, 0U, "all active mask");
}

static void test_group2_y_depth_sorting() {
    std::vector<SatEntitySlot> pool(21);
    for (std::size_t i = 0; i < 21; ++i) {
        pool[i].type = 0; // inactive
        pool[i].y = 0;
    }

    // Set 3 active slots with different Y values
    pool[2].type = 1;
    pool[2].y = 100; // Background

    pool[7].type = 1;
    pool[7].y = 300; // Foreground (largest Y)

    pool[14].type = 1;
    pool[14].y = 200; // Midground

    // Negative Y (< 0, <= -1 sentinel) should be skipped
    pool[18].type = 1;
    pool[18].y = -5;

    std::uint32_t done_mask = 0U;
    std::vector<std::size_t> order = sort_group2_y_depth_order(pool, &done_mask);

    expect_eq_u32(static_cast<std::uint32_t>(order.size()), 3U, "order size");
    expect_eq_u32(static_cast<std::uint32_t>(order[0]), 7U, "first is slot 7 (y=300)");
    expect_eq_u32(static_cast<std::uint32_t>(order[1]), 14U, "second is slot 14 (y=200)");
    expect_eq_u32(static_cast<std::uint32_t>(order[2]), 2U, "third is slot 2 (y=100)");

    // Done mask has slots 2, 7, 14 marked processed
    expect_true((done_mask & (1U << 7)) != 0U, "slot 7 in done_mask");
    expect_true((done_mask & (1U << 14)) != 0U, "slot 14 in done_mask");
    expect_true((done_mask & (1U << 2)) != 0U, "slot 2 in done_mask");
}

static void test_target_selection_and_pools() {
    std::vector<SatEntitySlot> g1(4);
    g1[0] = {1, 10, 0x00}; // NormalSat
    g1[2] = {2, 20, 0x01}; // CustomSat

    std::vector<SatEntitySlot> sp(4);
    sp[1] = {3, 0, 0x00};  // SpecialJumpTable

    std::vector<SatEntitySlot> g2(21);
    g2[1] = {1, 50, 0x01};  // CustomSat
    g2[3] = {1, 150, 0x00}; // NormalSat

    std::vector<SatEntitySlot> g4(8);
    g4[0] = {4, 12, 0x00}; // NormalSat

    std::vector<SatEntitySlot> g3(6);
    g3[5] = {5, 34, 0x01}; // CustomSat

    MasterSatDispatchResult res = dispatch_master_sat_pass(
        false, false, g1, sp, g2, g4, g3, 16U, 0x04U);

    expect_true(res.composite_flag_cleared, "composite flag cleared");
    expect_true(!res.demo_branch_taken, "demo branch not taken");
    expect_true(res.sat_terminated, "sat terminated");
    expect_eq_u32(res.terminated_sat_offset, 16U, "sat offset preserved");
    expect_eq_u32(res.vdp_sync_flags, 0x07U, "sync flags updated with mask 0x03");

    // Entries: 2 from g1, 1 from sp, 2 from g2, 1 from g4, 1 from g3 = 7 total
    expect_eq_u32(static_cast<std::uint32_t>(res.entries.size()), 7U, "total entries");

    // Check pass order:
    // 0: Group1 slot 0
    expect_true(res.entries[0].pool == SatPoolKind::Group1HighPriority, "entry 0 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[0].slot_index), 0U, "entry 0 slot");
    expect_true(res.entries[0].target == SatDispatchTarget::NormalSat, "entry 0 target");

    // 1: Group1 slot 2
    expect_true(res.entries[1].pool == SatPoolKind::Group1HighPriority, "entry 1 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[1].slot_index), 2U, "entry 1 slot");
    expect_true(res.entries[1].target == SatDispatchTarget::CustomSat, "entry 1 target");

    // 2: Special slot 1
    expect_true(res.entries[2].pool == SatPoolKind::SpecialHitSparks, "entry 2 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[2].slot_index), 1U, "entry 2 slot");
    expect_true(res.entries[2].target == SatDispatchTarget::SpecialJumpTable, "entry 2 target");

    // 3: Group2 slot 3 (y=150 before y=50)
    expect_true(res.entries[3].pool == SatPoolKind::Group2Entities, "entry 3 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[3].slot_index), 3U, "entry 3 slot");
    expect_true(res.entries[3].target == SatDispatchTarget::NormalSat, "entry 3 target");

    // 4: Group2 slot 1 (y=50)
    expect_true(res.entries[4].pool == SatPoolKind::Group2Entities, "entry 4 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[4].slot_index), 1U, "entry 4 slot");
    expect_true(res.entries[4].target == SatDispatchTarget::CustomSat, "entry 4 target");

    // 5: Group4 slot 0
    expect_true(res.entries[5].pool == SatPoolKind::Group4GroundShadow, "entry 5 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[5].slot_index), 0U, "entry 5 slot");
    expect_true(res.entries[5].target == SatDispatchTarget::NormalSat, "entry 5 target");

    // 6: Group3 slot 5
    expect_true(res.entries[6].pool == SatPoolKind::Group3Floor, "entry 6 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[6].slot_index), 5U, "entry 6 slot");
    expect_true(res.entries[6].target == SatDispatchTarget::CustomSat, "entry 6 target");
}

static void test_foreground_player_override() {
    std::vector<SatEntitySlot> empty;
    std::vector<SatEntitySlot> g2(21);
    g2[0] = {1, 100, 0x00}; // Player, NormalSat

    // When flag is false: player is only in Group2
    auto res_no_fg = dispatch_master_sat_pass(false, false, empty, empty, g2, empty, empty);
    expect_eq_u32(static_cast<std::uint32_t>(res_no_fg.entries.size()), 1U, "no fg entries count");
    expect_true(res_no_fg.entries[0].pool == SatPoolKind::Group2Entities, "no fg pool");

    // When flag is true: player is rendered again in foreground
    auto res_fg = dispatch_master_sat_pass(false, true, empty, empty, g2, empty, empty);
    expect_eq_u32(static_cast<std::uint32_t>(res_fg.entries.size()), 2U, "fg entries count");
    expect_true(res_fg.entries[1].pool == SatPoolKind::ForegroundPlayer, "fg pool");
    expect_true(res_fg.entries[1].target == SatDispatchTarget::NormalSatForeground, "fg target normal");

    // With flags_36 bit 0 set:
    g2[0].flags_36 = 0x01;
    auto res_fg_custom = dispatch_master_sat_pass(false, true, empty, empty, g2, empty, empty);
    expect_true(res_fg_custom.entries[1].target == SatDispatchTarget::CustomSatForeground, "fg target custom");
}

static void test_demo_mode_bypass() {
    std::vector<SatEntitySlot> g1(4);
    g1[0] = {1, 10, 0};

    auto res = dispatch_master_sat_pass(true, false, g1, {}, {}, {}, {});
    expect_true(res.demo_branch_taken, "demo branch taken");
    expect_true(res.entries.empty(), "entries empty in demo");
}

static void test_raw_ram_integration() {
    // 64KB simulated RAM at 0x00FF0000
    std::vector<std::uint8_t> ram(0x10000, 0);

    auto ram_off = [](std::uint32_t addr) { return static_cast<std::size_t>(addr - 0x00FF0000U); };

    // Set composite flag $FF1651 = 0xFF (should be cleared to 0)
    ram[ram_off(MasterSatDispatcherConfig::kCompositeFlagRam)] = 0xFFU;

    // Demo flag $FF1996 = 0
    ram[ram_off(MasterSatDispatcherConfig::kDemoActiveFlagRam)] = 0x00U;

    // Foreground flag $FF1995 = 0
    ram[ram_off(MasterSatDispatcherConfig::kForegroundPlayerFlagRam)] = 0x00U;

    // Initial SAT offset $FF188C = 0 (so link terminator advances to 8, link written at 8-5=3)
    ram[ram_off(MasterSatDispatcherConfig::kSatByteOffsetRam)] = 0x00U;
    ram[ram_off(MasterSatDispatcherConfig::kSatByteOffsetRam) + 1] = 0x00U;

    // Set dummy non-zero byte at SAT buffer byte 3
    ram[ram_off(MasterSatDispatcherConfig::kVdpSatBufferRam + 3)] = 0x42U;

    // Initial sync flags $FF164E = 0x10
    ram[ram_off(MasterSatDispatcherConfig::kVdpSyncFlagsRam)] = 0x10U;

    // Place an entity in Group 2 slot 4: type = 1, y = 250
    const std::size_t g2_slot4 = ram_off(MasterSatDispatcherConfig::kGroup2PoolRam + 4 * MasterSatDispatcherConfig::kGroup2SlotStride);
    ram[g2_slot4 + 0] = 0x00;
    ram[g2_slot4 + 1] = 0x01; // type = 1
    ram[g2_slot4 + 12] = 0x00;
    ram[g2_slot4 + 13] = 0xFA; // y = 250

    MasterSatDispatchResult res = dispatch_master_sat_raw(ram, 0x00FF0000U);

    // Verify composite flag cleared in RAM
    expect_eq_u32(ram[ram_off(MasterSatDispatcherConfig::kCompositeFlagRam)], 0U, "raw composite flag cleared");

    // Verify SAT link byte terminated with 0 at offset 3
    expect_eq_u32(ram[ram_off(MasterSatDispatcherConfig::kVdpSatBufferRam + 3)], 0U, "raw sat link byte terminated");

    // Verify sync flags updated in RAM (0x10 | 0x03 = 0x13)
    expect_eq_u32(ram[ram_off(MasterSatDispatcherConfig::kVdpSyncFlagsRam)], 0x13U, "raw sync flags updated");

    // Verify entity done mask written to $FF188E
    const std::size_t mask_off = ram_off(MasterSatDispatcherConfig::kEntityDoneMaskRam);
    std::uint32_t written_mask = (static_cast<std::uint32_t>(ram[mask_off]) << 24) |
                                 (static_cast<std::uint32_t>(ram[mask_off + 1]) << 16) |
                                 (static_cast<std::uint32_t>(ram[mask_off + 2]) << 8) |
                                 static_cast<std::uint32_t>(ram[mask_off + 3]);
    expect_eq_u32(written_mask, res.entity_done_mask, "raw done mask written");
}

int main() {
    std::cout << "Running M16.159 Master SAT Sprite Dispatcher tests...\n";

    test_rom_constants();
    test_group2_inactive_mask();
    test_group2_y_depth_sorting();
    test_target_selection_and_pools();
    test_foreground_player_override();
    test_demo_mode_bypass();
    test_raw_ram_integration();

    if (g_failures == 0) {
        std::cout << "ALL M16.159 TESTS PASSED!\n";
        return 0;
    }

    std::cerr << g_failures << " TEST(S) FAILED!\n";
    return 1;
}
