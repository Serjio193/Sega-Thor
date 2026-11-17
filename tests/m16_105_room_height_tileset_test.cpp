#include "game/world/room_height_tileset.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::world;

void test_height_band_tables() {
    // Bands: [2, 4, 2, -2] -> steps [16, 32, 16, -16]
    // height_lut: [0, 8, 16, 32, 48, 56, 64, 0xF000]
    const auto tbl = build_height_band_tables({2, 4, 2, -2});
    assert(tbl.band_steps_4w[0] == 16);
    assert(tbl.band_steps_4w[1] == 32);
    assert(tbl.band_steps_4w[2] == 16);
    assert(tbl.band_steps_4w[3] == -16);

    assert(tbl.height_lut_8w[0] == 0);
    assert(tbl.height_lut_8w[1] == 8);
    assert(tbl.height_lut_8w[2] == 16);
    assert(tbl.height_lut_8w[3] == 32);
    assert(tbl.height_lut_8w[4] == 48);
    assert(tbl.height_lut_8w[5] == 56);
    assert(tbl.height_lut_8w[6] == 64);
    assert(static_cast<std::uint16_t>(tbl.height_lut_8w[7]) == 0xF000U);
}

void test_quantize_camera_pos() {
    const auto q1 = quantize_camera_pos(103, 215);
    assert(q1.x == 96 && q1.y == 208);

    const auto q2 = quantize_camera_pos(-12, -1);
    assert(q2.x == 0 && q2.y == 0);
}

void test_vscroll_flush() {
    std::array<std::uint16_t, 40> col_buf{};
    std::array<std::uint16_t, 2> full_buf{};

    flush_vscroll_to_vdp_buffer(0x008BU, false, false, col_buf, full_buf);
    flush_vscroll_to_vdp_buffer(0x0012U, true, false, col_buf, full_buf);
    assert(full_buf[0] == 0x008BU && full_buf[1] == 0x0012U);
    assert(col_buf[0] == 0U && col_buf[1] == 0U);

    flush_vscroll_to_vdp_buffer(0x0040U, false, true, col_buf, full_buf);
    flush_vscroll_to_vdp_buffer(0x007FU, true, true, col_buf, full_buf);
    for (std::size_t i = 0U; i < 20U; ++i) {
        assert(col_buf[i * 2U] == 0x0040U);
        assert(col_buf[i * 2U + 1U] == 0x007FU);
    }
}

} // namespace

int main() {
    test_height_band_tables();
    test_quantize_camera_pos();
    test_vscroll_flush();
    return 0;
}
