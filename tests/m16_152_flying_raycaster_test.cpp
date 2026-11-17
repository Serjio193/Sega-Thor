#include "game/player/flying_raycaster.hpp"

#include <cassert>
#include <iostream>
#include <unordered_set>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    assert(FlyingRaycasterConfig::kFlyingHorizontalRaycastAddr == 0x009708U);
    assert(FlyingRaycasterConfig::kFlyingHorizontalRaycastEndAddr == 0x0097E6U);
    assert(FlyingRaycasterConfig::kFlyingVerticalRaycastAddr == 0x0097E6U);
    assert(FlyingRaycasterConfig::kFlyingVerticalRaycastEndAddr == 0x0098C2U);
    assert(FlyingRaycasterConfig::kFlyingSurfaceSampleExitAddr == 0x0098C2U);
    assert(FlyingRaycasterConfig::kPointWallProbeAddr == 0x0098ECU);
    assert(FlyingRaycasterConfig::kEntityReturnDispatcherAddr == 0x009382U);

    assert(FlyingRaycasterConfig::kDirPosX == 0x11U);
    assert(FlyingRaycasterConfig::kDirNegX == 0x13U);
    assert(FlyingRaycasterConfig::kDirPosY == 0x12U);
    assert(FlyingRaycasterConfig::kDirNegY == 0x10U);

    assert(FlyingRaycasterConfig::kCollisionWallBit == 6U);
    assert(FlyingRaycasterConfig::kCollisionWallMask == 0x40U);

    assert(FlyingRaycasterConfig::kPosXOffset == 0x08U);
    assert(FlyingRaycasterConfig::kSubpixelXOffset == 0x0AU);
    assert(FlyingRaycasterConfig::kPosYOffset == 0x0CU);
    assert(FlyingRaycasterConfig::kSubpixelYOffset == 0x0EU);
    assert(FlyingRaycasterConfig::kSurfaceHeightOffset == 0x14U);
    assert(FlyingRaycasterConfig::kCollisionFlags40Offset == 0x40U);
    assert(FlyingRaycasterConfig::kCollisionFlags41Offset == 0x41U);
    assert(FlyingRaycasterConfig::kRadiusOffset == 0x46U);
    assert(FlyingRaycasterConfig::kSurfaceByte6EOffset == 0x6EU);
    assert(FlyingRaycasterConfig::kSurfaceByte6FOffset == 0x6FU);
    assert(FlyingRaycasterConfig::kVelXOffset == 0x72U);
    assert(FlyingRaycasterConfig::kVelYOffset == 0x76U);
}

static void test_horizontal_raycast_pos_x() {
    FlyingRaycastEnvironment env{};

    // Move +10 from x=100 with no obstacles: should land at 110
    auto res_clear = calculate_flying_horizontal_raycast(
        100, 0, 10 << 16, 50, env);
    assert(res_clear.final_x == 110);
    assert(!res_clear.hit_wall_x);
    assert(res_clear.dir_code_x == 0x11U);
    assert(res_clear.step_count_a3 == 1U);

    // Solid obstacle at x = 104: should stop at 103 with wall hit
    env.point_probe = [](std::int16_t x, std::int16_t) {
        return (x == 104);
    };
    auto res_hit = calculate_flying_horizontal_raycast(
        100, 0, 10 << 16, 50, env);
    assert(res_hit.final_x == 103);
    assert(res_hit.hit_wall_x);
    assert(res_hit.dir_code_x == 0x11U);
}

static void test_horizontal_raycast_neg_x() {
    FlyingRaycastEnvironment env{};

    // Move -10 from x=100 with no obstacles: should land at 90
    auto res_clear = calculate_flying_horizontal_raycast(
        100, 0, -(10 << 16), 50, env);
    assert(res_clear.final_x == 90);
    assert(!res_clear.hit_wall_x);
    assert(res_clear.dir_code_x == 0x13U);
    assert(res_clear.step_count_a3 == 1U);

    // Solid obstacle at x = 95: should stop at 96 with wall hit
    env.point_probe = [](std::int16_t x, std::int16_t) {
        return (x == 95);
    };
    auto res_hit = calculate_flying_horizontal_raycast(
        100, 0, -(10 << 16), 50, env);
    assert(res_hit.final_x == 96);
    assert(res_hit.hit_wall_x);
    assert(res_hit.dir_code_x == 0x13U);
}

static void test_vertical_raycast_pos_y() {
    FlyingRaycastEnvironment env{};

    // Move +15 from y=200 without collisions: lands at 215
    auto res_clear = calculate_flying_vertical_raycast(
        100, 200, 0, 15 << 16, env);
    assert(res_clear.final_y == 215);
    assert(!res_clear.hit_wall_y);
    assert(res_clear.dir_code_y == 0x12U);
    assert(res_clear.step_count_a3 == 1U);

    // Moving +15 hits barrier at 208: stops at 207
    env.point_probe = [](std::int16_t, std::int16_t y) {
        return (y == 208);
    };
    auto res_hit = calculate_flying_vertical_raycast(
        100, 200, 0, 15 << 16, env);
    assert(res_hit.final_y == 207);
    assert(res_hit.hit_wall_y);
    assert(res_hit.dir_code_y == 0x12U);
}

static void test_vertical_raycast_neg_y() {
    FlyingRaycastEnvironment env{};

    // Move -15 from y=200 without collisions: lands at 185
    auto res_clear = calculate_flying_vertical_raycast(
        100, 200, 0, -(15 << 16), env);
    assert(res_clear.final_y == 185);
    assert(!res_clear.hit_wall_y);
    assert(res_clear.dir_code_y == 0x10U);
    assert(res_clear.step_count_a3 == 1U);

    // Barrier at y = 191: stops at 192
    env.point_probe = [](std::int16_t, std::int16_t y) {
        return (y == 191);
    };
    auto res_hit = calculate_flying_vertical_raycast(
        100, 200, 0, -(15 << 16), env);
    assert(res_hit.final_y == 192);
    assert(res_hit.hit_wall_y);
    assert(res_hit.dir_code_y == 0x10U);
}

static void test_dual_axis_and_surface_probe() {
    FlyingRaycastEnvironment env{};
    env.probe_surface_height = [](std::int16_t x, std::int16_t y, std::int16_t rad) {
        return static_cast<std::int16_t>(x + y + rad);
    };

    // Both X and Y velocities non-zero
    auto res = calculate_flying_dual_axis_raycast(
        100, 0, 10 << 16,
        200, 0, 15 << 16,
        4, env);

    assert(res.final_x == 110);
    assert(res.final_y == 215);
    assert(res.step_count_a3 == 2U);
    assert(res.ground_surface_height == (110 + 215 + 4));

    // Zero velocities: step_count_a3 == 0, probe_surface_height skipped
    auto res_idle = calculate_flying_dual_axis_raycast(
        100, 0, 0,
        200, 0, 0,
        4, env);
    assert(res_idle.final_x == 100);
    assert(res_idle.final_y == 200);
    assert(res_idle.step_count_a3 == 0U);
    assert(res_idle.ground_surface_height == 0);
}

static void test_boundary_clamp_and_entity_sweep() {
    FlyingRaycastEnvironment env{};
    // Clamp displacement to at most 5 pixels
    env.clamp_boundary_x = [](std::int16_t disp) {
        return std::min<std::int16_t>(disp, 5);
    };
    // Sweep shortens displacement to at most 3 pixels
    env.sweep_entities_x = [](std::int16_t disp) {
        return std::min<std::int16_t>(disp, 3);
    };

    auto res = calculate_flying_horizontal_raycast(
        100, 0, 10 << 16, 50, env);
    assert(res.final_x == 103);
    assert(!res.hit_wall_x);
}

static void test_entity_slot_step() {
    std::vector<std::uint8_t> slot(188, 0x00U);
    // X = 100, Y = 200, vel_x = +10 px, vel_y = -15 px, radius = 6
    slot[0x08] = 0x00U; slot[0x09] = 0x64U; // X = 100
    slot[0x0C] = 0x00U; slot[0x0D] = 0xC8U; // Y = 200
    slot[0x46] = 0x00U; slot[0x47] = 0x06U; // radius = 6

    // vel_x = +10.0 (0x000A0000)
    slot[0x72] = 0x00U; slot[0x73] = 0x0AU; slot[0x74] = 0x00U; slot[0x75] = 0x00U;
    // vel_y = -15.0 (0xFFF10000)
    slot[0x76] = 0xFFU; slot[0x77] = 0xF1U; slot[0x78] = 0x00U; slot[0x79] = 0x00U;

    FlyingRaycastEnvironment env{};
    // Point obstacle at (104, Y): hits wall in X
    env.point_probe = [](std::int16_t x, std::int16_t) {
        return (x == 104);
    };
    env.probe_surface_height = [](std::int16_t, std::int16_t, std::int16_t) {
        return static_cast<std::int16_t>(42);
    };

    auto res = step_flying_entity_simulation(slot, env);

    assert(res.hit_wall_x);
    assert(!res.hit_wall_y);
    assert(res.final_x == 103);
    assert(res.final_y == 185);

    // Slot X direction 0x11 | 0x40 (wall bit 6) = 0x51
    assert(slot[0x40] == 0x51U);
    // Slot Y direction 0x10 (no wall bit) = 0x10
    assert(slot[0x41] == 0x10U);

    // X pos written to +0x08
    assert(slot[0x08] == 0x00U && slot[0x09] == 0x67U); // 103
    // Y pos written to +0x0C
    assert(slot[0x0C] == 0x00U && slot[0x0D] == 0xB9U); // 185

    // Velocities cleared to 0
    assert(slot[0x72] == 0 && slot[0x73] == 0 && slot[0x74] == 0 && slot[0x75] == 0);
    assert(slot[0x76] == 0 && slot[0x77] == 0 && slot[0x78] == 0 && slot[0x79] == 0);

    // Ground surface height written to +0x14
    assert(slot[0x14] == 0x00U && slot[0x15] == 0x2AU); // 42
}

int main() {
    test_rom_constants();
    test_horizontal_raycast_pos_x();
    test_horizontal_raycast_neg_x();
    test_vertical_raycast_pos_y();
    test_vertical_raycast_neg_y();
    test_dual_axis_and_surface_probe();
    test_boundary_clamp_and_entity_sweep();
    test_entity_slot_step();

    std::cout << "All M16.152 Flying Raycaster tests passed successfully!\n";
    return 0;
}
