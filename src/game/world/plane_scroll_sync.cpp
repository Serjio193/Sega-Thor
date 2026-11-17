#include "game/world/plane_scroll_sync.hpp"

namespace oasis::game::world {

namespace {

[[nodiscard]] std::int32_t wrap_non_negative(
    std::int32_t value,
    std::int32_t modulus) noexcept {
    if (modulus <= 0) {
        return value;
    }
    std::int32_t r = value % modulus;
    if (r < 0) {
        r += modulus;
    }
    return r;
}

} // namespace

void update_plane_scroll_deltas(
    PlaneScrollSyncState& state,
    std::int32_t target_x,
    std::int32_t target_y) noexcept {
    const std::int32_t dx = target_x - state.cam_x;
    const std::int32_t dy = target_y - state.cam_y;

    if ((state.flags_27 & 0x02U) == 0U) {
        if (dx != 0) {
            const std::int32_t tile_dx = (target_x >> 3) - (state.cam_x >> 3);
            if (tile_dx != 0) {
                state.pending_dx_tiles =
                    static_cast<std::int16_t>(state.pending_dx_tiles + tile_dx);
                state.dirty_flags |= kDirtyBitColStream;
            }
        }
        if (dy != 0) {
            const std::int32_t tile_dy = (target_y >> 3) - (state.cam_y >> 3);
            if (tile_dy != 0) {
                state.pending_dy_tiles =
                    static_cast<std::int16_t>(state.pending_dy_tiles + tile_dy);
                state.dirty_flags |= kDirtyBitRowStream;
            }
        }
    }

    if (dx != 0) {
        state.vdp_hscroll = static_cast<std::uint16_t>(
            (static_cast<std::int32_t>(state.vdp_hscroll) - dx) & kHScrollMask);
        state.dirty_flags |= kDirtyBitHScroll;
    }
    if (dy != 0) {
        state.vdp_vscroll = static_cast<std::uint16_t>(
            (static_cast<std::int32_t>(state.vdp_vscroll) + dy) & kVScrollMask);
        state.dirty_flags |= kDirtyBitVScroll;
    }

    state.cam_x = target_x;
    state.cam_y = target_y;

    const std::int32_t map_px_w2 = static_cast<std::int32_t>(state.map_width_tiles) * 16;
    if (map_px_w2 > 0) {
        if (state.cam_x > kOverflowWrapMax) {
            state.cam_x -= map_px_w2;
        } else if (state.cam_x < kOverflowWrapMin) {
            state.cam_x += map_px_w2;
        }
    }

    const std::int32_t map_px_h2 = static_cast<std::int32_t>(state.map_height_tiles) * 16;
    if (map_px_h2 > 0) {
        if (state.cam_y > kOverflowWrapMax) {
            state.cam_y -= map_px_h2;
        } else if (state.cam_y < kOverflowWrapMin) {
            state.cam_y += map_px_h2;
        }
    }
}

void wrap_camera_to_map_bounds(PlaneScrollSyncState& state) noexcept {
    state.cam_x = wrap_non_negative(
        state.cam_x,
        static_cast<std::int32_t>(state.map_width_tiles) * 8);
    state.cam_y = wrap_non_negative(
        state.cam_y,
        static_cast<std::int32_t>(state.map_height_tiles) * 8);
}

void flush_hscroll_to_vdp_buffer(
    std::uint16_t vdp_hscroll,
    bool is_plane_b,
    bool per_line_mode,
    std::span<std::uint16_t> hscroll_line_buf,
    std::array<std::uint16_t, 2>& hscroll_full_buf) noexcept {
    const std::size_t lane = is_plane_b ? 1U : 0U;
    if (per_line_mode) {
        for (std::size_t i = lane; i < hscroll_line_buf.size(); i += 2U) {
            hscroll_line_buf[i] = vdp_hscroll;
        }
    } else {
        hscroll_full_buf[lane] = vdp_hscroll;
    }
}

} // namespace oasis::game::world
