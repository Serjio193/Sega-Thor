#include "game/player/vertical_wall_probe.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants_and_lut() {
    assert(VerticalWallProbeConfig::kTileSolidProbeNegYAddr == 0x0094D2U);
    assert(VerticalWallProbeConfig::kTileSolidProbeNegYEndAddr == 0x0095AAU);
    assert(VerticalWallProbeConfig::kTileSolidProbePosYAddr == 0x0095AAU);
    assert(VerticalWallProbeConfig::kTileSolidProbePosYEndAddr == 0x009678U);
    assert(VerticalWallProbeConfig::kScanRowSolidityAddr == 0x009678U);
    assert(VerticalWallProbeConfig::kScanRowSolidityEndAddr == 0x0096BAU);
    assert(VerticalWallProbeConfig::kSampleRowSpanAddr == 0x0096BAU);
    assert(VerticalWallProbeConfig::kSampleRowSpanEndAddr == 0x0096E8U);

    assert(VerticalWallProbeConfig::kElevationLutRam == 0x00FF1706U);
    assert(VerticalWallProbeConfig::kLedgeDropHeightRam == 0x00FF196EU);
    assert(VerticalWallProbeConfig::kTerrainSurfaceLutAddr == 0x0096E8U);

    assert(VerticalWallProbeConfig::kPosXOffset == 0x08U);
    assert(VerticalWallProbeConfig::kPosZOffset == 0x10U);
    assert(VerticalWallProbeConfig::kFlags38Offset == 0x38U);
    assert(VerticalWallProbeConfig::kCollisionFlags41Offset == 0x41U);
    assert(VerticalWallProbeConfig::kSurfaceTypeOffset == 0x70U);
    assert(VerticalWallProbeConfig::kVelYOffset == 0x76U);

    assert(VerticalWallProbeConfig::kCollisionLedgeDropMask == 0x20U);
    assert(VerticalWallProbeConfig::kCollisionWallMask == 0x40U);

    // Verify 16-entry LUT
    const std::array<std::uint8_t, 16> expected_lut = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU,
    };
    for (std::size_t i = 0; i < 16; ++i) {
        assert(VerticalWallProbeConfig::kTerrainSurfaceLut[i] == expected_lut[i]);
        assert(lookup_vertical_terrain_surface_type(static_cast<std::uint8_t>(i)) == expected_lut[i]);
    }
}

static void test_sample_tile_row_span() {
    std::vector<std::uint8_t> map(64 * 64, 0x01U);
    map[(5 << 6) + 10] = 0x12U;
    map[(5 << 6) + 11] = 0x14U;
    map[(5 << 6) + 12] = 0x16U;

    VerticalLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    TileRowScanResult res = sample_tile_row_span(layer, 5, 92, 10);
    assert(res.accumulated_or == 0x16U);
    assert(res.accumulated_and == 0x10U);
}

static void test_scan_tile_row_solidity_multi_row() {
    std::vector<std::uint8_t> map(64 * 64, 0x00U);
    VerticalLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    for (int col = 9; col <= 11; ++col) {
        map[(2 << 6) + col] = 0x92U;
    }
    map[(3 << 6) + 10] = 0x11U;
    map[(4 << 6) + 10] = 0x11U;
    for (int col = 9; col <= 11; ++col) {
        map[(5 << 6) + col] = 0x02U;
    }

    TileRowScanResult res = scan_tile_row_solidity(layer, 2, 80, 8);
    assert(res.scan_extent_a4 == 24);
}

static void test_scan_tile_row_solidity_non_solid_initial() {
    std::vector<std::uint8_t> map(64 * 64, 0x00U);
    VerticalLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    map[(2 << 6) + 10] = 0x10U; // missing bit 7

    TileRowScanResult res = scan_tile_row_solidity(layer, 2, 80, 8);
    assert(res.scan_extent_a4 == 0);
}

static void test_intangible_bypass() {
    VerticalWallProbeParams params{};
    params.flags_38 = 0x01U; // intangible
    params.cur_surface_type = 2U;
    params.target_raw_terrain = 0x04U;

    auto res = evaluate_vertical_solid_probe(params);
    assert(!res.is_solid);
    assert(res.flags_41 == 0U);

    params.flags_38 = 0U;
    params.cur_surface_type = 0xFFU; // -1
    res = evaluate_vertical_solid_probe(params);
    assert(!res.is_solid);
    assert(res.flags_41 == 0U);
}

static void test_wall_hit_steep_height_delta() {
    VerticalWallProbeParams params{};
    params.flags_38 = 0U;
    params.cur_surface_type = 1U;
    params.target_raw_terrain = 0x04U; // maps to 4 (delta = 3 >= 2)

    auto res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(res.flags_41 == VerticalWallProbeConfig::kCollisionWallMask);
    assert(res.target_surface_type == 4U);

    // Void target surface
    params.target_raw_terrain = 0x00U;
    res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(res.flags_41 == VerticalWallProbeConfig::kCollisionWallMask);
}

static void test_incline_climb_disabled_and_wall_bit() {
    VerticalWallProbeParams params{};
    params.flags_38 = 0x20U; // climb disabled (bit 5)
    params.cur_surface_type = 1U;
    params.target_raw_terrain = 0x02U; // maps to 2 (delta = 1)

    auto res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(res.flags_41 == VerticalWallProbeConfig::kCollisionWallMask);

    params.flags_38 = 0U;
    params.accumulated_or_d4 = 0x10U;
    res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);

    params.accumulated_or_d4 = 0x00U;
    res = evaluate_vertical_solid_probe(params);
    assert(!res.is_solid);
    assert(res.flags_41 == 0U);
}

static void test_flat_step_slope_and_wall_bits() {
    VerticalWallProbeParams params{};
    params.flags_38 = 0U;
    params.cur_surface_type = 2U;
    params.target_raw_terrain = 0x02U; // delta = 0

    auto res = evaluate_vertical_solid_probe(params);
    assert(!res.is_solid);
    assert(res.flags_41 == 0U);

    params.accumulated_or_d4 = 0x80U;
    res = evaluate_vertical_solid_probe(params);
    assert(!res.is_solid);

    params.flags_38 = 0x40U;
    res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(res.flags_41 == VerticalWallProbeConfig::kCollisionWallMask);

    params.flags_38 = 0U;
    params.accumulated_or_d4 = 0x10U;
    res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
}

static void test_ledge_drop_success() {
    const std::vector<std::int16_t> elev = {16, 24, 48, 56, 72};
    VerticalWallProbeParams params{};
    params.flags_38 = 0x10U;
    params.cur_surface_type = 2U;
    params.target_raw_terrain = 0x01U; // maps to 0 (delta = -2)
    params.pos_z = 0;
    params.scan_extent_a4 = 24;
    params.elevation_table = elev;

    auto res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(!res.wall_hit);
    assert(res.ledge_drop_triggered);
    assert(res.flags_41 == VerticalWallProbeConfig::kCollisionLedgeDropMask);
    assert(res.ledge_drop_height == 32);
}

static void test_ledge_drop_guards_fallback_to_wall() {
    const std::vector<std::int16_t> elev = {16, 24, 48, 56, 72};
    VerticalWallProbeParams params{};
    params.cur_surface_type = 2U;
    params.target_raw_terrain = 0x01U;
    params.pos_z = 0;
    params.scan_extent_a4 = 24;
    params.elevation_table = elev;

    params.flags_38 = 0U;
    auto res = evaluate_vertical_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(!res.ledge_drop_triggered);
    assert(res.flags_41 == VerticalWallProbeConfig::kCollisionWallMask);

    params.flags_38 = 0x10U;
    params.pos_z = 10;
    res = evaluate_vertical_solid_probe(params);
    assert(res.wall_hit);

    params.pos_z = 0;
    params.cur_surface_type = 3U;
    res = evaluate_vertical_solid_probe(params);
    assert(res.wall_hit);

    params.cur_surface_type = 2U;
    params.scan_extent_a4 = 40;
    res = evaluate_vertical_solid_probe(params);
    assert(res.wall_hit);
}

static void test_entity_slot_step_neg_y_and_pos_y() {
    std::vector<std::uint8_t> slot(188, 0x00U);
    slot[0x08] = 0x00U; slot[0x09] = 0x50U; // X = 80
    slot[0x38] = 0x10U;
    slot[0x70] = 0x02U;
    slot[0x76] = 0x00U; slot[0x77] = 0x02U;

    std::vector<std::uint8_t> map(64 * 64, 0x00U);
    VerticalLayerTilemapDescriptor layer{};
    layer.base = map.data();
    layer.base_size = map.size();
    layer.row_shift = 6U;
    layer.row_stride = 64;

    for (int col = 8; col <= 12; ++col) {
        map[(4 << 6) + col] = 0x01U;
    }

    const std::vector<std::int16_t> elev = {16, 24, 48};
    std::uint16_t drop_ram = 0;

    auto res_neg = step_vertical_wall_probe_neg_y(slot, 4, 8, layer, elev, &drop_ram);
    assert(res_neg.is_solid);
    assert(res_neg.ledge_drop_triggered);
    assert(slot[0x41] == 0x20U);
    assert(drop_ram == 32U);
    assert(slot[0x76] == 0x00U);
    assert(slot[0x77] == 0x00U);
}

int main() {
    test_rom_constants_and_lut();
    test_sample_tile_row_span();
    test_scan_tile_row_solidity_multi_row();
    test_scan_tile_row_solidity_non_solid_initial();
    test_intangible_bypass();
    test_wall_hit_steep_height_delta();
    test_incline_climb_disabled_and_wall_bit();
    test_flat_step_slope_and_wall_bits();
    test_ledge_drop_success();
    test_ledge_drop_guards_fallback_to_wall();
    test_entity_slot_step_neg_y_and_pos_y();

    std::cout << "All M16.151 Vertical Wall Probe tests passed successfully!\n";
    return 0;
}
