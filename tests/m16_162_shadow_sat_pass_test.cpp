#include "game/render/shadow_sat_pass.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

void test_shadow_constants() {
    static_assert(ShadowSatPassConfig::kShadowPassAddr == 0x00A6A0U);
    static_assert(ShadowSatPassConfig::kShadowPassEnd == 0x00A786U);
    static_assert(ShadowSatPassConfig::kPreCompositionHelperAddr == 0x00A342U);

    static_assert(ShadowSatPassConfig::kGroup1PoolRam == 0x00FF2954U);
    static_assert(ShadowSatPassConfig::kGroup1SlotCount == 4U);
    static_assert(ShadowSatPassConfig::kGroup1SlotStride == 0x5AU);

    static_assert(ShadowSatPassConfig::kGroup2PoolRam == 0x00FF19E8U);
    static_assert(ShadowSatPassConfig::kGroup2SlotCount == 21U);
    static_assert(ShadowSatPassConfig::kGroup2SlotStride == 0xBCU);

    static_assert(ShadowSatPassConfig::kGroup4PoolRam == 0x00FF2ABCU);
    static_assert(ShadowSatPassConfig::kGroup4SlotCount == 8U);
    static_assert(ShadowSatPassConfig::kGroup4SlotStride == 0x5AU);

    static_assert(ShadowSatPassConfig::kGroup3PoolRam == 0x00FF2D8CU);
    static_assert(ShadowSatPassConfig::kGroup3SlotCount == 6U);
    static_assert(ShadowSatPassConfig::kGroup3SlotStride == 0x5AU);

    static_assert(ShadowSatPassConfig::kShadowFlagMask == 0x04U);
    static_assert(ShadowSatPassConfig::kInitialMaxYSentinel == -1);
}

void test_shadow_filtering_and_mask() {
    std::vector<ShadowEntitySlot> g2(21);
    for (std::size_t i = 0; i < 21; ++i) {
        g2[i].type = 1;
        g2[i].y = 100;
        g2[i].flags_3a = 0; // bit 2 clear
    }

    // All inactive since bit 2 is clear
    std::uint32_t mask = build_shadow_inactive_mask(g2);
    assert(mask == 0x001FFFFFU);

    // Enable shadow on slot 4
    g2[4].flags_3a = 0x04U;
    mask = build_shadow_inactive_mask(g2);
    assert(mask == (0x001FFFFFU & ~(1U << 4)));

    // Slot with type <= 0 is inactive even if bit 2 is set
    g2[4].type = 0;
    mask = build_shadow_inactive_mask(g2);
    assert(mask == 0x001FFFFFU);
}

void test_descending_y_depth_order() {
    std::vector<ShadowEntitySlot> g2(21);
    for (std::size_t i = 0; i < 21; ++i) {
        g2[i].type = 1;
        g2[i].y = 0;
        g2[i].flags_3a = 0;
    }

    // Slots matching rom_shadow_sat_pass_test.py
    g2[1].flags_3a = 0x04U;
    g2[1].y = 100;

    g2[6].flags_3a = 0x04U;
    g2[6].y = 250;

    g2[12].flags_3a = 0x04U;
    g2[12].y = 180;
    g2[12].flags_36 = 1U; // Custom SAT target

    std::uint32_t done_mask = 0U;
    const std::vector<std::size_t> order = sort_group2_shadows_y_depth_order(g2, &done_mask);

    assert(order.size() == 3U);
    assert(order[0] == 6U);  // Y = 250
    assert(order[1] == 12U); // Y = 180
    assert(order[2] == 1U);  // Y = 100
    assert(done_mask == 0x001FFFFFU);
}

void test_elevation_stacking_and_tiers() {
    ShadowEntitySlot ent{};
    ent.type = 1;
    ent.y = 120;
    ent.ground_h = 10;
    ent.z = 0;
    ent.flags_3a = 0x04U;

    assert(ent.ground_contact_y() == 130);
    assert(ent.sprite_screen_y() == 130);
    assert(ent.shadow_tier() == ShadowScaleTier::Full);

    ent.z = 8;
    assert(ent.ground_contact_y() == 130);
    assert(ent.sprite_screen_y() == 122);
    assert(ent.shadow_tier() == ShadowScaleTier::Full);

    ent.z = 24;
    assert(ent.ground_contact_y() == 130);
    assert(ent.sprite_screen_y() == 106);
    assert(ent.shadow_tier() == ShadowScaleTier::Medium);

    ent.z = 48;
    assert(ent.ground_contact_y() == 130);
    assert(ent.sprite_screen_y() == 82);
    assert(ent.shadow_tier() == ShadowScaleTier::Small);

    ent.z = 50;
    assert(ent.ground_contact_y() == 130);
    assert(ent.sprite_screen_y() == 80);
    assert(ent.shadow_tier() == ShadowScaleTier::Tiny);
}

void test_multi_pool_shadow_pipeline() {
    std::vector<ShadowEntitySlot> g1(4);
    std::vector<ShadowEntitySlot> g2(21);
    std::vector<ShadowEntitySlot> g4(8);
    std::vector<ShadowEntitySlot> g3(6);

    // Group 1: slot 0 has shadow
    g1[0].type = 2;
    g1[0].y = 80;
    g1[0].ground_h = 5;
    g1[0].z = 2;
    g1[0].flags_3a = 0x04U;

    // Group 2: slot 3 has shadow
    g2[3].type = 3;
    g2[3].y = 150;
    g2[3].ground_h = 0;
    g2[3].z = 15;
    g2[3].flags_3a = 0x04U;

    // Group 4: slot 1 is active
    g4[1].type = 4;
    g4[1].y = 200;

    // Group 3: slot 2 is active
    g3[2].type = 5;
    g3[2].y = 50;

    const auto res = dispatch_shadow_sat_pass(g1, g2, g4, g3, 0U);

    assert(res.entries.size() == 4U);
    assert(res.entries[0].pool == SatPoolKind::Group1HighPriority);
    assert(res.entries[0].slot_index == 0U);
    assert(res.entries[0].tier == ShadowScaleTier::Full);

    assert(res.entries[1].pool == SatPoolKind::Group2Entities);
    assert(res.entries[1].slot_index == 3U);
    assert(res.entries[1].tier == ShadowScaleTier::Medium);

    assert(res.entries[2].pool == SatPoolKind::Group4GroundShadow);
    assert(res.entries[2].slot_index == 1U);

    assert(res.entries[3].pool == SatPoolKind::Group3Floor);
    assert(res.entries[3].slot_index == 2U);

    assert(res.sat_terminated == true);
    assert(res.terminated_sat_offset == 8U);
}

void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00U);
    const std::uint32_t ram_base = 0x00FF0000U;

    auto write_i16 = [&](std::uint32_t addr, std::int16_t val) {
        const std::size_t off = addr - ram_base;
        ram[off] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFF);
        ram[off + 1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFF);
    };

    // Current SAT byte offset = 0x20
    write_i16(ShadowSatPassConfig::kSatByteOffsetRam, 0x0020);

    // Group 2, slot 2: type=1, y=90, ground_h=4, z=10, flags_3a=0x04
    const std::uint32_t g2_slot2 = ShadowSatPassConfig::kGroup2PoolRam +
                                   static_cast<std::uint32_t>(2 * ShadowSatPassConfig::kGroup2SlotStride);
    write_i16(g2_slot2 + ShadowSatPassConfig::kTypeOffset, 1);
    write_i16(g2_slot2 + ShadowSatPassConfig::kYOffset, 90);
    write_i16(g2_slot2 + ShadowSatPassConfig::kGroundHOffset, 4);
    write_i16(g2_slot2 + ShadowSatPassConfig::kZOffset, 10);
    ram[g2_slot2 + ShadowSatPassConfig::kFlags3AOffset - ram_base] = 0x04U;

    // Pre-populate SAT buffer link byte with 0xFF
    const std::uint32_t link_addr = ShadowSatPassConfig::kVdpSatBufferRam + 0x0020U - 5U;
    ram[link_addr - ram_base] = 0xFFU;

    const auto res = dispatch_shadow_sat_raw(ram, ram_base);

    assert(res.entries.size() == 1U);
    assert(res.entries[0].pool == SatPoolKind::Group2Entities);
    assert(res.entries[0].slot_index == 2U);
    assert(res.entries[0].ground_y == 94);
    assert(res.entries[0].tier == ShadowScaleTier::Medium);
    assert(res.sat_terminated == true);
    assert(res.terminated_sat_offset == 0x0020U);

    // Verifies link terminator cleared byte at link_addr
    assert(ram[link_addr - ram_base] == 0x00U);
}

int main() {
    test_shadow_constants();
    test_shadow_filtering_and_mask();
    test_descending_y_depth_order();
    test_elevation_stacking_and_tiers();
    test_multi_pool_shadow_pipeline();
    test_raw_ram_integration();

    std::cout << "m16_162_shadow_sat_pass_test: All 6 test suites passed successfully.\n";
    return 0;
}
