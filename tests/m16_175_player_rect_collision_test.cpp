#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "game/collision/player_rect_collision.hpp"

using namespace oasis::game::collision;

namespace {

void test_constants() {
    static_assert(PlayerRectCollisionConfig::kPlayerInsideRectAddr == 0x00C17AU);
    static_assert(PlayerRectCollisionConfig::kPlayerInsideFailAddr == 0x00C1C4U);
    static_assert(PlayerRectCollisionConfig::kPlayerInsideRtsAddr == 0x00C1CCU);

    static_assert(PlayerRectCollisionConfig::kPlayerOverlapsRectAddr == 0x00C1CEU);
    static_assert(PlayerRectCollisionConfig::kPlayerOverlapsFailAddr == 0x00C218U);
    static_assert(PlayerRectCollisionConfig::kPlayerOverlapsRtsAddr == 0x00C220U);
    static_assert(PlayerRectCollisionConfig::kRangeEndAddr == 0x00C222U);
    static_assert(PlayerRectCollisionConfig::kRoutineSizeBytes == 168U);

    static_assert(PlayerRectCollisionConfig::kPlayerEntityRam == 0x00FF19E8U);
    static_assert(PlayerRectCollisionConfig::kPlayerWorldXOffset == 0x08U);
    static_assert(PlayerRectCollisionConfig::kPlayerWorldYOffset == 0x0CU);
    static_assert(PlayerRectCollisionConfig::kPlayerCollisionRadOffset == 0x46U);

    static_assert(PlayerRectCollisionConfig::kTileScaleShift == 3U);
    static_assert(PlayerRectCollisionConfig::kMaxExtentBias == 0x00070007U);
    static_assert(PlayerRectCollisionConfig::kTilePixelSpan == 7);

    std::cout << "[PASS] test_constants\n";
}

void test_coordinate_unpacking() {
    const TileRect rect{
        .x_min = 10,
        .y_min = 20,
        .x_max = 14,
        .y_max = 24
    };

    const PixelAabb2D bounds = unpack_tile_rect_pixel_bounds(rect);
    assert(bounds.x_min == 80);
    assert(bounds.y_min == 160);
    assert(bounds.x_max == 119); // 14 * 8 + 7
    assert(bounds.y_max == 199); // 24 * 8 + 7

    const std::uint32_t d1 = (10U << 16U) | 20U;
    const std::uint32_t d2 = (14U << 16U) | 24U;
    const PixelAabb2D packed_bounds = unpack_registers_pixel_bounds(d1, d2);
    assert(packed_bounds == bounds);

    std::cout << "[PASS] test_coordinate_unpacking\n";
}

void test_full_containment() {
    const TileRect rect{.x_min = 10, .y_min = 20, .x_max = 14, .y_max = 24};
    // Pixel bounds: [80..119] X, [160..199] Y

    // Completely inside: px = [90..110], py = [170..190]
    const PlayerCollisionState inside_player{
        .world_x = 100,
        .world_y = 180,
        .radius = 10
    };
    assert(is_player_inside_rect(rect, inside_player) == true);
    assert(does_player_overlap_rect(rect, inside_player) == true);

    // Exact flush boundary containment: px = [80..119], py = [160..199]
    // (not possible with integer radius, but bounds test directly)
    const PixelAabb2D exact_bounds = unpack_tile_rect_pixel_bounds(rect);
    assert(is_player_inside_pixel_bounds(exact_bounds, inside_player) == true);

    std::cout << "[PASS] test_full_containment\n";
}

void test_partial_overlap() {
    const TileRect rect{.x_min = 10, .y_min = 20, .x_max = 14, .y_max = 24};
    // Pixel bounds: [80..119] X, [160..199] Y

    // Protrudes left: px = [75..95], py = [170..190]
    const PlayerCollisionState protrude_left{
        .world_x = 85,
        .world_y = 180,
        .radius = 10
    };
    assert(is_player_inside_rect(rect, protrude_left) == false);
    assert(does_player_overlap_rect(rect, protrude_left) == true);

    // Protrudes right: px = [115..125], py = [170..190]
    const PlayerCollisionState protrude_right{
        .world_x = 120,
        .world_y = 180,
        .radius = 5
    };
    assert(is_player_inside_rect(rect, protrude_right) == false);
    assert(does_player_overlap_rect(rect, protrude_right) == true);

    // Protrudes top: px = [100..110], py = [155..165]
    const PlayerCollisionState protrude_top{
        .world_x = 105,
        .world_y = 160,
        .radius = 5
    };
    assert(is_player_inside_rect(rect, protrude_top) == false);
    assert(does_player_overlap_rect(rect, protrude_top) == true);

    // Protrudes bottom: px = [100..110], py = [195..205]
    const PlayerCollisionState protrude_bottom{
        .world_x = 105,
        .world_y = 200,
        .radius = 5
    };
    assert(is_player_inside_rect(rect, protrude_bottom) == false);
    assert(does_player_overlap_rect(rect, protrude_bottom) == true);

    std::cout << "[PASS] test_partial_overlap\n";
}

void test_edge_touch_and_disjoint() {
    const TileRect rect{.x_min = 10, .y_min = 20, .x_max = 14, .y_max = 24};
    // Pixel bounds: [80..119] X, [160..199] Y

    // Exactly touches left edge at X=80: px = [60..80]
    const PlayerCollisionState touch_left{
        .world_x = 70,
        .world_y = 180,
        .radius = 10
    };
    assert(is_player_inside_rect(rect, touch_left) == false);
    assert(does_player_overlap_rect(rect, touch_left) == true);

    // 1 pixel off left edge: px = [59..79]
    const PlayerCollisionState miss_left{
        .world_x = 69,
        .world_y = 180,
        .radius = 10
    };
    assert(is_player_inside_rect(rect, miss_left) == false);
    assert(does_player_overlap_rect(rect, miss_left) == false);

    // Exactly touches right edge at X=119: px = [119..139]
    const PlayerCollisionState touch_right{
        .world_x = 129,
        .world_y = 180,
        .radius = 10
    };
    assert(is_player_inside_rect(rect, touch_right) == false);
    assert(does_player_overlap_rect(rect, touch_right) == true);

    // 1 pixel off right edge: px = [120..140]
    const PlayerCollisionState miss_right{
        .world_x = 130,
        .world_y = 180,
        .radius = 10
    };
    assert(is_player_inside_rect(rect, miss_right) == false);
    assert(does_player_overlap_rect(rect, miss_right) == false);

    std::cout << "[PASS] test_edge_touch_and_disjoint\n";
}

void test_negative_coordinates() {
    const PixelAabb2D rect_bounds{
        .x_min = -100,
        .y_min = -80,
        .x_max = -50,
        .y_max = -40
    };

    const PlayerCollisionState inside{
        .world_x = -75,
        .world_y = -60,
        .radius = 5
    };
    assert(is_player_inside_pixel_bounds(rect_bounds, inside) == true);
    assert(does_player_overlap_pixel_bounds(rect_bounds, inside) == true);

    const PlayerCollisionState disjoint{
        .world_x = -40,
        .world_y = -60,
        .radius = 5
    }; // px = [-45, -35]
    assert(is_player_inside_pixel_bounds(rect_bounds, disjoint) == false);
    assert(does_player_overlap_pixel_bounds(rect_bounds, disjoint) == false);

    std::cout << "[PASS] test_negative_coordinates\n";
}

void test_packed_registers() {
    const std::uint32_t d1 = (10U << 16U) | 20U;
    const std::uint32_t d2 = (14U << 16U) | 24U;

    const PlayerCollisionState inside{.world_x = 100, .world_y = 180, .radius = 10};
    assert(is_player_inside_packed(d1, d2, inside) == true);
    assert(does_player_overlap_packed(d1, d2, inside) == true);

    const PlayerCollisionState partial{.world_x = 85, .world_y = 180, .radius = 10};
    assert(is_player_inside_packed(d1, d2, partial) == false);
    assert(does_player_overlap_packed(d1, d2, partial) == true);

    const PlayerCollisionState miss{.world_x = 69, .world_y = 180, .radius = 10};
    assert(is_player_inside_packed(d1, d2, miss) == false);
    assert(does_player_overlap_packed(d1, d2, miss) == false);

    std::cout << "[PASS] test_packed_registers\n";
}

void test_raw_memory_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00);

    const PlayerCollisionState player{
        .world_x = 100,
        .world_y = 180,
        .radius = 10
    };
    write_player_collision_raw(ram, player);

    const PlayerCollisionState read_back = read_player_collision_raw(ram);
    assert(read_back == player);

    const TileRect rect{.x_min = 10, .y_min = 20, .x_max = 14, .y_max = 24};
    assert(is_player_inside_rect_raw(ram, rect) == true);
    assert(does_player_overlap_rect_raw(ram, rect) == true);

    const std::uint32_t d1 = (10U << 16U) | 20U;
    const std::uint32_t d2 = (14U << 16U) | 24U;
    assert(is_player_inside_packed_raw(ram, d1, d2) == true);
    assert(does_player_overlap_packed_raw(ram, d1, d2) == true);

    // Move player outside
    write_player_collision_raw(ram, PlayerCollisionState{.world_x = 60, .world_y = 180, .radius = 10});
    assert(is_player_inside_rect_raw(ram, rect) == false);
    assert(does_player_overlap_rect_raw(ram, rect) == false);

    std::cout << "[PASS] test_raw_memory_integration\n";
}

} // namespace

int main() {
    test_constants();
    test_coordinate_unpacking();
    test_full_containment();
    test_partial_overlap();
    test_edge_touch_and_disjoint();
    test_negative_coordinates();
    test_packed_registers();
    test_raw_memory_integration();

    std::cout << "\nAll M16.175 Player Rect Collision tests passed successfully!\n";
    return 0;
}
