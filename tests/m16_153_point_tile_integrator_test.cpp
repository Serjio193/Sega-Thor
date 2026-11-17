#include "game/player/point_tile_integrator.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    assert(PointTileIntegratorConfig::kPointTileSolidityProbeAddr == 0x0098ECU);
    assert(PointTileIntegratorConfig::kSamplePointTileAttrAddr == 0x0099B8U);
    assert(PointTileIntegratorConfig::kReadPointTileRawAddr == 0x0099D6U);
    assert(PointTileIntegratorConfig::kElevationProbeCallAddr == 0x009AD0U);

    assert(PointTileIntegratorConfig::kRampFlagMask == 0x90U);
    assert(PointTileIntegratorConfig::kTileAttrMask == 0x0FU);

    assert(PointTileIntegratorConfig::kYPosOffset == 0x0CU);
    assert(PointTileIntegratorConfig::kAirHeightOffset == 0x10U);
    assert(PointTileIntegratorConfig::kGroundHeightOffset == 0x14U);
}

static void test_tile_attribute_reading() {
    std::vector<std::uint8_t> map(64 * 64, 0x00U);
    // At tile (col 5, row 7): offset = (7 << 6) + 5 = 453
    map[(7 << 6) + 5] = 0x97U;

    PointLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    // Point in tile (col 5, row 7): x = 44 (44 >> 3 = 5), y = 60 (60 >> 3 = 7)
    auto raw = read_point_tile_raw(layer, 44, 60);
    assert(raw == 0x97U);

    auto attr = sample_point_tile_attr(layer, 44, 60);
    assert(attr == 0x07U);
}

static void test_pit_void_target() {
    std::vector<std::uint8_t> map(64 * 64, 0x00U);
    PointLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    PointTileEnvironment env{};
    // Negative target elevation indicates pit / void
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(-1);
    };

    // Standard non-ramp tile: blocked wall
    auto res_wall = evaluate_point_tile_solidity(
        40, 50, 16, 0, layer, env);
    assert(res_wall.is_solid);
    assert(!res_wall.is_ramp);

    // Ramp tile at pit boundary (has 0x90 flags)
    map[(6 << 6) + 5] = 0x95U; // (40>>3=5, 50>>3=6)
    auto res_ramp = evaluate_point_tile_solidity(
        40, 50, 16, 0, layer, env);
    assert(res_ramp.is_ramp);
    assert(!res_ramp.is_solid);
}

static void test_cur_height_negative_entry() {
    PointLayerTilemapDescriptor layer{};
    PointTileEnvironment env{};
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(24);
    };

    // Entering from negative current height (void -> ground)
    auto res = evaluate_point_tile_solidity(
        40, 50, -1, 0, layer, env);
    assert(!res.is_solid);
    assert(res.final_ground_height == 24);
}

static void test_flat_ground_same_height() {
    PointLayerTilemapDescriptor layer{};
    PointTileEnvironment env{};
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(20);
    };

    // delta == 0: passable, keeps height
    auto res = evaluate_point_tile_solidity(
        40, 50, 20, 5, layer, env);
    assert(!res.is_solid);
    assert(res.final_ground_height == 20);
    assert(res.final_air_height == 5);
}

static void test_clearance_blocked() {
    PointLayerTilemapDescriptor layer{};
    PointTileEnvironment env{};
    // Ground rises from 16 to 24 (delta = -8)
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(24);
    };

    // Entity only has air_height = 2: 2 + (-8) = -6 < 0 -> blocked!
    auto res = evaluate_point_tile_solidity(
        40, 50, 16, 2, layer, env);
    assert(res.is_solid);
    assert(res.final_y == 50);
    assert(res.final_air_height == 2);
    assert(res.final_ground_height == 16);
}

static void test_slope_descending_and_ascending() {
    std::vector<std::uint8_t> map(64 * 64, 0x01U);
    PointLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    PointTileEnvironment env{};
    // Test 1: Descending slope (20 -> 16, delta = 4), air_h = 2
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(16);
    };
    auto res_desc = evaluate_point_tile_solidity(
        40, 100, 20, 2, layer, env);
    assert(!res_desc.is_solid);
    assert(res_desc.final_y == 104);
    assert(res_desc.final_air_height == 6);
    assert(res_desc.final_ground_height == 16);

    // Test 2: Ascending slope (16 -> 20, delta = -4), air_h = 5 (clearance 5-4 = 1 >= 0)
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(20);
    };
    auto res_asc = evaluate_point_tile_solidity(
        40, 100, 16, 5, layer, env);
    assert(!res_asc.is_solid);
    assert(res_asc.final_y == 96);
    assert(res_asc.final_air_height == 1);
    assert(res_asc.final_ground_height == 20);
}

static void test_slope_boundary_attribute_mismatch_stops_early() {
    std::vector<std::uint8_t> map(64 * 64, 0x01U);
    // At row 13 (y = 104): set different tile attribute 0x02
    for (int col = 0; col < 64; ++col) {
        map[(13 << 6) + col] = 0x02U;
    }

    PointLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    PointTileEnvironment env{};
    // Descending delta = 10, start at y = 101.
    // Tile 12 ends at y = 103 (103 & 7 = 7).
    // When y advances to 104 (104 & 7 = 0), it crosses into row 13 which has attribute 2 != 1!
    // It should step back Y -= 1 (to 103) and break!
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(10);
    };

    auto res = evaluate_point_tile_solidity(
        40, 101, 20, 0, layer, env);
    assert(!res.is_solid);
    assert(res.final_y == 103);
}

static void test_entity_slot_step() {
    std::vector<std::uint8_t> slot(188, 0x00U);
    // Y pos = 100 at +0x0C, air_height = 5 at +0x10, ground_height = 16 at +0x14
    slot[0x0C] = 0x00U; slot[0x0D] = 0x64U; // 100
    slot[0x10] = 0x00U; slot[0x11] = 0x05U; // 5
    slot[0x14] = 0x00U; slot[0x15] = 0x10U; // 16

    std::vector<std::uint8_t> map(64 * 64, 0x01U);
    PointLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    PointTileEnvironment env{};
    // Target ground rises to 20
    env.probe_elevation = [](std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(20);
    };

    auto res = step_point_tile_solidity_probe(slot, 40, 100, layer, env);
    assert(!res.is_solid);
    assert(res.final_y == 96);
    assert(res.final_air_height == 1);
    assert(res.final_ground_height == 20);

    // Verify slot updated
    assert(slot[0x0C] == 0x00U && slot[0x0D] == 0x60U); // 96
    assert(slot[0x10] == 0x00U && slot[0x11] == 0x01U); // 1
    assert(slot[0x14] == 0x00U && slot[0x15] == 0x14U); // 20
}

int main() {
    test_rom_constants();
    test_tile_attribute_reading();
    test_pit_void_target();
    test_cur_height_negative_entry();
    test_flat_ground_same_height();
    test_clearance_blocked();
    test_slope_descending_and_ascending();
    test_slope_boundary_attribute_mismatch_stops_early();
    test_entity_slot_step();

    std::cout << "All M16.153 Point Tile Integrator tests passed successfully!\n";
    return 0;
}
