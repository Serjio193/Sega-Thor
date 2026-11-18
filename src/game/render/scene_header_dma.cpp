#include "game/render/scene_header_dma.hpp"

#include <algorithm>
#include <numeric>

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

void write_aligned_coords_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::uint32_t packed_coord) noexcept
{
    const auto [cam_x, cam_y] = align_packed_camera_coord(packed_coord);
    write_be16(ram, plane_ram_offset + 0x00U, static_cast<std::uint16_t>(cam_x));
    write_be16(ram, plane_ram_offset + 0x04U, static_cast<std::uint16_t>(cam_y));
}

[[nodiscard]] SceneInitResult build_scene_init_result(
    const SceneHeaderConfig& header,
    const SceneHeaderState& unpacked,
    const std::array<std::size_t, kSceneBgPatternSlotCount>& slot_bytes,
    bool dma_queued) noexcept
{
    SceneInitResult result{};
    result.header = header;
    result.unpacked = unpacked;
    result.scroll_flags_d6 = header.scroll_flags;
    result.scroll_mode_high_d7 = static_cast<std::uint8_t>(header.vdp_scroll_mode >> 8U);
    result.decompressed_slot_bytes = slot_bytes;
    result.total_decompressed_bytes =
        std::accumulate(slot_bytes.begin(), slot_bytes.end(), std::size_t{0U});
    result.dma_queued = dma_queued;
    return result;
}

} // namespace

bool parse_scene_header(
    std::span<const std::uint8_t> header_bytes,
    SceneHeaderConfig& out) noexcept
{
    if (header_bytes.size() < kSceneHeaderMinByteSize) {
        return false;
    }

    out.plane_tilemap_ptr = read_be32(header_bytes, 0x04U);
    for (std::size_t i = 0U; i < kSceneBgPatternSlotCount; ++i) {
        out.bg_patterns[i] = header_bytes[0x08U + i];
    }
    for (std::size_t i = 0U; i < kElevationThresholdCount; ++i) {
        out.elevation_slices[i] = static_cast<std::int8_t>(header_bytes[0x0CU + i]);
    }
    out.bgm_track = read_be16(header_bytes, 0x10U);
    out.bgm_tempo = read_be16(header_bytes, 0x12U);
    out.bgm_mode = read_be16(header_bytes, 0x14U);
    out.scroll_flags = read_be16(header_bytes, 0x16U);
    out.vdp_scroll_mode = read_be16(header_bytes, 0x18U);
    return true;
}

void unpack_elevation_bands(
    const std::array<std::int8_t, kElevationThresholdCount>& elevation_slices,
    std::array<std::int16_t, kElevationThresholdCount>& unpacked_thresholds,
    std::array<std::int16_t, kHeightBandCount>& height_bands) noexcept
{
    // 0x00D44A..0x00D46A: 4 iterations building thresholds ($FF16FE) and bands ($FF1706)
    std::int16_t cum_elev = 0;
    std::size_t band_idx = 0U;
    for (std::size_t i = 0U; i < kElevationThresholdCount; ++i) {
        const auto threshold = static_cast<std::int16_t>(
            static_cast<std::int16_t>(elevation_slices[i]) * 8);
        unpacked_thresholds[i] = threshold;
        height_bands[band_idx++] = cum_elev;
        height_bands[band_idx++] = static_cast<std::int16_t>(cum_elev + (threshold >> 1));
        cum_elev = static_cast<std::int16_t>(cum_elev + threshold);
    }
    // 0x00D466: move.w #$f000, -$2(a2)
    height_bands[7] = kHeightBandTerminalSentinel;
}

void clear_camera_structs(
    TilemapCameraState& plane_a,
    TilemapCameraState& plane_b) noexcept
{
    // 0x00D46C..0x00D483: zeroes all 20 words (including control_flags) at $FF1716 and $FF173E
    plane_a = TilemapCameraState{};
    plane_a.control_flags = 0U;
    plane_b = TilemapCameraState{};
    plane_b.control_flags = 0U;
}

void clear_camera_structs(
    std::span<std::uint8_t, 0x10000> ram) noexcept
{
    std::fill_n(ram.begin() + kPlaneARamOffset, kCameraStructByteSize, std::uint8_t{0U});
    std::fill_n(ram.begin() + kPlaneBRamOffset, kCameraStructByteSize, std::uint8_t{0U});
}

void init_aligned_camera_coords(
    TilemapCameraState& plane_a,
    TilemapCameraState& plane_b,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b) noexcept
{
    const auto [ax, ay] = align_packed_camera_coord(initial_coord_a);
    plane_a.cam_x = ax;
    plane_a.cam_y = ay;

    const auto [bx, by] = align_packed_camera_coord(initial_coord_b);
    plane_b.cam_x = bx;
    plane_b.cam_y = by;
}

void init_aligned_camera_coords(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b) noexcept
{
    write_aligned_coords_to_ram(ram, kPlaneARamOffset, initial_coord_a);
    write_aligned_coords_to_ram(ram, kPlaneBRamOffset, initial_coord_b);
}

SceneHeaderState unpack_scene_header(
    const SceneHeaderConfig& config,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b) noexcept
{
    SceneHeaderState state{};
    unpack_elevation_bands(config.elevation_slices, state.unpacked_elevation, state.height_bands);
    const auto [ax, ay] = align_packed_camera_coord(initial_coord_a);
    const auto [bx, by] = align_packed_camera_coord(initial_coord_b);
    state.cam_a_x = ax;
    state.cam_a_y = ay;
    state.cam_b_x = bx;
    state.cam_b_y = by;
    return state;
}

std::array<std::size_t, kSceneBgPatternSlotCount> decompress_4_bg_patterns(
    std::span<const std::uint8_t> rom,
    const std::array<std::uint8_t, kSceneBgPatternSlotCount>& bg_patterns,
    std::span<std::uint8_t> staging_buffer) noexcept
{
    std::array<std::size_t, kSceneBgPatternSlotCount> slot_bytes{};
    for (std::size_t i = 0U; i < kSceneBgPatternSlotCount; ++i) {
        // 0x00D4E2..0x00D4E4: move.b (a2)+, d0; beq.b $d4f4 (index 0 skips decompression)
        const std::uint8_t pattern_index = bg_patterns[i];
        if (pattern_index == 0U) {
            continue;
        }

        const std::size_t slot_offset = i * kSceneBgPatternSlotStride;
        if (slot_offset >= staging_buffer.size()) {
            continue;
        }

        const std::size_t slot_cap = std::min(
            kSceneBgPatternSlotStride,
            staging_buffer.size() - slot_offset);
        slot_bytes[i] = decompress_bg_tile_pattern(
            rom,
            static_cast<std::uint16_t>(pattern_index),
            staging_buffer.subspan(slot_offset, slot_cap));
    }
    return slot_bytes;
}

void queue_16kb_vram_dma(
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback)
{
    // 0x00D50E..0x00D522: enqueue ($FF3FA8 >> 1, 0x0000, 0x2000)
    dma_queue.push_back(BgTileDmaCommand{
        kSceneVramDmaBusSource,
        kSceneVramDmaDest,
        kSceneVramDmaLengthWords,
    });
    // 0x00D52C: bset.b #1, $ff164e.l
    vint_sync_flags = static_cast<std::uint8_t>(vint_sync_flags | kBgTileDmaSyncBit);
    // 0x00D534..0x00D53C: await V-Int handler clearing bit 1
    if (vint_callback) {
        vint_callback(vint_sync_flags);
    }
}

bool queue_16kb_vram_dma_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool clear_vint_sync) noexcept
{
    const std::uint32_t queue_ptr = read_be32(ram, kDmaQueuePtrRamOffset);
    const std::size_t queue_offset = static_cast<std::size_t>(queue_ptr & 0xFFFFU);
    if (queue_offset > ram.size() - kDmaQueueEntrySizeBytes) {
        return false;
    }

    write_be32(ram, queue_offset + 0U, kSceneVramDmaBusSource);
    write_be16(ram, queue_offset + 4U, kSceneVramDmaDest);
    write_be16(ram, queue_offset + 6U, kSceneVramDmaLengthWords);
    write_be32(
        ram,
        kDmaQueuePtrRamOffset,
        queue_ptr + static_cast<std::uint32_t>(kDmaQueueEntrySizeBytes));

    ram[kVintSyncFlagsRamOffset] = static_cast<std::uint8_t>(
        ram[kVintSyncFlagsRamOffset] | kBgTileDmaSyncBit);
    if (clear_vint_sync) {
        ram[kVintSyncFlagsRamOffset] = static_cast<std::uint8_t>(
            ram[kVintSyncFlagsRamOffset] & static_cast<std::uint8_t>(~kBgTileDmaSyncBit));
    }
    return true;
}

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
    const VintSyncCallback& vint_callback)
{
    const SceneHeaderState unpacked = unpack_scene_header(header, initial_coord_a, initial_coord_b);
    clear_camera_structs(plane_a_cam, plane_b_cam);
    init_aligned_camera_coords(plane_a_cam, plane_b_cam, initial_coord_a, initial_coord_b);

    const auto slot_bytes = decompress_4_bg_patterns(rom, header.bg_patterns, staging_buffer);
    queue_16kb_vram_dma(dma_queue, vint_sync_flags, vint_callback);
    return build_scene_init_result(header, unpacked, slot_bytes, true);
}

SceneInitResult load_scene_header_and_init_vram_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    const SceneHeaderConfig& header,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b,
    bool clear_vint_sync) noexcept
{
    // 0x00D406..0x00D438: store BGM fields, VDP scroll mode, and 4 BG pattern indices
    write_be16(ram, kBgmTrackIdRamOffset, header.bgm_track);
    write_be16(ram, kBgmTempoRamOffset, header.bgm_tempo);
    write_be16(ram, kBgmSoundModeRamOffset, header.bgm_mode);
    write_be16(ram, kSceneScrollModeRamOffset, header.vdp_scroll_mode);
    for (std::size_t i = 0U; i < kSceneBgPatternSlotCount; ++i) {
        ram[kBgPatternIndicesRamOffset + i] = header.bg_patterns[i];
    }

    // 0x00D44A..0x00D46A: unpack elevation thresholds & height band LUT
    const SceneHeaderState unpacked = unpack_scene_header(header, initial_coord_a, initial_coord_b);
    for (std::size_t i = 0U; i < kElevationThresholdCount; ++i) {
        write_be16(
            ram,
            kElevationThresholdsRamOffset + (i * 2U),
            static_cast<std::uint16_t>(unpacked.unpacked_elevation[i]));
    }
    for (std::size_t i = 0U; i < kHeightBandCount; ++i) {
        write_be16(
            ram,
            kHeightBandsRamOffset + (i * 2U),
            static_cast<std::uint16_t>(unpacked.height_bands[i]));
    }

    // 0x00D46C..0x00D4C6: clear camera structs and write aligned initial X/Y coordinates
    clear_camera_structs(ram);
    init_aligned_camera_coords(ram, initial_coord_a, initial_coord_b);

    // 0x00D4C8..0x00D4FE: decompress up to 4 x 4KB BG patterns into $FF3FA8..$FF7FA7
    const auto slot_bytes = decompress_4_bg_patterns(
        rom,
        header.bg_patterns,
        ram.subspan(kSceneStagingRamOffset, kSceneStagingBufferByteSize));

    // 0x00D50E..0x00D53C: enqueue 16KB master VRAM DMA and sync $FF164E bit 1
    const bool dma_queued = queue_16kb_vram_dma_from_ram(ram, clear_vint_sync);
    return build_scene_init_result(header, unpacked, slot_bytes, dma_queued);
}

SceneInitResult load_scene_header_and_init_vram_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    std::span<const std::uint8_t> header_bytes,
    std::uint32_t initial_coord_a,
    std::uint32_t initial_coord_b,
    bool clear_vint_sync) noexcept
{
    SceneHeaderConfig header{};
    if (!parse_scene_header(header_bytes, header)) {
        return SceneInitResult{};
    }
    return load_scene_header_and_init_vram_from_ram(
        rom,
        ram,
        header,
        initial_coord_a,
        initial_coord_b,
        clear_vint_sync);
}

} // namespace thor::render
