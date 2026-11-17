#include "game/world/room_layer_loader.hpp"

#include <cassert>

namespace {

using namespace oasis::game::world;

void test_row_shift_helper() {
    assert(compute_row_shift(0U) == 0U);
    assert(compute_row_shift(32U) == 6U);  // 32 * 2 = 64 = 1 << 6
    assert(compute_row_shift(64U) == 7U);  // 64 * 2 = 128 = 1 << 7
    assert(compute_row_shift(128U) == 8U); // 128 * 2 = 256 = 1 << 8
}

void test_sample_2x1_room_header() {
    // Plane A: 2x1 (64x32 tiles) + collision bit 7 -> 0x82, 0x01
    // Plane B: 1x1 (32x32 tiles), no collision -> 0x01, 0x01
    const auto layout = decode_room_layer_header({0x82U, 0x01U, 0x01U, 0x01U});
    assert(layout.plane_a_vram == 0xC000U);
    assert(layout.plane_b_vram == 0xE000U);
    assert(layout.tileset_dma_words == 0x2000U);

    assert(layout.plane_a.tiles_w == 64U);
    assert(layout.plane_a.tiles_h == 32U);
    assert(layout.plane_a.row_shift == 7U);
    assert(layout.plane_a.tilemap_offset == 4U);
    assert(layout.plane_a.tilemap_bytes == 4096U);
    assert(layout.plane_a.has_collision);
    assert(layout.plane_a.collision_offset == 4100U);
    assert(layout.plane_a.collision_bytes == 2048U);

    assert(layout.plane_b.tiles_w == 32U);
    assert(layout.plane_b.tiles_h == 32U);
    assert(layout.plane_b.row_shift == 6U);
    assert(layout.plane_b.tilemap_offset == 6148U);
    assert(layout.plane_b.tilemap_bytes == 2048U);
    assert(!layout.plane_b.has_collision);
    assert(layout.plane_b.collision_offset == 0U);
    assert(layout.plane_b.collision_bytes == 0U);
}

void test_dual_collision_room_header() {
    // Both Plane A and Plane B have collision bit 7 set (e.g. 2x1 + 2x1)
    const auto layout = decode_room_layer_header({0x82U, 0x01U, 0x82U, 0x01U});
    assert(layout.plane_b.has_collision);
    assert(layout.plane_b.tilemap_offset == 6148U);
    assert(layout.plane_b.tilemap_bytes == 4096U);
    assert(layout.plane_b.collision_offset == 10244U);
    assert(layout.plane_b.collision_bytes == 2048U);
}

} // namespace

int main() {
    test_row_shift_helper();
    test_sample_2x1_room_header();
    test_dual_collision_room_header();
    return 0;
}
