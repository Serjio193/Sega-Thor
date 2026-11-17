#include "game/player/vertical_raycaster.hpp"

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
    assert(VerticalRaycasterConfig::kPosXClearAddr == 0x0091F6U);
    assert(VerticalRaycasterConfig::kPosYRaycasterAddr == 0x00920CU);
    assert(VerticalRaycasterConfig::kPosYRaycasterEndAddr == 0x0092B0U);
    assert(VerticalRaycasterConfig::kNegYRaycasterAddr == 0x0092B0U);
    assert(VerticalRaycasterConfig::kNegYRaycasterEndAddr == 0x00934EU);
    assert(VerticalRaycasterConfig::kFinalYWriteAddr == 0x00934EU);
    assert(VerticalRaycasterConfig::kFinalYWriteEndAddr == 0x009382U);
    assert(VerticalRaycasterConfig::kEntityReturnDispatcherAddr == 0x009382U);
    assert(VerticalRaycasterConfig::kEntityReturnDispatcherEndAddr == 0x00938EU);
    assert(VerticalRaycasterConfig::kTerrainSurfaceLutAddr == 0x0096E8U);
    assert(VerticalRaycasterConfig::kReturnVectorRamAddr == 0x00FF193CU);

    assert(VerticalRaycasterConfig::kDirCodePosY == 0x12U);
    assert(VerticalRaycasterConfig::kDirCodeNegY == 0x10U);

    assert(VerticalRaycasterConfig::kPosXOffset == 0x08U);
    assert(VerticalRaycasterConfig::kSubpixelXOffset == 0x0AU);
    assert(VerticalRaycasterConfig::kPosYOffset == 0x0CU);
    assert(VerticalRaycasterConfig::kSubpixelYOffset == 0x0EU);
    assert(VerticalRaycasterConfig::kSurfaceHeightOffset == 0x14U);
    assert(VerticalRaycasterConfig::kDirCodeXOffset == 0x40U);
    assert(VerticalRaycasterConfig::kDirCodeYOffset == 0x41U);
    assert(VerticalRaycasterConfig::kRadiusOffset == 0x46U);
    assert(VerticalRaycasterConfig::kTerrainByteOffset == 0x6EU);
    assert(VerticalRaycasterConfig::kTerrainAttrOffset == 0x6FU);
    assert(VerticalRaycasterConfig::kTerrainTypeOffset == 0x70U);
    assert(VerticalRaycasterConfig::kVelXOffset == 0x72U);
    assert(VerticalRaycasterConfig::kVelYOffset == 0x76U);
}

static void test_terrain_surface_lut() {
    constexpr std::array<std::uint8_t, 16> expected = {
        0xFFU, 0x00U, 0x02U, 0x01U, 0x04U, 0xFFU, 0x03U, 0xFFU,
        0x06U, 0x07U, 0xFFU, 0xFFU, 0x05U, 0xFFU, 0xFFU, 0xFFU,
    };
    for (std::size_t i = 0; i < 16; ++i) {
        assert(VerticalRaycasterConfig::kTerrainSurfaceLut[i] == expected[i]);
        assert(lookup_terrain_surface_type(static_cast<std::uint8_t>(i)) == expected[i]);
        assert(lookup_terrain_surface_type(static_cast<std::uint8_t>(i | 0x80U)) == expected[i]);
        assert(lookup_terrain_surface_type(static_cast<std::uint8_t>(i | 0x50U)) == expected[i]);
    }

    assert(lookup_terrain_surface_type(0x01) == 0x00);
    assert(lookup_terrain_surface_type(0x04) == 0x04);
    assert(lookup_terrain_surface_type(0x08) == 0x06);
    assert(lookup_terrain_surface_type(0x0C) == 0x05);
}

static void test_tile_row_distances() {
    assert(compute_tile_row_distance_down(0) == 7);
    assert(compute_tile_row_distance_down(1) == 6);
    assert(compute_tile_row_distance_down(6) == 1);
    assert(compute_tile_row_distance_down(7) == 0);
    assert(compute_tile_row_distance_down(108) == 3);

    assert(compute_tile_row_distance_up(0) == 0);
    assert(compute_tile_row_distance_up(1) == 1);
    assert(compute_tile_row_distance_up(6) == 6);
    assert(compute_tile_row_distance_up(7) == 7);
    assert(compute_tile_row_distance_up(108) == 4);
}

static void test_vertical_raycast_down() {
    VerticalRaycastEnvironment env{};
    std::unordered_set<std::int16_t> solid_rows;
    env.is_tile_solid = [&](std::int16_t /*x*/, std::int16_t row) {
        return solid_rows.count(row) > 0;
    };
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(60U), static_cast<std::uint8_t>(0x01U));
    };

    // Free movement +10 with no walls: start 100, radius 8, vel +10.0 (0x000A0000)
    auto res = calculate_vertical_raycast(50, 100, 0, 0x000A0000, 8, env);
    assert(res.final_y == 110);
    assert(!res.hit_wall);
    assert(res.surface_height == 60U);
    assert(res.terrain_type == 0x00U);

    // Wall at row 14: probe starts at 108, d4 = 3 -> advances probe to 111, row 13.
    // Next row 14 is solid -> stops at probe 111, final_y = 111 - 8 = 103.
    solid_rows.insert(14);
    res = calculate_vertical_raycast(50, 100, 0, 0x000A0000, 8, env);
    assert(res.hit_wall);
    assert(res.final_y == 103);
}

static void test_vertical_raycast_up() {
    VerticalRaycastEnvironment env{};
    std::unordered_set<std::int16_t> solid_rows;
    env.is_tile_solid = [&](std::int16_t /*x*/, std::int16_t row) {
        return solid_rows.count(row) > 0;
    };
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(75U), static_cast<std::uint8_t>(0x02U));
    };

    // Free movement -10 with no walls: start 100, radius 8, vel -10.0 (-0x000A0000)
    auto res = calculate_vertical_raycast(50, 100, 0, -0x000A0000, 8, env);
    assert(res.final_y == 90);
    assert(!res.hit_wall);
    assert(res.surface_height == 75U);
    assert(res.terrain_type == 0x02U);

    // Wall at row 10: probe starts at 92, d4 = 4 -> advances probe to 88, row 11.
    // Next row 10 is solid -> stops at probe 88, final_y = 88 + 8 = 96.
    solid_rows.insert(10);
    res = calculate_vertical_raycast(50, 100, 0, -0x000A0000, 8, env);
    assert(res.hit_wall);
    assert(res.final_y == 96);
}

static void test_subpixel_accumulation_and_boundary_callbacks() {
    VerticalRaycastEnvironment env{};
    env.clamp_boundary = [](std::int16_t disp) {
        return (disp > 5) ? 5 : disp;
    };
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(33U), static_cast<std::uint8_t>(0x04U));
    };

    // Start at 100 with subpixel 0x8000, vel = +1.5 pixels (0x00018000). Total = 100.5 + 1.5 = 102.0
    auto res = calculate_vertical_raycast(50, 100, 0x8000, 0x00018000, 8, env);
    assert(res.final_y == 102);
    assert(res.final_subpixel_y == 0x0000);
    assert(res.terrain_type == 0x04U);

    // Boundary clamp: requested disp = +10, clamped to +5
    res = calculate_vertical_raycast(50, 100, 0, 0x000A0000, 8, env);
    assert(res.final_y == 105);
}

static void test_zero_displacement_and_resting_fallback() {
    VerticalRaycastEnvironment env{};
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t y) {
        return std::make_pair(static_cast<std::uint16_t>(y + 5), static_cast<std::uint8_t>(0x08U));
    };

    // vel_y == 0: early return without probe
    auto res = calculate_vertical_raycast(50, 120, 0x2000, 0, 8, env);
    assert(res.final_y == 120);
    assert(res.final_subpixel_y == 0x2000);
    assert(res.surface_height == 0U);

    // Small subpixel step that results in 0 integer displacement
    res = calculate_vertical_raycast(50, 120, 0x1000, 0x00002000, 8, env);
    assert(res.final_y == 120);
    assert(res.final_subpixel_y == 0x3000);
    assert(res.surface_height == 0U);
}

static void test_slot_integration_and_return_dispatcher() {
    std::array<std::uint8_t, 188> slot{};
    slot.fill(0U);

    write_u16(slot, VerticalRaycasterConfig::kPosXOffset, 150U);
    write_u16(slot, VerticalRaycasterConfig::kPosYOffset, 200U);
    write_u16(slot, VerticalRaycasterConfig::kSubpixelYOffset, 0U);
    write_u16(slot, VerticalRaycasterConfig::kRadiusOffset, 8U);
    write_i32(slot, VerticalRaycasterConfig::kVelXOffset, 0x00020000); // stale vel_x
    write_i32(slot, VerticalRaycasterConfig::kVelYOffset, 0x00040000); // +4 pixels Y

    VerticalRaycastEnvironment env{};
    env.probe_surface = [](std::int16_t /*x*/, std::int16_t /*y*/) {
        return std::make_pair(static_cast<std::uint16_t>(88U), static_cast<std::uint8_t>(0x0CU));
    };

    // Step positive Y with Group 1 recovery address (0x008EAA)
    auto res = step_vertical_raycast(slot, 0x008EAAU, env);
    assert(res.final_y == 204);
    assert(slot[VerticalRaycasterConfig::kDirCodeYOffset] == VerticalRaycasterConfig::kDirCodePosY);
    assert(read_u16(slot, VerticalRaycasterConfig::kPosYOffset) == 204U);
    assert(read_u16(slot, VerticalRaycasterConfig::kSurfaceHeightOffset) == 88U);
    assert(slot[VerticalRaycasterConfig::kTerrainTypeOffset] == 0x05U); // 0x0C translates to 0x05
    assert(read_i32(slot, VerticalRaycasterConfig::kVelXOffset) == 0); // vel_x cleared at entry!
    assert(read_i32(slot, VerticalRaycasterConfig::kVelYOffset) == 0); // vel_y cleared at cleanup!
    assert(res.return_vector == 0x008EAAU);

    // Step negative Y with Group 2 recovery address (0x008ECC)
    write_i32(slot, VerticalRaycasterConfig::kVelYOffset, -0x00040000); // -4 pixels Y
    res = step_vertical_raycast(slot, 0x008ECCU, env);
    assert(res.final_y == 200);
    assert(slot[VerticalRaycasterConfig::kDirCodeYOffset] == VerticalRaycasterConfig::kDirCodeNegY);
    assert(read_u16(slot, VerticalRaycasterConfig::kPosYOffset) == 200U);
    assert(read_i32(slot, VerticalRaycasterConfig::kVelYOffset) == 0);
    assert(res.return_vector == 0x008ECCU);
}

int main() {
    std::cout << "Running M16.149 Vertical Raycaster Tests..." << std::endl;
    test_rom_constants();
    test_terrain_surface_lut();
    test_tile_row_distances();
    test_vertical_raycast_down();
    test_vertical_raycast_up();
    test_subpixel_accumulation_and_boundary_callbacks();
    test_zero_displacement_and_resting_fallback();
    test_slot_integration_and_return_dispatcher();
    std::cout << "All M16.149 Vertical Raycaster Tests Passed!" << std::endl;
    return 0;
}
