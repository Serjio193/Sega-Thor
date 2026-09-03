#include "game/world/byte_grid.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using oasis::game::world::ByteGridView;

    std::array<std::uint8_t, 64> grid{};
    grid[2U * 8U + 3U] = 0xA5U;
    grid[5U * 8U + 7U] = 0x3EU;

    const ByteGridView view(grid, 8, 3);
    assert(view.row_stride() == 8);
    assert(view.row_shift() == 3);

    assert(view.sample_cell(3, 2) == 0xA5U);
    assert(view.sample_world(3 * 8 + 7, 2 * 8 + 1) == 0xA5U);
    assert(view.terrain_code_world(3 * 8, 2 * 8) == 0x05U);

    assert(view.sample_cell(7, 5) == 0x3EU);
    assert(view.terrain_code_world(7 * 8, 5 * 8) == 0x0EU);

    std::array<std::uint8_t, 64> terrain{};
    terrain[1U * 8U + 1U] = 0xF3U;
    terrain[1U * 8U + 2U] = 0xA7U;
    terrain[2U * 8U + 1U] = 0xB3U;
    terrain[2U * 8U + 2U] = 0xE7U;
    const ByteGridView terrain_view(terrain, 8, 3);

    const auto one_cell = terrain_view.aggregate_world_square(12, 12, 0);
    assert(one_cell);
    assert(one_cell->any_bits == 0xF3U);
    assert(one_cell->common_bits == 0xF3U);
    assert(one_cell->common_terrain_code() == 0x03U);

    const auto four_cells = terrain_view.aggregate_world_square(15, 15, 1);
    assert(four_cells);
    assert(four_cells->any_bits == 0xF7U);
    assert(four_cells->common_bits == 0xA3U);
    assert(four_cells->common_terrain_code() == 0x03U);

    const auto boundary_cross = terrain_view.aggregate_world_square(8, 8, 1);
    assert(boundary_cross);
    assert(boundary_cross->any_bits == 0xF3U);
    assert(boundary_cross->common_bits == 0x00U);

    assert(!view.sample_cell(-1, 0));
    assert(!view.sample_cell(8, 0));
    assert(!view.sample_cell(0, 8));
    assert(!view.sample_world(-1, 0));
    assert(!view.sample_world(0, -1));
    assert(!terrain_view.aggregate_world_square(0, 0, 1));
    assert(!terrain_view.aggregate_world_square(63, 63, 2));

    return 0;
}
