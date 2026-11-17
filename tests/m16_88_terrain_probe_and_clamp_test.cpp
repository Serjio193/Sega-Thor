#include "game/world/terrain_probe_and_clamp.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::world;

void test_terrain_surface_lut() {
    constexpr std::array<std::uint8_t, 16> expected = {
        0xFFU, 0U, 2U, 1U, 4U, 0xFFU, 3U, 0xFFU,
        6U, 7U, 0xFFU, 0xFFU, 5U, 0xFFU, 0xFFU, 0xFFU
    };
    for (std::size_t i = 0; i < 16U; ++i) {
        assert(kTerrainSurfaceLut16[i] == expected[i]);
        assert(lookup_terrain_surface_effect(static_cast<std::uint8_t>(i)) == expected[i]);
        assert(lookup_terrain_surface_effect(static_cast<std::uint8_t>(0xA0U | i)) == expected[i]);
    }
    assert(select_terrain_layer_desc(false) == 0xFF1766U);
    assert(select_terrain_layer_desc(true) == 0xFF1770U);
}

void test_room_boundary_clamp() {
    // 32x32 tiles -> [0, 255] x [0, 255], radius = 10
    // Interior move: no clamp, flags = 0
    const auto interior = clamp_entity_to_room_bounds(100, 120, 4, -3, 10, 32, 32);
    assert(interior.dx == 4 && interior.dy == -3);
    assert(interior.col_x_flags == 0 && interior.col_y_flags == 0);

    // Right edge clamp: 240 + 10 + 10 = 260 > 255 -> dx = (255 - 10) - 240 = 5
    const auto right = clamp_entity_to_room_bounds(240, 120, 10, 0, 10, 32, 32);
    assert(right.dx == 5 && right.dy == 0);
    assert(right.col_x_flags == 0x48U && right.col_y_flags == 0);

    // Left edge clamp: 12 - 5 - 10 = -3 < 0 -> dx = 10 - 12 = -2
    const auto left = clamp_entity_to_room_bounds(12, 120, -5, 2, 10, 32, 32);
    assert(left.dx == -2 && left.dy == 2);
    assert(left.col_x_flags == 0x48U && left.col_y_flags == 0);

    // Bottom edge clamp: 242 + 8 + 10 = 260 > 255 -> dy = (255 - 10) - 242 = 3
    const auto bottom = clamp_entity_to_room_bounds(100, 242, 0, 8, 10, 32, 32);
    assert(bottom.dx == 0 && bottom.dy == 3);
    assert(bottom.col_x_flags == 0 && bottom.col_y_flags == 0x48U);

    // Top edge clamp: 14 - 8 - 10 = -4 < 0 -> dy = 10 - 14 = -4
    const auto top = clamp_entity_to_room_bounds(100, 14, 0, -8, 10, 32, 32);
    assert(top.dx == 0 && top.dy == -4);
    assert(top.col_x_flags == 0 && top.col_y_flags == 0x48U);

    // Corner clamp (top-left): both X and Y clamped
    const auto corner = clamp_entity_to_room_bounds(12, 14, -10, -10, 10, 32, 32);
    assert(corner.dx == -2 && corner.dy == -4);
    assert(corner.col_x_flags == 0x48U && corner.col_y_flags == 0x48U);
}

} // namespace

int main() {
    test_terrain_surface_lut();
    test_room_boundary_clamp();
    return 0;
}
