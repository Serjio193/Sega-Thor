#include "game/render/pass_b_sat_dispatcher.hpp"

#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

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
    expect_eq_u32(PassBSatDispatcherConfig::kPassBDispatcherAddr, 0x00A524U, "kPassBDispatcherAddr");
    expect_eq_u32(PassBSatDispatcherConfig::kPassBDispatcherEnd, 0x00A69AU, "kPassBDispatcherEnd");
    expect_eq_u32(PassBSatDispatcherConfig::kSpecialPoolJumpTableTarget, 0x00A69AU, "kSpecialPoolJumpTableTarget");

    expect_eq_u32(PassBSatDispatcherConfig::kCachedMaskRam, 0x00FF188EU, "kCachedMaskRam");
    expect_eq_u32(PassBSatDispatcherConfig::kForegroundPlayerFlagRam, 0x00FF1995U, "kForegroundPlayerFlagRam");
    expect_eq_u32(PassBSatDispatcherConfig::kVdpSatBufferRam, 0x00FF13CCU, "kVdpSatBufferRam");
    expect_eq_u32(PassBSatDispatcherConfig::kSatByteOffsetRam, 0x00FF188CU, "kSatByteOffsetRam");
    expect_eq_u32(PassBSatDispatcherConfig::kSatLinkCountRam, 0x00FF188AU, "kSatLinkCountRam");
    expect_eq_u32(PassBSatDispatcherConfig::kVdpSyncFlagsRam, 0x00FF164EU, "kVdpSyncFlagsRam");
    expect_eq_u32(PassBSatDispatcherConfig::kVdpSyncMask, 0x03U, "kVdpSyncMask");

    expect_eq_u32(PassBSatDispatcherConfig::kGroup1PoolRam, 0x00FF2954U, "kGroup1PoolRam");
    expect_eq_u32(PassBSatDispatcherConfig::kSpecialPoolRam, 0x00FF19C8U, "kSpecialPoolRam");
    expect_eq_u32(PassBSatDispatcherConfig::kGroup2PoolRam, 0x00FF19E8U, "kGroup2PoolRam");
    expect_eq_u32(PassBSatDispatcherConfig::kGroup4PoolRam, 0x00FF2ABCU, "kGroup4PoolRam");
    expect_eq_u32(PassBSatDispatcherConfig::kGroup3PoolRam, 0x00FF2D8CU, "kGroup3PoolRam");

    expect_eq_i32(PassBSatDispatcherConfig::kScreenWidth, 320, "kScreenWidth");
    expect_eq_i32(PassBSatDispatcherConfig::kScreenHeight, 224, "kScreenHeight");
    expect_eq_i32(PassBSatDispatcherConfig::kDefaultCullMargin, 64, "kDefaultCullMargin");
}

static void test_cached_mask_filtering() {
    std::vector<SatEntitySlot> pool(21);
    for (std::size_t i = 0; i < 21; ++i) {
        pool[i].type = 1;
        pool[i].y = static_cast<std::int16_t>(100 + i * 5);
    }

    // Mask out slots 0..9 as already processed / inactive
    const std::uint32_t cached_mask = (1U << 10) - 1U;

    std::uint32_t out_mask = 0U;
    auto order = sort_group2_pass_b_order(pool, cached_mask, &out_mask);

    // Only slots 10..20 should be dispatched (11 slots)
    expect_eq_u32(static_cast<std::uint32_t>(order.size()), 11U, "order count with cached mask");
    for (std::size_t slot : order) {
        expect_true(slot >= 10U, "slot >= 10");
    }

    // All 21 bits should be marked in out_mask
    expect_eq_u32(out_mask, 0x001FFFFFU, "all slots marked in final mask");
}

static void test_descending_y_sorting() {
    std::vector<SatEntitySlot> pool(21);
    for (auto& s : pool) {
        s.type = 0;
        s.y = 0;
    }

    pool[3] = {1, 120, 0};
    pool[8] = {1, 240, 0};
    pool[15] = {1, 180, 0};

    // Cache mask marks all slots except 3, 8, 15
    const std::uint32_t cached_mask = 0x001FFFFFU & ~((1U << 3) | (1U << 8) | (1U << 15));

    auto order = sort_group2_pass_b_order(pool, cached_mask);
    expect_eq_u32(static_cast<std::uint32_t>(order.size()), 3U, "order size");
    expect_eq_u32(static_cast<std::uint32_t>(order[0]), 8U, "slot 8 (y=240) first");
    expect_eq_u32(static_cast<std::uint32_t>(order[1]), 15U, "slot 15 (y=180) second");
    expect_eq_u32(static_cast<std::uint32_t>(order[2]), 3U, "slot 3 (y=120) third");
}

static void test_camera_window_filter() {
    CameraWindowFilter cam{};
    cam.camera_x = 100;
    cam.camera_y = 100;
    cam.width = 320;
    cam.height = 224;
    cam.margin = 32;

    // Inside window
    expect_true(cam.is_in_window(200, 200), "inside center");
    expect_true(cam.is_in_window(100 - 30, 200), "inside left margin");
    expect_true(cam.is_in_window(100 + 320 + 30, 200), "inside right margin");

    // Outside window
    expect_true(!cam.is_in_window(50, 200), "outside left");
    expect_true(!cam.is_in_window(500, 200), "outside right");
    expect_true(!cam.is_in_window(200, 50), "outside top");
    expect_true(!cam.is_in_window(200, 400), "outside bottom");

    // Test filter_cached_mask_by_camera_window
    std::vector<SatEntitySlot> g2(21);
    std::vector<std::int16_t> x_coords(21, 0);

    // Slot 1 is inside (x=200, y=200)
    g2[1] = {1, 200, 0};
    x_coords[1] = 200;

    // Slot 2 is outside (x=900, y=900)
    g2[2] = {1, 900, 0};
    x_coords[2] = 900;

    std::uint32_t init_mask = 0U;
    std::uint32_t filtered = filter_cached_mask_by_camera_window(g2, init_mask, cam, x_coords);

    // Slot 1 should NOT be set (remains candidate)
    expect_true((filtered & (1U << 1)) == 0U, "slot 1 inside window not culled");
    // Slot 2 should BE set (culled outside window)
    expect_true((filtered & (1U << 2)) != 0U, "slot 2 outside window culled");
}

static void test_pass_b_multi_pool_dispatch() {
    std::vector<SatEntitySlot> g1(4);
    g1[0] = {1, 10, 0x00}; // NormalSat

    std::vector<SatEntitySlot> sp(4);
    sp[2] = {2, 0, 0x00};  // Special (0x00A69A)

    std::vector<SatEntitySlot> g2(21);
    g2[4] = {1, 100, 0x01}; // CustomSat

    std::vector<SatEntitySlot> g4(8);
    g4[1] = {3, 50, 0x00};

    std::vector<SatEntitySlot> g3(6);
    g3[0] = {4, 20, 0x00};

    // Cached mask marks all g2 except slot 4
    std::uint32_t cached_mask = 0x001FFFFFU & ~(1U << 4);

    auto res = dispatch_pass_b_sat_pass(cached_mask, false, g1, sp, g2, g4, g3, 32U, 0x08U);

    expect_eq_u32(static_cast<std::uint32_t>(res.entries.size()), 5U, "total entries");
    expect_true(res.entries[0].pool == SatPoolKind::Group1HighPriority, "entry 0 pool");
    expect_true(res.entries[1].pool == SatPoolKind::SpecialHitSparks, "entry 1 pool");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[1].target), 0x00A69AU, "entry 1 target special 0x00A69A");
    expect_true(res.entries[2].pool == SatPoolKind::Group2Entities, "entry 2 pool");
    expect_true(res.entries[2].target == SatDispatchTarget::CustomSat, "entry 2 target custom");
    expect_true(res.entries[3].pool == SatPoolKind::Group4GroundShadow, "entry 3 pool");
    expect_true(res.entries[4].pool == SatPoolKind::Group3Floor, "entry 4 pool");

    expect_true(res.sat_terminated, "sat terminated");
    expect_eq_u32(res.terminated_sat_offset, 32U, "sat offset");
    expect_eq_u32(res.vdp_sync_flags, 0x0BU, "sync flags updated with mask 0x03");
}

static void test_raw_ram_simulation() {
    std::vector<std::uint8_t> ram(0x10000, 0);
    auto ram_off = [](std::uint32_t addr) { return static_cast<std::size_t>(addr - 0x00FF0000U); };

    // Cached mask at $FF188E: marks all g2 slots except slot 5
    const std::uint32_t mask_val = 0x001FFFFFU & ~(1U << 5);
    const std::size_t mask_pos = ram_off(PassBSatDispatcherConfig::kCachedMaskRam);
    ram[mask_pos + 0] = static_cast<std::uint8_t>((mask_val >> 24) & 0xFF);
    ram[mask_pos + 1] = static_cast<std::uint8_t>((mask_val >> 16) & 0xFF);
    ram[mask_pos + 2] = static_cast<std::uint8_t>((mask_val >> 8) & 0xFF);
    ram[mask_pos + 3] = static_cast<std::uint8_t>(mask_val & 0xFF);

    // Group 2 slot 5: type = 1, y = 150
    const std::size_t g2_slot5 = ram_off(PassBSatDispatcherConfig::kGroup2PoolRam + 5 * PassBSatDispatcherConfig::kGroup2SlotStride);
    ram[g2_slot5 + 0] = 0x00;
    ram[g2_slot5 + 1] = 0x01; // type = 1
    ram[g2_slot5 + 12] = 0x00;
    ram[g2_slot5 + 13] = 0x96; // y = 150

    // SAT offset = 16
    ram[ram_off(PassBSatDispatcherConfig::kSatByteOffsetRam)] = 0x00;
    ram[ram_off(PassBSatDispatcherConfig::kSatByteOffsetRam) + 1] = 0x10;

    // SAT buffer link byte at offset 16 - 5 = 11 set to dummy non-zero
    ram[ram_off(PassBSatDispatcherConfig::kVdpSatBufferRam + 11)] = 0x55;

    // Initial sync flags $FF164E = 0x04
    ram[ram_off(PassBSatDispatcherConfig::kVdpSyncFlagsRam)] = 0x04;

    auto res = dispatch_pass_b_sat_raw(ram, 0x00FF0000U);

    expect_eq_u32(static_cast<std::uint32_t>(res.entries.size()), 1U, "raw entries size");
    expect_eq_u32(static_cast<std::uint32_t>(res.entries[0].slot_index), 5U, "slot 5 emitted");

    // Verify SAT link byte terminated with 0
    expect_eq_u32(ram[ram_off(PassBSatDispatcherConfig::kVdpSatBufferRam + 11)], 0U, "sat link terminated");

    // Verify sync flags updated (0x04 | 0x03 = 0x07)
    expect_eq_u32(ram[ram_off(PassBSatDispatcherConfig::kVdpSyncFlagsRam)], 0x07U, "sync flags updated");

    // Verify done mask updated in RAM to 0x001FFFFF
    std::uint32_t written_mask = (static_cast<std::uint32_t>(ram[mask_pos]) << 24) |
                                 (static_cast<std::uint32_t>(ram[mask_pos + 1]) << 16) |
                                 (static_cast<std::uint32_t>(ram[mask_pos + 2]) << 8) |
                                 static_cast<std::uint32_t>(ram[mask_pos + 3]);
    expect_eq_u32(written_mask, 0x001FFFFFU, "all slots marked done in RAM");
}

int main() {
    std::cout << "Running M16.161 Pass B SAT Sprite Dispatcher tests...\n";

    test_rom_constants();
    test_cached_mask_filtering();
    test_descending_y_sorting();
    test_camera_window_filter();
    test_pass_b_multi_pool_dispatch();
    test_raw_ram_simulation();

    if (g_failures == 0) {
        std::cout << "ALL M16.161 TESTS PASSED!\n";
        return 0;
    }

    std::cerr << g_failures << " TEST(S) FAILED!\n";
    return 1;
}
