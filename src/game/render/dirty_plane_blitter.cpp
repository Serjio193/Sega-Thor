#include "game/render/dirty_plane_blitter.hpp"

namespace thor::render {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] constexpr std::uint32_t normalize_ram_address(std::uint32_t addr) noexcept {
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        return addr - 0x00FF0000U;
    }
    return addr;
}

[[nodiscard]] constexpr std::uint16_t decode_vram_addr_from_cmd(std::uint32_t vdp_cmd) noexcept {
    const auto cmd_hi = static_cast<std::uint16_t>((vdp_cmd >> 16U) & 0xFFFFU);
    const auto cmd_lo = static_cast<std::uint16_t>(vdp_cmd & 0xFFFFU);
    return static_cast<std::uint16_t>(((cmd_lo & 0x0003U) << 14U) | (cmd_hi & 0x3FFFU));
}

void write_vram_word(
    oasis::genesis::VdpState& vdp,
    std::uint16_t vram_addr,
    std::uint16_t word) noexcept
{
    const std::size_t addr = static_cast<std::size_t>(vram_addr);
    vdp.vram[addr] = static_cast<std::uint8_t>((word >> 8U) & 0xFFU);
    vdp.vram[(addr + 1U) & 0xFFFFU] = static_cast<std::uint8_t>(word & 0xFFU);
}

[[nodiscard]] bool clip_axis_inclusive(
    std::int16_t target_coord,
    std::uint16_t origin_coord,
    std::int16_t max_rel_inclusive,
    std::int16_t& inout_extent_minus_1,
    std::int16_t& out_rel_coord,
    std::uint16_t& out_skipped_cells) noexcept
{
    // Matches M68K `sub.w origin, d1/d2` using 16-bit wraparound difference.
    auto rel = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(target_coord) - origin_coord);
    out_skipped_cells = 0U;

    if (rel < 0) {
        inout_extent_minus_1 = static_cast<std::int16_t>(inout_extent_minus_1 + rel);
        if (inout_extent_minus_1 < 0) {
            return false; // Completely before viewport start (bmi.w $dd12)
        }
        out_skipped_cells = static_cast<std::uint16_t>(-static_cast<std::int32_t>(rel));
        rel = 0;
    }

    const auto rem_minus_1 = static_cast<std::int16_t>(max_rel_inclusive - rel);
    if (rem_minus_1 < 0) {
        return false; // Completely past viewport end (bmi.w $dd12)
    }

    // Matches `cmp.w d0, d3/d4; bls.w` unsigned <= clamp.
    if (static_cast<std::uint16_t>(inout_extent_minus_1) >
        static_cast<std::uint16_t>(rem_minus_1))
    {
        inout_extent_minus_1 = rem_minus_1;
    }
    out_rel_coord = rel;
    return true;
}

} // namespace

DirtyPlaneTransferPlan plan_dirty_plane_rect(
    const VisiblePlaneState& state,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1) noexcept
{
    DirtyPlaneTransferPlan plan{};
    if (width_minus_1 < 0 || height_minus_1 < 0) {
        return plan;
    }

    // 0x00DC26..0x00DC2C: only bit 1 of +$27(a3) is tested (bit 0 is ignored).
    if ((state.control_flags & kDirtyPlaneUnclippedModeBit) != 0U) {
        plan.valid = true;
        plan.unclipped_mode = true;
        plan.cols = static_cast<std::uint16_t>(width_minus_1 + 1);
        plan.rows = static_cast<std::uint16_t>(height_minus_1 + 1);
        // 0x00DD14..0x00DD26: wrap directly into 64x32 nametable without +$0C(a3).
        plan.vdp_start_col = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(target_x) - state.start_col) & kDirtyVdpColMask);
        plan.vdp_start_row = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(target_y) - state.start_row) & kDirtyVdpRowMask);
        return plan;
    }

    // 0x00DC30..0x00DC56: horizontal clip against [0, 41] (inclusive dbra count).
    std::int16_t w_m1 = width_minus_1;
    std::int16_t rel_x = 0;
    std::uint16_t skip_cols = 0U;
    if (!clip_axis_inclusive(
            target_x, state.start_col, kDirtyClipMaxRelCol, w_m1, rel_x, skip_cols))
    {
        plan.valid = true;
        plan.culled = true;
        return plan;
    }

    // 0x00DC58..0x00DC82: vertical clip against [0, 29] (inclusive dbra count).
    std::int16_t h_m1 = height_minus_1;
    std::int16_t rel_y = 0;
    std::uint16_t skip_rows = 0U;
    if (!clip_axis_inclusive(
            target_y, state.start_row, kDirtyClipMaxRelRow, h_m1, rel_y, skip_rows))
    {
        plan.valid = true;
        plan.culled = true;
        return plan;
    }

    // 0x00DC68..0x00DC6E: skip (-rel_y) source rows via +$24(a3) byte_stride_shift.
    if (skip_rows > 0U && state.byte_stride_shift >= 16U) {
        return plan;
    }

    const std::size_t skip_x_bytes = static_cast<std::size_t>(skip_cols) * 2U;
    const std::size_t skip_y_bytes = (skip_rows > 0U)
        ? (static_cast<std::size_t>(skip_rows) << state.byte_stride_shift)
        : 0U;

    // 0x00DC84..0x00DC98: combine +$0C(a3) ring offset with clipped screen coords.
    const auto base_col = static_cast<std::uint16_t>(state.vram_ring_offset >> 1U);
    const auto base_row = static_cast<std::uint16_t>(state.vram_ring_offset >> 7U);
    plan.valid = true;
    plan.culled = false;
    plan.unclipped_mode = false;
    plan.clipped_rel_x = rel_x;
    plan.clipped_rel_y = rel_y;
    plan.cols = static_cast<std::uint16_t>(w_m1 + 1);
    plan.rows = static_cast<std::uint16_t>(h_m1 + 1);
    plan.vdp_start_col = static_cast<std::uint16_t>(
        (base_col + static_cast<std::uint16_t>(rel_x)) & kDirtyVdpColMask);
    plan.vdp_start_row = static_cast<std::uint16_t>(
        (base_row + static_cast<std::uint16_t>(rel_y)) & kDirtyVdpRowMask);
    plan.source_byte_offset = skip_x_bytes + skip_y_bytes;
    return plan;
}

DirtyPlaneBlitResult vdp_sync_dirty_plane_rect(
    const VisiblePlaneState& state,
    std::span<const std::uint8_t> dirty_source_bytes,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1,
    oasis::genesis::VdpState& vdp) noexcept
{
    DirtyPlaneBlitResult res{};
    const DirtyPlaneTransferPlan plan =
        plan_dirty_plane_rect(state, target_x, target_y, width_minus_1, height_minus_1);
    res.plan = plan;
    res.unclipped_mode = plan.unclipped_mode;
    if (!plan.valid) {
        return res;
    }
    if (plan.culled) {
        res.ok = true;
        res.culled = true;
        return res;
    }
    if (state.width_tiles == 0U) {
        return res;
    }

    // 0x00DCB8..0x00DCBC / 0x00DD46..0x00DD4A: row stride d7 = width_tiles * 2.
    const std::uint64_t row_stride_bytes = static_cast<std::uint64_t>(state.width_tiles) * 2ULL;
    const std::uint64_t required_bytes =
        static_cast<std::uint64_t>(plan.source_byte_offset) +
        (static_cast<std::uint64_t>(plan.rows - 1U) * row_stride_bytes) +
        (static_cast<std::uint64_t>(plan.cols) * 2ULL);
    if (required_bytes > dirty_source_bytes.size()) {
        return res;
    }

    // 0x00DCC2..0x00DD0C / 0x00DD50..0x00DD9A: identical 122-byte toroidal row/col streamer.
    res.vdp_control_writes.reserve(static_cast<std::size_t>(plan.rows) * 2U);
    std::size_t row_src_byte = plan.source_byte_offset;
    std::uint16_t vdp_row = plan.vdp_start_row;

    for (std::uint16_t r = 0U; r < plan.rows; ++r) {
        std::uint16_t vdp_col = plan.vdp_start_col;
        std::uint32_t vdp_cmd = compute_dirty_vram_write_cmd(state.vram_base, vdp_col, vdp_row);
        res.vdp_control_writes.push_back(vdp_cmd);

        std::uint16_t vram_addr = decode_vram_addr_from_cmd(vdp_cmd);
        std::size_t src_byte = row_src_byte;

        for (std::uint16_t c = 0U; c < plan.cols; ++c) {
            const std::uint16_t tile_word = read_be16(dirty_source_bytes, src_byte);
            write_vram_word(vdp, vram_addr, tile_word);
            ++res.tiles_written;
            src_byte += 2U;

            // Advance VDP col; wrap at 64 and re-issue VDP control write (even on final tile).
            ++vdp_col;
            if (vdp_col >= kVdpNametableCols) {
                vdp_col = static_cast<std::uint16_t>(vdp_col - kVdpNametableCols);
                vdp_cmd = compute_dirty_vram_write_cmd(state.vram_base, vdp_col, vdp_row);
                res.vdp_control_writes.push_back(vdp_cmd);
                vram_addr = decode_vram_addr_from_cmd(vdp_cmd);
            } else {
                vram_addr = static_cast<std::uint16_t>(vram_addr + 2U);
            }
        }

        // Advance source row by 2 * width_tiles and VDP row mod 32 (no extra VDP cmd on wrap).
        row_src_byte += static_cast<std::size_t>(row_stride_bytes);
        ++vdp_row;
        if (vdp_row >= kVdpNametableRows) {
            vdp_row = static_cast<std::uint16_t>(vdp_row - kVdpNametableRows);
        }
    }

    res.ok = true;
    return res;
}

DirtyPlaneBlitResult vdp_sync_dirty_plane_rect(
    const VisiblePlaneState& state,
    const TilemapRectSyncRequest& request,
    oasis::genesis::VdpState& vdp) noexcept
{
    return vdp_sync_dirty_plane_rect(
        state,
        request.dst_tilemap_tail,
        request.dst_x,
        request.dst_y,
        request.width_minus_1,
        request.height_minus_1,
        vdp);
}

DirtyPlaneBlitResult vdp_sync_dirty_plane_rect_from_tilemap(
    const VisiblePlaneState& state,
    std::span<const std::uint8_t> full_tilemap_bytes,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1,
    oasis::genesis::VdpState& vdp) noexcept
{
    if (target_x < 0 || target_y < 0 || state.byte_stride_shift >= 16U) {
        return DirtyPlaneBlitResult{};
    }
    const std::uint64_t start_off =
        (static_cast<std::uint64_t>(target_y) << state.byte_stride_shift) +
        (static_cast<std::uint64_t>(target_x) * 2ULL);
    if (start_off > full_tilemap_bytes.size()) {
        return DirtyPlaneBlitResult{};
    }
    return vdp_sync_dirty_plane_rect(
        state,
        full_tilemap_bytes.subspan(static_cast<std::size_t>(start_off)),
        target_x,
        target_y,
        width_minus_1,
        height_minus_1,
        vdp);
}

DirtyPlaneBlitResult vdp_sync_dirty_plane_rect_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::uint32_t source_ram_addr,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1,
    oasis::genesis::VdpState& vdp) noexcept
{
    const std::uint32_t plane_off = normalize_ram_address(plane_ram_offset);
    const std::uint32_t src_off = normalize_ram_address(source_ram_addr);
    if (plane_off > ram.size() - 0x28U || source_ram_addr == 0U || src_off >= ram.size()) {
        return DirtyPlaneBlitResult{};
    }

    const VisiblePlaneState state = load_visible_plane_state_from_ram(ram, plane_off);
    return vdp_sync_dirty_plane_rect(
        state,
        std::span<const std::uint8_t>(ram).subspan(src_off),
        target_x,
        target_y,
        width_minus_1,
        height_minus_1,
        vdp);
}

DirtyPlaneBlitResult vdp_sync_dirty_plane_rect_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    const TilemapRectSyncRequest& request,
    oasis::genesis::VdpState& vdp) noexcept
{
    const std::uint32_t plane_off = normalize_ram_address(request.plane_ram_offset);
    if (plane_off > ram.size() - 0x28U) {
        return DirtyPlaneBlitResult{};
    }

    const VisiblePlaneState state = load_visible_plane_state_from_ram(ram, plane_off);
    return vdp_sync_dirty_plane_rect(state, request, vdp);
}

} // namespace thor::render
