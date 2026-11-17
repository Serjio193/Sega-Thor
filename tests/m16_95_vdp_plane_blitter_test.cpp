#include "game/world/vdp_plane_blitter.hpp"

#include <array>
#include <cassert>
#include <vector>

namespace {

using namespace oasis::game::world;

void test_blit_dimension_selection() {
    // 32x32 and 64x32 rooms select full-plane (64, 32)
    const auto d32 = select_blit_dimensions(32U, 32U, false);
    assert(d32.cols == 64U && d32.rows == 32U);

    const auto d64 = select_blit_dimensions(64U, 32U, false);
    assert(d64.cols == 64U && d64.rows == 32U);

    // Larger scrolling room selects viewport (41, 29) unless force_full_bit1 is set
    const auto d96 = select_blit_dimensions(96U, 64U, false);
    assert(d96.cols == 41U && d96.rows == 29U);

    const auto d96_forced = select_blit_dimensions(96U, 64U, true);
    assert(d96_forced.cols == 64U && d96_forced.rows == 32U);
}

void test_dual_modulo_wrap_blit() {
    // 96x64 scrolling room with unique tile IDs (y * 96 + x)
    constexpr std::uint16_t W = 96U;
    constexpr std::uint16_t H = 64U;
    std::vector<std::uint16_t> src(static_cast<std::size_t>(W) * H);
    for (std::uint16_t y = 0U; y < H; ++y) {
        for (std::uint16_t x = 0U; x < W; ++x) {
            src[static_cast<std::size_t>(y) * W + x] = static_cast<std::uint16_t>(y * W + x);
        }
    }

    std::array<std::uint16_t, kVdpNametableWords> vdp{};
    vdp.fill(0xFFFFU);

    // Start at (50, 20) with viewport (41, 29) -> wraps X across 64 in VDP and Y across 32 in VDP
    const auto dims = select_blit_dimensions(W, H, false);
    assert(blit_wrapped_plane_window(src, W, H, 50U, 20U, dims, vdp));

    // Check first cell: src(50, 20) -> dst(50, 20)
    assert(vdp[20U * 64U + 50U] == 20U * 96U + 50U);

    // Check X wrap in VDP: c=14 -> src_x=64, dst_x=(50+14)%64=0, src_y=20, dst_y=20
    assert(vdp[20U * 64U + 0U] == 20U * 96U + 64U);

    // Check Y wrap in VDP: r=12 -> src_y=32, dst_y=(20+12)%32=0, c=0 -> src_x=50, dst_x=50
    assert(vdp[0U * 64U + 50U] == 32U * 96U + 50U);

    // Check both X and Y wrap: r=28, c=40 -> src(90, 48), dst((50+40)%64=26, (20+28)%32=16)
    assert(vdp[16U * 64U + 26U] == 48U * 96U + 90U);

    // Unwritten cell outside 41x29 wrapped window remains 0xFFFF (e.g., dst(49, 19))
    assert(vdp[19U * 64U + 49U] == 0xFFFFU);
}

} // namespace

int main() {
    test_blit_dimension_selection();
    test_dual_modulo_wrap_blit();
    return 0;
}
