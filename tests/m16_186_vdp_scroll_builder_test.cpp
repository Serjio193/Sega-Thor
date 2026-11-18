#include "game/render/vdp_scroll_builder.hpp"
#include "../include/game/render/vdp_scroll_builder.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

using namespace oasis::game::render;

void expect(bool cond, const char* expr, int line) {
    if (!cond) {
        std::cerr << "[FAIL] line " << line << ": " << expr << "\n";
        std::abort();
    }
}

#define EXPECT(cond) expect((cond), #cond, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    const std::array<std::uint8_t, 0x10000>& ram,
    std::uint32_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(ram[offset]) << 8) | ram[offset + 1U]);
}

void test_constants_and_mode_codecs() {
    static_assert(kUpdateHScrollTableAddr == 0x00D2B4U);
    static_assert(kUpdateVScrollTableAddr == 0x00D344U);
    static_assert(kCommitScrollDmaFlagsAddr == 0x00D398U);

    static_assert(kScrollModeRegRamOffset == 0x16F0U);
    static_assert(kVintSyncFlagsRamOffset == 0x164EU);
    static_assert(kFullscreenHScrollPlaneA == 0x1344U);
    static_assert(kFullscreenHScrollPlaneB == 0x1346U);
    static_assert(kFullscreenVScrollPlaneA == 0x1348U);
    static_assert(kFullscreenVScrollPlaneB == 0x134AU);
    static_assert(kHScrollTableRamOffset == 0xFB5CU);
    static_assert(kVScrollTableRamOffset == 0xFF5CU);
    static_assert(kPlaneARamOffset == 0x1716U);
    static_assert(kPlaneBRamOffset == 0x173EU);

    static_assert(kScrollPendingHScrollBit == 0x08U);
    static_assert(kScrollPendingVScrollBit == 0x10U);
    static_assert(kScrollPendingDmaMask == 0x18U);
    static_assert(kScrollPendingDmaClearMask == 0xE7U);

    static_assert(kHScrollTableWordCount == 512U);
    static_assert(kVScrollTableWordCount == 40U);
    static_assert(kHScrollStripIterations == 29U);
    static_assert(kHScrollStripWordStride == 16U);
    static_assert(kHScrollScanlineOuterIterations * 4U == kHScrollScanlineEntryCount);
    static_assert(kHScrollScanlineEntryCount == 232U);
    static_assert(kVScrollColumnIterations == 20U);

    // When bit 1 == 0, bit 0 is ignored and HScrollMode is FullScreen.
    static_assert(decode_hscroll_mode(0x00U) == HScrollMode::FullScreen);
    static_assert(decode_hscroll_mode(0x01U) == HScrollMode::FullScreen);
    static_assert(decode_hscroll_mode(0x02U) == HScrollMode::TileStrip16px);
    static_assert(decode_hscroll_mode(0x03U) == HScrollMode::ScanlineParallax);

    static_assert(decode_vscroll_mode(0x00U) == VScrollMode::FullScreen);
    static_assert(decode_vscroll_mode(0x04U) == VScrollMode::TwoCellColumn16px);

    // Upper bits 3..7 are preserved when encoding bits 0..2.
    static_assert(encode_scroll_mode_bits(
                      HScrollMode::ScanlineParallax,
                      VScrollMode::TwoCellColumn16px,
                      0xA0U) == 0xA7U);
    static_assert(encode_scroll_mode_bits(
                      HScrollMode::TileStrip16px,
                      VScrollMode::FullScreen,
                      0x9FU) == 0x9AU);
}

void test_fullscreen_plane_a_and_b() {
    VdpScrollBuffers buffers{};
    buffers.hscroll_table.fill(0xCAFEU);
    buffers.vscroll_table.fill(0xBEEFU);

    TilemapCameraState plane_a{};
    plane_a.vdp_hscroll = 0x01A5U;
    plane_a.vdp_vscroll = 0x007BU;
    plane_a.dirty_flags = kDirtyBitTileColStrip | kDirtyBitPlaneAHScroll | kDirtyBitPlaneAVScroll;

    std::uint8_t mode_reg = 0x81U; // bit 7 preserved; bit 1 == 0, bit 2 == 0 -> FullScreen H & V
    EXPECT(vdp_update_hscroll_table(plane_a, buffers, mode_reg, true));

    EXPECT(buffers.fullscreen_hscroll_plane_a == 0x01A5U);
    EXPECT(buffers.fullscreen_hscroll_plane_b == 0x0000U);
    EXPECT(buffers.fullscreen_vscroll_plane_a == 0x007BU);
    EXPECT(buffers.fullscreen_vscroll_plane_b == 0x0000U);
    EXPECT(plane_a.dirty_flags == kDirtyBitTileColStrip); // bits 2 & 3 cleared; bit 0 kept
    EXPECT(mode_reg == (0x81U | kScrollPendingDmaMask));

    // Plane B updates only Plane B words and leaves Plane A and tables untouched.
    TilemapCameraState plane_b{};
    plane_b.vdp_hscroll = 0x0044U;
    plane_b.vdp_vscroll = 0x0022U;
    plane_b.dirty_flags = kDirtyBitPlaneAHScroll | kDirtyBitPlaneAVScroll;
    EXPECT(vdp_update_hscroll_table(plane_b, buffers, mode_reg, false));

    EXPECT(buffers.fullscreen_hscroll_plane_a == 0x01A5U);
    EXPECT(buffers.fullscreen_hscroll_plane_b == 0x0044U);
    EXPECT(buffers.fullscreen_vscroll_plane_a == 0x007BU);
    EXPECT(buffers.fullscreen_vscroll_plane_b == 0x0022U);
    EXPECT(plane_b.dirty_flags == 0x00U);
    EXPECT(buffers.hscroll_table[0] == 0xCAFEU);
    EXPECT(buffers.vscroll_table[0] == 0xBEEFU);
}

void test_hscroll_tile_strip_16px() {
    VdpScrollBuffers buffers{};
    buffers.hscroll_table.fill(0x5A5AU);

    TilemapCameraState plane_a{};
    plane_a.vdp_hscroll = 0x0123U;
    plane_a.dirty_flags = kDirtyBitPlaneAHScroll;

    std::uint8_t mode_reg = kScrollModeHTableBit; // bit 1=1, bit 0=0 -> TileStrip16px
    EXPECT(vdp_update_hscroll_table(plane_a, buffers, mode_reg, true));
    EXPECT(plane_a.dirty_flags == 0x00U);
    EXPECT(mode_reg == (kScrollModeHTableBit | kScrollPendingHScrollBit));
    EXPECT(buffers.fullscreen_hscroll_plane_a == 0U);

    // Plane A writes 29 entries at stride 16 words (32 bytes): indices 0, 16, ..., 448.
    for (std::size_t i = 0; i < kHScrollTableWordCount; ++i) {
        const bool expected_written = (i % kHScrollStripWordStride == 0U) &&
                                      (i / kHScrollStripWordStride < kHScrollStripIterations);
        EXPECT(buffers.hscroll_table[i] == (expected_written ? 0x0123U : 0x5A5AU));
    }

    // Plane B writes 29 entries at offset +1 word (stride 16 words): indices 1, 17, ..., 449.
    TilemapCameraState plane_b{};
    plane_b.vdp_hscroll = 0x01E7U;
    plane_b.dirty_flags = kDirtyBitPlaneAHScroll;
    EXPECT(vdp_update_hscroll_table(plane_b, buffers, mode_reg, false));

    for (std::size_t i = 0; i < kHScrollTableWordCount; ++i) {
        const std::size_t strip_idx = i / kHScrollStripWordStride;
        const std::size_t mod = i % kHScrollStripWordStride;
        if (strip_idx < kHScrollStripIterations && mod == 0U) {
            EXPECT(buffers.hscroll_table[i] == 0x0123U);
        } else if (strip_idx < kHScrollStripIterations && mod == 1U) {
            EXPECT(buffers.hscroll_table[i] == 0x01E7U);
        } else {
            EXPECT(buffers.hscroll_table[i] == 0x5A5AU);
        }
    }
}

void test_hscroll_scanline_parallax() {
    VdpScrollBuffers buffers{};
    buffers.hscroll_table.fill(0x7777U);

    TilemapCameraState plane_a{};
    plane_a.vdp_hscroll = 0x00F1U;
    plane_a.dirty_flags = kDirtyBitPlaneAHScroll;

    std::uint8_t mode_reg = kScrollModeHTableBit | kScrollModeHScanlineBit; // bit 1=1, bit 0=1
    EXPECT(vdp_update_hscroll_table(plane_a, buffers, mode_reg, true));
    EXPECT(mode_reg == (0x03U | kScrollPendingHScrollBit));

    // Plane A writes 58 * 4 = 232 words at even indices 0, 2, ..., 462.
    for (std::size_t i = 0; i < kHScrollTableWordCount; ++i) {
        const bool is_a_entry = (i < kHScrollScanlineEntryCount * 2U) && ((i % 2U) == 0U);
        EXPECT(buffers.hscroll_table[i] == (is_a_entry ? 0x00F1U : 0x7777U));
    }

    // Plane B writes 232 words at odd indices 1, 3, ..., 463; indices >= 464 remain untouched.
    TilemapCameraState plane_b{};
    plane_b.vdp_hscroll = 0x0188U;
    plane_b.dirty_flags = kDirtyBitPlaneAHScroll;
    EXPECT(vdp_update_hscroll_table(plane_b, buffers, mode_reg, false));

    for (std::size_t i = 0; i < kHScrollScanlineEntryCount * 2U; ++i) {
        EXPECT(buffers.hscroll_table[i] == (((i % 2U) == 0U) ? 0x00F1U : 0x0188U));
    }
    for (std::size_t i = kHScrollScanlineEntryCount * 2U; i < kHScrollTableWordCount; ++i) {
        EXPECT(buffers.hscroll_table[i] == 0x7777U);
    }
}

void test_vscroll_two_cell_column_16px() {
    VdpScrollBuffers buffers{};
    buffers.vscroll_table.fill(0x9999U);

    TilemapCameraState plane_a{};
    plane_a.vdp_vscroll = 0x00ABU;
    plane_a.dirty_flags = kDirtyBitPlaneAVScroll | kDirtyBitTileRowStrip;

    std::uint8_t mode_reg = kScrollModeVTableBit; // bit 2 == 1 -> TwoCellColumn16px
    EXPECT(vdp_update_vscroll_table(plane_a, buffers, mode_reg, true));
    EXPECT(plane_a.dirty_flags == kDirtyBitTileRowStrip);
    EXPECT(mode_reg == (kScrollModeVTableBit | kScrollPendingVScrollBit));
    EXPECT(buffers.fullscreen_vscroll_plane_a == 0U);

    // 20 entries at even word indices 0, 2, ..., 38; odd words untouched.
    for (std::size_t i = 0; i < kVScrollTableWordCount; ++i) {
        EXPECT(buffers.vscroll_table[i] == (((i % 2U) == 0U) ? 0x00ABU : 0x9999U));
    }

    // Plane B writes 20 entries at odd word indices 1, 3, ..., 39.
    TilemapCameraState plane_b{};
    plane_b.vdp_vscroll = 0x0034U;
    plane_b.dirty_flags = kDirtyBitPlaneAVScroll;
    EXPECT(vdp_update_vscroll_table(plane_b, buffers, mode_reg, false));
    EXPECT(plane_b.dirty_flags == 0x00U);

    for (std::size_t i = 0; i < kVScrollTableWordCount; ++i) {
        EXPECT(buffers.vscroll_table[i] == (((i % 2U) == 0U) ? 0x00ABU : 0x0034U));
    }
}

void test_dirty_flag_noop_and_vscroll_first_order() {
    VdpScrollBuffers buffers{};
    buffers.fullscreen_hscroll_plane_a = 0x1111U;
    buffers.fullscreen_vscroll_plane_a = 0x2222U;

    TilemapCameraState clean{};
    clean.vdp_hscroll = 0x0099U;
    clean.vdp_vscroll = 0x0088U;
    clean.dirty_flags = kDirtyBitTileColStrip | kDirtyBitTileRowStrip; // bits 2 & 3 clear

    std::uint8_t mode_reg = 0x85U;
    EXPECT(!vdp_update_vscroll_table(clean, buffers, mode_reg, true));
    EXPECT(!vdp_update_hscroll_table(clean, buffers, mode_reg, true));
    EXPECT(!vdp_update_hscroll_only(clean, buffers, mode_reg, true));
    EXPECT(clean.dirty_flags == (kDirtyBitTileColStrip | kDirtyBitTileRowStrip));
    EXPECT(mode_reg == 0x85U);
    EXPECT(buffers.fullscreen_hscroll_plane_a == 0x1111U);
    EXPECT(buffers.fullscreen_vscroll_plane_a == 0x2222U);

    // When ONLY VScroll dirty (bit 3) is set, vdp_update_hscroll_table (0x00D2B4) still
    // invokes vdp_update_vscroll_table (bsr.w $d344) first and skips HScroll.
    TilemapCameraState v_only{};
    v_only.vdp_hscroll = 0x01FFU;
    v_only.vdp_vscroll = 0x0055U;
    v_only.dirty_flags = kDirtyBitPlaneAVScroll;
    mode_reg = 0x00U;
    EXPECT(vdp_update_hscroll_table(v_only, buffers, mode_reg, true));
    EXPECT(v_only.dirty_flags == 0x00U);
    EXPECT(mode_reg == kScrollPendingVScrollBit); // bit 4 set, bit 3 clear
    EXPECT(buffers.fullscreen_vscroll_plane_a == 0x0055U);
    EXPECT(buffers.fullscreen_hscroll_plane_a == 0x1111U);
}

void test_commit_scroll_dma_flags_and_wrapper() {
    std::uint8_t mode_reg = 0xA7U; // mode bits=0x07, upper bits=0x80|0x20, pending bits=0x00
    std::uint8_t vint_sync = 0x02U; // e.g. DMA flush bit 1 already set

    // Nothing pending -> no-op.
    EXPECT(vdp_commit_scroll_dma_flags(mode_reg, vint_sync) == 0x00U);
    EXPECT(mode_reg == 0xA7U);
    EXPECT(vint_sync == 0x02U);

    // Pending HScroll (0x08) and VScroll (0x10) -> moved into vint_sync, cleared from mode_reg.
    mode_reg = 0xFFU;
    const std::uint8_t committed = vdp_commit_scroll_dma_flags(mode_reg, vint_sync);
    EXPECT(committed == kScrollPendingDmaMask);
    EXPECT(mode_reg == 0xE7U);
    EXPECT(vint_sync == (0x02U | kScrollPendingDmaMask));

    // Cleanroom wrapper test: updates H (TileStrip16px) + V (FullScreen) and commits DMA bits.
    VdpScrollBuffers buffers{};
    VdpScrollConfig config{HScrollMode::TileStrip16px, VScrollMode::FullScreen};
    std::uint8_t wrapper_sync = 0x04U;
    update_vdp_scroll_tables(buffers, config, true, 0x0077U, 0x0033U, true, false, wrapper_sync);
    EXPECT(buffers.hscroll_table[0] == 0x0077U);
    EXPECT(buffers.hscroll_table[448] == 0x0077U);
    EXPECT(buffers.fullscreen_vscroll_plane_a == 0x0000U);
    EXPECT(wrapper_sync == (0x04U | kScrollPendingHScrollBit));
}

void test_ram_window_roundtrips() {
    std::array<std::uint8_t, 0x10000> ram{};

    // Initialize Plane A ($FF1716) and Plane B ($FF173E) camera state in RAM.
    ram[kPlaneARamOffset + 0x01U] = 100U; // cam_x = 100
    ram[kPlaneARamOffset + 0x05U] = 50U;  // cam_y = 50
    ram[kPlaneARamOffset + 0x21U] = 40U;
    ram[kPlaneARamOffset + 0x23U] = 28U;
    ram[kPlaneARamOffset + 0x27U] = kControlEnableIntegration;

    ram[kPlaneBRamOffset + 0x0EU] = 0x01U;
    ram[kPlaneBRamOffset + 0x0FU] = 0x20U; // vdp_hscroll = 0x0120
    ram[kPlaneBRamOffset + 0x10U] = 0x00U;
    ram[kPlaneBRamOffset + 0x11U] = 0x45U; // vdp_vscroll = 0x0045
    ram[kPlaneBRamOffset + 0x26U] = kDirtyBitPlaneAHScroll | kDirtyBitPlaneAVScroll;

    // Integrate Plane A via M16.185: new_x=105 -> vdp_hscroll=507 (0x01FB), new_y=57 -> vdp_vscroll=7.
    integrate_scroll_deltas_from_ram(ram, kPlaneARamOffset, 105, 57);

    // Configure $FF16F0 = ScanlineParallax + TwoCellColumn16px + upper bit 7 preserved.
    ram[kScrollModeRegRamOffset] = 0x80U | 0x03U | 0x04U;
    ram[kVintSyncFlagsRamOffset] = 0x02U;

    EXPECT(vdp_update_hscroll_from_ram(ram, kPlaneARamOffset));
    EXPECT(vdp_update_hscroll_from_ram(ram, 0x00FF0000U | kPlaneBRamOffset));

    // Verify dirty bits cleared on both structs and pending bits set on $FF16F0.
    EXPECT((ram[kPlaneARamOffset + 0x26U] & (kDirtyBitPlaneAHScroll | kDirtyBitPlaneAVScroll)) == 0U);
    EXPECT(ram[kPlaneBRamOffset + 0x26U] == 0U);
    EXPECT(ram[kScrollModeRegRamOffset] == (0x87U | kScrollPendingDmaMask));

    // Verify big-endian ScanlineParallax entries in $FFFB5C (Plane A +0, Plane B +2).
    EXPECT(read_be16(ram, kHScrollTableRamOffset + 0U) == 0x01FBU);
    EXPECT(read_be16(ram, kHScrollTableRamOffset + 2U) == 0x0120U);
    EXPECT(read_be16(ram, kHScrollTableRamOffset + (231U * 4U)) == 0x01FBU);
    EXPECT(read_be16(ram, kHScrollTableRamOffset + (231U * 4U) + 2U) == 0x0120U);
    EXPECT(read_be16(ram, kHScrollTableRamOffset + (232U * 4U)) == 0x0000U);

    // Verify big-endian TwoCellColumn16px entries in $FFFF5C (Plane A +0, Plane B +2).
    EXPECT(read_be16(ram, kVScrollTableRamOffset + 0U) == 0x0007U);
    EXPECT(read_be16(ram, kVScrollTableRamOffset + 2U) == 0x0045U);
    EXPECT(read_be16(ram, kVScrollTableRamOffset + (19U * 4U)) == 0x0007U);
    EXPECT(read_be16(ram, kVScrollTableRamOffset + (19U * 4U) + 2U) == 0x0045U);

    // Commit pending flags from $FF16F0 into $FF164E.
    const std::uint8_t committed = vdp_commit_scroll_dma_flags_from_ram(ram);
    EXPECT(committed == kScrollPendingDmaMask);
    EXPECT(ram[kScrollModeRegRamOffset] == 0x87U);
    EXPECT(ram[kVintSyncFlagsRamOffset] == (0x02U | kScrollPendingDmaMask));

    // Verify fullscreen RAM path ($FF1344..$FF134A) and standalone vdp_update_vscroll_from_ram.
    ram[kScrollModeRegRamOffset] = 0x00U; // FullScreen H & V
    ram[kPlaneARamOffset + 0x0EU] = 0x00U;
    ram[kPlaneARamOffset + 0x0FU] = 0x99U;
    ram[kPlaneARamOffset + 0x10U] = 0x00U;
    ram[kPlaneARamOffset + 0x11U] = 0x55U;
    ram[kPlaneARamOffset + 0x26U] = kDirtyBitPlaneAVScroll;
    EXPECT(vdp_update_vscroll_from_ram(ram, kPlaneARamOffset));
    EXPECT(read_be16(ram, kFullscreenVScrollPlaneA) == 0x0055U);
    EXPECT(ram[kScrollModeRegRamOffset] == kScrollPendingVScrollBit);

    ram[kPlaneBRamOffset + 0x26U] = kDirtyBitPlaneAHScroll | kDirtyBitPlaneAVScroll;
    EXPECT(vdp_update_hscroll_from_ram(ram, kPlaneBRamOffset));
    EXPECT(read_be16(ram, kFullscreenHScrollPlaneB) == 0x0120U);
    EXPECT(read_be16(ram, kFullscreenVScrollPlaneB) == 0x0045U);

    // Calling again when clean must be a no-op, and out-of-bounds offset fails safely.
    EXPECT(!vdp_update_hscroll_from_ram(ram, kPlaneBRamOffset));
    EXPECT(!vdp_update_vscroll_from_ram(ram, kPlaneARamOffset));
    EXPECT(!vdp_update_hscroll_from_ram(ram, 0x10000U));
}

} // namespace

int main() {
    test_constants_and_mode_codecs();
    test_fullscreen_plane_a_and_b();
    test_hscroll_tile_strip_16px();
    test_hscroll_scanline_parallax();
    test_vscroll_two_cell_column_16px();
    test_dirty_flag_noop_and_vscroll_first_order();
    test_commit_scroll_dma_flags_and_wrapper();
    test_ram_window_roundtrips();

    std::cout << "[PASS] All M16.186 VDP scroll builder tests succeeded!\n";
    return 0;
}
