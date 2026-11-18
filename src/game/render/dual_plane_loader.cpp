#include "game/render/dual_plane_loader.hpp"

#include "game/graphics_decompress.hpp"

#include <algorithm>

namespace thor::render {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] PlaneHeaderSpec decode_plane_spec(
    std::uint8_t width_byte,
    std::uint8_t height_byte,
    std::uint16_t vram_base,
    std::uint8_t control_flags) noexcept {
    PlaneHeaderSpec spec{};
    spec.width_blocks = static_cast<std::uint8_t>(width_byte & DualPlaneLoaderConfig::kHeaderWidthBlocksMask);
    spec.height_blocks = height_byte;
    spec.has_aux_map = (width_byte & DualPlaneLoaderConfig::kHeaderAuxMapBit) != 0U;

    // 0x00D572..0x00D59E / 0x00D5D6..0x00D604: block counts * 32 -> 8x8 tile dimensions
    spec.width_tiles = static_cast<std::uint16_t>(static_cast<std::uint32_t>(spec.width_blocks) << 5U);
    const auto signed_h_blocks = static_cast<std::int16_t>(static_cast<std::int8_t>(height_byte));
    spec.height_tiles = static_cast<std::uint16_t>(static_cast<std::uint32_t>(signed_h_blocks) << 5U);

    spec.byte_stride_shift = compute_log2_byte_stride(spec.width_tiles);
    spec.word_stride_shift = static_cast<std::uint16_t>(spec.byte_stride_shift - 1U);

    // 0x00D5A8 / 0x00D60E: lsl.w d1, d0; andi.l #$ffff, d0 (16-bit masked primary byte size)
    const std::uint16_t primary_bytes = (spec.byte_stride_shift < 16U)
        ? static_cast<std::uint16_t>(static_cast<std::uint32_t>(spec.height_tiles) << spec.byte_stride_shift)
        : 0U;
    spec.primary_map_bytes = primary_bytes;
    spec.aux_map_bytes = spec.has_aux_map ? static_cast<std::uint32_t>(primary_bytes >> 1U) : 0U;
    spec.vram_base = vram_base;
    spec.control_flags = control_flags;
    return spec;
}

void write_plane_context_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t ctx_offset,
    std::uint32_t aux_offset,
    const PlaneContextRecord& ctx,
    const PlaneAuxContextRecord& aux) noexcept {
    write_be32(ram, static_cast<std::size_t>(ctx_offset) + 0x16U, ctx.tile_data_ptr);
    write_be16(ram, static_cast<std::size_t>(ctx_offset) + 0x1AU, ctx.vram_base);
    write_be16(ram, static_cast<std::size_t>(ctx_offset) + 0x20U, ctx.width_tiles);
    write_be16(ram, static_cast<std::size_t>(ctx_offset) + 0x22U, ctx.height_tiles);
    write_be16(ram, static_cast<std::size_t>(ctx_offset) + 0x24U, ctx.byte_stride_shift);
    ram[static_cast<std::size_t>(ctx_offset) + 0x27U] = ctx.control_flags;

    write_be32(ram, static_cast<std::size_t>(aux_offset) + 0x00U, aux.next_ptr);
    write_be16(ram, static_cast<std::size_t>(aux_offset) + 0x04U, aux.width_tiles);
    write_be16(ram, static_cast<std::size_t>(aux_offset) + 0x06U, aux.height_tiles);
    write_be16(ram, static_cast<std::size_t>(aux_offset) + 0x08U, aux.word_stride_shift);
}

void load_cram_palettes(
    std::span<const std::uint8_t> rom,
    const DualPlaneLayoutConfig& config,
    std::span<const std::uint8_t> decompressed_buffer,
    std::uint32_t scene_palette_offset,
    std::array<std::uint16_t, 64>& out_cram) noexcept {
    // 0x00D722: move.w $ff16f6.l, $ff134c.l
    out_cram[0] = config.fade_backdrop_color_ff16f6;

    // 0x00D70C..0x00D720: copy 30 bytes (15 words) from ROM 0x000476 to $FF134E
    if (kSystemPaletteRomAddr + kSystemPaletteCopyByteCount <= rom.size()) {
        for (std::size_t i = 0U; i < 15U; ++i) {
            out_cram[1U + i] = read_be16(rom, static_cast<std::size_t>(kSystemPaletteRomAddr) + i * 2U);
        }
    }

    // 0x00D72C..0x00D742: copy 32 bytes (16 words) from ROM 0x05D046 + ($FF16F8 << 5) to $FF136C
    const std::size_t scene_pal_rom_off =
        static_cast<std::size_t>(kScenePaletteTableRomAddr) +
        (static_cast<std::size_t>(config.scene_palette_index_ff16f8) << 5U);
    if (scene_pal_rom_off <= rom.size() &&
        rom.size() - scene_pal_rom_off >= kScenePaletteEntryByteCount) {
        for (std::size_t i = 0U; i < 16U; ++i) {
            out_cram[16U + i] = read_be16(rom, scene_pal_rom_off + i * 2U);
        }
    }

    // 0x00D744: bsr.w $cecc -> copy 64 bytes (32 words) from ($FF179E) to $FF138C
    const auto pal_off = static_cast<std::size_t>(scene_palette_offset);
    if (pal_off <= decompressed_buffer.size() &&
        decompressed_buffer.size() - pal_off >= kExtraScenePaletteByteCount) {
        for (std::size_t i = 0U; i < 32U; ++i) {
            out_cram[32U + i] = read_be16(decompressed_buffer, pal_off + i * 2U);
        }
    }
}

void sync_vdp_state(
    const DualPlaneInitResult& res,
    oasis::genesis::VdpState& vdp) noexcept {
    // 0x00D6B2..0x00D6C4: clear 256 longwords (1024 bytes) at VRAM 0xDC00
    std::fill_n(vdp.vram.begin() + static_cast<std::ptrdiff_t>(kVdpHScrollVramBase), 1024U, std::uint8_t{0U});
    // 0x00D6C6..0x00D6D8: clear 20 longwords (80 bytes) in VSRAM
    vdp.vsram.fill(0U);
    // 0x00D6EA..0x00D6F0 & 0x00D752..0x00D762: update VDP Reg 7 & Reg 12
    vdp.registers[7] = static_cast<std::uint8_t>(res.updated_vdp_reg7_shadow & 0xFFU);
    vdp.registers[12] = static_cast<std::uint8_t>(res.vdp_reg12_command & 0xFFU);
    // Sync 64-word CRAM staging buffer
    for (std::size_t i = 0U; i < res.cram_palette.size(); ++i) {
        write_be16(vdp.cram, i * 2U, res.cram_palette[i]);
    }
}

} // namespace

std::optional<DualPlaneHeader> unpack_dual_plane_header(
    std::span<const std::uint8_t> header_bytes,
    std::uint16_t plane_control_word,
    std::uint32_t payload_base_addr) noexcept {
    if (header_bytes.size() < 4U) {
        return std::nullopt;
    }

    // 0x00D5A2: ror.w #8, d6 puts upper byte in Plane A +$27; 0x00D608 restores lower byte for Plane B
    const auto ctrl_a = static_cast<std::uint8_t>((plane_control_word >> 8U) & 0xFFU);
    const auto ctrl_b = static_cast<std::uint8_t>(plane_control_word & 0xFFU);

    DualPlaneHeader hdr{};
    hdr.plane_a = decode_plane_spec(header_bytes[0], header_bytes[1], kPlaneAVramBase, ctrl_a);
    hdr.plane_b = decode_plane_spec(header_bytes[2], header_bytes[3], kPlaneBVramBase, ctrl_b);

    hdr.plane_a_data_addr = payload_base_addr;
    hdr.plane_a_aux_addr = hdr.plane_a_data_addr + hdr.plane_a.primary_map_bytes;
    hdr.plane_b_data_addr = hdr.plane_a_aux_addr + hdr.plane_a.aux_map_bytes;
    hdr.plane_b_aux_addr = hdr.plane_b_data_addr + hdr.plane_b.primary_map_bytes;
    hdr.scene_palette_addr = hdr.plane_b_aux_addr + hdr.plane_b.aux_map_bytes;
    hdr.dynamic_buffer_addr = hdr.scene_palette_addr + 0x40U;
    return hdr;
}

DualPlaneInitResult init_dual_plane_layout(
    std::span<const std::uint8_t> rom,
    const DualPlaneLayoutConfig& config,
    oasis::genesis::VdpState* vdp,
    const PlaneVramStreamCallback& plane_vram_streamer) {
    DualPlaneInitResult res{};
    if (static_cast<std::size_t>(config.primary_stream_addr) >= rom.size()) {
        return res;
    }

    // 0x00D542..0x00D552: decompress primary tilemap package at (a4) into $FF3FA8
    std::vector<std::uint8_t> staging(0x10000U, 0U);
    oasis::game::DecompressResult pass1{};
    try {
        pass1 = oasis::game::decompress_graphics(
            rom.subspan(static_cast<std::size_t>(config.primary_stream_addr)),
            staging);
    } catch (...) {
        return res;
    }
    if (pass1.output_size < 4U) {
        return res;
    }
    staging.resize(pass1.output_size);

    std::size_t rom_cursor = static_cast<std::size_t>(config.primary_stream_addr) + pass1.source_consumed;
    std::size_t ram_cursor = pass1.output_size;

    const std::uint32_t payload_base_addr = config.ram_staging_addr + 4U;
    const auto header_opt = unpack_dual_plane_header(
        std::span<const std::uint8_t>(staging).first(4U),
        config.plane_control_word,
        payload_base_addr);
    if (!header_opt.has_value()) {
        return res;
    }
    res.header = *header_opt;

    // 0x00D560..0x00D62A: populate Plane A and Plane B contexts & aux records
    res.plane_a = {
        res.header.plane_a_data_addr,
        res.header.plane_a.vram_base,
        res.header.plane_a.width_tiles,
        res.header.plane_a.height_tiles,
        res.header.plane_a.byte_stride_shift,
        res.header.plane_a.control_flags,
    };
    res.plane_a_aux = {
        res.header.plane_a_aux_addr,
        res.header.plane_a.width_tiles,
        res.header.plane_a.height_tiles,
        res.header.plane_a.word_stride_shift,
    };
    res.plane_b = {
        res.header.plane_b_data_addr,
        res.header.plane_b.vram_base,
        res.header.plane_b.width_tiles,
        res.header.plane_b.height_tiles,
        res.header.plane_b.byte_stride_shift,
        res.header.plane_b.control_flags,
    };
    res.plane_b_aux = {
        res.header.plane_b_aux_addr,
        res.header.plane_b.width_tiles,
        res.header.plane_b.height_tiles,
        res.header.plane_b.word_stride_shift,
    };

    res.scene_extra_palette_ptr = res.header.scene_palette_addr;
    res.dynamic_buffer_base_ptr = res.header.dynamic_buffer_addr;

    // 0x00D63C..0x00D658: optional 2nd pass overlay decompression when $FF16F1 bit 2 is set
    if ((config.feature_flags_ff16f1 & DualPlaneLoaderConfig::kFeatureOverlayDecompressBit) != 0U) {
        if (rom_cursor >= rom.size()) {
            return res;
        }
        res.overlay_buffer_ptr = config.ram_staging_addr + static_cast<std::uint32_t>(ram_cursor);
        std::vector<std::uint8_t> overlay_scratch(0x10000U, 0U);
        oasis::game::DecompressResult pass2{};
        try {
            pass2 = oasis::game::decompress_graphics(rom.subspan(rom_cursor), overlay_scratch);
        } catch (...) {
            return res;
        }
        staging.insert(
            staging.end(),
            overlay_scratch.begin(),
            overlay_scratch.begin() + static_cast<std::ptrdiff_t>(pass2.output_size));
        rom_cursor += pass2.source_consumed;
        ram_cursor += pass2.output_size;
    }

    res.final_rom_stream_ptr = static_cast<std::uint32_t>(rom_cursor);
    res.final_ram_alloc_ptr = config.ram_staging_addr + static_cast<std::uint32_t>(ram_cursor);

    // 0x00D666..0x00D6D8: scroll clears & VDP Reg 7 backdrop update
    res.line_hscroll_cleared = (config.scroll_mode_ff16f0 & DualPlaneLoaderConfig::kScrollModeLineHScrollBit) != 0U;
    res.col_vscroll_cleared = (config.scroll_mode_ff16f0 & DualPlaneLoaderConfig::kScrollModeColVScrollBit) != 0U;
    res.vdp_hscroll_vsram_cleared = true;

    const auto backdrop_low3 = static_cast<std::uint8_t>(config.backdrop_color & 0x07U);
    const auto reg7_low = static_cast<std::uint8_t>((static_cast<std::uint8_t>(config.vdp_reg7_shadow & 0xFFU) & 0xF8U) | backdrop_low3);
    res.updated_vdp_reg7_shadow = static_cast<std::uint16_t>((config.vdp_reg7_shadow & 0xFF00U) | reg7_low);

    res.vdp_control_writes.push_back(kVdpHScrollWriteCmd);
    res.vdp_control_writes.push_back(kVdpVsramWriteCmd);
    res.vdp_control_writes.push_back(res.updated_vdp_reg7_shadow);

    // 0x00D6F8..0x00D70A: stream Plane A and Plane B to VRAM ($D7C0)
    if (plane_vram_streamer) {
        plane_vram_streamer(res.plane_a, staging);
        plane_vram_streamer(res.plane_b, staging);
    }

    // 0x00D70C..0x00D746: system palette (0x000476), scene palette (0x05D046), extra palette ($CECC)
    const std::uint32_t scene_pal_offset = res.scene_extra_palette_ptr - config.ram_staging_addr;
    load_cram_palettes(rom, config, staging, scene_pal_offset, res.cram_palette);

    // 0x00D748..0x00D762: VDP Reg 12 ($8C89 shadow/highlight vs $8C81 standard H40)
    res.vdp_reg12_command = ((config.feature_flags_ff16f1 & DualPlaneLoaderConfig::kFeatureShadowHighlightBit) != 0U)
        ? kVdpReg12ShadowHiOn
        : kVdpReg12ShadowHiOff;
    res.vdp_control_writes.push_back(res.vdp_reg12_command);

    // 0x00D764..0x00D79B: camera limits from Plane A (or Plane B if $FF16F0 bit 7 is set)
    const PlaneContextRecord& cam_plane =
        ((config.scroll_mode_ff16f0 & DualPlaneLoaderConfig::kScrollModeCameraPlaneBBit) != 0U)
            ? res.plane_b
            : res.plane_a;
    res.camera_limits = compute_camera_scroll_limits(cam_plane.width_tiles, cam_plane.height_tiles);

    // 0x00D79C..0x00D7AE: status flags
    res.room_transition_active = 0x00U;
    res.plane_a_initialized = 0xFFU;
    res.plane_b_initialized = 0xFFU;
    res.decompressed_buffer = std::move(staging);

    if (vdp != nullptr) {
        sync_vdp_state(res, *vdp);
    }

    res.ok = true;
    return res;
}

DualPlaneInitResult init_dual_plane_layout_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t primary_stream_addr,
    std::uint16_t plane_control_word,
    std::uint8_t backdrop_color,
    oasis::genesis::VdpState* vdp,
    const PlaneVramStreamCallback& plane_vram_streamer) {
    DualPlaneLayoutConfig config{};
    config.primary_stream_addr = primary_stream_addr;
    config.plane_control_word = plane_control_word;
    config.backdrop_color = backdrop_color;
    config.vdp_reg7_shadow = read_be16(ram, kVdpReg7ShadowRamOffset);
    config.scroll_mode_ff16f0 = ram[kDualPlaneScrollModeRamOffset];
    config.feature_flags_ff16f1 = ram[kDualPlaneFeatureFlagsRamOffset];
    config.fade_backdrop_color_ff16f6 = read_be16(ram, kSceneFadeBackdropColorRamOffset);
    config.scene_palette_index_ff16f8 = read_be16(ram, kScenePaletteIndexRamOffset);
    config.ram_staging_addr = kTilemapDecompressBufferRamAddr;

    DualPlaneInitResult res = init_dual_plane_layout(rom, config, vdp, plane_vram_streamer);
    if (!res.ok) {
        return res;
    }

    // Write decompressed staging buffer at $FF3FA8 bounded inside 64KB RAM
    const std::size_t max_stage_bytes = ram.size() - static_cast<std::size_t>(kTilemapDecompressBufferRamOffset);
    const std::size_t stage_copy = std::min(res.decompressed_buffer.size(), max_stage_bytes);
    std::copy_n(
        res.decompressed_buffer.begin(),
        stage_copy,
        ram.begin() + static_cast<std::ptrdiff_t>(kTilemapDecompressBufferRamOffset));

    // Write Plane A ($FF1716 / $FF1766) and Plane B ($FF173E / $FF1770)
    write_plane_context_to_ram(ram, kPlaneARamOffset, kPlaneAAuxRamOffset, res.plane_a, res.plane_a_aux);
    write_plane_context_to_ram(ram, kPlaneBRamOffset, kPlaneBAuxRamOffset, res.plane_b, res.plane_b_aux);

    // Dynamic buffer pointers ($FF179E..$FF17AE)
    write_be32(ram, kSceneExtraPalettePtrRamOffset, res.scene_extra_palette_ptr);
    write_be32(ram, kDynamicBufferBaseRamOffset, res.dynamic_buffer_base_ptr);
    if ((config.feature_flags_ff16f1 & DualPlaneLoaderConfig::kFeatureOverlayDecompressBit) != 0U) {
        write_be32(ram, kOverlayDecompressBufferPtrRamOffset, res.overlay_buffer_ptr);
    }
    write_be32(ram, kFinalSceneRomPtrRamOffset, res.final_rom_stream_ptr);
    write_be32(ram, kFinalSceneRamAllocPtrRamOffset, res.final_ram_alloc_ptr);

    // Clear fullscreen H/V scroll and conditional scroll tables
    write_be32(ram, kDualPlaneFullscreenHScrollRamOffset, 0U);
    if (res.line_hscroll_cleared) {
        constexpr std::size_t kLineBytes = DualPlaneLoaderConfig::kLineHScrollLongwords * 4U;
        std::fill_n(
            ram.begin() + static_cast<std::ptrdiff_t>(kDualPlaneLineHScrollTableRamOffset),
            kLineBytes,
            std::uint8_t{0U});
    }
    write_be32(ram, kDualPlaneFullscreenVScrollRamOffset, 0U);
    if (res.col_vscroll_cleared) {
        constexpr std::size_t kColBytes = DualPlaneLoaderConfig::kColVScrollLongwords * 4U;
        std::fill_n(
            ram.begin() + static_cast<std::ptrdiff_t>(kDualPlaneColVScrollTableRamOffset),
            kColBytes,
            std::uint8_t{0U});
    }

    // VDP Reg 7 shadow ($FF0BF8)
    write_be16(ram, kVdpReg7ShadowRamOffset, res.updated_vdp_reg7_shadow);

    // 64-word CRAM palette buffer ($FF134C..$FF13CB)
    for (std::size_t i = 0U; i < res.cram_palette.size(); ++i) {
        write_be16(ram, static_cast<std::size_t>(kTargetFadeColor0RamOffset) + i * 2U, res.cram_palette[i]);
    }

    // Camera max scroll limits ($FF177A, $FF177C)
    write_be16(ram, kMaxCameraXScrollRamOffset, res.camera_limits.max_x_scroll);
    write_be16(ram, kMaxCameraYScrollRamOffset, res.camera_limits.max_y_scroll);

    // Initialization flags ($FF17C1..$FF17C3)
    ram[kRoomTransitionActiveRamOffset] = res.room_transition_active;
    ram[kPlaneAInitializedRamOffset] = res.plane_a_initialized;
    ram[kPlaneBInitializedRamOffset] = res.plane_b_initialized;

    return res;
}

} // namespace thor::render
