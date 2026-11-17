#include "game/player/slope_elevation_engine.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    static_assert(SlopeElevationEngineConfig::kRampSearchHandler1Addr == 0x0099F0U);
    static_assert(SlopeElevationEngineConfig::kRampSearchHandler2Addr == 0x009A60U);
    static_assert(SlopeElevationEngineConfig::kCustomElevationProbeAddr == 0x009AD0U);
    static_assert(SlopeElevationEngineConfig::kEntityElevationProbeAddr == 0x009AD6U);
    static_assert(SlopeElevationEngineConfig::kLut2Addr == 0x0096F8U);
    static_assert(SlopeElevationEngineConfig::kElevationTableRam == 0x00FF1706U);
    static_assert(SlopeElevationEngineConfig::kLayer1Ram == 0x00FF1766U);
    static_assert(SlopeElevationEngineConfig::kLayer2Ram == 0x00FF1770U);
    static_assert(SlopeElevationEngineConfig::kLayerSelectRam == 0x00FF16F0U);
    static_assert(SlopeElevationEngineConfig::kMaxRampScanTiles == 48U);
    static_assert(SlopeElevationEngineConfig::kRampBit4Mask == 0x10U);

    const auto& lut = SlopeElevationEngineConfig::kLut2SurfaceCodes;
    assert(lut[0] == 0xFFU);
    assert(lut[1] == 0x00U);
    assert(lut[2] == 0x02U);
    assert(lut[3] == 0x01U); // odd -> slope
    assert(lut[4] == 0x04U);
    assert(lut[5] == 0x04U);
    assert(lut[6] == 0x03U); // odd -> slope
    assert(lut[7] == 0x03U);
    assert(lut[8] == 0x06U);
    assert(lut[9] == 0x06U);
    assert(lut[10] == 0x06U);
    assert(lut[11] == 0x06U);
    assert(lut[12] == 0x05U); // odd -> slope
    assert(lut[13] == 0x05U);
    assert(lut[14] == 0x05U);
    assert(lut[15] == 0x05U);
}

static void test_flat_and_void_surface_elevation() {
    // 8x8 tilemap with row stride 8 (shift 3)
    std::vector<std::uint8_t> map(64, 0);
    // At tile (2, 2) -> attr 1 (flat tier 0, surface code 0)
    map[2 * 8 + 2] = 0x01;
    // At tile (3, 2) -> attr 2 (flat tier 1, surface code 2)
    map[2 * 8 + 3] = 0x02;
    // At tile (4, 2) -> attr 4 (flat tier 2, surface code 4)
    map[2 * 8 + 4] = 0x04;
    // At tile (5, 2) -> attr 0 (void/pit, surface code 0xFF)
    map[2 * 8 + 5] = 0x00;

    PointLayerTilemapDescriptor layer{map.data(), map.size(), 8, 3};

    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    // Flat tier 0: surface code 0 -> elev_table[0] = 0
    auto h0 = probe_tile_elevation(2 * 8 + 4, 2 * 8 + 4, 2 * 8 + 4, layer, elev_table);
    assert(h0 == 0);

    // Flat tier 1: surface code 2 -> elev_table[2] = 16
    auto h1 = probe_tile_elevation(3 * 8 + 4, 2 * 8 + 4, 2 * 8 + 4, layer, elev_table);
    assert(h1 == 16);

    // Flat tier 2: surface code 4 -> elev_table[4] = 32
    auto h2 = probe_tile_elevation(4 * 8 + 4, 2 * 8 + 4, 2 * 8 + 4, layer, elev_table);
    assert(h2 == 32);

    // Void/pit: surface code 0xFF -> elev_table[7] = -4096
    auto h_void = probe_tile_elevation(5 * 8 + 4, 2 * 8 + 4, 2 * 8 + 4, layer, elev_table);
    assert(h_void == -4096);
}

static void test_slope_fractional_interpolation() {
    // Tilemap with slope tile (attr 3 -> surface code 1)
    // Surface code 1: base = table[0], next = table[2]
    // Table: base = 0, next = 16 (delta = 16)
    std::vector<std::uint8_t> map(64, 0x02); // surrounding flat tier 1 (code 2)
    // Place single slope tile at row 2, col 2
    map[2 * 8 + 2] = 0x03; // attr 3 -> surface code 1

    PointLayerTilemapDescriptor layer{map.data(), map.size(), 8, 3};
    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    // Single tile ramp: total_span = 7.
    // At top of tile (entity_y & 7 == 0): d3 = 7 - 0 = 7.
    // fraction = (16 * 7) / 7 = 16. Height = 0 + 16 = 16!
    auto h_top = probe_tile_elevation(2 * 8 + 4, 2 * 8 + 0, 2 * 8 + 0, layer, elev_table);
    assert(h_top == 16);

    // At bottom of tile (entity_y & 7 == 7): d3 = 7 - 7 = 0.
    // fraction = (16 * 0) / 7 = 0. Height = 0 + 0 = 0!
    auto h_bot = probe_tile_elevation(2 * 8 + 4, 2 * 8 + 7, 2 * 8 + 7, layer, elev_table);
    assert(h_bot == 0);

    // Two-tile vertical slope: rows 2 and 3 both attr 3
    map[3 * 8 + 2] = 0x03;
    // Total span: 15 pixels.
    // Row 2, Y=0 (top of upper tile): entity_y = 16. entity_y & 7 = 0.
    // Upward: tile row 1 has attr 2 (surface code 2). upper code 2 > cur code 1, does NOT fallback.
    // Downward: tile row 3 has attr 3 (adds 8 to d3).
    // d2 = 0, d3 = 7 + 8 = 15. total_span = 15.
    // fraction = (16 * 15) / 15 = 16. Height = 16.
    auto h_multi_top = probe_tile_elevation(2 * 8 + 4, 2 * 8 + 0, 2 * 8 + 0, layer, elev_table);
    assert(h_multi_top == 16);

    // Row 3, Y=7 (bottom of lower tile): entity_y = 31. entity_y & 7 = 7.
    // d3 = 0, fraction = 0. Height = 0.
    auto h_multi_bot = probe_tile_elevation(2 * 8 + 4, 3 * 8 + 7, 3 * 8 + 7, layer, elev_table);
    assert(h_multi_bot == 0);
}

static void test_entity_elevation_probe() {
    std::vector<std::uint8_t> map1(64, 0x01); // Layer 1 has attr 1 (elev 0)
    std::vector<std::uint8_t> map2(64, 0x02); // Layer 2 has attr 2 (elev 16)

    PointLayerTilemapDescriptor layer1{map1.data(), map1.size(), 8, 3};
    PointLayerTilemapDescriptor layer2{map2.data(), map2.size(), 8, 3};

    std::vector<std::uint8_t> slot(0x80, 0);
    // Entity at (16, 16)
    slot[0x08] = 0x00; slot[0x09] = 0x10; // X = 16
    slot[0x0C] = 0x00; slot[0x0D] = 0x10; // Y = 16

    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    // Bit 7 of layer_select is 0 -> select Layer 1 -> elev 0
    auto h_l1 = probe_entity_elevation(slot, 0x00, layer1, layer2, elev_table);
    assert(h_l1 == 0);

    // Bit 7 of layer_select is 1 -> select Layer 2 -> elev 16
    auto h_l2 = probe_entity_elevation(slot, 0x80, layer1, layer2, elev_table);
    assert(h_l2 == 16);
}

static void test_ramp_search_handler_1() {
    // 8x60 tilemap (row stride 8, shift 3)
    std::vector<std::uint8_t> map(8 * 60, 0x10); // bit 4 set -> blocked non-ramp tiles

    // Place walkable ramp tile (bit 4 clear) at row 5, col 2: attr 1 (elev 0)
    map[5 * 8 + 2] = 0x01; // bit 4 is 0

    PointLayerTilemapDescriptor layer{map.data(), map.size(), 8, 3};
    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    // Entity at Y = 16 (row 2). Starts scanning at (2 + 1) = row 3.
    // Row 3: offset +8 (d5=8), bit 4 set.
    // Row 4: offset +16 (d5=16), bit 4 set.
    // Row 5: offset +24 (d5=24), bit 4 clear -> found candidate at d5=24!
    // candidate_y = 16 + 24 = 40.
    // Probed elevation at (row 5, col 2): attr 1 -> elev 0.
    // If prev_elevation = 40:
    // diff = 40 - 0 = 40 > 0.
    // diff - d5 = 40 - 24 = 16 >= 0 -> Success!
    auto res_success = evaluate_ramp_search_handler_1(2 * 8, 16, 40, layer, elev_table);
    assert(res_success.found_candidate);
    assert(res_success.branch == RampSearchBranch::ContinueSlopeTraversal);
    assert(res_success.final_y == 40);
    assert(res_success.scan_offset == 24);
    assert(res_success.new_elevation == 0);

    // If prev_elevation < 0: branches to PassableGroundCommit
    auto res_void_entry = evaluate_ramp_search_handler_1(2 * 8, 16, -1, layer, elev_table);
    assert(res_void_entry.found_candidate);
    assert(res_void_entry.branch == RampSearchBranch::PassableGroundCommit);

    // If prev_elevation <= new_elevation: branches to BlockedWall in handler 1
    auto res_blocked = evaluate_ramp_search_handler_1(2 * 8, 16, 0, layer, elev_table);
    assert(res_blocked.branch == RampSearchBranch::BlockedWall);

    // If no candidate found in 48 rows: returns BlockedWall
    std::vector<std::uint8_t> solid_map(8 * 60, 0x10);
    PointLayerTilemapDescriptor solid_layer{solid_map.data(), solid_map.size(), 8, 3};
    auto res_not_found = evaluate_ramp_search_handler_1(2 * 8, 16, 40, solid_layer, elev_table);
    assert(!res_not_found.found_candidate);
    assert(res_not_found.branch == RampSearchBranch::BlockedWall);
}

static void test_ramp_search_handler_2() {
    // In handler 2, failures branch to PassableGroundCommit instead of BlockedWall
    std::vector<std::uint8_t> solid_map(8 * 60, 0x10);
    PointLayerTilemapDescriptor solid_layer{solid_map.data(), solid_map.size(), 8, 3};
    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    auto res_not_found = evaluate_ramp_search_handler_2(2 * 8, 16, 40, solid_layer, elev_table);
    assert(!res_not_found.found_candidate);
    assert(res_not_found.branch == RampSearchBranch::PassableGroundCommit);
}

static void test_entity_slot_stepping() {
    std::vector<std::uint8_t> map(8 * 60, 0x10);
    map[5 * 8 + 2] = 0x01; // candidate at d5=24
    PointLayerTilemapDescriptor layer{map.data(), map.size(), 8, 3};
    std::array<std::int16_t, 8> elev_table = {0, 8, 16, 24, 32, 40, 48, -4096};

    std::vector<std::uint8_t> slot(0x80, 0);
    slot[0x0C] = 0x00; slot[0x0D] = 0x10; // Y = 16
    slot[0x10] = 0x00; slot[0x11] = 0x04; // air_h = 4
    slot[0x14] = 0x00; slot[0x15] = 0x00; // ground_h = 0

    // Step handler 1 success:
    auto res = step_ramp_search_handler_1(slot, 2 * 8, 16, 40, layer, elev_table);
    assert(res.branch == RampSearchBranch::ContinueSlopeTraversal);

    // Slot Y should be updated: 16 + 24 = 40
    std::int16_t updated_y = (static_cast<std::int16_t>(slot[0x0C]) << 8) | slot[0x0D];
    assert(updated_y == 40);

    // Air height should be updated: 4 + 24 = 28
    std::int16_t updated_air_h = (static_cast<std::int16_t>(slot[0x10]) << 8) | slot[0x11];
    assert(updated_air_h == 28);
}

int main() {
    std::cout << "Running M16.154 Slope Elevation Engine Tests..." << std::endl;
    test_rom_constants();
    test_flat_and_void_surface_elevation();
    test_slope_fractional_interpolation();
    test_entity_elevation_probe();
    test_ramp_search_handler_1();
    test_ramp_search_handler_2();
    test_entity_slot_stepping();
    std::cout << "All M16.154 tests PASSED!" << std::endl;
    return 0;
}
