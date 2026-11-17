#include "game/render/master_sat_compositor.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

void test_embedded_tables_and_constants() {
    static_assert(MasterSatCompositorConfig::kCompositorEntryAddr == 0x00A9A0U);
    static_assert(MasterSatCompositorConfig::kEarlyExitRtsAddr == 0x00ACFCU);
    static_assert(MasterSatCompositorConfig::kShadowTableAddr == 0x00ACFEU);
    static_assert(MasterSatCompositorConfig::kTileCountTableAddr == 0x00AD56U);
    static_assert(MasterSatCompositorConfig::kCompositorEndAddr == 0x00AD76U);

    // Verify 4 shadow presets against ROM
    assert(kShadowTable[0].y_offset == 3);
    assert(kShadowTable[0].size_word == 0x0000);
    assert(kShadowTable[0].tile_index == 0x0548);
    assert(kShadowTable[0].x_offset == 3);

    assert(kShadowTable[1].y_offset == 3);
    assert(kShadowTable[1].size_word == 0x0400);
    assert(kShadowTable[1].tile_index == 0x0549);
    assert(kShadowTable[1].x_offset == 8);

    assert(kShadowTable[2].y_offset == 3);
    assert(kShadowTable[2].size_word == 0x0800);
    assert(kShadowTable[2].tile_index == 0x054B);
    assert(kShadowTable[2].x_offset == 11);

    assert(kShadowTable[3].y_offset == 8);
    assert(kShadowTable[3].size_word == 0x0D00);
    assert(kShadowTable[3].tile_index == 0x054E);
    assert(kShadowTable[3].x_offset == 16);

    // Verify 4x4 tile count table
    for (std::size_t w = 1; w <= 4; ++w) {
        for (std::size_t h = 1; h <= 4; ++h) {
            const std::size_t idx = (w - 1) * 4 + (h - 1);
            assert(kSpriteTileCounts[idx] == w * h);
        }
    }

    assert(get_sprite_tile_count(0x00) == 1);  // 1x1
    assert(get_sprite_tile_count(0x05) == 4);  // 2x2
    assert(get_sprite_tile_count(0x0B) == 12); // 3x4
    assert(get_sprite_tile_count(0x0F) == 16); // 4x4
}

void test_viewport_clipping() {
    // Inside viewport
    assert(!is_screen_clipped_y(0x80));
    assert(!is_screen_clipped_x(0x80));
    assert(!is_screen_clipped_y(0x150));
    assert(!is_screen_clipped_x(0x1B0));

    // Outside viewport (top / left)
    assert(is_screen_clipped_y(0x50));
    assert(is_screen_clipped_x(0x50));

    // Outside viewport (bottom / right)
    assert(is_screen_clipped_y(0x170));
    assert(is_screen_clipped_x(0x1D0));
}

void test_world_to_screen_projection() {
    const std::int16_t world_x = 400;
    const std::int16_t world_y = 500;
    const std::int16_t elev_z = 25;
    const std::int16_t cam_x = 350;
    const std::int16_t cam_y = 420;

    std::int16_t sx = 0;
    std::int16_t sy = 0;
    project_world_to_screen(world_x, world_y, elev_z, cam_x, cam_y, sx, sy);

    assert(sx == 178); // 400 - 350 + 128
    assert(sy == 183); // (500 - 420 + 128) - 25
}

void test_frame_cache_and_quota() {
    const std::uint16_t quota = 32;

    // Cold cache (-1)
    const auto res_cold = evaluate_frame_cache(-1, 4, 50, quota);
    assert(res_cold.selected_frame == 4);
    assert(!res_cold.deferred);
    assert(res_cold.queue_dma);

    // Cache hit
    const auto res_hit = evaluate_frame_cache(4, 4, 10, quota);
    assert(res_hit.selected_frame == 4);
    assert(!res_hit.deferred);
    assert(!res_hit.queue_dma);

    // Quota exceeded with valid cache
    const auto res_exceed = evaluate_frame_cache(4, 5, 50, quota);
    assert(res_exceed.selected_frame == 4);
    assert(res_exceed.deferred);
    assert(!res_exceed.queue_dma);

    // Quota sufficient
    const auto res_ok = evaluate_frame_cache(4, 5, 10, quota);
    assert(res_ok.selected_frame == 5);
    assert(!res_ok.deferred);
    assert(res_ok.queue_dma);
}

void test_master_entity_composition() {
    MasterEntityRenderState ent{};
    ent.type = 1;
    ent.world_x = 400;
    ent.world_y = 500;
    ent.elev_z = 25;
    ent.ground_h = 10;
    ent.anim_frame = 3;
    ent.frame_cache = -1;
    ent.flags_36 = 0x00U; // visible
    ent.flags_3a = 0x04U; // shadow enabled, preset 0

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x05U, .tile_attr = 0x2000U, .x_offset = 0 },
        FrameSpritePiece{ .y_offset = 16, .size = 0x05U, .tile_attr = 0x2004U, .x_offset = 0 }
    };

    const auto res = compose_master_entity_sat(ent, pieces, 350, 420, 64, 0U, 0U);

    assert(!res.early_exited);
    assert(res.screen_x == 178);
    assert(res.screen_y == 183);
    assert(res.emitted_pieces.size() == 2U);
    assert(res.emitted_shadow.has_value());

    // 2 pieces + 1 shadow = 3 sprites (24 bytes)
    assert(res.final_sprite_count == 3U);
    assert(res.final_sat_offset == 24U);

    // Verify shadow attributes
    assert(res.emitted_shadow->tile_attr == (0x8000U | 0x0548U));
}

void test_visibility_early_exit() {
    MasterEntityRenderState ent{};
    ent.type = 1;
    ent.flags_36 = 0x02U; // invisible bit 1 set

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x05U, .tile_attr = 0x2000U, .x_offset = 0 }
    };

    const auto res = compose_master_entity_sat(ent, pieces, 0, 0, 64, 0U, 0U);
    assert(res.early_exited);
    assert(res.emitted_pieces.empty());
    assert(!res.emitted_shadow.has_value());
    assert(res.final_sprite_count == 0U);
}

void test_hardware_sprite_limit_capping() {
    MasterEntityRenderState ent{};
    ent.type = 1;
    ent.world_x = 400;
    ent.world_y = 500;
    ent.elev_z = 0;
    ent.flags_36 = 0;
    ent.flags_3a = 0x04U;

    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x05U, .tile_attr = 0x2000U, .x_offset = 0 },
        FrameSpritePiece{ .y_offset = 16, .size = 0x05U, .tile_attr = 0x2004U, .x_offset = 0 }
    };

    // Current sprites at 79: only 1 piece can fit, second piece and shadow suppressed
    const auto res = compose_master_entity_sat(ent, pieces, 350, 420, 64, 79U, 0U);
    assert(res.emitted_pieces.size() == 1U);
    assert(!res.emitted_shadow.has_value());
    assert(res.final_sprite_count == 80U);
}

void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00U);
    const std::uint32_t ram_base = 0x00FF0000U;

    auto write_i16 = [&](std::uint32_t addr, std::int16_t val) {
        const std::size_t off = addr - ram_base;
        ram[off] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFF);
        ram[off + 1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFF);
    };

    // VRAM quota = 5
    write_i16(MasterSatCompositorConfig::kVramQuotaRam, 5);

    // Initial sprites = 0, SAT offset = 0
    write_i16(MasterSatCompositorConfig::kSatLinkCountRam, 0);
    write_i16(MasterSatCompositorConfig::kSatByteOffsetRam, 0);

    // Camera Plane 1: cam_x = 0, cam_y = 0
    ram[0x00FF16F0U - ram_base] = 0x00U;
    write_i16(MasterSatCompositorConfig::kCameraPlane1Ram, 0);
    write_i16(MasterSatCompositorConfig::kCameraPlane1Ram + 2, 0);

    // Entity slot at $FF19E8
    const std::uint32_t ent_addr = 0x00FF19E8U;
    write_i16(ent_addr + MasterSatCompositorConfig::kTypeOffset, 1);
    write_i16(ent_addr + MasterSatCompositorConfig::kWorldXOffset, 50);
    write_i16(ent_addr + MasterSatCompositorConfig::kWorldYOffset, 50);
    write_i16(ent_addr + MasterSatCompositorConfig::kAnimFrameOffset, 2);
    write_i16(ent_addr + MasterSatCompositorConfig::kFrameCacheOffset, 1); // cached = 1
    ram[ent_addr + MasterSatCompositorConfig::kFlags36Offset - ram_base] = 0x00U;
    ram[ent_addr + MasterSatCompositorConfig::kFlags3AOffset - ram_base] = 0x00U;

    // Piece requiring 16 tiles (4x4), but quota is only 5 -> will defer!
    std::vector<FrameSpritePiece> pieces = {
        FrameSpritePiece{ .y_offset = 0, .size = 0x0FU, .tile_attr = 0x1000U, .x_offset = 0 }
    };

    const auto res = compose_master_entity_sat_raw(ram, ent_addr, pieces, ram_base);

    assert(res.cache_eval.deferred);
    // Verifies deferred flag $FF1651 was set to 0xFF
    assert(ram[MasterSatCompositorConfig::kDeferredFlagRam - ram_base] == 0xFFU);
    // Verifies cached frame remained 1
    const std::size_t cache_off = ent_addr + MasterSatCompositorConfig::kFrameCacheOffset - ram_base;
    const std::int16_t stored_cache = static_cast<std::int16_t>((ram[cache_off] << 8) | ram[cache_off + 1]);
    assert(stored_cache == 1);
}

int main() {
    test_embedded_tables_and_constants();
    test_viewport_clipping();
    test_world_to_screen_projection();
    test_frame_cache_and_quota();
    test_master_entity_composition();
    test_visibility_early_exit();
    test_hardware_sprite_limit_capping();
    test_raw_ram_integration();

    std::cout << "m16_164_master_sat_compositor_test: All 8 test suites passed successfully.\n";
    return 0;
}
