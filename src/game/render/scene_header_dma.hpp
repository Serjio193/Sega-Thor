#pragma once

#include "game/render/bg_tile_dma.hpp"
#include "game/render/scroll_integrator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace thor::render {

// ROM entrypoints and boundaries (0x00D406..0x00D542, 316 bytes).
inline constexpr std::uint32_t kLoadSceneHeaderAndInitVramAddr = 0x00D406U;
inline constexpr std::uint32_t kUnpackElevationBandsAddr = 0x00D44AU;
inline constexpr std::uint32_t kClearCameraStructsAddr = 0x00D46CU;
inline constexpr std::uint32_t kInitAlignedCameraCoordsAddr = 0x00D484U;
inline constexpr std::uint32_t kDecompress4BgPatternsAddr = 0x00D4C8U;
inline constexpr std::uint32_t kQueue16kbVramDmaAddr = 0x00D50EU;
inline constexpr std::uint32_t kSceneHeaderDmaEndAddr = 0x00D542U;

// 64KB RAM window ($FF0000..$FFFFFF) offsets; kPlaneARamOffset (0x1716),
// kPlaneBRamOffset (0x173E), kDmaQueuePtrRamOffset (0x1892), and
// kVintSyncFlagsRamOffset (0x164E) are imported from scroll_integrator/bg_tile_dma.
inline constexpr std::uint32_t kSceneScrollModeRamOffset = 0x16F0U;
inline constexpr std::uint32_t kBgmTrackIdRamOffset = 0x16F4U;
inline constexpr std::uint32_t kBgmTempoRamOffset = 0x16F6U;
inline constexpr std::uint32_t kBgmSoundModeRamOffset = 0x16F8U;
inline constexpr std::uint32_t kBgPatternIndicesRamOffset = 0x16FAU;
inline constexpr std::uint32_t kElevationThresholdsRamOffset = 0x16FEU;
inline constexpr std::uint32_t kHeightBandsRamOffset = 0x1706U;
inline constexpr std::uint32_t kSceneStagingRamAddr = 0x00FF3FA8U;
inline constexpr std::uint32_t kSceneStagingRamOffset = 0x3FA8U;

// 16KB master VRAM DMA parameters (0x00D50E..0x00D53C).
inline constexpr std::uint32_t kSceneVramDmaBusSource = 0x007F9FD4U; // $FF3FA8 >> 1
inline constexpr std::uint16_t kSceneVramDmaDest = 0x0000U;
inline constexpr std::uint16_t kSceneVramDmaLengthWords = 0x2000U;   // 8192 words = 16384 bytes
inline constexpr std::size_t kSceneStagingBufferByteSize = 0x4000U;  // 16KB (4 * 4KB)
inline constexpr std::size_t kSceneBgPatternSlotCount = 4U;
inline constexpr std::size_t kSceneBgPatternSlotStride = 0x1000U;    // 4096 bytes per slot

// Elevation, camera struct, and header layout constants.
inline constexpr std::size_t kElevationThresholdCount = 4U;
inline constexpr std::size_t kHeightBandCount = 8U;
inline constexpr std::uint16_t kHeightBandTerminalSentinelU16 = 0xF000U;
inline constexpr std::int16_t kHeightBandTerminalSentinel = static_cast<std::int16_t>(-4096);
inline constexpr std::size_t kCameraStructWordCount = 20U;
inline constexpr std::size_t kCameraStructByteSize = 40U;
inline constexpr std::uint32_t kCameraCoordAlignMask = 0xFFF8FFF8U;
inline constexpr std::size_t kSceneHeaderMinByteSize = 0x1AU; // +0x00..+0x19

/// Unpacked scene script header layout at register a1 (+0x04..+0x19).
struct SceneHeaderConfig {
    std::uint32_t plane_tilemap_ptr{0};            // +0x04(a1)
    std::array<std::uint8_t, 4> bg_patterns{};     // +0x08(a1) -> $FF16FA
    std::array<std::int8_t, 4> elevation_slices{}; // +0x0C(a1)
    std::uint16_t bgm_track{0};                    // +0x10(a1) -> $FF16F4
    std::uint16_t bgm_tempo{0};                    // +0x12(a1) -> $FF16F6
    std::uint16_t bgm_mode{0};                     // +0x14(a1) -> $FF16F8
    std::uint16_t scroll_flags{0};                 // +0x16(a1) -> d6
    std::uint16_t vdp_scroll_mode{0};              // +0x18(a1) -> $FF16F0 (d7 = >> 8)

    [[nodiscard]] constexpr bool operator==(const SceneHeaderConfig&) const noexcept = default;
};

/// Unpacked elevation tables and aligned dual-plane initial coordinates.
struct SceneHeaderState {
    std::array<std::int16_t, kElevationThresholdCount> unpacked_elevation{}; // $FF16FE
    std::array<std::int16_t, kHeightBandCount> height_bands{};               // $FF1706
    std::int16_t cam_a_x{0};
    std::int16_t cam_a_y{0};
    std::int16_t cam_b_x{0};
    std::int16_t cam_b_y{0};

    [[nodiscard]] constexpr bool operator==(const SceneHeaderState&) const noexcept = default;
};

/// Full output summary for load_scene_header_and_init_vram (0x00D406..0x00D542).
struct SceneInitResult {
    SceneHeaderConfig header{};
    SceneHeaderState unpacked{};
    std::uint16_t scroll_flags_d6{0};
    std::uint8_t scroll_mode_high_d7{0};
    std::array<std::size_t, kSceneBgPatternSlotCount> decompressed_slot_bytes{};
    std::size_t total_decompressed_bytes{0};
    bool dma_queued{false};
};

/// Parses a big-endian scene script header (>= 0x1A bytes) into SceneHeaderConfig.
[[nodiscard]] bool parse_scene_header(
    std::span<const std::uint8_t> header_bytes,
    SceneHeaderConfig& out) noexcept;

/// 0x00D44A: Sign-extends 4 slice thresholds (<< 3 -> $FF16FE) and builds 8
/// cumulative/midpoint height bands ($FF1706) terminated with sentinel 0xF000 at index 7.
void unpack_elevation_bands(
    const std::array<std::int8_t, kElevationThresholdCount>& elevation_slices,
    std::array<std::int16_t, kElevationThresholdCount>& unpacked_thresholds,
    std::array<std::int16_t, kHeightBandCount>& height_bands) noexcept;

/// 0x00D46C: Zeroes all fields of Plane A ($FF1716) and Plane B ($FF173E) camera structs.
void clear_camera_structs(
    TilemapCameraState& plane_a,
    TilemapCameraState& plane_b) noexcept;

/// 0x00D46C: Zeroes 20 words (40 bytes) each at $FF1716 and $FF173E in 64KB RAM.
void clear_camera_structs(
    std::span<std::uint8_t, 0x10000> ram) noexcept;

/// 0x00D48A..0x00D4A2: Masks packed (X << 16 | Y) by 0xFFF8FFF8 and clamps negative words to 0.
[[nodiscard]] constexpr std::pair<std::int16_t, std::int16_t> align_packed_camera_coord(
    std::uint32_t packed_coord) noexcept
{
    const std::uint32_t masked = packed_coord & kCameraCoordAlignMask;
    const std::int16_t raw_y = static_cast<std::int16_t>(masked & 0xFFFFU);
    const std::int16_t raw_x = static_cast<std::int16_t>((masked >> 16U) & 0xFFFFU);
    const std::int16_t aligned_x = (raw_x < 0) ? static_cast<std::int16_t>(0) : raw_x;
    const std::int16_t aligned_y = (raw_y < 0) ? static_cast<std::int16_t>(0) : raw_y;
    return {aligned_x, aligned_y};
}

/// 0x00D484: Aligns packed (X, Y) coordinates for Plane A and Plane B into cam_x/cam_y.
void init_aligned_camera_coords(
    TilemapCameraState& plane_a,
    TilemapCameraState& plane_b,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b) noexcept;

/// 0x00D484: Writes aligned Plane A/B cam_x (+0x00) and cam_y (+0x04) into 64KB RAM ($FF1716/$FF173E).
void init_aligned_camera_coords(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b) noexcept;

/// Cleanroom unpacker combining elevation band decoding and initial camera coordinate alignment.
[[nodiscard]] SceneHeaderState unpack_scene_header(
    const SceneHeaderConfig& config,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b) noexcept;

/// 0x00D4C8: Resolves 4 BG pattern indices against table 0x05CE96 (skipping 0) and
/// decompresses each into staging_buffer + i * 0x1000 via 0x003820.
std::array<std::size_t, kSceneBgPatternSlotCount> decompress_4_bg_patterns(
    std::span<const std::uint8_t> rom,
    const std::array<std::uint8_t, kSceneBgPatternSlotCount>& bg_patterns,
    std::span<std::uint8_t> staging_buffer) noexcept;

/// 0x00D50E: Enqueues (0x007F9FD4, 0x0000, 0x2000) DMA command, sets $FF164E bit 1,
/// and optionally invokes vint_callback(vint_sync_flags).
void queue_16kb_vram_dma(
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback = {});

/// 0x00D50E: Writes 8-byte BE descriptor (0x007F9FD4, 0x0000, 0x2000) at ($FF1892),
/// advances $FF1892 by 8, sets $FF164E bit 1, and clears it when clear_vint_sync is true.
bool queue_16kb_vram_dma_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool clear_vint_sync = true) noexcept;

/// 0x00D406: Full structured execution of scene header unpack, elevation LUT,
/// camera init, 4-slot BG tile decompression, and 16KB master VRAM DMA enqueue.
SceneInitResult load_scene_header_and_init_vram(
    std::span<const std::uint8_t> rom,
    const SceneHeaderConfig& header,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b,
    TilemapCameraState& plane_a_cam,
    TilemapCameraState& plane_b_cam,
    std::span<std::uint8_t> staging_buffer,
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback = {});

/// 64KB RAM window helper for 0x00D406 using a pre-parsed SceneHeaderConfig.
SceneInitResult load_scene_header_and_init_vram_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    const SceneHeaderConfig& header,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b,
    bool clear_vint_sync = true) noexcept;

/// 64KB RAM window helper for 0x00D406 parsing raw header bytes (>= 0x1A bytes).
SceneInitResult load_scene_header_and_init_vram_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    std::span<const std::uint8_t> header_bytes,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b,
    bool clear_vint_sync = true) noexcept;

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
