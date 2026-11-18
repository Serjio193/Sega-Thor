#include "game/render/visible_plane_streamer.hpp"

namespace thor::render {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] constexpr std::uint32_t normalize_ram_address(std::uint32_t addr) noexcept {
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        return addr - 0x00FF0000U;
    }
    return addr;
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

void clear_stream_tracking_fields(VisiblePlaneState& state) noexcept {
    // 0x00D7C0..0x00D7DE: zero +$0E, +$10, +$0C, +$08, +$0A, +$1C, +$1E
    state.vdp_hscroll = 0U;
    state.vdp_vscroll = 0U;
    state.vram_ring_offset = 0U;
    state.start_col = 0U;
    state.start_row = 0U;
    state.tile_col_step = 0;
    state.tile_row_step = 0;
}

} // namespace

VisiblePlaneStreamResult vdp_fill_plane_blank(
    std::uint16_t vram_base,
    oasis::genesis::VdpState& vdp) noexcept
{
    VisiblePlaneStreamResult res{};
    const std::uint32_t base_cmd = compute_vram_write_cmd(vram_base);
    res.vdp_control_writes.reserve(kVdpBlankFillRows);

    // 0x00D97C..0x00D99E: 29 rows (d2=0x1C) x 41 cols (d1=0x28) filled with 0x0780
    for (std::uint16_t row = 0U; row < kVdpBlankFillRows; ++row) {
        res.vdp_control_writes.push_back(
            base_cmd + (static_cast<std::uint32_t>(row) * kVdpCmdRowStep));
        const auto row_vram = static_cast<std::uint16_t>(
            vram_base + (static_cast<std::uint16_t>(row) * kVdpNametableRowByteStride));
        for (std::uint16_t col = 0U; col < kVdpBlankFillCols; ++col) {
            const auto dst_vram = static_cast<std::uint16_t>(row_vram + (col * 2U));
            write_vram_word(vdp, dst_vram, kVdpBlankTileWord);
        }
    }

    res.ok = true;
    res.filled_blank = true;
    res.dims = {kVdpBlankFillCols, kVdpBlankFillRows};
    res.tiles_written = res.dims.tile_count();
    return res;
}

VisiblePlaneStreamResult vdp_upload_visible_tilemap_plane(
    VisiblePlaneState& state,
    std::span<const std::uint8_t> tilemap_bytes,
    oasis::genesis::VdpState& vdp) noexcept
{
    clear_stream_tracking_fields(state);

    // 0x00D7DE..0x00D7E4: btst.b #0, $27(a3); beq.w $d95e
    if ((state.control_flags & kPlaneActiveBit) == 0U) {
        VisiblePlaneStreamResult res = vdp_fill_plane_blank(state.vram_base, vdp);
        res.updated_control_flags = state.control_flags;
        return res;
    }

    VisiblePlaneStreamResult res{};
    if (state.width_tiles == 0U || state.height_tiles == 0U || state.byte_stride_shift >= 16U) {
        res.updated_control_flags = state.control_flags;
        return res;
    }

    // 0x00D7E8..0x00D81C: compute wrapped tile origins (+$08, +$0A)
    state.start_col = wrap_tile_origin(state.cam_x, state.width_tiles);
    state.start_row = wrap_tile_origin(state.cam_y, state.height_tiles);

    // 0x00D820..0x00D84A: choose 64x32 (latching bit 1 if height==32 && width<=64) or 41x29
    const VisibleStreamDimensions dims = select_visible_stream_dimensions(
        state.width_tiles,
        state.height_tiles,
        state.control_flags);

    res.dims = dims;
    res.start_col = state.start_col;
    res.start_row = state.start_row;
    res.updated_control_flags = state.control_flags;

    const std::uint32_t base_cmd = compute_vram_write_cmd(state.vram_base);
    const std::size_t row_wrap_sub_bytes =
        static_cast<std::size_t>(state.height_tiles) << state.byte_stride_shift;
    const std::size_t width_bytes = static_cast<std::size_t>(state.width_tiles) * 2U;

    std::size_t row_byte_base =
        static_cast<std::size_t>(state.start_row) << state.byte_stride_shift;
    std::uint16_t cur_row = state.start_row;

    // 0x00D85A..0x00D890: ring_offset (d1) starts at 0 -> vdp_row = 0, vdp_col = 0
    std::uint16_t ring_offset = state.vram_ring_offset;
    auto vdp_row = static_cast<std::uint16_t>((ring_offset & 0x0F80U) >> 7U);
    const auto init_vdp_col = static_cast<std::uint16_t>((ring_offset & 0x007EU) >> 1U);

    res.vdp_control_writes.reserve(static_cast<std::size_t>(dims.rows) * 2U);

    for (std::uint16_t r = 0U; r < dims.rows; ++r) {
        // 0x00D8AE..0x00D8B8: write row VDP command
        res.vdp_control_writes.push_back(
            base_cmd + (static_cast<std::uint32_t>(ring_offset) << 16U));

        std::uint16_t row_ring = ring_offset;
        std::uint16_t vdp_col = init_vdp_col;
        std::uint16_t cur_col = state.start_col;
        std::size_t src_byte = row_byte_base + (static_cast<std::size_t>(cur_col) * 2U);

        for (std::uint16_t c = 0U; c < dims.cols; ++c) {
            if (src_byte > tilemap_bytes.size() || (tilemap_bytes.size() - src_byte) < 2U) {
                return res;
            }
            const std::uint16_t tile_word = read_be16(tilemap_bytes, src_byte);
            const auto vram_addr = static_cast<std::uint16_t>(state.vram_base + row_ring);
            write_vram_word(vdp, vram_addr, tile_word);
            ++res.tiles_written;

            // 0x00D8C6..0x00D8D2: advance source column with toroidal width wrap
            src_byte += 2U;
            ++cur_col;
            if (cur_col >= state.width_tiles) {
                cur_col = static_cast<std::uint16_t>(cur_col - state.width_tiles);
                src_byte -= width_bytes;
            }

            // 0x00D8D4..0x00D8F2: advance VDP ring column and wrap at 64 cells
            row_ring = static_cast<std::uint16_t>(row_ring + 2U);
            ++vdp_col;
            if (vdp_col >= kVdpNametableCols) {
                vdp_col = static_cast<std::uint16_t>(vdp_col - kVdpNametableCols);
                row_ring = static_cast<std::uint16_t>(row_ring - kVdpNametableRowByteStride);
                res.vdp_control_writes.push_back(
                    base_cmd + (static_cast<std::uint32_t>(row_ring) << 16U));
            }
        }

        // 0x00D8FC..0x00D91C: advance source row with toroidal height wrap
        row_byte_base += width_bytes;
        ++cur_row;
        if (cur_row >= state.height_tiles) {
            cur_row = static_cast<std::uint16_t>(cur_row - state.height_tiles);
            row_byte_base -= row_wrap_sub_bytes;
        }

        // 0x00D91E..0x00D93E: advance VDP ring row (+0x80) and wrap at 32 rows (-0x1000)
        ring_offset = static_cast<std::uint16_t>(ring_offset + kVdpNametableRowByteStride);
        ++vdp_row;
        if (vdp_row >= kVdpNametableRows) {
            vdp_row = static_cast<std::uint16_t>(vdp_row - kVdpNametableRows);
            ring_offset = static_cast<std::uint16_t>(ring_offset - kVdpNametableByteSize);
            res.vdp_control_writes.push_back(
                base_cmd + (static_cast<std::uint32_t>(ring_offset) << 16U));
        }
    }

    res.ok = true;
    return res;
}

VisiblePlaneState load_visible_plane_state_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept
{
    VisiblePlaneState state{};
    const std::uint32_t off = normalize_ram_address(plane_ram_offset);
    if (off > ram.size() - 0x28U) {
        return state;
    }

    state.cam_x = static_cast<std::int16_t>(read_be16(ram, off + 0x00U));
    state.cam_y = static_cast<std::int16_t>(read_be16(ram, off + 0x04U));
    state.start_col = read_be16(ram, off + 0x08U);
    state.start_row = read_be16(ram, off + 0x0AU);
    state.vram_ring_offset = read_be16(ram, off + 0x0CU);
    state.vdp_hscroll = read_be16(ram, off + 0x0EU);
    state.vdp_vscroll = read_be16(ram, off + 0x10U);
    state.tile_data_ptr = read_be32(ram, off + 0x16U);
    state.vram_base = read_be16(ram, off + 0x1AU);
    state.tile_col_step = static_cast<std::int16_t>(read_be16(ram, off + 0x1CU));
    state.tile_row_step = static_cast<std::int16_t>(read_be16(ram, off + 0x1EU));
    state.width_tiles = read_be16(ram, off + 0x20U);
    state.height_tiles = read_be16(ram, off + 0x22U);
    state.byte_stride_shift = read_be16(ram, off + 0x24U);
    state.control_flags = ram[off + 0x27U];
    return state;
}

void store_visible_plane_state_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    const VisiblePlaneState& state) noexcept
{
    const std::uint32_t off = normalize_ram_address(plane_ram_offset);
    if (off > ram.size() - 0x28U) {
        return;
    }

    write_be16(ram, off + 0x00U, static_cast<std::uint16_t>(state.cam_x));
    write_be16(ram, off + 0x04U, static_cast<std::uint16_t>(state.cam_y));
    write_be16(ram, off + 0x08U, state.start_col);
    write_be16(ram, off + 0x0AU, state.start_row);
    write_be16(ram, off + 0x0CU, state.vram_ring_offset);
    write_be16(ram, off + 0x0EU, state.vdp_hscroll);
    write_be16(ram, off + 0x10U, state.vdp_vscroll);
    write_be32(ram, off + 0x16U, state.tile_data_ptr);
    write_be16(ram, off + 0x1AU, state.vram_base);
    write_be16(ram, off + 0x1CU, static_cast<std::uint16_t>(state.tile_col_step));
    write_be16(ram, off + 0x1EU, static_cast<std::uint16_t>(state.tile_row_step));
    write_be16(ram, off + 0x20U, state.width_tiles);
    write_be16(ram, off + 0x22U, state.height_tiles);
    write_be16(ram, off + 0x24U, state.byte_stride_shift);
    ram[off + 0x27U] = state.control_flags;
}

VisiblePlaneStreamResult vdp_upload_visible_tilemap_plane_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    oasis::genesis::VdpState& vdp) noexcept
{
    const std::uint32_t off = normalize_ram_address(plane_ram_offset);
    if (off > ram.size() - 0x28U) {
        return VisiblePlaneStreamResult{};
    }

    VisiblePlaneState state = load_visible_plane_state_from_ram(ram, off);
    std::span<const std::uint8_t> tilemap_bytes{};
    if ((state.control_flags & kPlaneActiveBit) != 0U) {
        const std::uint32_t data_off = normalize_ram_address(state.tile_data_ptr);
        if (state.tile_data_ptr == 0U || data_off >= ram.size()) {
            clear_stream_tracking_fields(state);
            write_be16(ram, off + 0x08U, state.start_col);
            write_be16(ram, off + 0x0AU, state.start_row);
            write_be16(ram, off + 0x0CU, state.vram_ring_offset);
            write_be16(ram, off + 0x0EU, state.vdp_hscroll);
            write_be16(ram, off + 0x10U, state.vdp_vscroll);
            write_be16(ram, off + 0x1CU, static_cast<std::uint16_t>(state.tile_col_step));
            write_be16(ram, off + 0x1EU, static_cast<std::uint16_t>(state.tile_row_step));
            return VisiblePlaneStreamResult{};
        }
        tilemap_bytes = std::span<const std::uint8_t>(ram).subspan(data_off);
    }

    VisiblePlaneStreamResult res = vdp_upload_visible_tilemap_plane(state, tilemap_bytes, vdp);

    // Write back only the fields mutated by 0x00D7C0..0x00D94E
    write_be16(ram, off + 0x08U, state.start_col);
    write_be16(ram, off + 0x0AU, state.start_row);
    write_be16(ram, off + 0x0CU, state.vram_ring_offset);
    write_be16(ram, off + 0x0EU, state.vdp_hscroll);
    write_be16(ram, off + 0x10U, state.vdp_vscroll);
    write_be16(ram, off + 0x1CU, static_cast<std::uint16_t>(state.tile_col_step));
    write_be16(ram, off + 0x1EU, static_cast<std::uint16_t>(state.tile_row_step));
    ram[off + 0x27U] = state.control_flags;
    return res;
}

} // namespace thor::render
