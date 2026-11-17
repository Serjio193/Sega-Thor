#include "game/world/vdp_plane_scroll.hpp"

#include <array>
#include <cassert>
#include <vector>

namespace {

using namespace oasis::game::world;

std::vector<std::uint16_t> make_test_map(std::uint16_t w, std::uint16_t h) {
    std::vector<std::uint16_t> m(static_cast<std::size_t>(w) * h);
    for (std::uint16_t y = 0U; y < h; ++y) {
        for (std::uint16_t x = 0U; x < w; ++x) {
            m[static_cast<std::size_t>(y) * w + x] = static_cast<std::uint16_t>(y * 100U + x);
        }
    }
    return m;
}

void test_positive_and_negative_column_scroll() {
    const auto map = make_test_map(64U, 40U);
    std::array<std::uint16_t, kVdpScrollNametableWords> vdp{};
    vdp.fill(0xFFFFU);

    VdpPlaneScrollState st{};
    st.src_col = 23;
    st.src_row = 30; // wraps across map_height=40 during 29-row column stream
    st.vram_offset = static_cast<std::uint16_t>((20U << 7U) | (62U << 1U)); // vdp_row=20, vdp_col=62
    st.pending_dx_tiles = 1;
    st.map_width_tiles = 64U;
    st.map_height_tiles = 40U;

    // Positive +1 col: src_col -> 24, read_col = (24 + 40) % 64 = 0
    // vdp_col -> (62 + 1) % 64 = 63, write_col = (63 + 40) % 64 = 39
    assert(stream_plane_column(st, map, vdp));
    assert(st.pending_dx_tiles == 0);
    assert(st.src_col == 24);
    assert(st.vram_offset == static_cast<std::uint16_t>((20U << 7U) | (63U << 1U)));

    // Row 0 of stream: src(0, 30) -> vdp(39, 20)
    assert(vdp[20U * 64U + 39U] == 30U * 100U + 0U);
    // Row 10 of stream: src_row wraps 30+10=40 -> 0, vdp_row=(20+10)%32=30
    assert(vdp[30U * 64U + 39U] == 0U * 100U + 0U);
    // Row 12 of stream: src_row=2, vdp_row=(20+12)%32=0
    assert(vdp[0U * 64U + 39U] == 2U * 100U + 0U);

    // Negative -1 col: src_col -> 23, read_col = 23, vdp_col -> 62, write_col = 62
    st.pending_dx_tiles = -1;
    assert(stream_plane_column(st, map, vdp));
    assert(st.pending_dx_tiles == 0);
    assert(st.src_col == 23);
    assert(vdp[20U * 64U + 62U] == 30U * 100U + 23U);
}

void test_positive_and_negative_row_scroll() {
    const auto map = make_test_map(64U, 40U);
    std::array<std::uint16_t, kVdpScrollNametableWords> vdp{};
    vdp.fill(0xFFFFU);

    VdpPlaneScrollState st{};
    st.src_col = 50; // wraps across map_width=64 during 41-col row stream
    st.src_row = 11;
    st.vram_offset = static_cast<std::uint16_t>((31U << 7U) | (50U << 1U)); // vdp_row=31, vdp_col=50
    st.pending_dy_tiles = 1;
    st.map_width_tiles = 64U;
    st.map_height_tiles = 40U;

    // Positive +1 row: src_row -> 12, read_row = (12 + 28) % 40 = 0
    // vdp_row -> (31 + 1) % 32 = 0, write_row = (0 + 28) % 32 = 28
    assert(stream_plane_row(st, map, vdp));
    assert(st.pending_dy_tiles == 0);
    assert(st.src_row == 12);
    assert(st.vram_offset == static_cast<std::uint16_t>((0U << 7U) | (50U << 1U)));

    // First tile: src(50, 0) -> vdp(50, 28)
    assert(vdp[28U * 64U + 50U] == 0U * 100U + 50U);
    // Col 40 of stream: src_col=(50+40)%64=26, vdp_col=(50+40)%64=26
    assert(vdp[28U * 64U + 26U] == 0U * 100U + 26U);

    // Negative -1 row: src_row -> 11, read_row = 11, vdp_row -> 31, write_row = 31
    st.pending_dy_tiles = -1;
    assert(stream_plane_row(st, map, vdp));
    assert(st.pending_dy_tiles == 0);
    assert(st.src_row == 11);
    assert(vdp[31U * 64U + 50U] == 11U * 100U + 50U);
}

} // namespace

int main() {
    test_positive_and_negative_column_scroll();
    test_positive_and_negative_row_scroll();
    return 0;
}
