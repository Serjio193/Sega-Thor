#include "game/render/dynamic_fx_renderer_p2.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

static void test_constants_and_tables() {
    static_assert(DynamicFxP2Config::kHaloEmitterAddr == 0x00B56EU);
    static_assert(DynamicFxP2Config::kHaloExitRtsAddr == 0x00B5C6U);
    static_assert(DynamicFxP2Config::kHaloBitClearAddr == 0x00B5C8U);
    static_assert(DynamicFxP2Config::kHaloTableAddr == 0x00B5D0U);
    static_assert(DynamicFxP2Config::kSwirlEmitterAddr == 0x00B610U);
    static_assert(DynamicFxP2Config::kSwirlExitRtsAddr == 0x00B65EU);
    static_assert(DynamicFxP2Config::kSwirlTableAddr == 0x00B660U);
    static_assert(DynamicFxP2Config::kSecondarySwirlAddr == 0x00B66AU);
    static_assert(DynamicFxP2Config::kFxBlitterAddr == 0x00B6A6U);
    static_assert(DynamicFxP2Config::kFxBlitterExitRtsAddr == 0x00B72EU);
    static_assert(DynamicFxP2Config::kRawPieceBlitterAddr == 0x00B730U);
    static_assert(DynamicFxP2Config::kRawPieceBlitterExitRtsAddr == 0x00B798U);
    static_assert(DynamicFxP2Config::kDynamicFxP2RangeEnd == 0x00B79AU);

    assert(kHaloTable.size() == 21U);
    assert(kHaloTable[0].frame_flag == 255U);
    assert(kHaloTable[0].dx == 3 && kHaloTable[0].dy == 3);
    assert(kHaloTable[1].frame_flag == 6U);
    assert(kHaloTable[1].dx == 6 && kHaloTable[1].dy == 3);
    assert(kHaloTable[20].frame_flag == 4U);
    assert(kHaloTable[20].dx == -2 && kHaloTable[20].dy == 4);

    assert(kSwirlTable.size() == 9U);
    assert(kSwirlTable[0] == 20U && kSwirlTable[2] == 20U);
    assert(kSwirlTable[3] == 22U && kSwirlTable[5] == 22U);
    assert(kSwirlTable[6] == 24U && kSwirlTable[8] == 24U);
}

static void test_math_formulas_and_clipping() {
    // Tile attribute priority mask
    assert(compute_fx_tile_attr(0x0200U) == 0x055EU);
    assert(compute_fx_tile_attr(0x8200U) == 0x855EU);
    assert(compute_fx_tile_attr(0x0000U) == 0x055EU);
    assert(compute_fx_tile_attr(0x8000U) == 0x855EU);

    // Swirl Y offset: (preset >> 2) - 6
    assert(compute_swirl_y_offset(0x00U) == -6);
    assert(compute_swirl_y_offset(0x18U) == 0);
    assert(compute_swirl_y_offset(0x20U) == 2);

    // Secondary swirl Y offset: -((preset >> 2) + 2)
    assert(compute_secondary_swirl_y_offset(0x00U) == -2);
    assert(compute_secondary_swirl_y_offset(0x18U) == -8);

    // Viewport clipping: Y [0x60, 0x160], X [0x60, 0x1C0]
    assert(!is_dynamic_fx_p2_clipped(0x80, 0x80));
    assert(is_dynamic_fx_p2_clipped(0x50, 0x80));   // Y underflow
    assert(is_dynamic_fx_p2_clipped(0x170, 0x80));  // Y overflow
    assert(is_dynamic_fx_p2_clipped(0x80, 0x50));   // X underflow
    assert(is_dynamic_fx_p2_clipped(0x80, 0x1D0));  // X overflow
}

static void test_fx_blitter() {
    // Frame 4 (1 piece: size 0x00, y -4, tile 76, x -3)
    const auto pieces4 = get_fx_frame_pieces(4U);
    assert(pieces4.size() == 1U);
    assert(pieces4[0].size == 0x00U);
    assert(pieces4[0].y_offset == -4);
    assert(pieces4[0].tile_attr == 76U);

    auto emitted = blit_fx_frame(4U, 0x80, 0x80, 0x055EU, 0U);
    assert(emitted.size() == 1U);
    assert(emitted[0].y == 0x80 - 4);
    assert(emitted[0].x == 0x80 - 3);
    assert(emitted[0].tile_attr == 0x055EU + 76U);
    assert(emitted[0].link == 1U);

    // Frame 20 (2 pieces: size 0x05 each, 2x2)
    const auto pieces20 = get_fx_frame_pieces(20U);
    assert(pieces20.size() == 2U);
    emitted = blit_fx_frame(20U, 0x80, 0x80, 0x055EU, 5U);
    assert(emitted.size() == 2U);
    assert(emitted[0].size == 0x05U);
    assert(emitted[0].link == 6U);
    assert(emitted[1].size == 0x05U);
    assert(emitted[1].link == 7U);

    // 80 sprite limit check
    emitted = blit_fx_frame(20U, 0x80, 0x80, 0x055EU, 79U);
    assert(emitted.size() == 1U); // Only 1 can fit before hitting 80
}

static void test_raw_piece_blitter() {
    const auto pieces = get_fx_frame_pieces(4U);
    auto emitted = blit_raw_pieces(pieces, 0x00, 0x00, 0x055EU, 2U);
    assert(emitted.size() == 1U);
    assert(emitted[0].y == 0x00 + 0x80 - 4); // Screen bias 0x80 applied
    assert(emitted[0].x == 0x00 + 0x80 - 3);
    assert(emitted[0].link == 3U);
}

static void test_halo_emitter() {
    DynamicFxEntityStateP2 ent{};
    ent.world_x = 100;
    ent.world_y = 120;
    ent.elev_z = 15;
    ent.base_tile_18 = 0x8000U;
    ent.fx_flags_39 = DynamicFxP2Config::kHaloBitMask;
    ent.halo_counter = 2U; // Will decrement to 1 -> entry 1 (frame 6, dx 6, dy 3)

    bool completed = false;
    auto sp = step_halo_fx(ent, false, 0, 0, 0, completed);
    assert(!completed);
    assert(ent.halo_counter == 1U);
    assert(sp.size() == 1U);
    assert(sp[0].size == 0x00U);
    assert(sp[0].tile_attr == (0x855EU + 78U)); // 0x8000 | 0x055E + 78
    assert(sp[0].x == 100 - 0 + 0x80 + 6 - 4);  // dx = 6, unflip = -4
    assert(sp[0].y == 120 - 0 + 0x80 + 15 + 3 - 4); // dy = 3, piece y = -4

    // Pause test: counter should not decrement when is_paused = true
    sp = step_halo_fx(ent, true, 0, 0, 0, completed);
    assert(!completed);
    assert(ent.halo_counter == 1U);

    // Counter reaches 0: clears bit 2 of fx_flags_39 and sets completed = true
    sp = step_halo_fx(ent, false, 0, 0, 0, completed);
    assert(completed);
    assert(ent.halo_counter == 0U);
    assert(!(ent.fx_flags_39 & DynamicFxP2Config::kHaloBitMask));
    assert(sp.empty());
}

static void test_swirl_emitter() {
    DynamicFxEntityStateP2 ent{};
    ent.world_x = 50;
    ent.world_y = 70;
    ent.elev_z = 10; // Airborne! Swirl must be inactive!
    ent.base_tile_18 = 0x0000U;
    ent.shadow_preset_3d = 0x18U; // Y shift = 0
    ent.fx_flags_39 = DynamicFxP2Config::kSwirlBitMask;
    ent.swirl_counter = 0U;

    auto sp = step_swirl_fx(ent, false, 0, 0, 0);
    assert(sp.empty()); // Inactive when airborne

    // Grounded (elev_z == 0)
    ent.elev_z = 0;
    sp = step_swirl_fx(ent, false, 0, 0, 0);
    assert(ent.swirl_counter == 1U);
    assert(sp.size() == 2U); // Frame 20 has 2 pieces
    assert(sp[0].size == 0x05U);
    assert(sp[0].y == 70 + 0x80 + 0 + 0); // dy=0, piece y=0

    // Pause test
    sp = step_swirl_fx(ent, true, 0, 0, 0);
    assert(ent.swirl_counter == 1U); // Unchanged

    // Secondary swirl variant
    sp = step_swirl_fx(ent, false, 0, 0, 0, true);
    assert(ent.swirl_counter == 2U);
    assert(sp.size() == 2U); // Frame 21 (20 + 1) has 2 pieces
}

static void test_dispatch_dynamic_fx_p2() {
    DynamicFxEntityStateP2 ent{};
    ent.world_x = 60;
    ent.world_y = 60;
    ent.elev_z = 0;
    ent.base_tile_18 = 0x8000U;
    ent.shadow_preset_3d = 0x18U;
    ent.fx_flags_39 = DynamicFxP2Config::kSwirlBitMask | DynamicFxP2Config::kHaloBitMask;
    ent.halo_counter = 2U;
    ent.swirl_counter = 0U;

    auto res = dispatch_dynamic_fx_p2(ent, false, 0U, 0, 0);
    assert(!res.suppressed);
    assert(!res.emitted_swirl.empty());
    assert(!res.emitted_halo.empty());
    assert(res.final_sprite_count == res.emitted_swirl.size() + res.emitted_halo.size());

    // Global suppression check
    res = dispatch_dynamic_fx_p2(ent, false, 1U, 0, 0);
    assert(res.suppressed);
}

static void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0U); // 64KB RAM window at 0x00FF0000
    const std::uint32_t ent_addr = 0x00FF2000U;
    const std::size_t ent_off = ent_addr - 0x00FF0000U;

    // Set entity fields
    // world_x = 70, world_y = 70, base_tile = 0x8000
    ram[ent_off + DynamicFxP2Config::kWorldXOffset + 1] = 70U;
    ram[ent_off + DynamicFxP2Config::kWorldYOffset + 1] = 70U;
    ram[ent_off + DynamicFxP2Config::kBaseTileOffset] = 0x80U;
    ram[ent_off + DynamicFxP2Config::kFxModeFlagsOffset] = DynamicFxP2Config::kHaloBitMask;
    ram[ent_off + DynamicFxP2Config::kHaloCounterOffset] = 1U; // Will decrement to 0 -> completes!

    // sat_link = 4, sat_offset = 32
    const std::size_t link_off = DynamicFxP2Config::kSatLinkCountRam - 0x00FF0000U;
    ram[link_off + 1] = 4U;
    const std::size_t sat_off = DynamicFxP2Config::kSatByteOffsetRam - 0x00FF0000U;
    ram[sat_off + 1] = 32U;

    auto res = dispatch_dynamic_fx_p2_raw(ram, ent_addr, 0x00FF0000U);
    assert(!res.suppressed);
    assert(res.halo_completed);

    // Verify bit 2 cleared in entity RAM
    assert(!(ram[ent_off + DynamicFxP2Config::kFxModeFlagsOffset] & DynamicFxP2Config::kHaloBitMask));
    assert(ram[ent_off + DynamicFxP2Config::kHaloCounterOffset] == 0U);
}

int main() {
    test_constants_and_tables();
    test_math_formulas_and_clipping();
    test_fx_blitter();
    test_raw_piece_blitter();
    test_halo_emitter();
    test_swirl_emitter();
    test_dispatch_dynamic_fx_p2();
    test_raw_ram_integration();

    std::cout << "All M16.168 dynamic FX renderer p2 tests passed successfully!\n";
    return 0;
}
