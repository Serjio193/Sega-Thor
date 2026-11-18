#pragma once

#include "game/render/scroll_integrator.hpp"
#include "genesis/vdp_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace thor::render {

inline constexpr std::uint32_t kInitDualPlaneLayoutAddr = 0x00D542U;
inline constexpr std::uint32_t kInitDualPlaneLayoutReturnAddr = 0x00D7AEU;
inline constexpr std::uint32_t kComputeLog2ByteStrideAddr = 0x00D7B0U;
inline constexpr std::uint32_t kComputeLog2ByteStrideReturnAddr = 0x00D7BEU;
inline constexpr std::uint32_t kDualPlaneLoaderEndAddr = 0x00D7C0U;
inline constexpr std::uint32_t kPlaneVramStreamHelperAddr = 0x00D7C0U;
inline constexpr std::uint32_t kCopySceneExtraPaletteAddr = 0x00CECCU;

inline constexpr std::uint16_t kPlaneAVramBase = 0xC000U;
inline constexpr std::uint16_t kPlaneBVramBase = 0xE000U;
inline constexpr std::uint16_t kVdpHScrollVramBase = 0xDC00U;
inline constexpr std::uint32_t kVdpHScrollWriteCmd = 0x5C000003U;
inline constexpr std::uint32_t kVdpVsramWriteCmd = 0x40000010U;
inline constexpr std::uint16_t kVdpReg12ShadowHiOn = 0x8C89U;
inline constexpr std::uint16_t kVdpReg12ShadowHiOff = 0x8C81U;

inline constexpr std::uint32_t kSystemPaletteRomAddr = 0x000476U;
inline constexpr std::uint32_t kScenePaletteTableRomAddr = 0x05D046U;
inline constexpr std::size_t kSystemPaletteCopyByteCount = 30U;   // 15 words -> $FF134E
inline constexpr std::size_t kScenePaletteEntryByteCount = 32U;   // 16 words -> $FF136C
inline constexpr std::size_t kExtraScenePaletteByteCount = 64U;   // 32 words -> $FF138C
inline constexpr std::uint16_t kVdpScreenWidthTiles = 40U;        // 0x28 tiles (320 px)
inline constexpr std::uint16_t kVdpScreenHeightTiles = 28U;       // 0x1C tiles (224 px)

inline constexpr std::uint32_t kTilemapDecompressBufferRamAddr = 0x00FF3FA8U;
inline constexpr std::uint32_t kTilemapDecompressBufferRamOffset = 0x3FA8U;
inline constexpr std::uint32_t kTilemapPayloadRamAddr = 0x00FF3FACU;
inline constexpr std::uint32_t kTilemapPayloadRamOffset = 0x3FACU;

inline constexpr std::uint32_t kPlaneAAuxRamOffset = 0x1766U;
inline constexpr std::uint32_t kPlaneBAuxRamOffset = 0x1770U;
inline constexpr std::uint32_t kMaxCameraXScrollRamOffset = 0x177AU;
inline constexpr std::uint32_t kMaxCameraYScrollRamOffset = 0x177CU;
inline constexpr std::uint32_t kSceneExtraPalettePtrRamOffset = 0x179EU;
inline constexpr std::uint32_t kDynamicBufferBaseRamOffset = 0x17A2U;
inline constexpr std::uint32_t kOverlayDecompressBufferPtrRamOffset = 0x17A6U;
inline constexpr std::uint32_t kFinalSceneRomPtrRamOffset = 0x17AAU;
inline constexpr std::uint32_t kFinalSceneRamAllocPtrRamOffset = 0x17AEU;
inline constexpr std::uint32_t kRoomTransitionActiveRamOffset = 0x17C1U;
inline constexpr std::uint32_t kPlaneAInitializedRamOffset = 0x17C2U;
inline constexpr std::uint32_t kPlaneBInitializedRamOffset = 0x17C3U;

inline constexpr std::uint32_t kVdpReg7ShadowRamOffset = 0x0BF8U;
inline constexpr std::uint32_t kDualPlaneScrollModeRamOffset = 0x16F0U;
inline constexpr std::uint32_t kDualPlaneFeatureFlagsRamOffset = 0x16F1U;
inline constexpr std::uint32_t kSceneFadeBackdropColorRamOffset = 0x16F6U;
inline constexpr std::uint32_t kScenePaletteIndexRamOffset = 0x16F8U;
inline constexpr std::uint32_t kDualPlaneFullscreenHScrollRamOffset = 0x1344U;
inline constexpr std::uint32_t kDualPlaneFullscreenVScrollRamOffset = 0x1348U;
inline constexpr std::uint32_t kTargetFadeColor0RamOffset = 0x134CU;
inline constexpr std::uint32_t kSystemPaletteRamOffset = 0x134EU;
inline constexpr std::uint32_t kScenePaletteLine1RamOffset = 0x136CU;
inline constexpr std::uint32_t kScenePaletteLines2And3RamOffset = 0x138CU;
inline constexpr std::uint32_t kDualPlaneLineHScrollTableRamOffset = 0xFB5CU;
inline constexpr std::uint32_t kDualPlaneColVScrollTableRamOffset = 0xFF5CU;

/// Constants matching 0x00D542..0x00D7C0 RAM layout, flags, and VDP table sizes.
struct DualPlaneLoaderConfig {
    static constexpr std::uint32_t kMegaDriveRamBase = 0x00FF0000U;
    static constexpr std::uint32_t kPlaneAContextRamOffset = kPlaneARamOffset; // 0x1716
    static constexpr std::uint32_t kPlaneBContextRamOffset = kPlaneBRamOffset; // 0x173E
    static constexpr std::size_t kLineHScrollLongwords = 256U;                 // 1024 bytes
    static constexpr std::size_t kColVScrollLongwords = 20U;                   // 80 bytes
    static constexpr std::uint8_t kScrollModeLineHScrollBit = 0x02U;           // $FF16F0 bit 1
    static constexpr std::uint8_t kScrollModeColVScrollBit = 0x04U;            // $FF16F0 bit 2
    static constexpr std::uint8_t kScrollModeCameraPlaneBBit = 0x80U;          // $FF16F0 bit 7
    static constexpr std::uint8_t kFeatureOverlayDecompressBit = 0x04U;        // $FF16F1 bit 2
    static constexpr std::uint8_t kFeatureShadowHighlightBit = 0x20U;          // $FF16F1 bit 5
    static constexpr std::uint8_t kHeaderAuxMapBit = 0x80U;                    // header[0/2] bit 7
    static constexpr std::uint8_t kHeaderWidthBlocksMask = 0x7FU;
};

/// 0x00D7B0..0x00D7BE: Computes log2(width_tiles * 2), matching `add.w d0, d0` followed
/// by `lsr.w #1, d0` / `bcs` counting trailing zero bits (e.g. 32->6, 64->7, 128->8).
[[nodiscard]] constexpr std::uint16_t compute_log2_byte_stride(
    std::uint16_t width_tiles) noexcept {
    std::uint16_t d0 = static_cast<std::uint16_t>(width_tiles + width_tiles);
    if (d0 == 0U) {
        return 0U;
    }
    std::uint16_t d1 = 0U;
    while ((d0 & 1U) == 0U) {
        d0 = static_cast<std::uint16_t>(d0 >> 1U);
        ++d1;
    }
    return d1;
}

/// Unpacked metadata for a single tilemap plane from the 4-byte header at $FF3FA8.
struct PlaneHeaderSpec {
    std::uint8_t width_blocks{0U};       // 32-tile block count (header[0/2] & 0x7F)
    std::uint8_t height_blocks{0U};      // 32-tile block count (header[1/3])
    bool has_aux_map{false};             // Bit 7 of header[0/2] (1B/tile collision/aux map)
    std::uint16_t width_tiles{0U};       // +$20: width in 8x8 tiles (blocks << 5)
    std::uint16_t height_tiles{0U};      // +$22: height in 8x8 tiles (int8_t(blocks) << 5)
    std::uint16_t byte_stride_shift{0U}; // +$24: log2(width_tiles * 2)
    std::uint16_t word_stride_shift{0U}; // Aux +$08: byte_stride_shift - 1
    std::uint32_t primary_map_bytes{0U}; // (height_tiles << byte_stride_shift) & 0xFFFF
    std::uint32_t aux_map_bytes{0U};     // has_aux_map ? (primary_map_bytes >> 1) : 0
    std::uint16_t vram_base{0U};         // +$1A: 0xC000 (Plane A) or 0xE000 (Plane B)
    std::uint8_t control_flags{0U};      // +$27: scroll flags (upper/lower byte of d6)

    [[nodiscard]] constexpr std::uint32_t total_payload_bytes() const noexcept {
        return primary_map_bytes + aux_map_bytes;
    }

    [[nodiscard]] constexpr bool operator==(const PlaneHeaderSpec&) const noexcept = default;
};

/// Unpacked 4-byte dual tilemap header ($FF3FA8..$FF3FAB) with layout pointers.
struct DualPlaneHeader {
    PlaneHeaderSpec plane_a{};
    PlaneHeaderSpec plane_b{};
    std::uint32_t plane_a_data_addr{kTilemapPayloadRamAddr};
    std::uint32_t plane_a_aux_addr{0U};
    std::uint32_t plane_b_data_addr{0U};
    std::uint32_t plane_b_aux_addr{0U};
    std::uint32_t scene_palette_addr{0U};   // $FF179E
    std::uint32_t dynamic_buffer_addr{0U};  // $FF17A2 ($FF179E + 0x40)

    [[nodiscard]] constexpr bool operator==(const DualPlaneHeader&) const noexcept = default;
};

/// 40-byte plane context fields populated at $FF1716 (Plane A) and $FF173E (Plane B).
struct PlaneContextRecord {
    std::uint32_t tile_data_ptr{0U};     // +$16
    std::uint16_t vram_base{0U};         // +$1A
    std::uint16_t width_tiles{0U};       // +$20
    std::uint16_t height_tiles{0U};      // +$22
    std::uint16_t byte_stride_shift{0U}; // +$24
    std::uint8_t control_flags{0U};      // +$27

    [[nodiscard]] constexpr bool operator==(const PlaneContextRecord&) const noexcept = default;
};

/// 10-byte auxiliary plane context fields populated at $FF1766 (Plane A) and $FF1770 (Plane B).
struct PlaneAuxContextRecord {
    std::uint32_t next_ptr{0U};          // +$00
    std::uint16_t width_tiles{0U};       // +$04
    std::uint16_t height_tiles{0U};      // +$06
    std::uint16_t word_stride_shift{0U}; // +$08

    [[nodiscard]] constexpr bool operator==(const PlaneAuxContextRecord&) const noexcept = default;
};

/// Camera maximum scroll bounds in pixels stored at $FF177A and $FF177C.
struct CameraScrollLimits {
    std::uint16_t max_x_scroll{0U}; // ($20(a0) - 40) * 8, clamped >= 0
    std::uint16_t max_y_scroll{0U}; // ($22(a0) - 28) * 8, clamped >= 0

    [[nodiscard]] constexpr bool operator==(const CameraScrollLimits&) const noexcept = default;
};

/// Inputs matching ROM 0x00D542..0x00D7AE entry conditions.
struct DualPlaneLayoutConfig {
    std::uint32_t primary_stream_addr{0U};                      // (a4) compressed ROM address
    std::uint16_t plane_control_word{0U};                       // d6: hi=Plane A +$27, lo=Plane B +$27
    std::uint8_t backdrop_color{0U};                            // d7 & 0x07 -> VDP Reg 7 low 3 bits
    std::uint16_t vdp_reg7_shadow{0x8700U};                     // $FF0BF8 initial word
    std::uint8_t scroll_mode_ff16f0{0U};                        // $FF16F0 scroll & camera track bits
    std::uint8_t feature_flags_ff16f1{0U};                      // $FF16F1 overlay & shadow/hi bits
    std::uint16_t fade_backdrop_color_ff16f6{0U};               // $FF16F6 -> $FF134C
    std::uint16_t scene_palette_index_ff16f8{0U};               // $FF16F8 -> 0x05D046 table index
    std::uint32_t ram_staging_addr{kTilemapDecompressBufferRamAddr}; // $FF3FA8 base address
};

/// Full cleanroom outputs produced by 0x00D542..0x00D7AE.
struct DualPlaneInitResult {
    bool ok{false};
    DualPlaneHeader header{};
    PlaneContextRecord plane_a{};
    PlaneAuxContextRecord plane_a_aux{};
    PlaneContextRecord plane_b{};
    PlaneAuxContextRecord plane_b_aux{};
    std::uint32_t scene_extra_palette_ptr{0U};  // $FF179E
    std::uint32_t dynamic_buffer_base_ptr{0U};  // $FF17A2
    std::uint32_t overlay_buffer_ptr{0U};       // $FF17A6 (0 if bit 2 of $FF16F1 clear)
    std::uint32_t final_rom_stream_ptr{0U};     // $FF17AA
    std::uint32_t final_ram_alloc_ptr{0U};      // $FF17AE
    std::uint16_t updated_vdp_reg7_shadow{0U};  // $FF0BF8
    std::uint16_t vdp_reg12_command{kVdpReg12ShadowHiOff};
    bool line_hscroll_cleared{false};
    bool col_vscroll_cleared{false};
    bool vdp_hscroll_vsram_cleared{false};
    std::array<std::uint16_t, 64> cram_palette{}; // $FF134C..$FF13CB
    CameraScrollLimits camera_limits{};           // $FF177A, $FF177C
    std::uint8_t room_transition_active{0U};      // $FF17C1 (sf.b -> 0x00)
    std::uint8_t plane_a_initialized{0xFFU};      // $FF17C2 (st.b -> 0xFF)
    std::uint8_t plane_b_initialized{0xFFU};      // $FF17C3 (st.b -> 0xFF)
    std::vector<std::uint8_t> decompressed_buffer{};
    std::vector<std::uint32_t> vdp_control_writes{};
};

/// Optional callback modeling `bsr.w $d7c0` plane VRAM upload at 0x00D6FE and 0x00D708.
using PlaneVramStreamCallback = std::function<void(
    const PlaneContextRecord& plane_ctx,
    std::span<const std::uint8_t> staging_buffer)>;

/// Unpacks the 4-byte dual tilemap header ($FF3FA8) and computes Plane A/B layout pointers.
/// Returns std::nullopt if fewer than 4 header bytes are provided.
[[nodiscard]] std::optional<DualPlaneHeader> unpack_dual_plane_header(
    std::span<const std::uint8_t> header_bytes,
    std::uint16_t plane_control_word = 0U,
    std::uint32_t payload_base_addr = kTilemapPayloadRamAddr) noexcept;

/// Models 0x00D77C..0x00D796: computes `(width_tiles - 40) * 8` and `(height_tiles - 28) * 8`,
/// clamped at 0 minimum for planes narrower/shorter than the 40x28-tile H40/V28 viewport.
[[nodiscard]] constexpr CameraScrollLimits compute_camera_scroll_limits(
    std::uint16_t width_tiles,
    std::uint16_t height_tiles) noexcept {
    const std::uint16_t max_x = (width_tiles > kVdpScreenWidthTiles)
        ? static_cast<std::uint16_t>((width_tiles - kVdpScreenWidthTiles) << 3U)
        : 0U;
    const std::uint16_t max_y = (height_tiles > kVdpScreenHeightTiles)
        ? static_cast<std::uint16_t>((height_tiles - kVdpScreenHeightTiles) << 3U)
        : 0U;
    return {max_x, max_y};
}

/// Models 0x00D542..0x00D7AE: decompresses primary tilemap package (and optional overlay pass),
/// unpacks Plane A/B context records, clears scroll tables/VDP scroll state, loads system &
/// scene CRAM palettes, sets VDP Reg 7 & Reg 12, and computes camera scroll limits.
[[nodiscard]] DualPlaneInitResult init_dual_plane_layout(
    std::span<const std::uint8_t> rom,
    const DualPlaneLayoutConfig& config,
    oasis::genesis::VdpState* vdp = nullptr,
    const PlaneVramStreamCallback& plane_vram_streamer = {});

/// 64KB RAM window helper modeling 0x00D542..0x00D7AE against Work RAM ($FF0000..$FFFFFF):
/// reads $FF0BF8, $FF16F0, $FF16F1, $FF16F6, $FF16F8 from ram, runs initialization, and writes
/// decompressed staging data, Plane A/B contexts, aux records, scroll clears, CRAM shadow,
/// camera bounds, and status flags safely within the 64KB RAM span.
[[nodiscard]] DualPlaneInitResult init_dual_plane_layout_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t primary_stream_addr,
    std::uint16_t plane_control_word = 0U,
    std::uint8_t backdrop_color = 0U,
    oasis::genesis::VdpState* vdp = nullptr,
    const PlaneVramStreamCallback& plane_vram_streamer = {});

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
