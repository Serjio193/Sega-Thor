#include "game/world/vdp_plane_blitter.hpp"

namespace oasis::game::world {

BlitDimensions select_blit_dimensions(
    std::uint16_t tiles_w,
    std::uint16_t tiles_h,
    bool force_full_bit1) noexcept {
    if (force_full_bit1 || (tiles_h == 32U && tiles_w <= 64U)) {
        return {kFullPlaneBlitCols, kFullPlaneBlitRows};
    }
    return {kViewportBlitCols, kViewportBlitRows};
}

bool blit_wrapped_plane_window(
    std::span<const std::uint16_t> src_tilemap,
    std::uint16_t tiles_w,
    std::uint16_t tiles_h,
    std::uint16_t start_col,
    std::uint16_t start_row,
    BlitDimensions dims,
    std::span<std::uint16_t, kVdpNametableWords> dst_nametable) noexcept {
    if (tiles_w == 0U || tiles_h == 0U) {
        return false;
    }
    const std::size_t required =
        static_cast<std::size_t>(tiles_w) * static_cast<std::size_t>(tiles_h);
    if (src_tilemap.size() < required) {
        return false;
    }

    for (std::uint16_t r = 0U; r < dims.rows; ++r) {
        const std::uint16_t src_y = static_cast<std::uint16_t>((start_row + r) % tiles_h);
        const std::uint16_t dst_y =
            static_cast<std::uint16_t>((start_row + r) % kVdpNametableRows);
        const std::size_t src_row_off = static_cast<std::size_t>(src_y) * tiles_w;
        const std::size_t dst_row_off = static_cast<std::size_t>(dst_y) * kVdpNametableCols;

        for (std::uint16_t c = 0U; c < dims.cols; ++c) {
            const std::uint16_t src_x = static_cast<std::uint16_t>((start_col + c) % tiles_w);
            const std::uint16_t dst_x =
                static_cast<std::uint16_t>((start_col + c) % kVdpNametableCols);
            dst_nametable[dst_row_off + dst_x] = src_tilemap[src_row_off + src_x];
        }
    }

    return true;
}

} // namespace oasis::game::world
