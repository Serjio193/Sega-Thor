#include "game/player/bounding_box_pushable_latch.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    static_assert(BoundingBoxPushableLatchConfig::kSampleEntityBoundingBoxDispatchAddr == 0x009BC2U);
    static_assert(BoundingBoxPushableLatchConfig::kSampleBoundingTileBoxAddr == 0x009BF2U);
    static_assert(BoundingBoxPushableLatchConfig::kReadPointTileLayerAddr == 0x009C40U);
    static_assert(BoundingBoxPushableLatchConfig::kScanPushableBlockSolidCollisionAddr == 0x009C7CU);
    static_assert(BoundingBoxPushableLatchConfig::kLatchEntityGroundSurfaceAddr == 0x009D00U);
    static_assert(BoundingBoxPushableLatchConfig::kTerrainSurfaceLutAddr == 0x0096E8U);
    static_assert(BoundingBoxPushableLatchConfig::kPushableBlockPoolRam == 0x00FF2D8CU);
    static_assert(BoundingBoxPushableLatchConfig::kLayer1Ram == 0x00FF1766U);
    static_assert(BoundingBoxPushableLatchConfig::kLayer2Ram == 0x00FF1770U);
    static_assert(BoundingBoxPushableLatchConfig::kLayerSelectRam == 0x00FF16F0U);
    static_assert(BoundingBoxPushableLatchConfig::kPushableBlockType == 0x0056U);
    static_assert(BoundingBoxPushableLatchConfig::kPushableBlockSlotCount == 6U);
    static_assert(BoundingBoxPushableLatchConfig::kPushableBlockSlotStride == 90U);

    const auto& lut = BoundingBoxPushableLatchConfig::kTerrainSurfaceLut;
    assert(lut[0] == 0xFFU);
    assert(lut[1] == 0x00U);
    assert(lut[2] == 0x02U);
    assert(lut[3] == 0x01U);
    assert(lut[4] == 0x04U);
    assert(lut[6] == 0x03U);
    assert(lut[8] == 0x06U);
    assert(lut[9] == 0x07U);
    assert(lut[12] == 0x05U);
}

static void test_sample_bounding_tile_box() {
    // 8x8 tilemap with row stride 8 (shift 3)
    std::vector<std::uint8_t> map(64, 0x00);
    // Center at (24, 24) -> col 3, row 3.
    // Radius = 8 pixels:
    // col span: (24 - 8) >> 3 .. (24 + 8) >> 3 -> 16 >> 3 .. 32 >> 3 -> 2 .. 4 (3 columns)
    // row span: (24 - 8) >> 3 .. (24 + 8) >> 3 -> 2 .. 4 (3 rows)
    map[2 * 8 + 2] = 0x01;
    map[2 * 8 + 3] = 0x02;
    map[2 * 8 + 4] = 0x04;
    map[3 * 8 + 3] = 0x0F;

    PointLayerTilemapDescriptor layer{map.data(), map.size(), 8, 3};

    auto res = sample_bounding_tile_box(24, 24, 8, layer);
    // OR accumulator should contain bits 0, 1, 2, 3 -> 0x0F
    assert(res.or_accum == 0x0FU);
    // Some tiles in the 3x3 box are 0x00, so AND accumulator should be 0x00
    assert(res.and_accum == 0x00U);

    // If all tiles are uniform:
    std::vector<std::uint8_t> uniform_map(64, 0x12);
    PointLayerTilemapDescriptor uniform_layer{uniform_map.data(), uniform_map.size(), 8, 3};
    auto uniform_res = sample_bounding_tile_box(24, 24, 8, uniform_layer);
    assert(uniform_res.or_accum == 0x12U);
    assert(uniform_res.and_accum == 0x12U);
}

static void test_sample_entity_bounding_tile_box_dispatch() {
    std::vector<std::uint8_t> map1(64, 0x10);
    std::vector<std::uint8_t> map2(64, 0x20);

    PointLayerTilemapDescriptor layer1{map1.data(), map1.size(), 8, 3};
    PointLayerTilemapDescriptor layer2{map2.data(), map2.size(), 8, 3};

    std::vector<std::uint8_t> slot(0x80, 0);
    // X = 16 (col 2), Y = 16 (row 2), rad = 4
    slot[0x08] = 0x00; slot[0x09] = 0x10;
    slot[0x0C] = 0x00; slot[0x0D] = 0x10;
    slot[0x46] = 0x00; slot[0x47] = 0x04;

    // Bit 7 of layer_select is 0 -> select Layer 1
    auto res1 = sample_entity_bounding_tile_box_dispatch(slot, 0x00, layer1, layer2);
    assert(res1.or_accum == 0x10U);
    assert(res1.and_accum == 0x10U);

    // Bit 7 of layer_select is 1 -> select Layer 2
    auto res2 = sample_entity_bounding_tile_box_dispatch(slot, 0x80, layer1, layer2);
    assert(res2.or_accum == 0x20U);
    assert(res2.and_accum == 0x20U);
}

static void test_read_point_tile_layer() {
    std::vector<std::uint8_t> map1(64, 0x00);
    std::vector<std::uint8_t> map2(64, 0x00);
    map1[2 * 8 + 2] = 0xAA;
    map2[2 * 8 + 2] = 0xBB;

    PointLayerTilemapDescriptor layer1{map1.data(), map1.size(), 8, 3};
    PointLayerTilemapDescriptor layer2{map2.data(), map2.size(), 8, 3};

    std::vector<std::uint8_t> slot(0x80, 0);
    slot[0x08] = 0x00; slot[0x09] = 0x10; // X = 16 (col 2)
    slot[0x0C] = 0x00; slot[0x0D] = 0x10; // Y = 16 (row 2)

    auto byte1 = read_point_tile_layer(slot, 0x00, layer1, layer2);
    assert(byte1 == 0xAA);

    auto byte2 = read_point_tile_layer(slot, 0x80, layer1, layer2);
    assert(byte2 == 0xBB);
}

static void test_scan_pushable_block_solid_collision() {
    std::vector<PushableBlockDesc> pool = {
        {0x0056, 100, 100, 16, 16}, // slot 0: pushable block at (100, 100), radius (16, 16)
        {0x0040, 200, 200, 16, 16}, // slot 1: other type (ignored)
        {0x0000, 0, 0, 0, 0},
        {0x0000, 0, 0, 0, 0},
        {0x0000, 0, 0, 0, 0},
        {0x0000, 0, 0, 0, 0},
    };

    // Entity at (110, 105) with radius (8, 8):
    // Ent bounds: X=[102, 118], Y=[97, 113]
    // Block bounds: X=[84, 116], Y=[84, 116]
    // Overlap on both axes -> HIT!
    bool hit = scan_pushable_block_solid_collision(110, 105, 8, 8, pool);
    assert(hit);

    // Entity at (200, 200) overlaps slot 1, but slot 1 is type 0x0040 -> MISS!
    bool miss_wrong_type = scan_pushable_block_solid_collision(200, 200, 8, 8, pool);
    assert(!miss_wrong_type);

    // Entity far away -> MISS!
    bool miss_far = scan_pushable_block_solid_collision(300, 300, 8, 8, pool);
    assert(!miss_far);
}

static void test_scan_pushable_block_solid_collision_raw() {
    std::vector<std::uint8_t> entity_slot(0x80, 0);
    // Entity at (110, 105), radius (8, 8)
    entity_slot[0x08] = 0x00; entity_slot[0x09] = 110;
    entity_slot[0x0C] = 0x00; entity_slot[0x0D] = 105;
    entity_slot[0x42] = 0x00; entity_slot[0x43] = 8;
    entity_slot[0x44] = 0x00; entity_slot[0x45] = 8;

    // 6 slots x 90 bytes = 540 bytes
    std::vector<std::uint8_t> pool_bytes(6 * 90, 0);

    // Slot 0: type 0x0056, at (100, 100), radius (16, 16)
    pool_bytes[0x00] = 0x00; pool_bytes[0x01] = 0x56;
    pool_bytes[0x08] = 0x00; pool_bytes[0x09] = 100;
    pool_bytes[0x0C] = 0x00; pool_bytes[0x0D] = 100;
    pool_bytes[0x42] = 0x00; pool_bytes[0x43] = 16;
    pool_bytes[0x44] = 0x00; pool_bytes[0x45] = 16;

    bool hit = scan_pushable_block_solid_collision_raw(entity_slot, pool_bytes);
    assert(hit);

    // Move entity out of range:
    entity_slot[0x08] = 0x01; entity_slot[0x09] = 0xF4; // X = 500
    bool miss = scan_pushable_block_solid_collision_raw(entity_slot, pool_bytes);
    assert(!miss);
}

static void test_latch_entity_ground_surface() {
    // 8x8 tilemap filled with attr 2 (surface code 2 in LUT1)
    std::vector<std::uint8_t> map(64, 0x02);
    PointLayerTilemapDescriptor layer{map.data(), map.size(), 8, 3};

    std::vector<std::uint8_t> slot(0x80, 0);
    // Entity at (16, 16), bounding radius = 4
    slot[0x08] = 0x00; slot[0x09] = 0x10;
    slot[0x0C] = 0x00; slot[0x0D] = 0x10;
    slot[0x46] = 0x00; slot[0x47] = 0x04;

    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    latch_entity_ground_surface(slot, 0x00, layer, layer, elev_table);

    // +0x6F: or_accum = 0x02
    assert(slot[0x6F] == 0x02);
    // +0x6E: and_accum = 0x02
    assert(slot[0x6E] == 0x02);
    // +0x70: surface type = LUT1[2] = 0x02
    assert(slot[0x70] == 0x02);
    // +0x14: ground elevation = 16
    std::int16_t ground_h = (static_cast<std::int16_t>(slot[0x14]) << 8) | slot[0x15];
    assert(ground_h == 16);
}

int main() {
    std::cout << "Running M16.155 Bounding Box, Pushable Block & Latch Tests..." << std::endl;
    test_rom_constants();
    test_sample_bounding_tile_box();
    test_sample_entity_bounding_tile_box_dispatch();
    test_read_point_tile_layer();
    test_scan_pushable_block_solid_collision();
    test_scan_pushable_block_solid_collision_raw();
    test_latch_entity_ground_surface();
    std::cout << "All M16.155 tests PASSED!" << std::endl;
    return 0;
}
