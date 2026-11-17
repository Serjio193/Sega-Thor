#include "game/render/dynamic_fx_renderer_p1.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

static void test_constants_and_tables() {
    static_assert(DynamicFxP1Config::kMasterFxDispatcherAddr == 0x00B34CU);
    static_assert(DynamicFxP1Config::kDispatcherExitRtsAddr == 0x00B382U);
    static_assert(DynamicFxP1Config::kAuraEmitterAddr == 0x00B3D2U);
    static_assert(DynamicFxP1Config::kAuraTableAddr == 0x00B42CU);
    static_assert(DynamicFxP1Config::kWaveRingEmitterAddr == 0x00B45CU);
    static_assert(DynamicFxP1Config::kWaveSmallTableAddr == 0x00B4C8U);
    static_assert(DynamicFxP1Config::kWaveLargeTableAddr == 0x00B4E0U);
    static_assert(DynamicFxP1Config::kRippleEmitterAddr == 0x00B4FEU);
    static_assert(DynamicFxP1Config::kRippleBitClearAddr == 0x00B54EU);
    static_assert(DynamicFxP1Config::kRippleTableAddr == 0x00B556U);
    static_assert(DynamicFxP1Config::kDynamicFxP1RangeEnd == 0x00B56EU);

    assert(kAuraTable.size() == 16U);
    assert(kAuraTable[0].tile_or_flag == 255U);
    assert(kAuraTable[1].tile_or_flag == 7U);
    assert(kAuraTable[1].dx == 4 && kAuraTable[1].dy == 4);
    assert(kAuraTable[10].tile_or_flag == 7U);
    assert(kAuraTable[10].dx == -4 && kAuraTable[10].dy == -4);
    assert(kAuraTable[15].tile_or_flag == 10U);

    assert(kWaveSmallTable.size() == 8U);
    assert(kWaveSmallTable[0].tile_or_flag == 0U);
    assert(kWaveSmallTable[0].dy == -8);
    assert(kWaveSmallTable[7].tile_or_flag == 3U);

    assert(kWaveLargeTable.size() == 10U);
    assert(kWaveLargeTable[0].tile_or_flag == 30U);
    assert(kWaveLargeTable[0].dy == -12);
    assert(kWaveLargeTable[9].tile_or_flag == 34U);

    assert(kRippleTable.size() == 8U);
    assert(kRippleTable[0].tile_or_flag == 59U);
    assert(kRippleTable[7].tile_or_flag == 62U);
}

static void test_suppression_and_flag_checks() {
    assert(should_render_dynamic_fx(0U, 0x01U));
    assert(!should_render_dynamic_fx(1U, 0x01U));
    assert(!should_render_dynamic_fx(0U, 0x00U));
    assert(!should_render_dynamic_fx(0xFFU, 0xFFU));

    assert(has_aura_fx(0x01U));
    assert(!has_aura_fx(0xFEU));
    assert(has_wave_ring_fx(0x08U));
    assert(!has_wave_ring_fx(0xF7U));
    assert(has_ground_ripple_fx(0x10U));
    assert(!has_ground_ripple_fx(0xEFU));

    DynamicFxEntityState ent{};
    ent.fx_flags_39 = 0x01U;
    auto res = dispatch_dynamic_fx_p1(ent, 1U, 0, 0);
    assert(res.suppressed);
    assert(!res.emitted_aura.has_value());
    assert(!res.emitted_wave.has_value());
    assert(!res.emitted_ripple.has_value());

    ent.fx_flags_39 = 0x00U;
    res = dispatch_dynamic_fx_p1(ent, 0U, 0, 0);
    assert(res.suppressed);
}

static void test_aura_emitter() {
    DynamicFxEntityState ent{};
    ent.world_x = 100;
    ent.world_y = 120;
    ent.elev_z = 10;
    ent.fx_flags_39 = DynamicFxP1Config::kAuraBitMask;
    ent.aura_counter_9b = 0U;

    // Frame 1: counter becomes 1, idx = (1 >> 2) & 0xF = 0 -> tile 255 (skipped)
    auto sp1 = step_aura_fx(ent, 0, 0, 0);
    assert(!sp1.has_value());
    assert(ent.aura_counter_9b == 1U);

    // Advance to counter = 4 -> idx = 1 -> tile 7, dx=4, dy=4
    ent.aura_counter_9b = 3U;
    auto sp2 = step_aura_fx(ent, 0, 0, 5);
    assert(sp2.has_value());
    assert(ent.aura_counter_9b == 4U);
    assert(sp2->size == 0x00U); // 1x1
    assert(sp2->link == 6U);
    assert(sp2->tile_attr == (0x8500U | 7U));
    assert(sp2->x == 100 - 0 + 128 + 4); // 232
    assert(sp2->y == (120 - 0 + 128) - 10 + 4); // 242

    // Counter wrapping at 80
    ent.aura_counter_9b = 79U;
    (void)step_aura_fx(ent, 0, 0, 0);
    assert(ent.aura_counter_9b == 0U);
}

static void test_wave_ring_emitter() {
    // Small wave: shadow_preset <= 8
    DynamicFxEntityState ent_small{};
    ent_small.world_x = 50;
    ent_small.world_y = 70;
    ent_small.ground_h = 5;
    ent_small.shadow_preset_3d = 8U;
    ent_small.wave_counter_98 = 0U;

    auto sp1 = step_wave_ring_fx(ent_small, 0, 0, 2);
    assert(sp1.has_value());
    assert(ent_small.wave_counter_98 == 1U);
    assert(sp1->size == 0x05U); // 2x2
    assert(sp1->link == 3U);
    assert(sp1->tile_attr == (0x8500U | 0U));
    assert(sp1->x == 50 + 128 + 0);
    assert(sp1->y == 70 + 128 + 5 - 8);

    // Small wave wrapping at 8
    ent_small.wave_counter_98 = 7U;
    (void)step_wave_ring_fx(ent_small, 0, 0, 0);
    assert(ent_small.wave_counter_98 == 0U);

    // Large wave: shadow_preset > 8
    DynamicFxEntityState ent_large{};
    ent_large.world_x = 80;
    ent_large.world_y = 90;
    ent_large.ground_h = 0;
    ent_large.shadow_preset_3d = 9U;
    ent_large.wave_counter_98 = 0U;

    auto sp2 = step_wave_ring_fx(ent_large, 0, 0, 10);
    assert(sp2.has_value());
    assert(ent_large.wave_counter_98 == 1U);
    assert(sp2->size == 0x05U); // 2x2
    assert(sp2->link == 11U);
    assert(sp2->tile_attr == (0x8500U | 30U));
    assert(sp2->y == 90 + 128 + 0 - 12);

    // Large wave wrapping at 10
    ent_large.wave_counter_98 = 9U;
    (void)step_wave_ring_fx(ent_large, 0, 0, 0);
    assert(ent_large.wave_counter_98 == 0U);
}

static void test_ground_ripple_emitter() {
    DynamicFxEntityState ent{};
    ent.world_x = 60;
    ent.world_y = 60;
    ent.ground_h = 2;
    ent.fx_flags_39 = DynamicFxP1Config::kGroundRippleBitMask;
    ent.wave_counter_98 = 0U;

    auto sp = step_ground_ripple_fx(ent, 0, 0, 0);
    assert(sp.has_value());
    assert(sp->tile_attr == (0x8500U | 59U));
    assert(sp->size == 0x05U);
    assert(ent.wave_counter_98 == 1U);
    assert(ent.fx_flags_39 & DynamicFxP1Config::kGroundRippleBitMask);

    // Advance to end of ripple (count reaching 8)
    ent.wave_counter_98 = 7U;
    sp = step_ground_ripple_fx(ent, 0, 0, 0);
    assert(sp.has_value());
    assert(ent.wave_counter_98 == 0U);
    // Bit 4 must be cleared!
    assert(!(ent.fx_flags_39 & DynamicFxP1Config::kGroundRippleBitMask));
}

static void test_clipping_and_limits() {
    DynamicFxEntityState ent{};
    ent.world_x = 2000; // Far out of viewport
    ent.world_y = 2000;
    ent.fx_flags_39 = DynamicFxP1Config::kAuraBitMask;
    ent.aura_counter_9b = 4U; // points to valid tile 7

    auto sp = step_aura_fx(ent, 0, 0, 0);
    assert(!sp.has_value()); // Clipped

    // 80 sprite limit check
    ent.world_x = 50;
    ent.world_y = 50;
    sp = step_aura_fx(ent, 0, 0, 80U);
    assert(!sp.has_value());
}

static void test_dispatch_all_fx_combined() {
    DynamicFxEntityState ent{};
    ent.world_x = 50;
    ent.world_y = 50;
    ent.elev_z = 0;
    ent.ground_h = 0;
    ent.fx_flags_39 = DynamicFxP1Config::kAuraBitMask |
                      DynamicFxP1Config::kWaveRingBitMask |
                      DynamicFxP1Config::kGroundRippleBitMask;
    ent.shadow_preset_3d = 5U;
    ent.aura_counter_9b = 3U; // becomes 4 -> valid aura
    ent.wave_counter_98 = 0U;

    auto res = dispatch_dynamic_fx_p1(ent, 0U, 0, 0, 0U, 0U);
    assert(!res.suppressed);
    assert(res.emitted_aura.has_value());
    assert(res.emitted_wave.has_value());
    assert(res.emitted_ripple.has_value());
    assert(res.final_sprite_count == 3U);
    assert(res.final_sat_offset == 24U);
}

static void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0U); // 64KB RAM window at 0x00FF0000
    const std::uint32_t ent_addr = 0x00FF2000U;
    const std::size_t ent_off = ent_addr - 0x00FF0000U;

    // Set entity fields
    // world_x = 60, world_y = 60, fx_flags_39 = bit 4 (ripple)
    ram[ent_off + DynamicFxP1Config::kWorldXOffset + 1] = 60U;
    ram[ent_off + DynamicFxP1Config::kWorldYOffset + 1] = 60U;
    ram[ent_off + DynamicFxP1Config::kFxModeFlagsOffset] = DynamicFxP1Config::kGroundRippleBitMask;
    ram[ent_off + DynamicFxP1Config::kWaveRippleCounterOffset] = 7U; // last frame

    // sat_link = 2, sat_offset = 16
    const std::size_t link_off = DynamicFxP1Config::kSatLinkCountRam - 0x00FF0000U;
    ram[link_off + 1] = 2U;
    const std::size_t sat_off = DynamicFxP1Config::kSatByteOffsetRam - 0x00FF0000U;
    ram[sat_off + 1] = 16U;

    auto res = dispatch_dynamic_fx_p1_raw(ram, ent_addr, 0x00FF0000U);
    assert(!res.suppressed);
    assert(res.emitted_ripple.has_value());
    assert(res.ripple_completed);
    assert(res.final_sprite_count == 3U);
    assert(res.final_sat_offset == 24U);

    // Verify entity ram updated
    assert(!(ram[ent_off + DynamicFxP1Config::kFxModeFlagsOffset] & DynamicFxP1Config::kGroundRippleBitMask));
    assert(ram[ent_off + DynamicFxP1Config::kWaveRippleCounterOffset] == 0U);

    // Verify link & offset updated in RAM
    assert(ram[link_off + 1] == 3U);
    assert(ram[sat_off + 1] == 24U);

    // Verify SAT entry written
    const std::size_t sat_buf_off = DynamicFxP1Config::kVdpSatBufferRam - 0x00FF0000U + 16U;
    assert(ram[sat_buf_off + 2] == 0x05U); // 2x2
    assert(ram[sat_buf_off + 3] == 3U);    // link
}

int main() {
    test_constants_and_tables();
    test_suppression_and_flag_checks();
    test_aura_emitter();
    test_wave_ring_emitter();
    test_ground_ripple_emitter();
    test_clipping_and_limits();
    test_dispatch_all_fx_combined();
    test_raw_ram_integration();

    std::cout << "All M16.167 dynamic FX renderer p1 tests passed successfully!\n";
    return 0;
}
