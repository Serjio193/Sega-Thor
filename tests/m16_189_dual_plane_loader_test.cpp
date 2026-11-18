#include "game/render/dual_plane_loader.hpp"
#include "game/render/vdp_scroll_builder.hpp"

#include "core/rom.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace thor::render;

void require_check(bool condition, const char* expr) {
    if (!condition) {
        std::cerr << "[FAIL] M16.189 check failed: " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void test_constants_and_aliases() {
    static_assert(kInitDualPlaneLayoutAddr == 0x00D542U);
    static_assert(kInitDualPlaneLayoutReturnAddr == 0x00D7AEU);
    static_assert(kComputeLog2ByteStrideAddr == 0x00D7B0U);
    static_assert(kComputeLog2ByteStrideReturnAddr == 0x00D7BEU);
    static_assert(kDualPlaneLoaderEndAddr == 0x00D7C0U);
    static_assert(kPlaneVramStreamHelperAddr == 0x00D7C0U);
    static_assert(kCopySceneExtraPaletteAddr == 0x00CECCU);

    static_assert(kPlaneAVramBase == 0xC000U);
    static_assert(kPlaneBVramBase == 0xE000U);
    static_assert(kVdpHScrollVramBase == 0xDC00U);
    static_assert(kVdpHScrollWriteCmd == 0x5C000003U);
    static_assert(kVdpVsramWriteCmd == 0x40000010U);
    static_assert(kVdpReg12ShadowHiOn == 0x8C89U);
    static_assert(kVdpReg12ShadowHiOff == 0x8C81U);

    static_assert(kSystemPaletteRomAddr == 0x000476U);
    static_assert(kScenePaletteTableRomAddr == 0x05D046U);
    static_assert(kTilemapDecompressBufferRamAddr == 0x00FF3FA8U);
    static_assert(kTilemapDecompressBufferRamOffset == 0x3FA8U);
    static_assert(kTilemapPayloadRamAddr == 0x00FF3FACU);
    static_assert(kTilemapPayloadRamOffset == 0x3FACU);

    static_assert(DualPlaneLoaderConfig::kPlaneAContextRamOffset == 0x1716U);
    static_assert(DualPlaneLoaderConfig::kPlaneBContextRamOffset == 0x173EU);
    static_assert(kPlaneAAuxRamOffset == 0x1766U);
    static_assert(kPlaneBAuxRamOffset == 0x1770U);
    static_assert(kMaxCameraXScrollRamOffset == 0x177AU);
    static_assert(kMaxCameraYScrollRamOffset == 0x177CU);
    static_assert(kSceneExtraPalettePtrRamOffset == 0x179EU);
    static_assert(kDynamicBufferBaseRamOffset == 0x17A2U);
    static_assert(kOverlayDecompressBufferPtrRamOffset == 0x17A6U);
    static_assert(kFinalSceneRomPtrRamOffset == 0x17AAU);
    static_assert(kFinalSceneRamAllocPtrRamOffset == 0x17AEU);
    static_assert(kRoomTransitionActiveRamOffset == 0x17C1U);
    static_assert(kPlaneAInitializedRamOffset == 0x17C2U);
    static_assert(kPlaneBInitializedRamOffset == 0x17C3U);
    static_assert(kVdpReg7ShadowRamOffset == 0x0BF8U);
    static_assert(kTargetFadeColor0RamOffset == 0x134CU);
    static_assert(kSystemPaletteRamOffset == 0x134EU);
    static_assert(kScenePaletteLine1RamOffset == 0x136CU);
    static_assert(kScenePaletteLines2And3RamOffset == 0x138CU);
    static_assert(kDualPlaneLineHScrollTableRamOffset == 0xFB5CU);
    static_assert(kDualPlaneColVScrollTableRamOffset == 0xFF5CU);

    static_assert(oasis::game::render::kInitDualPlaneLayoutAddr == kInitDualPlaneLayoutAddr);
    static_assert(sizeof(oasis::game::render::DualPlaneHeader) == sizeof(DualPlaneHeader));
}

void test_compute_log2_byte_stride() {
    static_assert(compute_log2_byte_stride(0U) == 0U);
    static_assert(compute_log2_byte_stride(1U) == 1U);
    static_assert(compute_log2_byte_stride(32U) == 6U);
    static_assert(compute_log2_byte_stride(64U) == 7U);
    static_assert(compute_log2_byte_stride(128U) == 8U);
    static_assert(compute_log2_byte_stride(256U) == 9U);
    static_assert(compute_log2_byte_stride(512U) == 10U);
    static_assert(compute_log2_byte_stride(1024U) == 11U);
    static_assert(compute_log2_byte_stride(2048U) == 12U);
    static_assert(compute_log2_byte_stride(0x8000U) == 0U); // 16-bit wrap matches add.w d0,d0

    CHECK(compute_log2_byte_stride(0U) == 0U);
    CHECK(compute_log2_byte_stride(32U) == 6U);
    CHECK(compute_log2_byte_stride(64U) == 7U);
    CHECK(compute_log2_byte_stride(128U) == 8U);
    CHECK(compute_log2_byte_stride(256U) == 9U);
}

void test_unpack_dual_plane_header() {
    const std::array<std::uint8_t, 3> too_short{0x82U, 0x02U, 0x00U};
    CHECK(!unpack_dual_plane_header(too_short).has_value());

    // Beach / Town layout: 82 02 00 00 (Plane A = 64x64 + aux; Plane B = empty)
    const std::array<std::uint8_t, 4> hdr_beach{0x82U, 0x02U, 0x00U, 0x00U};
    const auto beach = unpack_dual_plane_header(hdr_beach, 0x1234U);
    CHECK(beach.has_value());
    CHECK(beach->plane_a.width_blocks == 2U);
    CHECK(beach->plane_a.height_blocks == 2U);
    CHECK(beach->plane_a.has_aux_map);
    CHECK(beach->plane_a.width_tiles == 64U);
    CHECK(beach->plane_a.height_tiles == 64U);
    CHECK(beach->plane_a.byte_stride_shift == 7U);
    CHECK(beach->plane_a.word_stride_shift == 6U);
    CHECK(beach->plane_a.primary_map_bytes == 8192U);
    CHECK(beach->plane_a.aux_map_bytes == 4096U);
    CHECK(beach->plane_a.vram_base == kPlaneAVramBase);
    CHECK(beach->plane_a.control_flags == 0x12U);

    CHECK(beach->plane_b.width_tiles == 0U);
    CHECK(beach->plane_b.height_tiles == 0U);
    CHECK(!beach->plane_b.has_aux_map);
    CHECK(beach->plane_b.byte_stride_shift == 0U);
    CHECK(beach->plane_b.word_stride_shift == 0xFFFFU);
    CHECK(beach->plane_b.primary_map_bytes == 0U);
    CHECK(beach->plane_b.aux_map_bytes == 0U);
    CHECK(beach->plane_b.vram_base == kPlaneBVramBase);
    CHECK(beach->plane_b.control_flags == 0x34U);

    CHECK(beach->plane_a_data_addr == 0x00FF3FACU);
    CHECK(beach->plane_a_aux_addr == 0x00FF5FACU);
    CHECK(beach->plane_b_data_addr == 0x00FF6FACU);
    CHECK(beach->plane_b_aux_addr == 0x00FF6FACU);
    CHECK(beach->scene_palette_addr == 0x00FF6FACU);
    CHECK(beach->dynamic_buffer_addr == 0x00FF6FECU);

    // Dual active layout: 02 01 84 02 (Plane A 64x32, Plane B 128x64 + aux)
    const std::array<std::uint8_t, 4> hdr_dual{0x02U, 0x01U, 0x84U, 0x02U};
    const auto dual = unpack_dual_plane_header(hdr_dual, 0x0103U);
    CHECK(dual.has_value());
    CHECK(dual->plane_a.primary_map_bytes == 4096U);
    CHECK(dual->plane_a.aux_map_bytes == 0U);
    CHECK(dual->plane_b.width_tiles == 128U);
    CHECK(dual->plane_b.height_tiles == 64U);
    CHECK(dual->plane_b.byte_stride_shift == 8U);
    CHECK(dual->plane_b.word_stride_shift == 7U);
    CHECK(dual->plane_b.primary_map_bytes == 16384U);
    CHECK(dual->plane_b.aux_map_bytes == 8192U);
    CHECK(dual->scene_palette_addr == 0x00FF3FACU + 4096U + 16384U + 8192U);
    CHECK(dual->dynamic_buffer_addr == dual->scene_palette_addr + 0x40U);
}

void test_camera_scroll_limits() {
    static_assert(compute_camera_scroll_limits(64U, 32U) == CameraScrollLimits{192U, 32U});
    static_assert(compute_camera_scroll_limits(64U, 64U) == CameraScrollLimits{192U, 288U});
    static_assert(compute_camera_scroll_limits(128U, 64U) == CameraScrollLimits{704U, 288U});
    static_assert(compute_camera_scroll_limits(40U, 28U) == CameraScrollLimits{0U, 0U});
    static_assert(compute_camera_scroll_limits(32U, 24U) == CameraScrollLimits{0U, 0U});

    const auto limits = compute_camera_scroll_limits(64U, 64U);
    CHECK(limits.max_x_scroll == 192U);
    CHECK(limits.max_y_scroll == 288U);
}

struct SyntheticRomFixture {
    std::vector<std::uint8_t> rom;
    std::uint32_t stream_addr{0x060000U};
    std::uint32_t expected_pass1_consumed{0U};
    std::uint32_t expected_pass2_consumed{0U};
};

SyntheticRomFixture make_synthetic_rom() {
    SyntheticRomFixture fx{};
    fx.rom.assign(0x061000U, 0x00U);

    // System palette at 0x000476 (15 words = 30 bytes)
    for (std::size_t i = 0U; i < 15U; ++i) {
        write_be16(fx.rom, static_cast<std::size_t>(kSystemPaletteRomAddr) + i * 2U,
                   static_cast<std::uint16_t>(0x0200U + i * 2U));
    }

    // Scene palette table at 0x05D046: populate index 0 and index 3 (32 bytes each)
    for (std::size_t idx : {0U, 3U}) {
        const std::size_t base = static_cast<std::size_t>(kScenePaletteTableRomAddr) + (idx << 5U);
        for (std::size_t w = 0U; w < 16U; ++w) {
            write_be16(fx.rom, base + w * 2U,
                       static_cast<std::uint16_t>(0x0400U + idx * 0x20U + w));
        }
    }

    // Build Format A primary stream for header {0x81, 0x01, 0x02, 0x01}:
    // Plane A: 32x32 tiles + aux -> 2048 primary + 1024 aux = 3072 bytes
    // Plane B: 64x32 tiles       -> 4096 primary + 0 aux    = 4096 bytes
    // Extra palette at $FF179E   -> 64 bytes (32 distinct words)
    std::vector<std::uint8_t> pass1;
    pass1.push_back(0x00U); // span lo placeholder
    pass1.push_back(0x00U); // span hi placeholder
    // Literal 4-byte header
    pass1.push_back(0x04U);
    pass1.push_back(0x81U);
    pass1.push_back(0x01U);
    pass1.push_back(0x02U);
    pass1.push_back(0x01U);
    // Repeat 2048 bytes of 0xAA (extended repeat: 0x57, 0xFC)
    pass1.push_back(0x57U);
    pass1.push_back(0xFCU);
    pass1.push_back(0xAAU);
    // Repeat 1024 bytes of 0xBB (extended repeat: 0x53, 0xFC)
    pass1.push_back(0x53U);
    pass1.push_back(0xFCU);
    pass1.push_back(0xBBU);
    // Repeat 4096 bytes of 0x55 (extended repeat: 0x5F, 0xFC)
    pass1.push_back(0x5FU);
    pass1.push_back(0xFCU);
    pass1.push_back(0x55U);
    // Literal 64 bytes extra palette (0x20, 0x40 + 64 bytes)
    pass1.push_back(0x20U);
    pass1.push_back(0x40U);
    for (std::size_t w = 0U; w < 32U; ++w) {
        const auto color = static_cast<std::uint16_t>(0x0600U + w * 2U);
        pass1.push_back(static_cast<std::uint8_t>((color >> 8U) & 0xFFU));
        pass1.push_back(static_cast<std::uint8_t>(color & 0xFFU));
    }
    const auto span1 = static_cast<std::uint16_t>(pass1.size());
    pass1[0] = static_cast<std::uint8_t>(span1 & 0xFFU);
    pass1[1] = static_cast<std::uint8_t>((span1 >> 8U) & 0xFFU);
    pass1.push_back(0x00U); // terminator

    // Pass 2 overlay stream placed immediately after Pass 1: "OVLAY2" (6 bytes)
    const std::array<std::uint8_t, 10> pass2{
        0x09U, 0x00U, 0x06U, 'O', 'V', 'L', 'A', 'Y', '2', 0x00U};

    fx.expected_pass1_consumed = static_cast<std::uint32_t>(pass1.size());
    fx.expected_pass2_consumed = static_cast<std::uint32_t>(pass2.size());

    std::copy(pass1.begin(), pass1.end(), fx.rom.begin() + static_cast<std::ptrdiff_t>(fx.stream_addr));
    std::copy(pass2.begin(), pass2.end(),
              fx.rom.begin() + static_cast<std::ptrdiff_t>(fx.stream_addr + fx.expected_pass1_consumed));
    return fx;
}

void test_init_dual_plane_layout_passes_and_vdp() {
    const auto fx = make_synthetic_rom();

    // Error cases: OOB stream address & non-compressed garbage
    DualPlaneLayoutConfig bad_cfg{};
    bad_cfg.primary_stream_addr = 0x00FFFFFFU;
    CHECK(!init_dual_plane_layout(fx.rom, bad_cfg).ok);

    bad_cfg.primary_stream_addr = 0x000100U;
    CHECK(!init_dual_plane_layout(fx.rom, bad_cfg).ok);

    // 1. Single-pass layout with Plane A camera limits and Reg 12 standard H40 (0x8C81)
    DualPlaneLayoutConfig cfg{};
    cfg.primary_stream_addr = fx.stream_addr;
    cfg.plane_control_word = 0x0103U;
    cfg.backdrop_color = 0xF5U; // masked to low 3 bits -> 5
    cfg.vdp_reg7_shadow = 0x8723U; // low 3 bits (3) replaced with 5 -> 0x8725
    cfg.scroll_mode_ff16f0 = 0x00U;
    cfg.feature_flags_ff16f1 = 0x00U;
    cfg.fade_backdrop_color_ff16f6 = 0x0ACEU;
    cfg.scene_palette_index_ff16f8 = 3U;

    oasis::genesis::VdpState vdp{};
    std::fill_n(vdp.vram.begin() + static_cast<std::ptrdiff_t>(kVdpHScrollVramBase), 1024U, std::uint8_t{0x99U});
    vdp.vsram.fill(0x77U);

    std::vector<std::uint16_t> streamed_vram_bases;
    const auto res1 = init_dual_plane_layout(
        fx.rom, cfg, &vdp,
        [&](const PlaneContextRecord& ctx, std::span<const std::uint8_t> buf) {
            streamed_vram_bases.push_back(ctx.vram_base);
            CHECK(buf.size() == 4U + 2048U + 1024U + 4096U + 64U);
        });

    CHECK(res1.ok);
    CHECK(streamed_vram_bases.size() == 2U);
    CHECK(streamed_vram_bases[0] == kPlaneAVramBase);
    CHECK(streamed_vram_bases[1] == kPlaneBVramBase);
    CHECK(res1.overlay_buffer_ptr == 0U);
    CHECK(res1.final_rom_stream_ptr == fx.stream_addr + fx.expected_pass1_consumed);
    CHECK(res1.final_ram_alloc_ptr == kTilemapDecompressBufferRamAddr + 7236U);
    CHECK(res1.updated_vdp_reg7_shadow == 0x8725U);
    CHECK(res1.vdp_reg12_command == kVdpReg12ShadowHiOff);
    CHECK(!res1.line_hscroll_cleared);
    CHECK(!res1.col_vscroll_cleared);
    CHECK(res1.vdp_hscroll_vsram_cleared);
    CHECK(res1.camera_limits == (CameraScrollLimits{0U, 32U})); // Plane A 32x32 -> max_x=0, max_y=32
    CHECK(res1.room_transition_active == 0x00U);
    CHECK(res1.plane_a_initialized == 0xFFU);
    CHECK(res1.plane_b_initialized == 0xFFU);

    // Verify CRAM palettes loaded into both result and VdpState
    CHECK(res1.cram_palette[0] == 0x0ACEU);
    CHECK(res1.cram_palette[1] == 0x0200U);
    CHECK(res1.cram_palette[15] == 0x021CU);
    CHECK(res1.cram_palette[16] == 0x0460U);
    CHECK(res1.cram_palette[31] == 0x046FU);
    CHECK(res1.cram_palette[32] == 0x0600U);
    CHECK(res1.cram_palette[63] == 0x063EU);
    CHECK(read_be16(vdp.cram, 0U) == 0x0ACEU);
    CHECK(read_be16(vdp.cram, 32U * 2U) == 0x0600U);
    CHECK(vdp.registers[7] == 0x25U);
    CHECK(vdp.registers[12] == 0x81U);
    for (std::size_t i = 0U; i < 1024U; ++i) {
        CHECK(vdp.vram[kVdpHScrollVramBase + i] == 0U);
    }
    for (const auto b : vdp.vsram) {
        CHECK(b == 0U);
    }

    // 2. Two-pass overlay + Shadow/Highlight + Plane B camera tracking ($FF16F0 bit 7)
    cfg.scroll_mode_ff16f0 = 0x86U; // bit 7 (Plane B camera), bit 2 (col vscroll), bit 1 (line hscroll)
    cfg.feature_flags_ff16f1 = 0x24U; // bit 5 (Shadow/Highlight), bit 2 (2nd pass overlay)
    const auto res2 = init_dual_plane_layout(fx.rom, cfg, &vdp);

    CHECK(res2.ok);
    CHECK(res2.overlay_buffer_ptr == kTilemapDecompressBufferRamAddr + 7236U);
    CHECK(res2.final_rom_stream_ptr == fx.stream_addr + fx.expected_pass1_consumed + fx.expected_pass2_consumed);
    CHECK(res2.final_ram_alloc_ptr == res2.overlay_buffer_ptr + 6U);
    CHECK(res2.decompressed_buffer.size() == 7236U + 6U);
    CHECK(res2.decompressed_buffer[7236U] == 'O');
    CHECK(res2.decompressed_buffer[7241U] == '2');
    CHECK(res2.vdp_reg12_command == kVdpReg12ShadowHiOn);
    CHECK(vdp.registers[12] == 0x89U);
    CHECK(res2.line_hscroll_cleared);
    CHECK(res2.col_vscroll_cleared);
    CHECK(res2.camera_limits == (CameraScrollLimits{192U, 32U})); // Plane B 64x32 -> max_x=192, max_y=32
}

void test_init_dual_plane_layout_from_ram() {
    const auto fx = make_synthetic_rom();
    std::array<std::uint8_t, 0x10000> ram{};

    // Fill scroll tables with sentinel pattern to verify conditional clearing
    std::fill_n(ram.begin() + static_cast<std::ptrdiff_t>(kDualPlaneLineHScrollTableRamOffset),
                1024U, std::uint8_t{0xC3U});
    std::fill_n(ram.begin() + static_cast<std::ptrdiff_t>(kDualPlaneColVScrollTableRamOffset),
                80U, std::uint8_t{0xD4U});
    ram[kDualPlaneLineHScrollTableRamOffset - 1U] = 0xEEU;
    ram[kDualPlaneColVScrollTableRamOffset + 80U] = 0xEFU;

    write_be16(ram, kVdpReg7ShadowRamOffset, 0x8717U);
    ram[kDualPlaneScrollModeRamOffset] = 0x06U; // Clear line hscroll (bit 1) + col vscroll (bit 2)
    ram[kDualPlaneFeatureFlagsRamOffset] = 0x24U; // Overlay (bit 2) + Shadow/Hi (bit 5)
    write_be16(ram, kSceneFadeBackdropColorRamOffset, 0x0444U);
    write_be16(ram, kScenePaletteIndexRamOffset, 3U);
    ram[kRoomTransitionActiveRamOffset] = 0x01U;

    const auto res = init_dual_plane_layout_from_ram(
        fx.rom, ram, fx.stream_addr, 0x1122U, 0x02U);
    CHECK(res.ok);

    // Check Plane A ($FF1716 / $FF1766)
    CHECK(read_be32(ram, kPlaneARamOffset + 0x16U) == 0x00FF3FACU);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x1AU) == 0xC000U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x20U) == 32U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x22U) == 32U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x24U) == 6U);
    CHECK(ram[kPlaneARamOffset + 0x27U] == 0x11U);
    CHECK(read_be32(ram, kPlaneAAuxRamOffset + 0x00U) == 0x00FF3FACU + 2048U);
    CHECK(read_be16(ram, kPlaneAAuxRamOffset + 0x04U) == 32U);
    CHECK(read_be16(ram, kPlaneAAuxRamOffset + 0x06U) == 32U);
    CHECK(read_be16(ram, kPlaneAAuxRamOffset + 0x08U) == 5U);

    // Check Plane B ($FF173E / $FF1770)
    CHECK(read_be32(ram, kPlaneBRamOffset + 0x16U) == 0x00FF3FACU + 3072U);
    CHECK(read_be16(ram, kPlaneBRamOffset + 0x1AU) == 0xE000U);
    CHECK(read_be16(ram, kPlaneBRamOffset + 0x20U) == 64U);
    CHECK(read_be16(ram, kPlaneBRamOffset + 0x22U) == 32U);
    CHECK(read_be16(ram, kPlaneBRamOffset + 0x24U) == 7U);
    CHECK(ram[kPlaneBRamOffset + 0x27U] == 0x22U);
    CHECK(read_be32(ram, kPlaneBAuxRamOffset + 0x00U) == 0x00FF3FACU + 3072U + 4096U);
    CHECK(read_be16(ram, kPlaneBAuxRamOffset + 0x08U) == 6U);

    // Check dynamic buffer pointers, scroll clears, CRAM, VDP Reg 7, camera limits, flags
    CHECK(read_be32(ram, kSceneExtraPalettePtrRamOffset) == 0x00FF3FACU + 7168U);
    CHECK(read_be32(ram, kDynamicBufferBaseRamOffset) == 0x00FF3FACU + 7168U + 0x40U);
    CHECK(read_be32(ram, kOverlayDecompressBufferPtrRamOffset) == kTilemapDecompressBufferRamAddr + 7236U);
    CHECK(read_be32(ram, kFinalSceneRomPtrRamOffset) == fx.stream_addr + fx.expected_pass1_consumed + fx.expected_pass2_consumed);
    CHECK(read_be32(ram, kFinalSceneRamAllocPtrRamOffset) == kTilemapDecompressBufferRamAddr + 7242U);

    CHECK(read_be32(ram, kDualPlaneFullscreenHScrollRamOffset) == 0U);
    CHECK(read_be32(ram, kDualPlaneFullscreenVScrollRamOffset) == 0U);
    CHECK(ram[kDualPlaneLineHScrollTableRamOffset - 1U] == 0xEEU);
    CHECK(ram[kDualPlaneColVScrollTableRamOffset + 80U] == 0xEFU);
    for (std::size_t i = 0U; i < 1024U; ++i) {
        CHECK(ram[kDualPlaneLineHScrollTableRamOffset + i] == 0U);
    }
    for (std::size_t i = 0U; i < 80U; ++i) {
        CHECK(ram[kDualPlaneColVScrollTableRamOffset + i] == 0U);
    }

    CHECK(read_be16(ram, kVdpReg7ShadowRamOffset) == 0x8712U);
    CHECK(read_be16(ram, kTargetFadeColor0RamOffset) == 0x0444U);
    CHECK(read_be16(ram, kSystemPaletteRamOffset) == 0x0200U);
    CHECK(read_be16(ram, kScenePaletteLine1RamOffset) == 0x0460U);
    CHECK(read_be16(ram, kScenePaletteLines2And3RamOffset) == 0x0600U);
    CHECK(read_be16(ram, kMaxCameraXScrollRamOffset) == 0U);
    CHECK(read_be16(ram, kMaxCameraYScrollRamOffset) == 32U);
    CHECK(ram[kRoomTransitionActiveRamOffset] == 0x00U);
    CHECK(ram[kPlaneAInitializedRamOffset] == 0xFFU);
    CHECK(ram[kPlaneBInitializedRamOffset] == 0xFFU);
    CHECK(ram[kTilemapDecompressBufferRamOffset + 7236U] == 'O');
}

void test_real_rom_if_available() {
    const std::array<std::filesystem::path, 3> candidates{
        "local-roms/Beyond Oasis (USA).md",
        "../local-roms/Beyond Oasis (USA).md",
        "../Sega-Thor/local-roms/Beyond Oasis (USA).md",
    };

    std::filesystem::path rom_path;
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate, ec)) {
            rom_path = candidate;
            break;
        }
        ec.clear();
    }
    if (rom_path.empty()) {
        return; // Optional check: CI runs without commercial ROMs
    }

    const auto rom = oasis::Rom::load(rom_path);

    // Town stream (0x213850): header 82 02 00 00 -> 64x64 + aux Plane A, empty Plane B
    DualPlaneLayoutConfig town_cfg{};
    town_cfg.primary_stream_addr = 0x213850U;
    town_cfg.plane_control_word = 0x0100U;
    town_cfg.scene_palette_index_ff16f8 = 0U;
    const auto town = init_dual_plane_layout(rom.bytes(), town_cfg);
    CHECK(town.ok);
    CHECK(town.plane_a.width_tiles == 64U);
    CHECK(town.plane_a.height_tiles == 64U);
    CHECK(town.plane_a.byte_stride_shift == 7U);
    CHECK(town.plane_b.width_tiles == 0U);
    CHECK(town.camera_limits == (CameraScrollLimits{192U, 288U}));
    CHECK(town.cram_palette[1] == 0x0008U);  // First word of ROM 0x000476
    CHECK(town.cram_palette[15] == 0x0EEEU); // Last word of ROM 0x000476
    CHECK(town.cram_palette[17] == 0x0026U); // Word 1 of ROM 0x05D046

    // Spirit Sanctuary stream (0x259F78): header 02 01 82 01 -> 64x32 + 64x32 aux
    DualPlaneLayoutConfig sanct_cfg{};
    sanct_cfg.primary_stream_addr = 0x259F78U;
    sanct_cfg.plane_control_word = 0x0101U;
    const auto sanct = init_dual_plane_layout(rom.bytes(), sanct_cfg);
    CHECK(sanct.ok);
    CHECK(sanct.plane_a.width_tiles == 64U);
    CHECK(sanct.plane_a.height_tiles == 32U);
    CHECK(sanct.plane_b.width_tiles == 64U);
    CHECK(sanct.plane_b.height_tiles == 32U);
    CHECK(sanct.scene_extra_palette_ptr == 0x00FF3FACU + 4096U + 4096U + 2048U);
    CHECK(sanct.camera_limits == (CameraScrollLimits{192U, 32U}));
}

} // namespace

int main() {
    test_constants_and_aliases();
    test_compute_log2_byte_stride();
    test_unpack_dual_plane_header();
    test_camera_scroll_limits();
    test_init_dual_plane_layout_passes_and_vdp();
    test_init_dual_plane_layout_from_ram();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.189 dual plane loader tests succeeded!\n";
    return 0;
}
