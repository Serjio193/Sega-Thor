#include "game/player/world_boundary_clamps.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    static_assert(WorldBoundaryClampsConfig::kClampPosXAddr == 0x009D4EU);
    static_assert(WorldBoundaryClampsConfig::kClampNegXAddr == 0x009D70U);
    static_assert(WorldBoundaryClampsConfig::kClampPosYAddr == 0x009D92U);
    static_assert(WorldBoundaryClampsConfig::kClampNegYAddr == 0x009DB4U);
    static_assert(WorldBoundaryClampsConfig::kBoundaryClampEndAddr == 0x009DD6U);
    static_assert(WorldBoundaryClampsConfig::kBoundaryCollisionFlag == 0x48U);

    static_assert(WorldBoundaryClampsConfig::kXPosOffset == 0x08U);
    static_assert(WorldBoundaryClampsConfig::kYPosOffset == 0x0CU);
    static_assert(WorldBoundaryClampsConfig::kFlagsXOffset == 0x40U);
    static_assert(WorldBoundaryClampsConfig::kFlagsYOffset == 0x41U);
    static_assert(WorldBoundaryClampsConfig::kBoundingRadiusOffset == 0x46U);

    WorldMapBounds bounds{100, 60};
    assert(bounds.max_x() == 799);
    assert(bounds.max_y() == 479);
}

static void test_clamp_pos_x() {
    std::uint8_t flags_x = 0;
    // width = 100 tiles -> max_x = 799
    // x = 100, rad = 8 -> right_edge = 108, remaining = 691
    auto res1 = clamp_step_pos_x(10, 100, 8, 100, flags_x);
    assert(res1.clamped_step == 10);
    assert(!res1.boundary_hit);
    assert(flags_x == 0);

    // x = 790, rad = 5 -> right_edge = 795, remaining = 4
    auto res2 = clamp_step_pos_x(10, 790, 5, 100, flags_x);
    assert(res2.clamped_step == 4);
    assert(!res2.boundary_hit);
    assert(flags_x == 0);

    // x = 794, rad = 5 -> right_edge = 799, remaining = 0
    auto res3 = clamp_step_pos_x(10, 794, 5, 100, flags_x);
    assert(res3.clamped_step == 0);
    assert(res3.boundary_hit);
    assert(flags_x == 0x48);
}

static void test_clamp_neg_x() {
    std::uint8_t flags_x = 0;
    // x = 50, rad = 6 -> remaining = 44
    auto res1 = clamp_step_neg_x(10, 50, 6, flags_x);
    assert(res1.clamped_step == 10);
    assert(!res1.boundary_hit);
    assert(flags_x == 0);

    // x = 12, rad = 6 -> remaining = 6
    auto res2 = clamp_step_neg_x(10, 12, 6, flags_x);
    assert(res2.clamped_step == 6);
    assert(!res2.boundary_hit);
    assert(flags_x == 0);

    // x = 6, rad = 6 -> remaining = 0
    auto res3 = clamp_step_neg_x(10, 6, 6, flags_x);
    assert(res3.clamped_step == 0);
    assert(res3.boundary_hit);
    assert(flags_x == 0x48);

    // x = 4, rad = 6 -> remaining = -2 < 0
    flags_x = 0;
    auto res4 = clamp_step_neg_x(10, 4, 6, flags_x);
    assert(res4.clamped_step == 0);
    assert(res4.boundary_hit);
    assert(flags_x == 0x48);
}

static void test_clamp_pos_y() {
    std::uint8_t flags_y = 0;
    // height = 60 tiles -> max_y = 479
    // y = 100, rad = 8 -> bottom_edge = 108, remaining = 371
    auto res1 = clamp_step_pos_y(10, 100, 8, 60, flags_y);
    assert(res1.clamped_step == 10);
    assert(!res1.boundary_hit);
    assert(flags_y == 0);

    // y = 470, rad = 5 -> bottom_edge = 475, remaining = 4
    auto res2 = clamp_step_pos_y(10, 470, 5, 60, flags_y);
    assert(res2.clamped_step == 4);
    assert(!res2.boundary_hit);
    assert(flags_y == 0);

    // y = 475, rad = 4 -> bottom_edge = 479, remaining = 0
    auto res3 = clamp_step_pos_y(5, 475, 4, 60, flags_y);
    assert(res3.clamped_step == 0);
    assert(res3.boundary_hit);
    assert(flags_y == 0x48);
}

static void test_clamp_neg_y() {
    std::uint8_t flags_y = 0;
    // y = 50, rad = 6 -> remaining = 44
    auto res1 = clamp_step_neg_y(10, 50, 6, flags_y);
    assert(res1.clamped_step == 10);
    assert(!res1.boundary_hit);
    assert(flags_y == 0);

    // y = 12, rad = 6 -> remaining = 6
    auto res2 = clamp_step_neg_y(10, 12, 6, flags_y);
    assert(res2.clamped_step == 6);
    assert(!res2.boundary_hit);
    assert(flags_y == 0);

    // y = 6, rad = 6 -> remaining = 0
    auto res3 = clamp_step_neg_y(10, 6, 6, flags_y);
    assert(res3.clamped_step == 0);
    assert(res3.boundary_hit);
    assert(flags_y == 0x48);

    // y = 2, rad = 6 -> remaining = -4 < 0
    flags_y = 0;
    auto res4 = clamp_step_neg_y(5, 2, 6, flags_y);
    assert(res4.clamped_step == 0);
    assert(res4.boundary_hit);
    assert(flags_y == 0x48);
}

static void test_entity_slot_stepping() {
    std::vector<std::uint8_t> slot(0x80, 0);
    // Entity at (794, 474), rad = 5
    slot[0x08] = 0x03; slot[0x09] = 0x1A; // X = 794
    slot[0x0C] = 0x01; slot[0x0D] = 0xDA; // Y = 474
    slot[0x46] = 0x00; slot[0x47] = 0x05; // rad = 5
    slot[0x40] = 0x00; // flags_x
    slot[0x41] = 0x00; // flags_y

    // Step pos X against width 100 (max_x = 799)
    // right_edge = 794 + 5 = 799 -> remaining = 0 -> hit!
    auto res_x = step_entity_clamp_pos_x(slot, 10, 100);
    assert(res_x.clamped_step == 0);
    assert(res_x.boundary_hit);
    assert(slot[0x40] == 0x48);

    // Step pos Y against height 60 (max_y = 479)
    // bottom_edge = 474 + 5 = 479 -> remaining = 0 -> hit!
    auto res_y = step_entity_clamp_pos_y(slot, 10, 60);
    assert(res_y.clamped_step == 0);
    assert(res_y.boundary_hit);
    assert(slot[0x41] == 0x48);
}

int main() {
    std::cout << "Running M16.156 World Boundary Clamps Tests..." << std::endl;
    test_rom_constants();
    test_clamp_pos_x();
    test_clamp_neg_x();
    test_clamp_pos_y();
    test_clamp_neg_y();
    test_entity_slot_stepping();
    std::cout << "All M16.156 tests PASSED!" << std::endl;
    return 0;
}
