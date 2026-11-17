#include "game/render/static_fast_sat_compositor.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

void test_constants_and_dispatch_selection() {
    static_assert(StaticFastSatCompositorConfig::kStaticCompositorAddr == 0x00AD76U);
    static_assert(StaticFastSatCompositorConfig::kStaticExitRtsAddr == 0x00B010U);
    static_assert(StaticFastSatCompositorConfig::kDynamicTailBsrAddr == 0x00B012U);
    static_assert(StaticFastSatCompositorConfig::kFastCompositorAddr == 0x00B022U);
    static_assert(StaticFastSatCompositorConfig::kBufferBaseSubtractAddr == 0x00B14AU);
    static_assert(StaticFastSatCompositorConfig::kFastExitRtsAddr == 0x00B15CU);
    static_assert(StaticFastSatCompositorConfig::kCompositorRangeEnd == 0x00B15EU);

    // Dispatch mode selection: bit 0 of flags_36
    assert(select_sat_compositor_tier(0x00U) == 0x00AD76U);
    assert(select_sat_compositor_tier(0x02U) == 0x00AD76U);
    assert(select_sat_compositor_tier(0x01U) == 0x00A9A0U);
    assert(select_sat_compositor_tier(0x07U) == 0x00A9A0U);

    // Visibility: bit 1 of flags_36
    assert(is_static_entity_visible(0x00U) == true);
    assert(is_static_entity_visible(0x01U) == true);
    assert(is_static_entity_visible(0x02U) == false);
    assert(is_static_entity_visible(0x03U) == false);
}

void test_viewport_clipping() {
    // Center inside viewport
    assert(!is_static_viewport_clipped(0x100, 0x100));

    // Top-left boundary
    assert(!is_static_viewport_clipped(0x60, 0x60));

    // Bottom-right boundary
    assert(!is_static_viewport_clipped(0x160, 0x1C0));

    // Off-screen top
    assert(is_static_viewport_clipped(0x5F, 0x100));

    // Off-screen bottom
    assert(is_static_viewport_clipped(0x161, 0x100));

    // Off-screen left
    assert(is_static_viewport_clipped(0x100, 0x5F));

    // Off-screen right
    assert(is_static_viewport_clipped(0x100, 0x1C1));
}

void test_static_compositor_execution() {
    MasterEntityRenderState ent{};
    ent.type = 1;
    ent.world_x = 300;
    ent.world_y = 250;
    ent.elev_z = 10;
    ent.ground_h = 5;
    ent.flags_36 = 0x00U; // visible, static
    ent.flags_3a = 0x04U; // shadow enabled, preset 0

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x05U, .tile_attr = 0x1000U, .x_offset = 0 },
        FrameSpritePiece{ .y_offset = 16, .size = 0x05U, .tile_attr = 0x1004U, .x_offset = 0 }
    };

    // cam = (200, 150)
    // screen_x = 300 - 200 + 128 = 228
    // screen_y = (250 - 150 + 128) - 10 = 218
    const auto res = compose_static_entity_sat(ent, pieces, 200, 150, 0U, 0U);

    assert(!res.early_exited);
    assert(res.screen_x == 228);
    assert(res.screen_y == 218);
    assert(res.emitted_pieces.size() == 2U);
    assert(res.emitted_shadow.has_value());

    // 2 pieces + 1 shadow = 3 sprites (24 bytes)
    assert(res.final_sprite_count == 3U);
    assert(res.final_sat_offset == 24U);

    // Verify pieces
    assert(res.emitted_pieces[0].y == 218);
    assert(res.emitted_pieces[0].x == 228);
    assert(res.emitted_pieces[0].link == 1U);

    assert(res.emitted_pieces[1].y == 234); // 218 + 16
    assert(res.emitted_pieces[1].x == 228);
    assert(res.emitted_pieces[1].link == 2U);

    // Verify shadow
    assert(res.emitted_shadow->link == 3U);
    assert(res.emitted_shadow->tile_attr == (0x8000U | 0x0548U));
}

void test_static_compositor_early_exit() {
    MasterEntityRenderState ent{};
    ent.type = 1;
    ent.flags_36 = 0x02U; // invisible bit 1 set

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x05U, .tile_attr = 0x1000U, .x_offset = 0 }
    };

    const auto res = compose_static_entity_sat(ent, pieces, 0, 0, 0U, 0U);
    assert(res.early_exited);
    assert(res.emitted_pieces.empty());
    assert(!res.emitted_shadow.has_value());
    assert(res.final_sprite_count == 0U);
}

void test_fast_shadowless_compositor() {
    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x00U, .tile_attr = 0x0500U, .x_offset = 0 },
        FrameSpritePiece{ .y_offset = 8, .size = 0x00U, .tile_attr = 0x0501U, .x_offset = 0 }
    };

    // world = (300, 250), elev = 15, cam = (200, 150)
    // screen_x = 300 - 200 + 128 = 228
    // screen_y = (250 - 150 + 128) - 15 = 213
    const auto res = compose_fast_shadowless_sat(300, 250, 15, pieces, 200, 150, 5U, 40U);

    assert(res.screen_x == 228);
    assert(res.screen_y == 213);
    assert(res.emitted_pieces.size() == 2U);
    assert(res.final_sprite_count == 7U);
    assert(res.final_sat_offset == 56U); // 40 + 16

    assert(res.emitted_pieces[0].y == 213);
    assert(res.emitted_pieces[0].link == 6U);
    assert(res.emitted_pieces[1].y == 221); // 213 + 8
    assert(res.emitted_pieces[1].link == 7U);
}

void test_hardware_sprite_limit_capping() {
    MasterEntityRenderState ent{};
    ent.type = 1;
    ent.world_x = 200;
    ent.world_y = 200;
    ent.flags_36 = 0;
    ent.flags_3a = 0x04U;

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x05U, .tile_attr = 0x1000U, .x_offset = 0 },
        FrameSpritePiece{ .y_offset = 16, .size = 0x05U, .tile_attr = 0x1004U, .x_offset = 0 }
    };

    // Current sprites at 79: only 1 piece fits, second piece and shadow suppressed
    const auto res_static = compose_static_entity_sat(ent, pieces, 100, 100, 79U, 0U);
    assert(res_static.emitted_pieces.size() == 1U);
    assert(!res_static.emitted_shadow.has_value());
    assert(res_static.final_sprite_count == 80U);

    // Fast compositor at 80: 0 pieces fit
    const auto res_fast = compose_fast_shadowless_sat(200, 200, 0, pieces, 100, 100, 80U, 0U);
    assert(res_fast.emitted_pieces.empty());
    assert(res_fast.final_sprite_count == 80U);
}

void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00U);
    const std::uint32_t ram_base = 0x00FF0000U;

    auto write_i16 = [&](std::uint32_t addr, std::int16_t val) {
        const std::size_t off = addr - ram_base;
        ram[off] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFF);
        ram[off + 1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFF);
    };

    // Current sprites = 4, SAT offset = 32
    write_i16(StaticFastSatCompositorConfig::kSatLinkCountRam, 4);
    write_i16(StaticFastSatCompositorConfig::kSatByteOffsetRam, 32);

    // Camera Plane 1: cam_x = 50, cam_y = 50
    ram[0x00FF16F0U - ram_base] = 0x00U;
    write_i16(StaticFastSatCompositorConfig::kCameraPlane1Ram, 50);
    write_i16(StaticFastSatCompositorConfig::kCameraPlane1Ram + 2, 50);

    // Entity slot at $FF19E8
    const std::uint32_t ent_addr = 0x00FF19E8U;
    write_i16(ent_addr + MasterSatCompositorConfig::kTypeOffset, 1);
    write_i16(ent_addr + MasterSatCompositorConfig::kWorldXOffset, 100);
    write_i16(ent_addr + MasterSatCompositorConfig::kWorldYOffset, 100);
    write_i16(ent_addr + MasterSatCompositorConfig::kElevZOffset, 0);
    write_i16(ent_addr + MasterSatCompositorConfig::kGroundHOffset, 0);
    ram[ent_addr + MasterSatCompositorConfig::kFlags36Offset - ram_base] = 0x00U;
    ram[ent_addr + MasterSatCompositorConfig::kFlags3AOffset - ram_base] = 0x00U; // no shadow

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x00U, .tile_attr = 0x2000U, .x_offset = 0 }
    };

    const auto res_static = compose_static_entity_sat_raw(ram, ent_addr, pieces, ram_base);
    assert(res_static.emitted_pieces.size() == 1U);
    assert(res_static.final_sprite_count == 5U);
    assert(res_static.final_sat_offset == 40U);

    // Verify stored RAM state
    const std::size_t link_off = StaticFastSatCompositorConfig::kSatLinkCountRam - ram_base;
    const std::uint16_t stored_sprites = static_cast<std::uint16_t>((ram[link_off] << 8) | ram[link_off + 1]);
    assert(stored_sprites == 5U);

    // Run fast compositor raw with 1 piece
    const auto res_fast = compose_fast_shadowless_sat_raw(ram, 120, 120, 0, pieces, ram_base);
    assert(res_fast.emitted_pieces.size() == 1U);
    assert(res_fast.final_sprite_count == 6U);
    assert(res_fast.final_sat_offset == 48U);
}

int main() {
    test_constants_and_dispatch_selection();
    test_viewport_clipping();
    test_static_compositor_execution();
    test_static_compositor_early_exit();
    test_fast_shadowless_compositor();
    test_hardware_sprite_limit_capping();
    test_raw_ram_integration();

    std::cout << "m16_165_static_fast_sat_compositor_test: All 7 test suites passed successfully.\n";
    return 0;
}
