#include "game/player/horizontal_wall_probe.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void write_i32(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) {
    auto u = static_cast<std::uint32_t>(val);
    buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
}

static std::int32_t read_i32(std::span<const std::uint8_t> buf, std::size_t offset) {
    std::uint32_t u = (static_cast<std::uint32_t>(buf[offset]) << 24U) |
                      (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
                      (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
                      static_cast<std::uint32_t>(buf[offset + 3U]);
    return static_cast<std::int32_t>(u);
}

static void test_rom_constants() {
    assert(HorizontalWallProbeConfig::kTileSolidProbeXAddr == 0x00938EU);
    assert(HorizontalWallProbeConfig::kTileSolidProbeXEndAddr == 0x009460U);
    assert(HorizontalWallProbeConfig::kScanColumnSolidityAddr == 0x009460U);
    assert(HorizontalWallProbeConfig::kScanColumnSolidityEndAddr == 0x0094A2U);
    assert(HorizontalWallProbeConfig::kSampleColumnSpanAddr == 0x0094A2U);
    assert(HorizontalWallProbeConfig::kSampleColumnSpanEndAddr == 0x0094D2U);

    assert(HorizontalWallProbeConfig::kElevationLutRam == 0x00FF1706U);
    assert(HorizontalWallProbeConfig::kLedgeDropHeightRam == 0x00FF196EU);
    assert(HorizontalWallProbeConfig::kTerrainSurfaceLutAddr == 0x0096E8U);

    assert(HorizontalWallProbeConfig::kPosYOffset == 0x0CU);
    assert(HorizontalWallProbeConfig::kPosZOffset == 0x10U);
    assert(HorizontalWallProbeConfig::kFlags38Offset == 0x38U);
    assert(HorizontalWallProbeConfig::kCollisionFlags40Offset == 0x40U);
    assert(HorizontalWallProbeConfig::kSurfaceTypeOffset == 0x70U);
    assert(HorizontalWallProbeConfig::kVelYOffset == 0x76U);

    assert(HorizontalWallProbeConfig::kFlagIntangibleBit == 0U);
    assert(HorizontalWallProbeConfig::kFlagLedgeDropCapableBit == 4U);
    assert(HorizontalWallProbeConfig::kFlagNoClimbBit == 5U);
    assert(HorizontalWallProbeConfig::kFlagSlopeCheckBit == 6U);

    assert(HorizontalWallProbeConfig::kCollisionLedgeDropBit == 5U);
    assert(HorizontalWallProbeConfig::kCollisionWallBit == 6U);
    assert(HorizontalWallProbeConfig::kCollisionLedgeDropMask == 0x20U);
    assert(HorizontalWallProbeConfig::kCollisionWallMask == 0x40U);
}

static void test_intangible_entity_passable() {
    WallProbeParams params{};
    params.flags_38 = 0x01U; // intangible bit 0 set
    params.cur_surface_type = 0U;
    params.target_raw_terrain = 0x04U;
    params.accumulated_or_d4 = 0x10U; // solid wall bit

    auto res = evaluate_tile_solid_probe(params);
    assert(!res.is_solid);
    assert(!res.wall_hit);
    assert(res.flags_40 == 0U);
}

static void test_negative_current_surface_passable() {
    WallProbeParams params{};
    params.flags_38 = 0x00U;
    params.cur_surface_type = 0xFFU; // signed negative
    params.target_raw_terrain = 0x04U;

    auto res = evaluate_tile_solid_probe(params);
    assert(!res.is_solid);
    assert(!res.wall_hit);
    assert(res.flags_40 == 0U);
}

static void test_steep_wall_and_void_pit_collision() {
    // 1. delta >= 2 (target=4, cur=1 -> delta=3)
    WallProbeParams params{};
    params.flags_38 = 0x00U;
    params.cur_surface_type = 1U;
    params.target_raw_terrain = 0x04U; // maps to 4
    auto res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(res.flags_40 == HorizontalWallProbeConfig::kCollisionWallMask);

    // 2. Void / pit: target raw terrain = 0 (maps to 0xFF)
    params.cur_surface_type = 0U;
    params.target_raw_terrain = 0x00U;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
    assert(res.flags_40 == HorizontalWallProbeConfig::kCollisionWallMask);
}

static void test_same_level_and_slope_edge() {
    WallProbeParams params{};
    params.cur_surface_type = 0U;
    params.target_raw_terrain = 0x01U; // maps to 0 -> delta = 0

    // No flags in d4: passable
    params.accumulated_or_d4 = 0x00U;
    auto res = evaluate_tile_solid_probe(params);
    assert(!res.is_solid);

    // Wall solid bit 4 in d4
    params.accumulated_or_d4 = 0x10U;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);

    // Slope bit 7 in d4, without slope check flag in flags_38
    params.accumulated_or_d4 = 0x80U;
    params.flags_38 = 0x00U;
    res = evaluate_tile_solid_probe(params);
    assert(!res.is_solid);

    // Slope bit 7 in d4, WITH slope check flag in flags_38 (bit 6 = 0x40)
    params.flags_38 = 0x40U;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
}

static void test_one_step_incline_and_decline() {
    WallProbeParams params{};

    // 1. Incline delta = 1 (target=1 [0x03], cur=0)
    params.cur_surface_type = 0U;
    params.target_raw_terrain = 0x03U; // maps to 1

    // Normal climb: passable
    params.flags_38 = 0x00U;
    params.accumulated_or_d4 = 0x00U;
    auto res = evaluate_tile_solid_probe(params);
    assert(!res.is_solid);

    // Climb disabled: flags_38 bit 5 (0x20) set -> blocked
    params.flags_38 = 0x20U;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);

    // 2. Decline delta = -1 (target=0 [0x01], cur=1)
    params.flags_38 = 0x00U;
    params.cur_surface_type = 1U;
    params.target_raw_terrain = 0x01U; // maps to 0 -> delta = -1
    res = evaluate_tile_solid_probe(params);
    assert(!res.is_solid);

    // Wall bit 4 set in d4 -> blocked
    params.accumulated_or_d4 = 0x10U;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.wall_hit);
}

static void test_ledge_drop_trigger() {
    std::array<std::int16_t, 8> elevation = {16, 20, 40, 45, 60, 65, 80, 85};
    WallProbeParams params{};
    params.elevation_table = elevation;

    // cur = 2, target = 0 (delta = -2)
    params.cur_surface_type = 2U;
    params.target_raw_terrain = 0x01U; // maps to 0
    params.flags_38 = 0x10U;           // bit 4 (can ledge drop)
    params.pos_z = 0;                  // on ground
    params.scan_extent_a4 = 20;

    // Drop height = elev[2] - elev[0] = 40 - 16 = 24 >= 20 -> ledge drop triggers!
    auto res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(res.ledge_drop_triggered);
    assert(!res.wall_hit);
    assert(res.flags_40 == HorizontalWallProbeConfig::kCollisionLedgeDropMask);
    assert(res.ledge_drop_height == 24);

    // Air height != 0 (airborne entity cannot ledge drop): wall collision
    params.pos_z = 10;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(!res.ledge_drop_triggered);
    assert(res.wall_hit);
    assert(res.flags_40 == HorizontalWallProbeConfig::kCollisionWallMask);
    params.pos_z = 0;

    // Odd surface type (cur = 3): odd surface cannot ledge drop
    params.cur_surface_type = 3U;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(!res.ledge_drop_triggered);
    assert(res.wall_hit);
    params.cur_surface_type = 2U;

    // Drop height < scan extent (e.g. extent = 30 > 24): wall collision
    params.scan_extent_a4 = 30;
    res = evaluate_tile_solid_probe(params);
    assert(res.is_solid);
    assert(!res.ledge_drop_triggered);
    assert(res.wall_hit);
}

static void test_slot_integration() {
    std::array<std::uint8_t, 188> slot{};
    slot.fill(0U);

    std::array<std::int16_t, 8> elevation = {16, 20, 40, 45, 60, 65, 80, 85};
    slot[HorizontalWallProbeConfig::kFlags38Offset] = 0x10U; // ledge drop capable
    slot[HorizontalWallProbeConfig::kSurfaceTypeOffset] = 2U;
    slot[HorizontalWallProbeConfig::kCollisionFlags40Offset] = 0x00U;
    write_i32(slot, HorizontalWallProbeConfig::kPosZOffset, 0); // grounded
    write_i32(slot, HorizontalWallProbeConfig::kVelYOffset, 0x00030000); // active vel_y

    std::uint16_t ledge_drop_ram = 0U;

    auto res = step_horizontal_wall_probe(
        slot,
        0x01U, // target maps to 0
        0x00U, // d4
        20,    // extent a4
        elevation,
        &ledge_drop_ram);

    assert(res.is_solid);
    assert(res.ledge_drop_triggered);
    assert(slot[HorizontalWallProbeConfig::kCollisionFlags40Offset] == 0x20U);
    assert(read_i32(slot, HorizontalWallProbeConfig::kVelYOffset) == 0); // vel_y cleared!
    assert(ledge_drop_ram == 24U);
}

static void test_sample_tile_column_span() {
    // 32-byte mock tile buffer: 4 rows x 8 columns, row_stride = 8, col_shift = 3
    std::array<std::uint8_t, 32> buffer{};
    buffer.fill(0x05U); // fill with 0x05
    buffer[8] = 0x15U;  // row 1 col 0 has bit 4 set

    LayerTilemapDescriptor desc{};
    desc.base = buffer.data();
    desc.base_size = buffer.size();
    desc.row_stride = 8;
    desc.col_shift = 3;

    // center_y = 12 (row 1), radius = 6 -> span rows 0 to 2
    auto scan = sample_tile_column_span(desc, 0, 12, 6);
    assert((scan.accumulated_or & 0x10U) != 0U); // bit 4 accumulated
    assert((scan.accumulated_and & 0x01U) != 0U); // bit 0 preserved across all rows
}

int main() {
    std::cout << "Running M16.150 Horizontal Wall Probe Tests..." << std::endl;
    test_rom_constants();
    test_intangible_entity_passable();
    test_negative_current_surface_passable();
    test_steep_wall_and_void_pit_collision();
    test_same_level_and_slope_edge();
    test_one_step_incline_and_decline();
    test_ledge_drop_trigger();
    test_slot_integration();
    test_sample_tile_column_span();
    std::cout << "All M16.150 Horizontal Wall Probe Tests Passed!" << std::endl;
    return 0;
}
