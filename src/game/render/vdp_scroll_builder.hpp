#pragma once

#include "game/render/scroll_integrator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace thor::render {

inline constexpr std::uint32_t kUpdateHScrollTableAddr = 0x00D2B4U;
inline constexpr std::uint32_t kUpdateVScrollTableAddr = 0x00D344U;
inline constexpr std::uint32_t kCommitScrollDmaFlagsAddr = 0x00D398U;

// 64KB RAM window ($FF0000..$FFFFFF) offsets; kPlaneARamOffset (0x1716) and
// kPlaneBRamOffset (0x173E) are defined in scroll_integrator.hpp.
inline constexpr std::uint32_t kScrollModeRegRamOffset = 0x16F0U;
inline constexpr std::uint32_t kVintSyncFlagsRamOffset = 0x164EU;
inline constexpr std::uint32_t kFullscreenHScrollPlaneA = 0x1344U;
inline constexpr std::uint32_t kFullscreenHScrollPlaneB = 0x1346U;
inline constexpr std::uint32_t kFullscreenVScrollPlaneA = 0x1348U;
inline constexpr std::uint32_t kFullscreenVScrollPlaneB = 0x134AU;
inline constexpr std::uint32_t kHScrollTableRamOffset = 0xFB5CU;
inline constexpr std::uint32_t kVScrollTableRamOffset = 0xFF5CU;

// $FF16F0 scroll mode and $FF164E V-Int pending DMA bitmasks.
inline constexpr std::uint8_t kScrollModeHScanlineBit = 0x01U;    // bit 0: 1=scanline, 0=16px strip (when bit 1=1)
inline constexpr std::uint8_t kScrollModeHTableBit = 0x02U;       // bit 1: 0=fullscreen, 1=table ($FFFB5C)
inline constexpr std::uint8_t kScrollModeVTableBit = 0x04U;       // bit 2: 0=fullscreen, 1=2-cell table ($FFFF5C)
inline constexpr std::uint8_t kScrollPendingHScrollBit = 0x08U;   // bit 3: pending HScroll DMA sync
inline constexpr std::uint8_t kScrollPendingVScrollBit = 0x10U;   // bit 4: pending VScroll DMA sync
inline constexpr std::uint8_t kScrollPendingDmaMask = 0x18U;      // bits 3 & 4
inline constexpr std::uint8_t kScrollPendingDmaClearMask = 0xE7U; // ~0x18U

// VDP scroll table dimensions and ROM loop strides.
inline constexpr std::size_t kHScrollTableWordCount = 512U;
inline constexpr std::size_t kVScrollTableWordCount = 40U;
inline constexpr std::size_t kHScrollStripIterations = 29U;
inline constexpr std::size_t kHScrollStripWordStride = 16U;          // 32 bytes
inline constexpr std::size_t kHScrollScanlineOuterIterations = 58U;  // 58 * 4 = 232 entries
inline constexpr std::size_t kHScrollScanlineEntryCount = 232U;
inline constexpr std::size_t kHScrollScanlineWordStride = 2U;        // 4 bytes
inline constexpr std::size_t kVScrollColumnIterations = 20U;
inline constexpr std::size_t kVScrollColumnWordStride = 2U;          // 4 bytes

/// Horizontal scroll write mode decoded from $FF16F0 bits 1 and 0.
enum class HScrollMode : std::uint8_t {
    FullScreen = 0,
    TileStrip16px = 1,
    ScanlineParallax = 2,
};

/// Vertical scroll write mode decoded from $FF16F0 bit 2.
enum class VScrollMode : std::uint8_t {
    FullScreen = 0,
    TwoCellColumn16px = 1,
};

/// Cleanroom scroll mode configuration matching $FF16F0 mode bits 0..2.
struct VdpScrollConfig {
    HScrollMode h_mode{HScrollMode::FullScreen};
    VScrollMode v_mode{VScrollMode::FullScreen};
};

/// VDP RAM scroll staging buffers ($FF1344..$FF134A, $FFFB5C, $FFFF5C).
struct VdpScrollBuffers {
    std::uint16_t fullscreen_hscroll_plane_a{0};
    std::uint16_t fullscreen_hscroll_plane_b{0};
    std::uint16_t fullscreen_vscroll_plane_a{0};
    std::uint16_t fullscreen_vscroll_plane_b{0};
    std::array<std::uint16_t, kHScrollTableWordCount> hscroll_table{};
    std::array<std::uint16_t, kVScrollTableWordCount> vscroll_table{};
};

/// Decodes HScroll mode from $FF16F0 (bit 1 == 0 -> FullScreen regardless of bit 0).
[[nodiscard]] constexpr HScrollMode decode_hscroll_mode(std::uint8_t scroll_mode_reg) noexcept {
    if ((scroll_mode_reg & kScrollModeHTableBit) == 0U) {
        return HScrollMode::FullScreen;
    }
    return ((scroll_mode_reg & kScrollModeHScanlineBit) != 0U)
        ? HScrollMode::ScanlineParallax
        : HScrollMode::TileStrip16px;
}

/// Decodes VScroll mode from $FF16F0 bit 2.
[[nodiscard]] constexpr VScrollMode decode_vscroll_mode(std::uint8_t scroll_mode_reg) noexcept {
    return ((scroll_mode_reg & kScrollModeVTableBit) != 0U)
        ? VScrollMode::TwoCellColumn16px
        : VScrollMode::FullScreen;
}

/// Encodes HScrollMode and VScrollMode into $FF16F0 bits 0..2 while preserving bits 3..7.
[[nodiscard]] constexpr std::uint8_t encode_scroll_mode_bits(
    HScrollMode h_mode,
    VScrollMode v_mode,
    std::uint8_t existing_reg = 0U) noexcept
{
    std::uint8_t reg = static_cast<std::uint8_t>(existing_reg & ~0x07U);
    if (h_mode == HScrollMode::TileStrip16px) {
        reg = static_cast<std::uint8_t>(reg | kScrollModeHTableBit);
    } else if (h_mode == HScrollMode::ScanlineParallax) {
        reg = static_cast<std::uint8_t>(reg | kScrollModeHTableBit | kScrollModeHScanlineBit);
    }
    if (v_mode == VScrollMode::TwoCellColumn16px) {
        reg = static_cast<std::uint8_t>(reg | kScrollModeVTableBit);
    }
    return reg;
}

/// 0x00D344: Checks and clears dirty bit 3 (0x08) on state.dirty_flags; if set,
/// marks pending bit 4 (0x10) on scroll_mode_reg and writes vdp_vscroll (+0x10) to
/// fullscreen VScroll ($FF1348/$FF134A) or 20-entry 2-cell VScroll table ($FFFF5C).
bool vdp_update_vscroll_table(
    TilemapCameraState& state,
    VdpScrollBuffers& buffers,
    std::uint8_t& scroll_mode_reg,
    bool is_plane_a) noexcept;

/// 0x00D2B8..0x00D342: Checks and clears dirty bit 2 (0x04) on state.dirty_flags;
/// if set, marks pending bit 3 (0x08) on scroll_mode_reg and writes vdp_hscroll (+0x0E)
/// to fullscreen HScroll ($FF1344/$FF1346), 16px strip table, or scanline table ($FFFB5C).
bool vdp_update_hscroll_only(
    TilemapCameraState& state,
    VdpScrollBuffers& buffers,
    std::uint8_t& scroll_mode_reg,
    bool is_plane_a) noexcept;

/// 0x00D2B4: Invokes vdp_update_vscroll_table (matching `bsr.w $d344`) followed by
/// vdp_update_hscroll_only. Returns true if either VScroll or HScroll updated.
bool vdp_update_hscroll_table(
    TilemapCameraState& state,
    VdpScrollBuffers& buffers,
    std::uint8_t& scroll_mode_reg,
    bool is_plane_a) noexcept;

/// 0x00D398: Extracts pending bits 3 & 4 (0x18) from scroll_mode_reg ($FF16F0),
/// clears them from scroll_mode_reg (&= 0xE7), and ORs them into vint_sync_flags ($FF164E).
std::uint8_t vdp_commit_scroll_dma_flags(
    std::uint8_t& scroll_mode_reg,
    std::uint8_t& vint_sync_flags) noexcept;

/// Cleanroom convenience wrapper combining vdp_update_hscroll_table and vdp_commit_scroll_dma_flags.
void update_vdp_scroll_tables(
    VdpScrollBuffers& buffers,
    const VdpScrollConfig& config,
    bool is_plane_a,
    std::uint16_t hscroll_val,
    std::uint16_t vscroll_val,
    bool h_dirty,
    bool v_dirty,
    std::uint8_t& vint_sync_flags) noexcept;

/// Loads VdpScrollBuffers from the 64KB RAM window ($FF1344..$FF134A, $FFFB5C, $FFFF5C).
[[nodiscard]] VdpScrollBuffers load_scroll_buffers_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept;

/// Stores VdpScrollBuffers back to the 64KB RAM window ($FF1344..$FF134A, $FFFB5C, $FFFF5C).
void store_scroll_buffers_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const VdpScrollBuffers& buffers) noexcept;

/// 64KB RAM window helper for 0x00D344 (accepts 16-bit offsets like 0x1716 or $00FF1716).
bool vdp_update_vscroll_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept;

/// 64KB RAM window helper for 0x00D2B4 (accepts 16-bit offsets like 0x1716 or $00FF1716).
bool vdp_update_hscroll_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept;

/// 64KB RAM window helper for 0x00D398 ($FF16F0 -> $FF164E).
std::uint8_t vdp_commit_scroll_dma_flags_from_ram(
    std::span<std::uint8_t, 0x10000> ram) noexcept;

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
