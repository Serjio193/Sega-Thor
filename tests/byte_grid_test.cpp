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

    assert(!view.sample_cell(-1, 0));
    assert(!view.sample_cell(8, 0));
    assert(!view.sample_cell(0, 8));
    assert(!view.sample_world(-1, 0));
    assert(!view.sample_world(0, -1));

    return 0;
}
