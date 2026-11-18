#pragma once

#include "game/render/tilemap_rect_ops.hpp"
#include "game/render/visible_plane_streamer.hpp"
#include "genesis/vdp_state.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace thor::render {

// ROM entrypoints and boundaries (0x00DC26..0x00DDA2, 380 bytes, 127 instructions).
// Note: kVdpSyncDirtyPlaneRectAddr (0x00DC26U) is imported from tilemap_rect_ops.hpp.
inline constexpr std::uint32_t kVdpSyncDirtyPlaneRectClippedAddr = 0x00DC30U;
inline constexpr std::uint32_t kVdpSyncDirtyPlaneRectClippedRtsAddr = 0x00DD12U;
inline constexpr std::uint32_t kVdpSyncDirtyPlaneRectUnclippedAddr = 0x00DD14U;
inline constexpr std::uint32_t kDirtyPlaneBlitterEndAddr = 0x00DDA2U;
inline constexpr std::size_t kDirtyPlaneBlitterByteCount = 380U;

// Viewport clipping bounds and VDP ring masks (0x00DC26..0x00DDA2).
// Note: 0x00DC46 (#$29) and 0x00DC72 (#$1D) clamp inclusive dbra counts (d3, d4),
// yielding max streamed extents of 42 cols (0..41) x 30 rows (0..29).
// Only bit 1 of +$27(a3) is tested at 0x00DC26; bit 0 (active flag) is not checked.
inline constexpr std::int16_t kDirtyClipMaxRelCol = 41;
inline constexpr std::int16_t kDirtyClipMaxRelRow = 29;
inline constexpr std::uint16_t kDirtyClipMaxCols = 42U;
inline constexpr std::uint16_t kDirtyClipMaxRows = 30U;
inline constexpr std::uint8_t kDirtyPlaneUnclippedModeBit = kPlaneFull64x32LatchBit;
inline constexpr std::uint16_t kDirtyVdpColMask = 0x003FU;
inline constexpr std::uint16_t kDirtyVdpRowMask = 0x001FU;

/// Resolved clipping geometry, source byte offset, and initial 64x32 VDP ring cell.
struct DirtyPlaneTransferPlan {
    bool valid{false};
    bool culled{false};
    bool unclipped_mode{false};
    std::int16_t clipped_rel_x{0};
    std::int16_t clipped_rel_y{0};
    std::uint16_t cols{0U};
    std::uint16_t rows{0U};
    std::uint16_t vdp_start_col{0U};
    std::uint16_t vdp_start_row{0U};
    std::size_t source_byte_offset{0U};

    [[nodiscard]] constexpr std::size_t tile_count() const noexcept {
        return static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows);
    }

    [[nodiscard]] constexpr bool operator==(const DirtyPlaneTransferPlan&) const noexcept = default;
};

/// Result summary and VDP control command trace from 0x00DC26..0x00DDA2.
struct DirtyPlaneBlitResult {
    bool ok{false};
    bool culled{false};
    bool unclipped_mode{false};
    DirtyPlaneTransferPlan plan{};
    std::size_t tiles_written{0U};
    std::vector<std::uint32_t> vdp_control_writes{};
};

/// 0x00DCA4..0x00DCD2 / 0x00DD32..0x00DD60: Encodes (vram_base, vdp_col, vdp_row)
/// into the 32-bit VDP control port write command `(cmd_hi << 16) | cmd_lo`.
[[nodiscard]] constexpr std::uint32_t compute_dirty_vram_write_cmd(
    std::uint16_t vram_base,
    std::uint16_t vdp_col,
    std::uint16_t vdp_row) noexcept
{
    const auto cmd_hi_base = static_cast<std::uint16_t>((vram_base & 0x3FFFU) | 0x4000U);
    const auto cmd_lo = static_cast<std::uint16_t>((vram_base >> 14U) & 0x0003U);
    const auto ring_offset = static_cast<std::uint16_t>(
        ((vdp_row & kDirtyVdpRowMask) << 7U) + ((vdp_col & kDirtyVdpColMask) << 1U));
    const auto cmd_hi = static_cast<std::uint16_t>(cmd_hi_base + ring_offset);
    return (static_cast<std::uint32_t>(cmd_hi) << 16U) | cmd_lo;
}

/// 0x00DC26..0x00DC98 & 0x00DD14..0x00DD26: Resolves mode (bit 1 of control_flags),
/// viewport clipping (42x30 inclusive dbra bounds), source skip offset, and VDP cell.
[[nodiscard]] DirtyPlaneTransferPlan plan_dirty_plane_rect(
    const VisiblePlaneState& state,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1) noexcept;

/// 0x00DC26..0x00DDA2: Blits dirty rectangle from `dirty_source_bytes` (matching M68K a1
/// at `(target_x, target_y)`, row stride `2 * state.width_tiles`) into `vdp.vram`.
DirtyPlaneBlitResult vdp_sync_dirty_plane_rect(
    const VisiblePlaneState& state,
    std::span<const std::uint8_t> dirty_source_bytes,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1,
    oasis::genesis::VdpState& vdp) noexcept;

/// Overload consuming a TilemapRectSyncRequest forwarded by 0x00DB64 / 0x00DB68 (`0x00DBC0`).
DirtyPlaneBlitResult vdp_sync_dirty_plane_rect(
    const VisiblePlaneState& state,
    const TilemapRectSyncRequest& request,
    oasis::genesis::VdpState& vdp) noexcept;

/// Convenience variant addressing `full_tilemap_bytes` starting at map origin `(0, 0)` (+$16).
DirtyPlaneBlitResult vdp_sync_dirty_plane_rect_from_tilemap(
    const VisiblePlaneState& state,
    std::span<const std::uint8_t> full_tilemap_bytes,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1,
    oasis::genesis::VdpState& vdp) noexcept;

/// 64KB Work RAM window helper for 0x00DC26: loads plane context at `plane_ram_offset`,
/// resolves `source_ram_addr` (a1, $FFxxxx or 16-bit offset), and blits into `vdp.vram`.
DirtyPlaneBlitResult vdp_sync_dirty_plane_rect_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::uint32_t source_ram_addr,
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t width_minus_1,
    std::int16_t height_minus_1,
    oasis::genesis::VdpState& vdp) noexcept;

/// 64KB Work RAM window helper for 0x00DC26 consuming a TilemapRectSyncRequest directly.
DirtyPlaneBlitResult vdp_sync_dirty_plane_rect_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    const TilemapRectSyncRequest& request,
    oasis::genesis::VdpState& vdp) noexcept;

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
