#include "game/player/horizontal_raycaster.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <vector>

using namespace oasis::game::player;

static void write_u16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) {
    buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

static void write_i32(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) {
    auto u = static_cast<std::uint32_t>(val);
    buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
}

static std::uint16_t read_u16(std::span<const std::uint8_t> buf, std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8U) |
        static_cast<std::uint16_t>(buf[offset + 1U]));
}

static std::int32_t read_i32(std::span<const std::uint8_t> buf, std::size_t offset) {
    std::uint32_t u = (static_cast<std::uint32_t>(buf[offset]) << 24U) |
                      (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
                      (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
                      static_cast<std::uint32_t>(buf[offset + 3U]);
    return static_cast<std::int32_t>(u);
}

static void test_rom_constants() {
    assert(HorizontalRaycasterConfig::kPosXRaycasterAddr == 0x009084U);
    assert(HorizontalRaycasterConfig::kPosXRaycasterEndAddr == 0x009128U);
    assert(HorizontalRaycasterConfig::kNegXRaycasterAddr == 0x009128U);
    assert(HorizontalRaycasterConfig::kNegXRaycasterEndAddr == 0x0091C6U);
    assert(HorizontalRaycasterConfig::kRaycastCleanupAddr == 0x0091C6U);
    assert(HorizontalRaycasterConfig::kRaycastCleanupEndAddr == 0x0091F6U);
    assert(HorizontalRaycasterConfig::kTerrainSurfaceLutAddr == 0x0096E8U);

    assert(HorizontalRaycasterConfig::kDirCodePosX == 0x11U);
    assert(HorizontalRaycasterConfig::kDirCodeNegX == 0x13U);

    assert(HorizontalRaycasterConfig::kPosXOffset == 0x08U);
    assert(HorizontalRaycasterConfig::kSubpixelXOffset == 0x0AU);
    assert(HorizontalRaycasterConfig::kPosYOffset == 0x0CU);
    assert(HorizontalRaycasterConfig::kSurfaceHeightOffset == 0x14U);
    assert(HorizontalRaycasterConfig::kDirCodeOffset == 0x40U);
    assert(HorizontalRaycasterConfig::kRadiusXOffset == 0x46U);
    assert(HorizontalRaycasterConfig::kTerrainByteOffset == 0x6EU);
    assert(HorizontalRaycasterConfig::kTerrainAttrOffset == 0x6FU);
    assert(HorizontalRaycasterConfig::kTerrainTypeOffset == 0x70U);
    assert(HorizontalRaycasterConfig::kVelXOffset == 0x72U);
}

static void test_terrain_surface_lut() {
    constexpr std::array<std::uint8_t, 16> expected = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU,
    };
    for (std::size_t i = 0; i < 16; ++i) {
        assert(HorizontalRaycasterConfig::kTerrainSurfaceLut[i] == expected[i]);
        assert(lookup_terrain_surface_type(static_cast<std::uint8_t>(i)) == expected[i]);
        assert(lookup_terrain_surface_type(static_cast<std::uint8_t>(i | 0x80U)) == expected[i]);
        assert(lookup_terrain_surface_type(static_cast<std::uint8_t>(i | 0x30U)) == expected[i]);
    }

    assert(lookup_terrain_surface_type(0x01) == 0x00);
    assert(lookup_terrain_surface_type(0x04) == 0x04);
    assert(lookup_terrain_surface_type(0x08) == 0x06);
    assert(lookup_terrain_surface_type(0x0C) == 0x05);
    assert(lookup_terrain_surface_type(0x11) == 0x00);
}

static void test_tile_distances() {
    assert(compute_tile_distance_right(0) == 7);
    assert(compute_tile_distance_right(1) == 6);
    assert(compute_tile_distance_right(6) == 1);
    assert(compute_tile_distance_right(7) == 0);
    assert(compute_tile_distance_right(108) == 3);

    assert(compute_tile_distance_left(0) == 0);
    assert(compute_tile_distance_left(1) == 1);
    assert(compute_tile_distance_left(6) == 6);
    assert(compute_tile_distance_left(7) == 7);
    assert(compute_tile_distance_left(108) == 4);
}

static void test_raycast_movement_positive_x() {
    RaycastEnvironment env{};
    std::unordered_set<std::int16_t> solid_cols;
    env.is_tile_solid = [&](std::int16_t col, std::int16_t /*y*/) {
        return solid_cols.count(col) > 0;
    };
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(42U), static_cast<std::uint8_t>(0x01U));
    };

    // Free movement +10 with no walls: start 100, radius 8, vel +10.0 (0x000A0000)
    auto res = calculate_horizontal_raycast(100, 0, 0x000A0000, 50, 8, env);
    assert(res.final_x == 110);
    assert(!res.hit_wall);
    assert(res.surface_height == 42U);
    assert(res.terrain_type == 0x00U);

    // Wall at col 14: probe starts at 108, d4 = 3 -> advances probe to 111, col 13.
    // Next tile is col 14, solid -> stops at probe 111, final_x = 111 - 8 = 103.
    solid_cols.insert(14);
    res = calculate_horizontal_raycast(100, 0, 0x000A0000, 50, 8, env);
    assert(res.hit_wall);
    assert(res.final_x == 103);
}

static void test_raycast_movement_negative_x() {
    RaycastEnvironment env{};
    std::unordered_set<std::int16_t> solid_cols;
    env.is_tile_solid = [&](std::int16_t col, std::int16_t /*y*/) {
        return solid_cols.count(col) > 0;
    };
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(55U), static_cast<std::uint8_t>(0x02U));
    };

    // Free movement -10 with no walls: start 100, radius 8, vel -10.0 (-0x000A0000)
    auto res = calculate_horizontal_raycast(100, 0, -0x000A0000, 50, 8, env);
    assert(res.final_x == 90);
    assert(!res.hit_wall);
    assert(res.surface_height == 55U);
    assert(res.terrain_type == 0x02U);

    // Wall at col 10: probe starts at 92, d4 = 4 -> advances probe to 88, col 11.
    // Next tile is col 10, solid -> stops at probe 88, final_x = 88 + 8 = 96.
    solid_cols.insert(10);
    res = calculate_horizontal_raycast(100, 0, -0x000A0000, 50, 8, env);
    assert(res.hit_wall);
    assert(res.final_x == 96);
}

static void test_subpixel_accumulation_and_boundary_callbacks() {
    RaycastEnvironment env{};
    env.clamp_boundary = [](std::int16_t disp) {
        return (disp > 5) ? 5 : disp;
    };
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(10U), static_cast<std::uint8_t>(0x04U));
    };

    // Start at 100 with subpixel 0x8000, vel = +1.5 pixels (0x00018000). Total = 100.5 + 1.5 = 102.0
    auto res = calculate_horizontal_raycast(100, 0x8000, 0x00018000, 50, 8, env);
    assert(res.final_x == 102);
    assert(res.final_subpixel == 0x0000);
    assert(res.terrain_type == 0x04U);

    // Boundary clamp: requested disp = +10, clamped to +5
    res = calculate_horizontal_raycast(100, 0, 0x000A0000, 50, 8, env);
    assert(res.final_x == 105);
}

static void test_zero_displacement_and_resting_fallback() {
    RaycastEnvironment env{};
    env.probe_surface = [](std::int16_t x, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(x + 10), static_cast<std::uint8_t>(0x08U));
    };

    // vel_x == 0
    auto res = calculate_horizontal_raycast(120, 0x2000, 0, 40, 8, env);
    assert(res.final_x == 120);
    assert(res.final_subpixel == 0x2000);
    assert(res.surface_height == 130U);
    assert(res.terrain_type == 0x06U);

    // Small subpixel step that results in 0 integer displacement
    res = calculate_horizontal_raycast(120, 0x1000, 0x00002000, 40, 8, env);
    assert(res.final_x == 120);
    assert(res.final_subpixel == 0x3000);
    assert(res.surface_height == 130U);
    assert(res.terrain_type == 0x06U);
}

static void test_slot_integration() {
    std::array<std::uint8_t, 188> slot{};
    slot.fill(0U);

    write_u16(slot, HorizontalRaycasterConfig::kPosXOffset, 200U);
    write_u16(slot, HorizontalRaycasterConfig::kSubpixelXOffset, 0U);
    write_u16(slot, HorizontalRaycasterConfig::kPosYOffset, 150U);
    write_u16(slot, HorizontalRaycasterConfig::kRadiusXOffset, 8U);
    write_i32(slot, HorizontalRaycasterConfig::kVelXOffset, 0x00040000); // +4 pixels

    RaycastEnvironment env{};
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(25U), static_cast<std::uint8_t>(0x0CU));
    };

    auto res = step_horizontal_raycast(slot, env);
    assert(res.final_x == 204);
    assert(slot[HorizontalRaycasterConfig::kDirCodeOffset] == HorizontalRaycasterConfig::kDirCodePosX);
    assert(read_u16(slot, HorizontalRaycasterConfig::kPosXOffset) == 204U);
    assert(read_u16(slot, HorizontalRaycasterConfig::kSurfaceHeightOffset) == 25U);
    assert(slot[HorizontalRaycasterConfig::kTerrainTypeOffset] == 0x05U); // 0x0C translates to 0x05
    assert(read_i32(slot, HorizontalRaycasterConfig::kVelXOffset) == 0); // vel_x cleared!

    // Step negative X
    write_i32(slot, HorizontalRaycasterConfig::kVelXOffset, -0x00040000); // -4 pixels
    res = step_horizontal_raycast(slot, env);
    assert(res.final_x == 200);
    assert(slot[HorizontalRaycasterConfig::kDirCodeOffset] == HorizontalRaycasterConfig::kDirCodeNegX);
    assert(read_u16(slot, HorizontalRaycasterConfig::kPosXOffset) == 200U);
    assert(read_i32(slot, HorizontalRaycasterConfig::kVelXOffset) == 0);
}

int main() {
    std::cout << "Running M16.148 Horizontal Raycaster Tests..." << std::endl;
    test_rom_constants();
    test_terrain_surface_lut();
    test_tile_distances();
    test_raycast_movement_positive_x();
    test_raycast_movement_negative_x();
    test_subpixel_accumulation_and_boundary_callbacks();
    test_zero_displacement_and_resting_fallback();
    test_slot_integration();
    std::cout << "All M16.148 Horizontal Raycaster Tests Passed!" << std::endl;
    return 0;
}
