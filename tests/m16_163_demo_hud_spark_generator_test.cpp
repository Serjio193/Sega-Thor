#include "game/render/demo_hud_spark_generator.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::render;

void test_constants_and_mappings() {
    static_assert(DemoHudSparkConfig::kDemoOverlayAddr == 0x00A7E6U);
    static_assert(DemoHudSparkConfig::kSparkGeneratorAddr == 0x00A8F0U);
    static_assert(DemoHudSparkConfig::kRoutineEndAddr == 0x00A9A0U);

    static_assert(DemoHudSparkConfig::kDemoStatusRam == 0x00FF1656U);
    static_assert(DemoHudSparkConfig::kDemoSyncRam == 0x00FF17C0U);
    static_assert(DemoHudSparkConfig::kCameraSelectRam == 0x00FF16F0U);
    static_assert(DemoHudSparkConfig::kCameraPlane1Ram == 0x00FF1716U);
    static_assert(DemoHudSparkConfig::kCameraPlane2Ram == 0x00FF173EU);
    static_assert(DemoHudSparkConfig::kVdpSatBufferRam == 0x00FF13CCU);
    static_assert(DemoHudSparkConfig::kSatByteOffsetRam == 0x00FF188CU);
    static_assert(DemoHudSparkConfig::kSatLinkCountRam == 0x00FF188AU);
    static_assert(DemoHudSparkConfig::kDmaQueueRam == 0x00FF1892U);
    static_assert(DemoHudSparkConfig::kSpecialPoolRam == 0x00FF19C8U);

    static_assert(DemoHudSparkConfig::kMaxSatSprites == 80U);

    static_assert(DemoHudSparkConfig::kMode3DmaSrc == 0x000AA1FDU);
    static_assert(DemoHudSparkConfig::kMode3DmaVram == 0xF360U);
    static_assert(DemoHudSparkConfig::kMode3DmaWords == 0x00C0U);
    static_assert(DemoHudSparkConfig::kMode3OverlayY == 228);

    static_assert(DemoHudSparkConfig::kMode2DmaSrc == 0x000AA1ADU);
    static_assert(DemoHudSparkConfig::kMode2DmaVram == 0xF360U);
    static_assert(DemoHudSparkConfig::kMode2DmaWords == 0x0050U);
    static_assert(DemoHudSparkConfig::kMode2OverlayY == 228);

    static_assert(DemoHudSparkConfig::kScreenCoordBias == 120);
    static_assert(DemoHudSparkConfig::kScreenClipMargin == 112);
    static_assert(DemoHudSparkConfig::kMaxScreenClippedY == 240);
    static_assert(DemoHudSparkConfig::kMaxScreenClippedX == 336);
}

void test_demo_overlay_suppression() {
    // Bit 7 set suppresses overlay
    const auto res = process_demo_overlay(0x0080U, 0U, 0U);
    assert(res.sprite_count == 0U);
    assert(!res.dma.has_value());
    assert(res.sprites.empty());
}

void test_demo_overlay_mode_3() {
    // Bit 6 set -> Mode 3 (3 sprites)
    const auto res = process_demo_overlay(0x0040U, 0U, 10U);
    assert(res.sprite_count == 3U);
    assert(res.dma.has_value());
    assert(res.dma->source_addr == DemoHudSparkConfig::kMode3DmaSrc);
    assert(res.dma->vram_dest == DemoHudSparkConfig::kMode3DmaVram);
    assert(res.dma->word_count == DemoHudSparkConfig::kMode3DmaWords);

    assert(res.sprites.size() == 3U);
    assert(res.sprites[0].x == DemoHudSparkConfig::kMode3X0);
    assert(res.sprites[1].x == DemoHudSparkConfig::kMode3X1);
    assert(res.sprites[2].x == DemoHudSparkConfig::kMode3X2);
    assert(res.sprites[0].y == DemoHudSparkConfig::kMode3OverlayY);

    // If current_sync == 1, DMA is suppressed
    const auto res_no_dma = process_demo_overlay(0x0040U, 1U, 10U);
    assert(res_no_dma.sprite_count == 3U);
    assert(!res_no_dma.dma.has_value());
    assert(res_no_dma.sprites.size() == 3U);
}

void test_demo_overlay_mode_2() {
    // Bit 6 clear -> Mode 2 (2 sprites)
    const auto res = process_demo_overlay(0x0000U, 0U, 5U);
    assert(res.sprite_count == 2U);
    assert(res.dma.has_value());
    assert(res.dma->source_addr == DemoHudSparkConfig::kMode2DmaSrc);
    assert(res.dma->vram_dest == DemoHudSparkConfig::kMode2DmaVram);
    assert(res.dma->word_count == DemoHudSparkConfig::kMode2DmaWords);

    assert(res.sprites.size() == 2U);
    assert(res.sprites[0].x == DemoHudSparkConfig::kMode2X0);
    assert(res.sprites[1].x == DemoHudSparkConfig::kMode2X1);
    assert(res.sprites[0].y == DemoHudSparkConfig::kMode2OverlayY);

    // If current_sync == 0xFF, DMA is suppressed
    const auto res_no_dma = process_demo_overlay(0x0000U, 0xFFU, 5U);
    assert(res_no_dma.sprite_count == 2U);
    assert(!res_no_dma.dma.has_value());
    assert(res_no_dma.sprites.size() == 2U);
}

void test_demo_overlay_sprite_cap() {
    // Current sprites at 79, Mode 3 asks for 3 sprites: only 1 can fit before reaching 80
    const auto res = process_demo_overlay(0x0040U, 0U, 79U);
    assert(res.sprite_count == 3U);
    assert(res.sprites.size() == 1U);

    // Current sprites at 80: 0 can fit
    const auto res_full = process_demo_overlay(0x0040U, 0U, 80U);
    assert(res_full.sprites.empty());
}

void test_spark_particle_processing() {
    // Test lifetime expiry clears type
    SparkParticleState spark1{};
    spark1.type = 1;
    spark1.world_x = 100;
    spark1.world_y = 100;
    spark1.lifetime = 1;

    const auto res1 = process_spark_particle(spark1, 100, 100, 10U);
    assert(!res1.emitted);
    assert(res1.expired);
    assert(spark1.type == 0);

    // Test in-bounds emission
    SparkParticleState spark2{};
    spark2.type = 1;
    spark2.world_x = 150;
    spark2.world_y = 150;
    spark2.lifetime = 10;

    const auto res2 = process_spark_particle(spark2, 100, 100, 10U);
    assert(res2.emitted);
    assert(!res2.expired);
    // screen_x = 150 - 100 + 120 = 170
    // screen_y = 150 - 100 + 120 = 170
    assert(res2.screen_x == 170);
    assert(res2.screen_y == 170);
    assert(spark2.lifetime == 9);

    // Test out-of-bounds drop
    SparkParticleState spark3{};
    spark3.type = 1;
    spark3.world_x = 600;
    spark3.world_y = 600;
    spark3.lifetime = 10;

    const auto res3 = process_spark_particle(spark3, 100, 100, 10U);
    assert(!res3.emitted);
    assert(!res3.expired);

    // Test sprite limit suppression
    SparkParticleState spark4{};
    spark4.type = 1;
    spark4.world_x = 150;
    spark4.world_y = 150;
    spark4.lifetime = 10;

    const auto res4 = process_spark_particle(spark4, 100, 100, 80U);
    assert(!res4.emitted);
}

void test_step_spark_pool() {
    std::vector<SparkParticleState> sparks(4);
    sparks[0] = SparkParticleState{1, 150, 150, 5}; // in bounds
    sparks[1] = SparkParticleState{1, 160, 160, 5}; // in bounds
    sparks[2] = SparkParticleState{1, 900, 900, 5}; // out of bounds
    sparks[3] = SparkParticleState{0, 0, 0, 0};     // inactive

    std::uint16_t sprites = 10U;
    std::uint16_t sat_offset = 80U;

    const auto results = step_spark_pool(sparks, 100, 100, sprites, sat_offset);
    assert(results.size() == 4U);
    assert(results[0].emitted);
    assert(results[1].emitted);
    assert(!results[2].emitted);
    assert(!results[3].emitted);

    assert(sprites == 12U);
    assert(sat_offset == 96U);
}

void test_raw_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00U);
    const std::uint32_t ram_base = 0x00FF0000U;

    auto write_i16 = [&](std::uint32_t addr, std::int16_t val) {
        const std::size_t off = addr - ram_base;
        ram[off] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFF);
        ram[off + 1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFF);
    };

    // Mode 3 active (bit 6 = 1), sync = 0
    write_i16(DemoHudSparkConfig::kDemoStatusRam, 0x0040);
    ram[DemoHudSparkConfig::kDemoSyncRam - ram_base] = 0x00U;

    // Current sprites = 2, SAT offset = 16
    write_i16(DemoHudSparkConfig::kSatLinkCountRam, 2);
    write_i16(DemoHudSparkConfig::kSatByteOffsetRam, 16);

    // Camera Plane 1: cam_x = 100, cam_y = 100
    ram[DemoHudSparkConfig::kCameraSelectRam - ram_base] = 0x00U; // Plane 1
    write_i16(DemoHudSparkConfig::kCameraPlane1Ram, 100);
    write_i16(DemoHudSparkConfig::kCameraPlane1Ram + 2, 100);

    // Special pool slot 0: active spark at (150, 150), lifetime 5
    const std::uint32_t sp0 = DemoHudSparkConfig::kSpecialPoolRam;
    write_i16(sp0 + DemoHudSparkConfig::kSparkTypeOffset, 1);
    write_i16(sp0 + DemoHudSparkConfig::kSparkYOffset, 150);
    write_i16(sp0 + DemoHudSparkConfig::kSparkXOffset, 150);
    write_i16(sp0 + DemoHudSparkConfig::kSparkLifetimeOffset, 5);

    const auto res = process_demo_hud_spark_raw(ram, ram_base);

    assert(res.demo_overlay.sprite_count == 3U);
    assert(res.demo_overlay.dma.has_value());
    assert(res.spark_results[0].emitted);

    // 2 initial + 3 overlay + 1 spark = 6 sprites
    assert(res.final_sprite_count == 6U);
    // 16 initial + 3*8 (24) + 1*8 = 48 bytes
    assert(res.final_sat_offset == 48U);

    // Verify RAM writes
    const std::size_t link_off = DemoHudSparkConfig::kSatLinkCountRam - ram_base;
    const std::uint16_t stored_sprites = static_cast<std::uint16_t>((ram[link_off] << 8) | ram[link_off + 1]);
    assert(stored_sprites == 6U);
}

int main() {
    test_constants_and_mappings();
    test_demo_overlay_suppression();
    test_demo_overlay_mode_3();
    test_demo_overlay_mode_2();
    test_demo_overlay_sprite_cap();
    test_spark_particle_processing();
    test_step_spark_pool();
    test_raw_ram_integration();

    std::cout << "m16_163_demo_hud_spark_generator_test: All 8 test suites passed successfully.\n";
    return 0;
}
