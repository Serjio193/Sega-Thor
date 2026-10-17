#include "game/world/byte_grid.hpp"
#include "core/rom_identity.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

std::vector<std::uint8_t> read_bytes(const char* path) {
    std::ifstream input(path, std::ios::binary);
    assert(input);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

void verify_screen_root_view(const char* path,
                             const char* decoded_sha,
                             const char* grid_sha) {
    const auto parent = read_bytes(path);
    assert(parent.size() == 12460U);
    assert(oasis::calculate_sha256(parent) == decoded_sha);
    const auto view = oasis::game::world::ByteGridView::from_parent_output(
        parent, 0x204U, 64, 64, 64, 6);
    assert(view && view->valid());
    assert(view->data().data() == parent.data() + 0x204U);
    assert(view->data().size() == 4096U);
    assert(oasis::calculate_sha256(view->data()) == grid_sha);

    for (std::int32_t y = 0; y < 64; ++y) {
        for (std::int32_t x = 0; x < 64; ++x) {
            const auto expected = parent[0x204U +
                static_cast<std::size_t>(y) * 64U + static_cast<std::size_t>(x)];
            assert(view->sample_cell(x, y) == expected);
            assert(view->sample_world(x * 8 + 7, y * 8 + 7) == expected);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    using oasis::game::world::ByteGridView;

    if (argc == 3) {
        verify_screen_root_view(
            argv[1],
            "c3d9ed483fa26e1998950b6dec679c2310918c1c4329d06490c589691853939d",
            "6c0567502f15e5971d02476df859bdf26f8b8b66a897a21a3b76ddd7534f48b3");
        verify_screen_root_view(
            argv[2],
            "3627f94da968f2ee5a059c155dada60e9c0a62cc82e713b1c5d858712df3419f",
            "6c4c80a378361a9e8b5b55b409bff23833886392130a7bca806fa29e20ce107a");
        std::cout << "two exact decoded screen-root ByteGridViews verified\n";
        return 0;
    }
    assert(argc == 1);

    std::array<std::uint8_t, 64> grid{};
    grid[2U * 8U + 3U] = 0xA5U;
    grid[5U * 8U + 7U] = 0x3EU;

    const ByteGridView view(grid, 8, 8, 8, 3);
    assert(view.valid());
    assert(view.width() == 8);
    assert(view.height() == 8);
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
    const ByteGridView terrain_view(terrain, 8, 8, 8, 3);

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

    const auto one_cell_column = terrain_view.aggregate_cell_column_world_y(1, 12, 0);
    assert(one_cell_column);
    assert(one_cell_column->any_bits == 0xF3U);
    assert(one_cell_column->common_bits == 0xF3U);
    const auto three_row_column = terrain_view.aggregate_cell_column_world_y(1, 16, 8);
    assert(three_row_column);
    assert(three_row_column->any_bits == 0xF3U);
    assert(three_row_column->common_bits == 0x00U);
    assert(!terrain_view.aggregate_cell_column_world_y(8, 12, 0));
    assert(!terrain_view.aggregate_cell_column_world_y(1, 0, 1));
    assert(!terrain_view.aggregate_cell_column_world_y(1, 63, 8));

    assert(!view.sample_cell(-1, 0));
    assert(!view.sample_cell(8, 0));
    assert(!view.sample_cell(0, 8));
    assert(!view.sample_world(-1, 0));
    assert(!view.sample_world(0, -1));
    assert(!terrain_view.aggregate_world_square(0, 0, 1));
    assert(!terrain_view.aggregate_world_square(63, 63, 2));

    std::array<std::uint8_t, 12460> parent_a{};
    std::array<std::uint8_t, 12460> parent_b{};
    parent_a[0x204U + 3U * 64U + 2U] = 0xA5U;
    parent_a[0x204U + 63U] = 0x17U;
    parent_a[0x204U + 64U] = 0x18U;
    parent_a[0x204U + 4095U] = 0x19U;
    parent_b[0x204U + 3U * 64U + 2U] = 0x3CU;
    const auto setup_0100 = ByteGridView::from_parent_output(
        parent_a, 0x204U, 64, 64, 64, 6);
    const auto setup_0101 = ByteGridView::from_parent_output(
        parent_b, 0x204U, 64, 64, 64, 6);
    assert(setup_0100 && setup_0101);
    assert(setup_0100->data().data() == parent_a.data() + 0x204U);
    assert(setup_0101->data().data() == parent_b.data() + 0x204U);
    assert(setup_0100->sample_world(0, 0) == 0U);
    assert(setup_0100->sample_world(16, 24) == 0xA5U);
    assert(setup_0101->sample_world(16, 24) == 0x3CU);
    assert(setup_0100->sample_world(511, 0) == 0x17U);
    assert(setup_0100->sample_world(0, 8) == 0x18U);
    assert(setup_0100->sample_world(511, 511) == 0x19U);
    assert(setup_0100->sample_cell(63, 63) == 0x19U);
    assert(!setup_0100->sample_cell(64, 0));
    assert(!setup_0100->sample_cell(0, 64));
    assert(!ByteGridView::from_parent_output(parent_a, 0x204U, 64, 64, 64, 5));
    assert(!ByteGridView::from_parent_output(parent_a, 0x204U, 64, 64, 64, 6 + 1));
    assert(!ByteGridView::from_parent_output(parent_a, parent_a.size() - 1U,
                                             64, 64, 64, 6));

    return 0;
}
