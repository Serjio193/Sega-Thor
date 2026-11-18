#pragma once

#include "game/render/dual_plane_loader.hpp"
#include "game/render/scroll_integrator.hpp"
#include "genesis/vdp_state.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace thor::render {

// ROM entrypoints and boundaries (0x00D7C0..0x00D9A4, 484 bytes).
// Note: kPlaneVramStreamHelperAddr (0x00D7C0U) is imported from dual_plane_loader.hpp.
inline constexpr std::uint32_t kVdpUploadVisibleTilemapPlaneAddr = 0x00D7C0U;
inline constexpr std::uint32_t kVdpFillPlaneBlankSafeAddr = 0x00D950U;
inline constexpr std::uint32_t kVdpFillPlaneBlankRawAddr = 0x00D95EU;
inline constexpr std::uint32_t kVdpFillPlaneBlankBodyAddr = 0x00D962U;
inline constexpr std::uint32_t kVisiblePlaneStreamerEndAddr = 0x00D9A4U;

// VDP nametable geometry, tile blank fill, and stream loop counts.
// Note: M68K `dbra` executes (count + 1) iterations, so 0x0028001C streams 41x29 tiles
// and 0x003F001F streams 64x32 tiles; 0x00D97C (d1=0x28, d2=0x1C) blanks 41x29 tiles.
inline constexpr std::uint16_t kVdpBlankTileWord = 0x0780U;
inline constexpr std::uint16_t kVdpNametableCols = 64U;
inline constexpr std::uint16_t kVdpNametableRows = 32U;
inline constexpr std::size_t kVdpNametableRowByteStride = 0x80U; // 128 bytes
inline constexpr std::size_t kVdpNametableByteSize = 0x1000U;     // 4096 bytes
inline constexpr std::uint16_t kVdpVisibleStreamCols = 41U;       // 0x28 + 1
inline constexpr std::uint16_t kVdpVisibleStreamRows = 29U;       // 0x1C + 1
inline constexpr std::uint16_t kVdpFullPlaneStreamCols = 64U;     // 0x3F + 1
inline constexpr std::uint16_t kVdpFullPlaneStreamRows = 32U;     // 0x1F + 1
inline constexpr std::uint16_t kVdpBlankFillCols = 41U;           // 0x28 + 1
inline constexpr std::uint16_t kVdpBlankFillRows = 29U;           // 0x1C + 1
inline constexpr std::uint32_t kVdpCmdRowStep = 0x00800000U;      // +0x80 VRAM per row

// Plane context control flags (+$27).
inline constexpr std::uint8_t kPlaneActiveBit = 0x01U;         // bit 0: 1=stream, 0=blank fill
inline constexpr std::uint8_t kPlaneFull64x32LatchBit = 0x02U; // bit 1: 64x32 full-plane mode

/// Streamed tile window dimensions (cols x rows).
struct VisibleStreamDimensions {
    std::uint16_t cols{0U};
    std::uint16_t rows{0U};

    [[nodiscard]] constexpr std::size_t tile_count() const noexcept {
        return static_cast<std::size_t>(cols) * rows;
    }

    [[nodiscard]] constexpr bool operator==(const VisibleStreamDimensions&) const noexcept = default;
};

/// Plane context fields accessed by 0x00D7C0..0x00D9A4 at $FF1716 (Plane A) / $FF173E (Plane B).
struct VisiblePlaneState {
    std::int16_t cam_x{0};               // +0x00
    std::int16_t cam_y{0};               // +0x04
    std::uint16_t start_col{0U};         // +0x08 (Out)
    std::uint16_t start_row{0U};         // +0x0A (Out)
    std::uint16_t vram_ring_offset{0U};  // +0x0C (Zeroed at 0x00D7CA)
    std::uint16_t vdp_hscroll{0U};       // +0x0E (Zeroed at 0x00D7C2)
    std::uint16_t vdp_vscroll{0U};       // +0x10 (Zeroed at 0x00D7C6)
    std::uint32_t tile_data_ptr{0U};     // +0x16
    std::uint16_t vram_base{0U};         // +0x1A (0xC000 or 0xE000)
    std::int16_t tile_col_step{0};       // +0x1C (Zeroed at 0x00D7D6)
    std::int16_t tile_row_step{0};       // +0x1E (Zeroed at 0x00D7DA)
    std::uint16_t width_tiles{0U};       // +0x20
    std::uint16_t height_tiles{0U};      // +0x22
    std::uint16_t byte_stride_shift{0U}; // +0x24 (log2(width_tiles * 2))
    std::uint8_t control_flags{0U};      // +0x27 (bit 0=active, bit 1=64x32 latch)

    [[nodiscard]] constexpr bool operator==(const VisiblePlaneState&) const noexcept = default;
};

/// Result summary and VDP control command trace from streaming or blanking a plane.
struct VisiblePlaneStreamResult {
    bool ok{false};
    bool filled_blank{false};
    VisibleStreamDimensions dims{};
    std::uint16_t start_col{0U};
    std::uint16_t start_row{0U};
    std::uint8_t updated_control_flags{0U};
    std::size_t tiles_written{0U};
    std::vector<std::uint32_t> vdp_control_writes{};
};

/// 0x00D892..0x00D8A8 / 0x00D962..0x00D974: Encodes 16-bit VRAM address into a 32-bit VDP write command.
[[nodiscard]] constexpr std::uint32_t compute_vram_write_cmd(std::uint16_t vram_addr) noexcept {
    const std::uint32_t high = static_cast<std::uint32_t>((vram_addr & 0x3FFFU) | 0x4000U);
    const std::uint32_t low = static_cast<std::uint32_t>((vram_addr >> 14U) & 0x0003U);
    return (high << 16U) | low;
}

/// 0x00D7E8..0x00D81C: Converts pixel camera coordinate via `asr.w #3` and wraps into [0, plane_tiles).
[[nodiscard]] constexpr std::uint16_t wrap_tile_origin(
    std::int16_t cam_coord_px,
    std::uint16_t plane_tiles) noexcept
{
    const auto limit = static_cast<std::int16_t>(plane_tiles);
    if (limit <= 0) {
        return 0U;
    }
    std::int16_t tile = static_cast<std::int16_t>(cam_coord_px >> 3);
    while (tile < 0) {
        tile = static_cast<std::int16_t>(tile + limit);
    }
    while (tile >= limit) {
        tile = static_cast<std::int16_t>(tile - limit);
    }
    return static_cast<std::uint16_t>(tile);
}

/// 0x00D820..0x00D84A: Selects (64, 32) if bit 1 of control_flags is set or if
/// (height_tiles == 32 && width_tiles <= 64) (latching bit 1); otherwise (41, 29).
[[nodiscard]] constexpr VisibleStreamDimensions select_visible_stream_dimensions(
    std::uint16_t width_tiles,
    std::uint16_t height_tiles,
    std::uint8_t& control_flags) noexcept
{
    if ((control_flags & kPlaneFull64x32LatchBit) != 0U) {
        return {kVdpFullPlaneStreamCols, kVdpFullPlaneStreamRows};
    }
    if (height_tiles == 32U && width_tiles <= 64U) {
        control_flags = static_cast<std::uint8_t>(control_flags | kPlaneFull64x32LatchBit);
        return {kVdpFullPlaneStreamCols, kVdpFullPlaneStreamRows};
    }
    return {kVdpVisibleStreamCols, kVdpVisibleStreamRows};
}

/// Constructs VisiblePlaneState from a DualPlaneLoader PlaneContextRecord and camera coordinates.
[[nodiscard]] constexpr VisiblePlaneState make_visible_plane_state(
    const PlaneContextRecord& ctx,
    std::int16_t cam_x = 0,
    std::int16_t cam_y = 0) noexcept
{
    VisiblePlaneState state{};
    state.cam_x = cam_x;
    state.cam_y = cam_y;
    state.tile_data_ptr = ctx.tile_data_ptr;
    state.vram_base = ctx.vram_base;
    state.width_tiles = ctx.width_tiles;
    state.height_tiles = ctx.height_tiles;
    state.byte_stride_shift = ctx.byte_stride_shift;
    state.control_flags = ctx.control_flags;
    return state;
}

/// 0x00D950 / 0x00D95E / 0x00D962: Fills 41x29 tiles starting at vram_base with blank tile 0x0780.
VisiblePlaneStreamResult vdp_fill_plane_blank(
    std::uint16_t vram_base,
    oasis::genesis::VdpState& vdp) noexcept;

/// 0x00D7C0..0x00D94E: Clears scroll/step fields, checks bit 0 (blank fallback to 0x00D95E),
/// normalizes toroidal origins, latches 64x32 if eligible, and streams tiles into VdpState VRAM.
/// tilemap_bytes starts at byte 0 of the plane's tilemap payload (+$16).
VisiblePlaneStreamResult vdp_upload_visible_tilemap_plane(
    VisiblePlaneState& state,
    std::span<const std::uint8_t> tilemap_bytes,
    oasis::genesis::VdpState& vdp) noexcept;

/// Loads VisiblePlaneState from the 64KB Work RAM window at plane_ram_offset (e.g. 0x1716 or 0x173E).
[[nodiscard]] VisiblePlaneState load_visible_plane_state_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept;

/// Stores VisiblePlaneState fields into the 64KB Work RAM window at plane_ram_offset.
void store_visible_plane_state_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    const VisiblePlaneState& state) noexcept;

/// 64KB RAM window helper for 0x00D7C0: loads plane context at plane_ram_offset, resolves
/// tile_data_ptr ($FF0000..$FFFFFF or 16-bit offset) in ram, streams to VdpState VRAM,
/// and writes updated +$08, +$0A, +$0C, +$0E, +$10, +$1C, +$1E, +$27 back to ram.
VisiblePlaneStreamResult vdp_upload_visible_tilemap_plane_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    oasis::genesis::VdpState& vdp) noexcept;

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
