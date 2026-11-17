#include "game/render/weapon_overlay_compositor.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

void test_constants_and_offsets() {
    static_assert(WeaponOverlayCompositorConfig::kCompositorEntryAddr == 0x00B15EU);
    static_assert(WeaponOverlayCompositorConfig::kDmaPathEntryAddr == 0x00B166U);
    static_assert(WeaponOverlayCompositorConfig::kDescriptorTableAddr == 0x0003F2FAU);
    static_assert(WeaponOverlayCompositorConfig::kDmaPathExitRtsAddr == 0x00B28CU);
    static_assert(WeaponOverlayCompositorConfig::kNonDmaPathEntryAddr == 0x00B28EU);
    static_assert(WeaponOverlayCompositorConfig::kNonDmaPathExitRtsAddr == 0x00B34AU);
    static_assert(WeaponOverlayCompositorConfig::kCompositorRangeEnd == 0x00B34CU);

    static_assert(WeaponOverlayCompositorConfig::kModeWordOffset == 0x3EU);
    static_assert(WeaponOverlayCompositorConfig::kAuxVramBaseOffset == 0x5AU);
    static_assert(WeaponOverlayCompositorConfig::kWeaponFrameCacheOffset == 0x5CU);
    static_assert(WeaponOverlayCompositorConfig::kNonDmaModeMask == 0x0200U);
}

void test_mode_routing_and_descriptor_offset() {
    // Mode routing bit 9 test
    assert(requires_weapon_dma(0x0000U) == true);
    assert(requires_weapon_dma(0x0100U) == true);
    assert(requires_weapon_dma(0x0200U) == false);
    assert(requires_weapon_dma(0x0300U) == false);

    // Descriptor formula: 0x0003F2FA + (entity_type * 16) - 20
    assert(get_weapon_descriptor_offset(2U) == 0x0003F306U);
    assert(get_weapon_descriptor_offset(3U) == 0x0003F316U);
}

void test_viewport_clipping() {
    // Inside screen
    assert(!is_weapon_overlay_clipped(0x80, 0x90));
    assert(!is_weapon_overlay_clipped(0x60, 0x60));
    assert(!is_weapon_overlay_clipped(0x160, 0x1C0));

    // Outside screen Y
    assert(is_weapon_overlay_clipped(0x50, 0x90));
    assert(is_weapon_overlay_clipped(0x170, 0x90));

    // Outside screen X
    assert(is_weapon_overlay_clipped(0x80, 0x50));
    assert(is_weapon_overlay_clipped(0x80, 0x1D0));
}

void test_dma_path_composition() {
    WeaponOverlayEntityState ent{};
    ent.type = 2; // Player
    ent.world_x = 250;
    ent.world_y = 200;
    ent.elev_z = 10;
    ent.mode_word = 0x0000U; // DMA mode
    ent.aux_vram_base = 0x0400U;
    ent.weapon_frame = 0x12;
    ent.weapon_frame_cache = 0x10; // different -> triggers DMA

    std::vector<WeaponOverlayPiece> pieces = {
        WeaponOverlayPiece{ .y_offset = -4, .size = 0x05U, .tile_attr = 0x0010U, .x_offset = 8 },
        WeaponOverlayPiece{ .y_offset = 12, .size = 0x05U, .tile_attr = 0x0014U, .x_offset = 8 }
    };

    // cam = (150, 100)
    // screen_x = 250 - 150 + 128 = 228
    // screen_y = (200 - 100 + 128) - 10 = 218
    const auto res = compose_weapon_overlay(ent, pieces, 150, 100, 2U, 16U);

    assert(res.path == WeaponOverlayPath::DmaStream);
    assert(res.dma_queued == true);
    assert(res.updated_frame_cache == 0x12);
    assert(res.screen_x == 228);
    assert(res.screen_y == 218);

    assert(res.emitted_pieces.size() == 2U);
    assert(res.final_sprite_count == 4U);
    assert(res.final_sat_offset == 32U);

    // Piece 0: y = 218 - 4 = 214, x = 228 + 8 = 236, attr = 0x0010 + 0x0400 = 0x0410
    assert(res.emitted_pieces[0].y == 214);
    assert(res.emitted_pieces[0].x == 236);
    assert(res.emitted_pieces[0].tile_attr == 0x0410U);
    assert(res.emitted_pieces[0].link == 3U);

    // Piece 1: y = 218 + 12 = 230, x = 228 + 8 = 236, attr = 0x0014 + 0x0400 = 0x0414
    assert(res.emitted_pieces[1].y == 230);
    assert(res.emitted_pieces[1].x == 236);
    assert(res.emitted_pieces[1].tile_attr == 0x0414U);
    assert(res.emitted_pieces[1].link == 4U);
}

void test_nondma_path_composition() {
    WeaponOverlayEntityState ent{};
    ent.type = 3;
    ent.world_x = 200;
    ent.world_y = 200;
    ent.elev_z = 0;
    ent.mode_word = 0x0200U; // Non-DMA static mode
    ent.aux_vram_base = 0x0600U;
    ent.weapon_frame = 0x05;
    ent.weapon_frame_cache = 0x01;

    std::vector<WeaponOverlayPiece> pieces = {
        WeaponOverlayPiece{ .y_offset = 0, .size = 0x00U, .tile_attr = 0x0001U, .x_offset = 0 }
    };

    const auto res = compose_weapon_overlay(ent, pieces, 100, 100, 0U, 0U);

    assert(res.path == WeaponOverlayPath::NonDmaStatic);
    assert(res.dma_queued == false); // Non-DMA never queues
    assert(res.emitted_pieces.size() == 1U);
    assert(res.emitted_pieces[0].tile_attr == 0x0601U);
}

void test_sprite_limit_capping() {
    WeaponOverlayEntityState ent{};
    ent.world_x = 200;
    ent.world_y = 200;
    ent.mode_word = 0;

    std::vector<WeaponOverlayPiece> pieces = {
        WeaponOverlayPiece{ .y_offset = 0, .size = 0x00U, .tile_attr = 0, .x_offset = 0 },
        WeaponOverlayPiece{ .y_offset = 8, .size = 0x00U, .tile_attr = 0, .x_offset = 0 }
    };

    // Current sprites at 79: only 1 piece fits
    const auto res = compose_weapon_overlay(ent, pieces, 100, 100, 79U, 0U);
    assert(res.emitted_pieces.size() == 1U);
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

    // Current sprites = 1, SAT offset = 8
    write_i16(WeaponOverlayCompositorConfig::kSatLinkCountRam, 1);
    write_i16(WeaponOverlayCompositorConfig::kSatByteOffsetRam, 8);

    // Camera Plane 1: cam_x = 50, cam_y = 50
    ram[0x00FF16F0U - ram_base] = 0x00U;
    write_i16(WeaponOverlayCompositorConfig::kCameraPlane1Ram, 50);
    write_i16(WeaponOverlayCompositorConfig::kCameraPlane1Ram + 2, 50);

    // Entity slot at $FF19E8
    const std::uint32_t ent_addr = 0x00FF19E8U;
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kTypeOffset, 2);
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kWorldXOffset, 150);
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kWorldYOffset, 150);
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kElevZOffset, 0);
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kModeWordOffset, 0x0000); // DMA mode
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kAuxVramBaseOffset, 0x0500);
    write_i16(ent_addr + WeaponOverlayCompositorConfig::kWeaponFrameCacheOffset, 0x0001); // cached = 1

    std::vector<WeaponOverlayPiece> pieces = {
        WeaponOverlayPiece{ .y_offset = 0, .size = 0x00U, .tile_attr = 0x0002U, .x_offset = 0 }
    };

    // New weapon frame = 2 (triggers DMA and updates cache)
    const auto res = compose_weapon_overlay_raw(ram, ent_addr, 2, pieces, ram_base);

    assert(res.path == WeaponOverlayPath::DmaStream);
    assert(res.dma_queued == true);
    assert(res.final_sprite_count == 2U);
    assert(res.final_sat_offset == 16U);

    // Verify cache updated in RAM at +0x5C
    const std::size_t cache_off = ent_addr + WeaponOverlayCompositorConfig::kWeaponFrameCacheOffset - ram_base;
    const std::int16_t stored_cache = static_cast<std::int16_t>((ram[cache_off] << 8) | ram[cache_off + 1]);
    assert(stored_cache == 2);

    // Verify link count in RAM
    const std::size_t link_off = WeaponOverlayCompositorConfig::kSatLinkCountRam - ram_base;
    const std::uint16_t stored_sprites = static_cast<std::uint16_t>((ram[link_off] << 8) | ram[link_off + 1]);
    assert(stored_sprites == 2U);
}

int main() {
    test_constants_and_offsets();
    test_mode_routing_and_descriptor_offset();
    test_viewport_clipping();
    test_dma_path_composition();
    test_nondma_path_composition();
    test_sprite_limit_capping();
    test_raw_ram_integration();

    std::cout << "m16_166_weapon_overlay_compositor_test: All 7 test suites passed successfully.\n";
    return 0;
}
