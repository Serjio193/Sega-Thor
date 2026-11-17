#include "game/world/player_tile_box.hpp"

#include <cassert>

namespace {

using namespace oasis::game::world;

void test_inside_tile_box_boundaries() {
    // Tile box (2, 3) .. (5, 6) => pixel box [16, 47] x [24, 55]
    constexpr std::int32_t r = 4;
    // Exact min corner: px - 4 == 16, py - 4 == 24
    assert(player_inside_tile_box(20, 28, r, 2, 3, 5, 6));
    // Exact max corner (+7 inclusive bias): px + 4 == 47, py + 4 == 55
    assert(player_inside_tile_box(43, 51, r, 2, 3, 5, 6));

    // One pixel past min/max boundaries fails containment
    assert(!player_inside_tile_box(19, 28, r, 2, 3, 5, 6));
    assert(!player_inside_tile_box(20, 27, r, 2, 3, 5, 6));
    assert(!player_inside_tile_box(44, 51, r, 2, 3, 5, 6));
    assert(!player_inside_tile_box(43, 52, r, 2, 3, 5, 6));
}

void test_overlaps_tile_box_boundaries() {
    // Tile box (2, 3) .. (5, 6) => pixel box [16, 47] x [24, 55]
    constexpr std::int32_t r = 4;
    // Touching min corner: px + 4 == 16, py + 4 == 24
    assert(player_overlaps_tile_box(12, 20, r, 2, 3, 5, 6));
    // Touching max corner (+7 inclusive bias): px - 4 == 47, py - 4 == 55
    assert(player_overlaps_tile_box(51, 59, r, 2, 3, 5, 6));

    // One pixel outside each edge fails overlap
    assert(!player_overlaps_tile_box(11, 20, r, 2, 3, 5, 6));
    assert(!player_overlaps_tile_box(12, 19, r, 2, 3, 5, 6));
    assert(!player_overlaps_tile_box(52, 59, r, 2, 3, 5, 6));
    assert(!player_overlaps_tile_box(51, 60, r, 2, 3, 5, 6));

    // Partial overlap on +X edge is overlap=true, inside=false
    assert(player_overlaps_tile_box(44, 40, r, 2, 3, 5, 6));
    assert(!player_inside_tile_box(44, 40, r, 2, 3, 5, 6));
}

void test_single_tile_box_zero_radius() {
    // Single tile (1, 1) .. (1, 1) => [8, 15] x [8, 15]
    assert(player_inside_tile_box(8, 8, 0, 1, 1, 1, 1));
    assert(player_inside_tile_box(15, 15, 0, 1, 1, 1, 1));
    assert(!player_inside_tile_box(16, 15, 0, 1, 1, 1, 1));
    assert(player_overlaps_tile_box(15, 15, 0, 1, 1, 1, 1));
    assert(!player_overlaps_tile_box(16, 15, 0, 1, 1, 1, 1));
}

} // namespace

int main() {
    test_inside_tile_box_boundaries();
    test_overlaps_tile_box_boundaries();
    test_single_tile_box_zero_radius();
    return 0;
}
