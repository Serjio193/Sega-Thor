#include "game/world/plane_scroll_sync.hpp"

#include <array>
#include <cassert>
#include <vector>

namespace {

using namespace oasis::game::world;

void test_scroll_deltas_and_masks() {
    PlaneScrollSyncState st{};
    st.cam_x = 6;
    st.cam_y = 7;
    st.vdp_hscroll = 2;
    st.vdp_vscroll = 250;
    st.map_width_tiles = 64U;  // 512 px
    st.map_height_tiles = 32U; // 256 px

    // Crosses 8px boundary on both X (6 -> 10, +1 tile) and Y (7 -> 14, +1 tile)
    // vdp_hscroll = (2 - 4) & 0x1FF = 0x1FE; vdp_vscroll = (250 + 7) & 0xFF = 1
    update_plane_scroll_deltas(st, 10, 14);
    assert(st.cam_x == 10 && st.cam_y == 14);
    assert(st.pending_dx_tiles == 1 && st.pending_dy_tiles == 1);
    assert(st.vdp_hscroll == 0x01FEU);
    assert(st.vdp_vscroll == 1U);
    assert(st.dirty_flags == (kDirtyBitColStream | kDirtyBitRowStream | kDirtyBitHScroll | kDirtyBitVScroll));

    // Sub-tile move (10 -> 12): no tile crossing, only HScroll dirty
    st.dirty_flags = 0U;
    update_plane_scroll_deltas(st, 12, 14);
    assert(st.pending_dx_tiles == 1);
    assert(st.dirty_flags == kDirtyBitHScroll);
    assert(st.vdp_hscroll == 0x01FCU);

    // flags_27 bit 1 suppresses tile-crossing stream deltas
    st.flags_27 = 0x02U;
    st.dirty_flags = 0U;
    update_plane_scroll_deltas(st, 28, 30);
    assert(st.pending_dx_tiles == 1 && st.pending_dy_tiles == 1);
    assert(st.dirty_flags == (kDirtyBitHScroll | kDirtyBitVScroll));
}

void test_anti_overflow_and_map_bounds_wrap() {
    PlaneScrollSyncState st{};
    st.map_width_tiles = 64U;  // 512 px -> 2x = 1024
    st.map_height_tiles = 32U; // 256 px -> 2x = 512
    st.cam_x = 9990;
    st.cam_y = -9990;

    update_plane_scroll_deltas(st, 10005, -10005);
    assert(st.cam_x == 10005 - 1024);
    assert(st.cam_y == -10005 + 512);

    st.cam_x = -20;
    st.cam_y = 260;
    wrap_camera_to_map_bounds(st);
    assert(st.cam_x == 512 - 20);
    assert(st.cam_y == 4);
}

void test_hscroll_flush_plane_a_and_b() {
    std::vector<std::uint16_t> line_buf(8U, 0U);
    std::array<std::uint16_t, 2> full_buf{0U, 0U};

    // Full-screen Plane A (+0) and Plane B (+2)
    flush_hscroll_to_vdp_buffer(0x0123U, false, false, line_buf, full_buf);
    flush_hscroll_to_vdp_buffer(0x0045U, true, false, line_buf, full_buf);
    assert(full_buf[0] == 0x0123U && full_buf[1] == 0x0045U);
    assert(line_buf[0] == 0U && line_buf[1] == 0U);

    // Per-line Plane A & Plane B
    flush_hscroll_to_vdp_buffer(0x0111U, false, true, line_buf, full_buf);
    flush_hscroll_to_vdp_buffer(0x0222U, true, true, line_buf, full_buf);
    for (std::size_t i = 0U; i < 4U; ++i) {
        assert(line_buf[i * 2U] == 0x0111U);
        assert(line_buf[i * 2U + 1U] == 0x0222U);
    }
}

} // namespace

int main() {
    test_scroll_deltas_and_masks();
    test_anti_overflow_and_map_bounds_wrap();
    test_hscroll_flush_plane_a_and_b();
    return 0;
}
