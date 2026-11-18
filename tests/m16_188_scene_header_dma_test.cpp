#include "game/render/scene_header_dma.hpp"
#include "../include/game/render/scene_header_dma.hpp"

#include "core/rom.hpp"
#include "core/sha256.hpp"
#include "game/resource_loader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace oasis::game::render;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.188 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

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

std::vector<std::uint8_t> make_synthetic_rom_with_4k_patterns() {
    // Format A stream generating 4096 bytes of a chosen fill byte (prefix 0x11, 0x22 + 4094 * fill):
    // span = 2 + 3 + 3 = 8 (0x0008), 2 literal bytes + extended repeat (0x0FFA + 4 = 4094), terminator 0x00.
    const auto make_stream = [](std::uint8_t head0, std::uint8_t head1, std::uint8_t fill) {
        return std::array<std::uint8_t, 9>{
            0x08U, 0x00U,
            0x02U, head0, head1,
            0x5FU, 0xFAU, fill,
            0x00U};
    };

    // Short Format A stream producing 7 bytes ("ABCABCA").
    const std::array<std::uint8_t, 9> stream_short{
        0x08U, 0x00U, 0x03U, 'A', 'B', 'C', 0x80U, 0x03U, 0x00U};

    constexpr std::uint32_t kAddr1 = 0x060000U;
    constexpr std::uint32_t kAddr2 = 0x060100U;
    constexpr std::uint32_t kAddr3 = 0x060200U;
    constexpr std::uint32_t kAddr4 = 0x060300U;
    constexpr std::size_t kRomSize = 0x060400U;

    std::vector<std::uint8_t> rom(kRomSize, 0x00U);
    write_be32(rom, kBgTilePatternTableAddr + 0U * 4U, 0x00000000U); // Index 0: null / skipped
    write_be32(rom, kBgTilePatternTableAddr + 1U * 4U, kAddr1);
    write_be32(rom, kBgTilePatternTableAddr + 2U * 4U, kAddr2);
    write_be32(rom, kBgTilePatternTableAddr + 3U * 4U, kAddr3);
    write_be32(rom, kBgTilePatternTableAddr + 4U * 4U, kAddr4);

    const auto s1 = make_stream(0x11U, 0x22U, 0xA1U);
    const auto s2 = make_stream(0x33U, 0x44U, 0xB2U);
    const auto s4 = make_stream(0x55U, 0x66U, 0xD4U);
    std::copy(s1.begin(), s1.end(), rom.begin() + kAddr1);
    std::copy(s2.begin(), s2.end(), rom.begin() + kAddr2);
    std::copy(stream_short.begin(), stream_short.end(), rom.begin() + kAddr3);
    std::copy(s4.begin(), s4.end(), rom.begin() + kAddr4);
    return rom;
}

void test_constants_and_header_parser() {
    static_assert(kLoadSceneHeaderAndInitVramAddr == 0x00D406U);
    static_assert(kUnpackElevationBandsAddr == 0x00D44AU);
    static_assert(kClearCameraStructsAddr == 0x00D46CU);
    static_assert(kInitAlignedCameraCoordsAddr == 0x00D484U);
    static_assert(kDecompress4BgPatternsAddr == 0x00D4C8U);
    static_assert(kQueue16kbVramDmaAddr == 0x00D50EU);
    static_assert(kSceneHeaderDmaEndAddr == 0x00D542U);

    static_assert(kSceneScrollModeRamOffset == 0x16F0U);
    static_assert(kBgmTrackIdRamOffset == 0x16F4U);
    static_assert(kBgmTempoRamOffset == 0x16F6U);
    static_assert(kBgmSoundModeRamOffset == 0x16F8U);
    static_assert(kBgPatternIndicesRamOffset == 0x16FAU);
    static_assert(kElevationThresholdsRamOffset == 0x16FEU);
    static_assert(kHeightBandsRamOffset == 0x1706U);
    static_assert(kPlaneARamOffset == 0x1716U);
    static_assert(kPlaneBRamOffset == 0x173EU);
    static_assert(kSceneStagingRamAddr == 0x00FF3FA8U);
    static_assert(kSceneStagingRamOffset == 0x3FA8U);
    static_assert((kSceneStagingRamAddr >> 1U) == kSceneVramDmaBusSource);
    static_assert(kSceneVramDmaBusSource == 0x007F9FD4U);
    static_assert(kSceneVramDmaDest == 0x0000U);
    static_assert(kSceneVramDmaLengthWords == 0x2000U);
    static_assert(kDmaQueuePtrRamOffset == 0x1892U);
    static_assert(kVintSyncFlagsRamOffset == 0x164EU);

    // Parse valid 26-byte big-endian header and reject short span.
    std::array<std::uint8_t, kSceneHeaderMinByteSize> raw{};
    write_be32(raw, 0x04U, 0x001E7236U);
    raw[0x08U] = 1U;
    raw[0x09U] = 2U;
    raw[0x0AU] = 0U;
    raw[0x0BU] = 4U;
    raw[0x0CU] = 8U;
    raw[0x0DU] = 8U;
    raw[0x0EU] = 8U;
    raw[0x0FU] = 0xFFU; // -1 signed
    write_be16(raw, 0x10U, 0x0106U);
    write_be16(raw, 0x12U, 0x04CAU);
    write_be16(raw, 0x14U, 0x0004U);
    write_be16(raw, 0x16U, 0x0100U);
    write_be16(raw, 0x18U, 0x2040U);

    SceneHeaderConfig cfg{};
    CHECK(!parse_scene_header(std::span<const std::uint8_t>(raw).first(kSceneHeaderMinByteSize - 1U), cfg));
    CHECK(parse_scene_header(raw, cfg));
    CHECK(cfg.plane_tilemap_ptr == 0x001E7236U);
    CHECK((cfg.bg_patterns == std::array<std::uint8_t, 4>{1U, 2U, 0U, 4U}));
    CHECK((cfg.elevation_slices == std::array<std::int8_t, 4>{8, 8, 8, -1}));
    CHECK(cfg.bgm_track == 0x0106U);
    CHECK(cfg.bgm_tempo == 0x04CAU);
    CHECK(cfg.bgm_mode == 0x0004U);
    CHECK(cfg.scroll_flags == 0x0100U);
    CHECK(cfg.vdp_scroll_mode == 0x2040U);
}

void test_unpack_elevation_bands() {
    std::array<std::int16_t, 4> thresholds{};
    std::array<std::int16_t, 8> bands{};

    // Positive + negative slices: [8, 4, -2, 10]
    // Thresholds (<< 3): [64, 32, -16, 80]
    // Bands: [0, 32, 64, 80, 96, 88, 80, -4096 (0xF000)]
    unpack_elevation_bands({8, 4, -2, 10}, thresholds, bands);
    CHECK((thresholds == std::array<std::int16_t, 4>{64, 32, -16, 80}));
    CHECK((bands == std::array<std::int16_t, 8>{0, 32, 64, 80, 96, 88, 80, kHeightBandTerminalSentinel}));
    CHECK(static_cast<std::uint16_t>(bands[7]) == kHeightBandTerminalSentinelU16);
}

void test_clear_and_align_camera_coords() {
    // Packed (X << 16 | Y) masked by 0xFFF8FFF8, negative clamped to 0.
    static_assert(align_packed_camera_coord(0x0067004DU) == std::pair<std::int16_t, std::int16_t>{96, 72});
    static_assert(align_packed_camera_coord(0xFFF0002BU) == std::pair<std::int16_t, std::int16_t>{0, 40});
    static_assert(align_packed_camera_coord(0x0037FFF1U) == std::pair<std::int16_t, std::int16_t>{48, 0});
    static_assert(align_packed_camera_coord(0x80038007U) == std::pair<std::int16_t, std::int16_t>{0, 0});
    static_assert(align_packed_camera_coord(0x7FFF7FFFU) == std::pair<std::int16_t, std::int16_t>{32760, 32760});

    TilemapCameraState plane_a{};
    plane_a.cam_x = 123;
    plane_a.vdp_hscroll = 456;
    plane_a.control_flags = 0xFFU;
    TilemapCameraState plane_b = plane_a;

    clear_camera_structs(plane_a, plane_b);
    CHECK(plane_a.cam_x == 0);
    CHECK(plane_a.vdp_hscroll == 0U);
    CHECK(plane_a.control_flags == 0U);
    CHECK(plane_b.control_flags == 0U);

    init_aligned_camera_coords(plane_a, plane_b, 0x012B006FU, 0xFFF4001AU);
    CHECK(plane_a.cam_x == 296); // 0x012B & 0xFFF8 = 0x0128 = 296
    CHECK(plane_a.cam_y == 104); // 0x006F & 0xFFF8 = 0x0068 = 104
    CHECK(plane_b.cam_x == 0);   // -12 clamped to 0
    CHECK(plane_b.cam_y == 24);  // 0x001A & 0xFFF8 = 24

    // Cleanroom unpack_scene_header wrapper matches.
    SceneHeaderConfig cfg{};
    cfg.elevation_slices = {8, 8, 8, -1};
    const SceneHeaderState unpacked = unpack_scene_header(cfg, 0x012B006FU, 0xFFF4001AU);
    CHECK((unpacked.unpacked_elevation == std::array<std::int16_t, 4>{64, 64, 64, -8}));
    CHECK((unpacked.height_bands == std::array<std::int16_t, 8>{0, 32, 64, 96, 128, 160, 192, -4096}));
    CHECK(unpacked.cam_a_x == 296);
    CHECK(unpacked.cam_a_y == 104);
    CHECK(unpacked.cam_b_x == 0);
    CHECK(unpacked.cam_b_y == 24);
}

void test_decompress_4_bg_patterns_and_dma() {
    const auto rom = make_synthetic_rom_with_4k_patterns();
    std::vector<std::uint8_t> staging(kSceneStagingBufferByteSize, 0xEEU);

    // Pattern 0 in slot 2 must be skipped (leaves 0xEE sentinel untouched in slot 2).
    const auto slot_bytes = decompress_4_bg_patterns(rom, {1U, 2U, 0U, 4U}, staging);
    CHECK((slot_bytes == std::array<std::size_t, 4>{4096U, 4096U, 0U, 4096U}));
    CHECK(staging[0U] == 0x11U);
    CHECK(staging[1U] == 0x22U);
    CHECK(staging[2U] == 0xA1U);
    CHECK(staging[4095U] == 0xA1U);
    CHECK(staging[0x1000U + 0U] == 0x33U);
    CHECK(staging[0x1000U + 4095U] == 0xB2U);
    CHECK(staging[0x2000U + 0U] == 0xEEU);
    CHECK(staging[0x2000U + 4095U] == 0xEEU);
    CHECK(staging[0x3000U + 0U] == 0x55U);
    CHECK(staging[0x3000U + 4095U] == 0xD4U);

    // Short staging buffer (only 1 slot) handles slots 1..3 safely without OOB access.
    std::vector<std::uint8_t> one_slot(kSceneBgPatternSlotStride, 0U);
    const auto partial_bytes = decompress_4_bg_patterns(rom, {3U, 1U, 2U, 4U}, one_slot);
    CHECK((partial_bytes == std::array<std::size_t, 4>{7U, 0U, 0U, 0U}));
    CHECK(one_slot[0] == 'A');
    CHECK(one_slot[6] == 'A');

    // Structured load_scene_header_and_init_vram with V-Int sync callback.
    SceneHeaderConfig cfg{};
    cfg.bg_patterns = {1U, 0U, 3U, 4U};
    cfg.elevation_slices = {4, 4, 4, 4};
    cfg.scroll_flags = 0x1234U;
    cfg.vdp_scroll_mode = 0x8302U;

    TilemapCameraState plane_a{};
    TilemapCameraState plane_b{};
    plane_a.control_flags = 1U;
    staging.assign(kSceneStagingBufferByteSize, 0xCCU);
    std::vector<BgTileDmaCommand> queue;
    std::uint8_t vint_flags = 0x10U;
    std::uint8_t observed_vint_flags = 0U;

    const SceneInitResult res = load_scene_header_and_init_vram(
        rom,
        cfg,
        0x00450027U, // -> (64, 32)
        0x00130009U, // -> (16, 8)
        plane_a,
        plane_b,
        staging,
        queue,
        vint_flags,
        [&](std::uint8_t& flags) {
            observed_vint_flags = flags;
            flags = static_cast<std::uint8_t>(flags & ~kBgTileDmaSyncBit);
        });

    CHECK(res.dma_queued);
    CHECK(res.scroll_flags_d6 == 0x1234U);
    CHECK(res.scroll_mode_high_d7 == 0x83U);
    CHECK(res.total_decompressed_bytes == (4096U + 7U + 4096U));
    CHECK(plane_a.cam_x == 64);
    CHECK(plane_a.cam_y == 32);
    CHECK(plane_a.control_flags == 0U);
    CHECK(plane_b.cam_x == 16);
    CHECK(plane_b.cam_y == 8);
    CHECK(queue.size() == 1U);
    CHECK(queue[0] == (BgTileDmaCommand{kSceneVramDmaBusSource, kSceneVramDmaDest, kSceneVramDmaLengthWords}));
    CHECK(observed_vint_flags == 0x12U);
    CHECK(vint_flags == 0x10U);
}

void test_ram_window_roundtrip() {
    const auto rom = make_synthetic_rom_with_4k_patterns();
    std::array<std::uint8_t, 0x10000> ram{};

    // Pre-fill camera structs and guard bytes around $FF3FA8..$FF7FA7.
    std::fill_n(ram.begin() + kPlaneARamOffset, kCameraStructByteSize * 2U, std::uint8_t{0xAAU});
    ram[kSceneStagingRamOffset - 1U] = 0x7EU;
    ram[kSceneStagingRamOffset + kSceneStagingBufferByteSize] = 0x7FU;
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FF10B4U);
    ram[kVintSyncFlagsRamOffset] = 0x40U;

    std::array<std::uint8_t, kSceneHeaderMinByteSize> raw_header{};
    write_be32(raw_header, 0x04U, 0x001F0000U);
    raw_header[0x08U] = 1U;
    raw_header[0x09U] = 2U;
    raw_header[0x0AU] = 3U;
    raw_header[0x0BU] = 4U;
    raw_header[0x0CU] = 6U;
    raw_header[0x0DU] = 6U;
    raw_header[0x0EU] = 6U;
    raw_header[0x0FU] = 6U;
    write_be16(raw_header, 0x10U, 0x0012U);
    write_be16(raw_header, 0x12U, 0x0034U);
    write_be16(raw_header, 0x14U, 0x0056U);
    write_be16(raw_header, 0x16U, 0x0078U);
    write_be16(raw_header, 0x18U, 0x2300U);

    const SceneInitResult res = load_scene_header_and_init_vram_from_ram(
        rom,
        ram,
        raw_header,
        0x008F005BU, // -> (136, 88)
        0xFFF20017U, // -> (0, 16)
        false);

    CHECK(res.dma_queued);
    CHECK(res.scroll_mode_high_d7 == 0x23U);
    CHECK(res.total_decompressed_bytes == (4096U + 4096U + 7U + 4096U));

    // Verify RAM scalar fields & BG pattern index bytes.
    CHECK(read_be16(ram, kSceneScrollModeRamOffset) == 0x2300U);
    CHECK(read_be16(ram, kBgmTrackIdRamOffset) == 0x0012U);
    CHECK(read_be16(ram, kBgmTempoRamOffset) == 0x0034U);
    CHECK(read_be16(ram, kBgmSoundModeRamOffset) == 0x0056U);
    CHECK(ram[kBgPatternIndicesRamOffset + 0U] == 1U);
    CHECK(ram[kBgPatternIndicesRamOffset + 1U] == 2U);
    CHECK(ram[kBgPatternIndicesRamOffset + 2U] == 3U);
    CHECK(ram[kBgPatternIndicesRamOffset + 3U] == 4U);

    // Verify unpacked elevation thresholds ($FF16FE) and height bands ($FF1706).
    for (std::size_t i = 0U; i < 4U; ++i) {
        CHECK(read_be16(ram, kElevationThresholdsRamOffset + i * 2U) == 48U);
    }
    CHECK(read_be16(ram, kHeightBandsRamOffset + 0U) == 0U);
    CHECK(read_be16(ram, kHeightBandsRamOffset + 2U) == 24U);
    CHECK(read_be16(ram, kHeightBandsRamOffset + 12U) == 144U);
    CHECK(read_be16(ram, kHeightBandsRamOffset + 14U) == kHeightBandTerminalSentinelU16);

    // Verify camera structs were zeroed except cam_x (+0x00) and cam_y (+0x04).
    CHECK(static_cast<std::int16_t>(read_be16(ram, kPlaneARamOffset + 0x00U)) == 136);
    CHECK(static_cast<std::int16_t>(read_be16(ram, kPlaneARamOffset + 0x04U)) == 88);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x0EU) == 0U);
    CHECK(ram[kPlaneARamOffset + 0x27U] == 0U);
    CHECK(static_cast<std::int16_t>(read_be16(ram, kPlaneBRamOffset + 0x00U)) == 0);
    CHECK(static_cast<std::int16_t>(read_be16(ram, kPlaneBRamOffset + 0x04U)) == 16);
    CHECK(ram[kPlaneBRamOffset + 0x27U] == 0U);

    // Verify staging guard bytes untouched and 16KB DMA descriptor written at $FF10B4.
    CHECK(ram[kSceneStagingRamOffset - 1U] == 0x7EU);
    CHECK(ram[kSceneStagingRamOffset + kSceneStagingBufferByteSize] == 0x7FU);
    CHECK(ram[kSceneStagingRamOffset + 0x0000U] == 0x11U);
    CHECK(ram[kSceneStagingRamOffset + 0x1000U] == 0x33U);
    CHECK(ram[kSceneStagingRamOffset + 0x2000U] == 'A');
    CHECK(ram[kSceneStagingRamOffset + 0x3000U] == 0x55U);

    CHECK(read_be32(ram, 0x10B4U) == kSceneVramDmaBusSource);
    CHECK(read_be16(ram, 0x10B8U) == kSceneVramDmaDest);
    CHECK(read_be16(ram, 0x10BAU) == kSceneVramDmaLengthWords);
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FF10BCU);
    CHECK(ram[kVintSyncFlagsRamOffset] == 0x42U);

    // Out-of-bounds DMA queue pointer fails safely without mutating $FF1892 or $FF164E.
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FFFFF9U);
    ram[kVintSyncFlagsRamOffset] = 0x00U;
    CHECK(!queue_16kb_vram_dma_from_ram(ram));
    CHECK(read_be32(ram, kDmaQueuePtrRamOffset) == 0x00FFFFF9U);
    CHECK(ram[kVintSyncFlagsRamOffset] == 0x00U);
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
    SceneHeaderConfig cfg{};
    cfg.bg_patterns = {1U, 2U, 3U, 4U};
    cfg.elevation_slices = {8, 8, 8, -1};
    cfg.vdp_scroll_mode = 0x2040U;

    std::array<std::uint8_t, 0x10000> ram{};
    write_be32(ram, kDmaQueuePtrRamOffset, 0x00FF10B4U);
    const SceneInitResult res = load_scene_header_and_init_vram_from_ram(
        rom.bytes(),
        ram,
        cfg,
        0x00600003U,
        0x00000000U);

    CHECK(res.dma_queued);
    CHECK((res.decompressed_slot_bytes == std::array<std::size_t, 4>{4096U, 4096U, 4096U, 4096U}));
    CHECK(res.total_decompressed_bytes == kSceneStagingBufferByteSize);
    CHECK(oasis::core::compute_sha256(
              std::span<const std::uint8_t>(ram).subspan(
                  kSceneStagingRamOffset + 0x2000U,
                  kSceneBgPatternSlotStride)) == oasis::game::kResourceOutputSha256);
    CHECK(read_be32(ram, 0x10B4U) == kSceneVramDmaBusSource);
    CHECK(read_be16(ram, 0x10B8U) == 0x0000U);
    CHECK(read_be16(ram, 0x10BAU) == kSceneVramDmaLengthWords);
}

} // namespace

int main() {
    test_constants_and_header_parser();
    test_unpack_elevation_bands();
    test_clear_and_align_camera_coords();
    test_decompress_4_bg_patterns_and_dma();
    test_ram_window_roundtrip();
    test_real_rom_if_available();

    std::cout << "[PASS] All M16.188 scene header DMA tests succeeded!\n";
    return 0;
}
