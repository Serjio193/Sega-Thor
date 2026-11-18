#include "game/render/vdp_scroll_builder.hpp"

namespace thor::render {

namespace {

[[nodiscard]] inline std::uint16_t read_be16(
    std::span<const std::uint8_t, 0x10000> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8) | data[offset + 1U]);
}

inline void write_be16(
    std::span<std::uint8_t, 0x10000> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] constexpr std::uint32_t normalize_plane_ram_offset(
    std::uint32_t plane_ram_offset) noexcept
{
    if (plane_ram_offset >= 0x00FF0000U && plane_ram_offset <= 0x00FFFFFFU) {
        return plane_ram_offset - 0x00FF0000U;
    }
    return plane_ram_offset;
}

inline void fill_plane_table_entries(
    std::span<std::uint16_t> table,
    std::size_t lane_offset,
    std::size_t entry_count,
    std::size_t word_stride,
    std::uint16_t value) noexcept
{
    for (std::size_t i = 0; i < entry_count; ++i) {
        const std::size_t idx = lane_offset + (i * word_stride);
        if (idx < table.size()) {
            table[idx] = value;
        }
    }
}

template <typename UpdateFn>
bool run_plane_ram_scroll_update(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    UpdateFn&& update_fn) noexcept
{
    const std::uint32_t offset = normalize_plane_ram_offset(plane_ram_offset);
    if (offset + 0x28U > ram.size()) {
        return false;
    }

    TilemapCameraState state = load_state_from_ram(ram, offset);
    VdpScrollBuffers buffers = load_scroll_buffers_from_ram(ram);
    std::uint8_t scroll_mode_reg = ram[kScrollModeRegRamOffset];
    const bool is_plane_a = (offset == kPlaneARamOffset);

    const bool updated = update_fn(state, buffers, scroll_mode_reg, is_plane_a);
    if (updated) {
        ram[offset + 0x26U] = state.dirty_flags;
        ram[kScrollModeRegRamOffset] = scroll_mode_reg;
        store_scroll_buffers_to_ram(ram, buffers);
    }
    return updated;
}

} // namespace

bool vdp_update_vscroll_table(
    TilemapCameraState& state,
    VdpScrollBuffers& buffers,
    std::uint8_t& scroll_mode_reg,
    bool is_plane_a) noexcept
{
    // 0x00D344: bclr.b #3, $26(a0); beq.w $d396
    const bool was_dirty = (state.dirty_flags & kDirtyBitPlaneAVScroll) != 0U;
    state.dirty_flags = static_cast<std::uint8_t>(state.dirty_flags & ~kDirtyBitPlaneAVScroll);
    if (!was_dirty) {
        return false;
    }

    // 0x00D34E: bset.b #4, $ff16f0.l
    scroll_mode_reg = static_cast<std::uint8_t>(scroll_mode_reg | kScrollPendingVScrollBit);

    // 0x00D356: move.w $10(a0), d0; 0x00D35A: btst.b #2, $ff16f0.l
    const std::uint16_t vscroll_val = state.vdp_vscroll;
    if (decode_vscroll_mode(scroll_mode_reg) == VScrollMode::TwoCellColumn16px) {
        // 0x00D366..0x00D380: 20 iterations (dbra $13), stride 4 bytes (2 words)
        const std::size_t lane = is_plane_a ? 0U : 1U;
        fill_plane_table_entries(
            buffers.vscroll_table,
            lane,
            kVScrollColumnIterations,
            kVScrollColumnWordStride,
            vscroll_val);
    } else {
        // 0x00D384..0x00D394: single word at $FF1348 (Plane A) or $FF134A (Plane B)
        if (is_plane_a) {
            buffers.fullscreen_vscroll_plane_a = vscroll_val;
        } else {
            buffers.fullscreen_vscroll_plane_b = vscroll_val;
        }
    }
    return true;
}

bool vdp_update_hscroll_only(
    TilemapCameraState& state,
    VdpScrollBuffers& buffers,
    std::uint8_t& scroll_mode_reg,
    bool is_plane_a) noexcept
{
    // 0x00D2B8: bclr.b #2, $26(a0); beq.w $d342
    const bool was_dirty = (state.dirty_flags & kDirtyBitPlaneAHScroll) != 0U;
    state.dirty_flags = static_cast<std::uint8_t>(state.dirty_flags & ~kDirtyBitPlaneAHScroll);
    if (!was_dirty) {
        return false;
    }

    // 0x00D2C2: bset.b #3, $ff16f0.l
    scroll_mode_reg = static_cast<std::uint8_t>(scroll_mode_reg | kScrollPendingHScrollBit);

    // 0x00D2CA: move.w $e(a0), d0
    const std::uint16_t hscroll_val = state.vdp_hscroll;
    const std::size_t lane = is_plane_a ? 0U : 1U;

    switch (decode_hscroll_mode(scroll_mode_reg)) {
    case HScrollMode::FullScreen:
        // 0x00D330..0x00D340: single word at $FF1344 (Plane A) or $FF1346 (Plane B)
        if (is_plane_a) {
            buffers.fullscreen_hscroll_plane_a = hscroll_val;
        } else {
            buffers.fullscreen_hscroll_plane_b = hscroll_val;
        }
        break;
    case HScrollMode::TileStrip16px:
        // 0x00D310..0x00D32C: 29 iterations (dbra $1c), stride 32 bytes (16 words)
        fill_plane_table_entries(
            buffers.hscroll_table,
            lane,
            kHScrollStripIterations,
            kHScrollStripWordStride,
            hscroll_val);
        break;
    case HScrollMode::ScanlineParallax:
        // 0x00D2E6..0x00D30C: 58 outer iterations (dbra $39) * 4 writes = 232 entries, stride 4 bytes (2 words)
        fill_plane_table_entries(
            buffers.hscroll_table,
            lane,
            kHScrollScanlineEntryCount,
            kHScrollScanlineWordStride,
            hscroll_val);
        break;
    }
    return true;
}

bool vdp_update_hscroll_table(
    TilemapCameraState& state,
    VdpScrollBuffers& buffers,
    std::uint8_t& scroll_mode_reg,
    bool is_plane_a) noexcept
{
    // 0x00D2B4: bsr.w $d344
    const bool v_updated = vdp_update_vscroll_table(state, buffers, scroll_mode_reg, is_plane_a);
    const bool h_updated = vdp_update_hscroll_only(state, buffers, scroll_mode_reg, is_plane_a);
    return v_updated || h_updated;
}

std::uint8_t vdp_commit_scroll_dma_flags(
    std::uint8_t& scroll_mode_reg,
    std::uint8_t& vint_sync_flags) noexcept
{
    // 0x00D398..0x00D3B0:
    // move.b $ff16f0.l, d0; andi.b #$e7, $ff16f0.l; andi.b #$18, d0; or.b d0, $ff164e.l
    const std::uint8_t pending = static_cast<std::uint8_t>(scroll_mode_reg & kScrollPendingDmaMask);
    scroll_mode_reg = static_cast<std::uint8_t>(scroll_mode_reg & kScrollPendingDmaClearMask);
    vint_sync_flags = static_cast<std::uint8_t>(vint_sync_flags | pending);
    return pending;
}

void update_vdp_scroll_tables(
    VdpScrollBuffers& buffers,
    const VdpScrollConfig& config,
    bool is_plane_a,
    std::uint16_t hscroll_val,
    std::uint16_t vscroll_val,
    bool h_dirty,
    bool v_dirty,
    std::uint8_t& vint_sync_flags) noexcept
{
    TilemapCameraState state{};
    state.vdp_hscroll = hscroll_val;
    state.vdp_vscroll = vscroll_val;
    state.dirty_flags = static_cast<std::uint8_t>(
        (h_dirty ? kDirtyBitPlaneAHScroll : 0U) |
        (v_dirty ? kDirtyBitPlaneAVScroll : 0U));

    std::uint8_t scroll_mode_reg = encode_scroll_mode_bits(config.h_mode, config.v_mode);
    vdp_update_hscroll_table(state, buffers, scroll_mode_reg, is_plane_a);
    vdp_commit_scroll_dma_flags(scroll_mode_reg, vint_sync_flags);
}

VdpScrollBuffers load_scroll_buffers_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept
{
    VdpScrollBuffers buffers{};
    buffers.fullscreen_hscroll_plane_a = read_be16(ram, kFullscreenHScrollPlaneA);
    buffers.fullscreen_hscroll_plane_b = read_be16(ram, kFullscreenHScrollPlaneB);
    buffers.fullscreen_vscroll_plane_a = read_be16(ram, kFullscreenVScrollPlaneA);
    buffers.fullscreen_vscroll_plane_b = read_be16(ram, kFullscreenVScrollPlaneB);

    for (std::size_t i = 0; i < kHScrollTableWordCount; ++i) {
        buffers.hscroll_table[i] = read_be16(ram, kHScrollTableRamOffset + (i * 2U));
    }
    for (std::size_t i = 0; i < kVScrollTableWordCount; ++i) {
        buffers.vscroll_table[i] = read_be16(ram, kVScrollTableRamOffset + (i * 2U));
    }
    return buffers;
}

void store_scroll_buffers_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const VdpScrollBuffers& buffers) noexcept
{
    write_be16(ram, kFullscreenHScrollPlaneA, buffers.fullscreen_hscroll_plane_a);
    write_be16(ram, kFullscreenHScrollPlaneB, buffers.fullscreen_hscroll_plane_b);
    write_be16(ram, kFullscreenVScrollPlaneA, buffers.fullscreen_vscroll_plane_a);
    write_be16(ram, kFullscreenVScrollPlaneB, buffers.fullscreen_vscroll_plane_b);

    for (std::size_t i = 0; i < kHScrollTableWordCount; ++i) {
        write_be16(ram, kHScrollTableRamOffset + (i * 2U), buffers.hscroll_table[i]);
    }
    for (std::size_t i = 0; i < kVScrollTableWordCount; ++i) {
        write_be16(ram, kVScrollTableRamOffset + (i * 2U), buffers.vscroll_table[i]);
    }
}

bool vdp_update_vscroll_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept
{
    return run_plane_ram_scroll_update(
        ram,
        plane_ram_offset,
        [](TilemapCameraState& state,
           VdpScrollBuffers& buffers,
           std::uint8_t& scroll_mode_reg,
           bool is_plane_a) noexcept {
            return vdp_update_vscroll_table(state, buffers, scroll_mode_reg, is_plane_a);
        });
}

bool vdp_update_hscroll_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept
{
    return run_plane_ram_scroll_update(
        ram,
        plane_ram_offset,
        [](TilemapCameraState& state,
           VdpScrollBuffers& buffers,
           std::uint8_t& scroll_mode_reg,
           bool is_plane_a) noexcept {
            return vdp_update_hscroll_table(state, buffers, scroll_mode_reg, is_plane_a);
        });
}

std::uint8_t vdp_commit_scroll_dma_flags_from_ram(
    std::span<std::uint8_t, 0x10000> ram) noexcept
{
    return vdp_commit_scroll_dma_flags(
        ram[kScrollModeRegRamOffset],
        ram[kVintSyncFlagsRamOffset]);
}

} // namespace thor::render
