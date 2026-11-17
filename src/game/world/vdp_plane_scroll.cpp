#include "game/world/vdp_plane_scroll.hpp"

namespace oasis::game::world {

namespace {

[[nodiscard]] bool validate_scroll_inputs(
    const VdpPlaneScrollState& state,
    std::span<const std::uint16_t> src_tilemap) noexcept {
    if (state.map_width_tiles == 0U || state.map_height_tiles == 0U) {
        return false;
    }
    const std::size_t required =
        static_cast<std::size_t>(state.map_width_tiles) *
        static_cast<std::size_t>(state.map_height_tiles);
    return src_tilemap.size() >= required;
}

[[nodiscard]] std::uint16_t wrap_signed_index(
    std::int32_t value,
    std::uint16_t modulus) noexcept {
    const std::int32_t m = static_cast<std::int32_t>(modulus);
    std::int32_t r = value % m;
    if (r < 0) {
        r += m;
    }
    return static_cast<std::uint16_t>(r);
}

} // namespace

bool stream_plane_column(
    VdpPlaneScrollState& state,
    std::span<const std::uint16_t> src_tilemap,
    std::span<std::uint16_t, kVdpScrollNametableWords> vdp_nametable) noexcept {
    if (!validate_scroll_inputs(state, src_tilemap)) {
        return false;
    }

    std::uint16_t col_byte = static_cast<std::uint16_t>(state.vram_offset & kVdpColByteMask);
    const std::uint16_t row_byte = static_cast<std::uint16_t>(state.vram_offset & kVdpRowByteMask);

    std::uint16_t read_col = 0U;
    std::uint16_t write_col_byte = 0U;
    if (state.pending_dx_tiles >= 0) {
        --state.pending_dx_tiles;
        state.src_col = static_cast<std::int16_t>(
            wrap_signed_index(static_cast<std::int32_t>(state.src_col) + 1, state.map_width_tiles));
        read_col = wrap_signed_index(
            static_cast<std::int32_t>(state.src_col) + kScrollForwardAheadCols,
            state.map_width_tiles);
        col_byte = static_cast<std::uint16_t>((col_byte + 2U) & kVdpColByteMask);
        write_col_byte = static_cast<std::uint16_t>(
            (col_byte + kScrollForwardAheadColBytes) & kVdpColByteMask);
    } else {
        ++state.pending_dx_tiles;
        state.src_col = static_cast<std::int16_t>(
            wrap_signed_index(static_cast<std::int32_t>(state.src_col) - 1, state.map_width_tiles));
        read_col = static_cast<std::uint16_t>(state.src_col);
        col_byte = static_cast<std::uint16_t>((col_byte - 2U) & kVdpColByteMask);
        write_col_byte = col_byte;
    }
    state.vram_offset = static_cast<std::uint16_t>(row_byte | col_byte);

    std::uint16_t cur_src_row = wrap_signed_index(state.src_row, state.map_height_tiles);
    std::uint16_t cur_vdp_row_byte = row_byte;
    for (std::uint16_t i = 0U; i < kScrollTilesPerColumn; ++i) {
        const std::size_t vdp_word = static_cast<std::size_t>(cur_vdp_row_byte | write_col_byte) >> 1U;
        const std::size_t src_idx =
            static_cast<std::size_t>(cur_src_row) * state.map_width_tiles + read_col;
        vdp_nametable[vdp_word] = src_tilemap[src_idx];

        ++cur_src_row;
        if (cur_src_row >= state.map_height_tiles &&
            state.map_height_tiles < kScrollWrapSentinelHeight) {
            cur_src_row = 0U;
        } else if (cur_src_row >= state.map_height_tiles) {
            cur_src_row = static_cast<std::uint16_t>(cur_src_row % state.map_height_tiles);
        }
        cur_vdp_row_byte = static_cast<std::uint16_t>(
            (cur_vdp_row_byte + kVdpScrollRowStrideBytes) & kVdpRowByteMask);
    }

    return true;
}

bool stream_plane_row(
    VdpPlaneScrollState& state,
    std::span<const std::uint16_t> src_tilemap,
    std::span<std::uint16_t, kVdpScrollNametableWords> vdp_nametable) noexcept {
    if (!validate_scroll_inputs(state, src_tilemap)) {
        return false;
    }

    const std::uint16_t col_byte = static_cast<std::uint16_t>(state.vram_offset & kVdpColByteMask);
    std::uint16_t row_byte = static_cast<std::uint16_t>(state.vram_offset & kVdpRowByteMask);

    std::uint16_t read_row = 0U;
    std::uint16_t write_row_byte = 0U;
    if (state.pending_dy_tiles >= 0) {
        --state.pending_dy_tiles;
        state.src_row = static_cast<std::int16_t>(
            wrap_signed_index(static_cast<std::int32_t>(state.src_row) + 1, state.map_height_tiles));
        read_row = wrap_signed_index(
            static_cast<std::int32_t>(state.src_row) + kScrollForwardAheadRows,
            state.map_height_tiles);
        row_byte = static_cast<std::uint16_t>(
            (row_byte + kVdpScrollRowStrideBytes) & kVdpRowByteMask);
        write_row_byte = static_cast<std::uint16_t>(
            (row_byte + kScrollForwardAheadRowBytes) & kVdpRowByteMask);
    } else {
        ++state.pending_dy_tiles;
        state.src_row = static_cast<std::int16_t>(
            wrap_signed_index(static_cast<std::int32_t>(state.src_row) - 1, state.map_height_tiles));
        read_row = static_cast<std::uint16_t>(state.src_row);
        row_byte = static_cast<std::uint16_t>(
            (row_byte - kVdpScrollRowStrideBytes) & kVdpRowByteMask);
        write_row_byte = row_byte;
    }
    state.vram_offset = static_cast<std::uint16_t>(row_byte | col_byte);

    std::uint16_t cur_src_col = wrap_signed_index(state.src_col, state.map_width_tiles);
    std::uint16_t cur_vdp_col_byte = col_byte;
    const std::size_t src_row_base = static_cast<std::size_t>(read_row) * state.map_width_tiles;
    for (std::uint16_t i = 0U; i < kScrollTilesPerRow; ++i) {
        const std::size_t vdp_word = static_cast<std::size_t>(write_row_byte | cur_vdp_col_byte) >> 1U;
        vdp_nametable[vdp_word] = src_tilemap[src_row_base + cur_src_col];

        cur_src_col = static_cast<std::uint16_t>((cur_src_col + 1U) % state.map_width_tiles);
        cur_vdp_col_byte = static_cast<std::uint16_t>((cur_vdp_col_byte + 2U) & kVdpColByteMask);
    }

    return true;
}

} // namespace oasis::game::world
