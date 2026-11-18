#include "game/render/visible_plane_streamer.hpp"
#include "../include/game/render/visible_plane_streamer.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

using namespace oasis::game::render;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.190 check failed at line " << line << ": " << expr << '\n';
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

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] std::vector<std::uint8_t> make_tilemap_bytes(
    std::uint16_t width_tiles,
    std::uint16_t height_tiles)
{
    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(width_tiles) * height_tiles * 2U,
        0U);
    for (std::uint16_t r = 0U; r < height_tiles; ++r) {
        for (std::uint16_t c = 0U; c < width_tiles; ++c) {
            const auto tile_id = static_cast<std::uint16_t>((r << 8U) | c);
            const std::size_t off =
                (static_cast<std::size_t>(r) * width_tiles + c) * 2U;
            write_be16(bytes, off, tile_id);
        }
    }
    return bytes;
}

void test_constants_and_vdp_commands() {
    static_assert(kVdpUploadVisibleTilemapPlaneAddr == 0x00D7C0U);
    static_assert(kPlaneVramStreamHelperAddr == 0x00D7C0U);
    static_assert(kVdpFillPlaneBlankSafeAddr == 0x00D950U);
    static_assert(kVdpFillPlaneBlankRawAddr == 0x00D95EU);
    static_assert(kVdpFillPlaneBlankBodyAddr == 0x00D962U);
    static_assert(kVisiblePlaneStreamerEndAddr == 0x00D9A4U);

    static_assert(kVdpBlankTileWord == 0x0780U);
    static_assert(kVdpVisibleStreamCols == 41U);
    static_assert(kVdpVisibleStreamRows == 29U);
    static_assert(kVdpFullPlaneStreamCols == 64U);
    static_assert(kVdpFullPlaneStreamRows == 32U);
    static_assert(kVdpBlankFillCols == 41U);
    static_assert(kVdpBlankFillRows == 29U);
    static_assert(kVdpCmdRowStep == 0x00800000U);

    // VDP VRAM write command encoding (0x00D892 / 0x00D962)
    static_assert(compute_vram_write_cmd(0xC000U) == 0x40000003U);
    static_assert(compute_vram_write_cmd(0xE000U) == 0x60000003U);
    static_assert(compute_vram_write_cmd(0xC080U) == 0x40800003U);
    static_assert(compute_vram_write_cmd(0x0000U) == 0x40000000U);
    static_assert(compute_vram_write_cmd(0x8000U) == 0x40000002U);

    // Toroidal tile origin normalization (`asr.w #3` + wrap [0, plane_tiles))
    static_assert(wrap_tile_origin(0, 64U) == 0U);
    static_assert(wrap_tile_origin(7, 64U) == 0U);
    static_assert(wrap_tile_origin(8, 64U) == 1U);
    static_assert(wrap_tile_origin(-1, 64U) == 63U);
    static_assert(wrap_tile_origin(-9, 64U) == 62U);
    static_assert(wrap_tile_origin(512, 64U) == 0U);
    static_assert(wrap_tile_origin(1032, 64U) == 1U); // 129 % 64 = 1
    static_assert(wrap_tile_origin(100, 0U) == 0U);   // Zero-width guard
}

void test_dimension_selection_and_bit1_latch() {
    // 1. Standard large map (128x64), bit 1 clear -> 41x29 (0x28+1 x 0x1C+1), bit 1 NOT set
    std::uint8_t flags = kPlaneActiveBit;
    const auto d1 = select_visible_stream_dimensions(128U, 64U, flags);
    CHECK((d1 == VisibleStreamDimensions{41U, 29U}));
    CHECK(flags == kPlaneActiveBit);

    // 2. 64x64 map -> height != 32, keeps 41x29 and does NOT latch bit 1
    flags = kPlaneActiveBit;
    const auto d2 = select_visible_stream_dimensions(64U, 64U, flags);
    CHECK((d2 == VisibleStreamDimensions{41U, 29U}));
    CHECK(flags == kPlaneActiveBit);

    // 3. 128x32 map -> width > 64, keeps 41x29 and does NOT latch bit 1
    flags = kPlaneActiveBit;
    const auto d3 = select_visible_stream_dimensions(128U, 32U, flags);
    CHECK((d3 == VisibleStreamDimensions{41U, 29U}));
    CHECK(flags == kPlaneActiveBit);

    // 4. 64x32 or 32x32 map (height == 32 && width <= 64) -> latches bit 1 and picks 64x32
    flags = kPlaneActiveBit;
    const auto d4 = select_visible_stream_dimensions(64U, 32U, flags);
    CHECK((d4 == VisibleStreamDimensions{64U, 32U}));
    CHECK(flags == (kPlaneActiveBit | kPlaneFull64x32LatchBit));

    flags = kPlaneActiveBit;
    const auto d5 = select_visible_stream_dimensions(32U, 32U, flags);
    CHECK((d5 == VisibleStreamDimensions{64U, 32U}));
    CHECK(flags == (kPlaneActiveBit | kPlaneFull64x32LatchBit));

    // 5. Non-32 smaller height (e.g. 32x16) -> `cmpi.w #$20, $22(a3); bne` stays 41x29
    flags = kPlaneActiveBit;
    const auto d6 = select_visible_stream_dimensions(32U, 16U, flags);
    CHECK((d6 == VisibleStreamDimensions{41U, 29U}));
    CHECK(flags == kPlaneActiveBit);

    // 6. Bit 1 already set -> forces 64x32 even for 128x64
    flags = kPlaneActiveBit | kPlaneFull64x32LatchBit;
    const auto d7 = select_visible_stream_dimensions(128U, 64U, flags);
    CHECK((d7 == VisibleStreamDimensions{64U, 32U}));
}

void test_blank_tile_plane_clearer() {
    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0xAAU);

    // Direct vdp_fill_plane_blank(0xE000) clears 41x29 = 1189 tiles with 0x0780
    const auto direct_res = vdp_fill_plane_blank(kPlaneBVramBase, vdp);
    CHECK(direct_res.ok);
    CHECK(direct_res.filled_blank);
    CHECK((direct_res.dims == VisibleStreamDimensions{41U, 29U}));
    CHECK(direct_res.tiles_written == 41U * 29U);
    CHECK(direct_res.vdp_control_writes.size() == 29U);
    CHECK(direct_res.vdp_control_writes[0] == 0x60000003U);
    CHECK(direct_res.vdp_control_writes[28] == (0x60000003U + 28U * 0x00800000U));

    for (std::size_t r = 0U; r < 29U; ++r) {
        const std::size_t row_base = kPlaneBVramBase + (r * kVdpNametableRowByteStride);
        for (std::size_t c = 0U; c < 41U; ++c) {
            CHECK(read_be16(vdp.vram, row_base + c * 2U) == kVdpBlankTileWord);
        }
        // Column 41..63 untouched
        CHECK(read_be16(vdp.vram, row_base + 41U * 2U) == 0xAAAAU);
    }
    // Row 29 untouched
    CHECK(read_be16(vdp.vram, kPlaneBVramBase + 29U * kVdpNametableRowByteStride) == 0xAAAAU);

    // Inactive plane (bit 0 == 0) in vdp_upload_visible_tilemap_plane clears scroll/step fields
    // and falls back to 0x00D95E blank fill.
    vdp.vram.fill(0x55U);
    VisiblePlaneState inactive{};
    inactive.vram_base = kPlaneAVramBase;
    inactive.vdp_hscroll = 12U;
    inactive.vdp_vscroll = 34U;
    inactive.vram_ring_offset = 56U;
    inactive.start_col = 7U;
    inactive.start_row = 8U;
    inactive.tile_col_step = 3;
    inactive.tile_row_step = -2;
    inactive.control_flags = 0U; // Bit 0 clear -> inactive

    const auto fallback_res = vdp_upload_visible_tilemap_plane(inactive, {}, vdp);
    CHECK(fallback_res.ok);
    CHECK(fallback_res.filled_blank);
    CHECK(fallback_res.tiles_written == 41U * 29U);
    CHECK(inactive.vdp_hscroll == 0U);
    CHECK(inactive.vdp_vscroll == 0U);
    CHECK(inactive.vram_ring_offset == 0U);
    CHECK(inactive.start_col == 0U);
    CHECK(inactive.start_row == 0U);
    CHECK(inactive.tile_col_step == 0);
    CHECK(inactive.tile_row_step == 0);
    CHECK(read_be16(vdp.vram, kPlaneAVramBase) == kVdpBlankTileWord);
    CHECK(read_be16(vdp.vram, kPlaneAVramBase + 28U * kVdpNametableRowByteStride + 40U * 2U) == kVdpBlankTileWord);
    CHECK(read_be16(vdp.vram, kPlaneAVramBase + 29U * kVdpNametableRowByteStride) == 0x5555U);
}

void test_stream_41x29_with_toroidal_wrap() {
    // 128x64 map: width_tiles=128, height_tiles=64, byte_stride_shift=8 (256 bytes/row)
    constexpr std::uint16_t kMapW = 128U;
    constexpr std::uint16_t kMapH = 64U;
    const auto tilemap = make_tilemap_bytes(kMapW, kMapH);

    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0xFFU);

    VisiblePlaneState state{};
    // Place origin near wrap boundary: cam_x = -8 -> start_col = 127; cam_y = (62 * 8) -> start_row = 62
    state.cam_x = -8;
    state.cam_y = 62 * 8;
    state.vram_base = kPlaneAVramBase;
    state.width_tiles = kMapW;
    state.height_tiles = kMapH;
    state.byte_stride_shift = compute_log2_byte_stride(kMapW);
    state.control_flags = kPlaneActiveBit;
    state.vdp_hscroll = 99U;
    state.tile_col_step = 4;

    const auto res = vdp_upload_visible_tilemap_plane(state, tilemap, vdp);
    CHECK(res.ok);
    CHECK(!res.filled_blank);
    CHECK((res.dims == VisibleStreamDimensions{41U, 29U}));
    CHECK(res.start_col == 127U);
    CHECK(res.start_row == 62U);
    CHECK(res.updated_control_flags == kPlaneActiveBit); // Bit 1 not latched for 128x64
    CHECK(res.tiles_written == 41U * 29U);
    CHECK(res.vdp_control_writes.size() == 29U);
    CHECK(state.vdp_hscroll == 0U);
    CHECK(state.tile_col_step == 0);

    // Verify every cell wrapped modulo (128, 64) and rows 29..31 / cols 41..63 are untouched
    for (std::uint16_t r = 0U; r < 29U; ++r) {
        const std::uint16_t src_r = static_cast<std::uint16_t>((62U + r) % kMapH);
        const std::size_t row_vram = kPlaneAVramBase + (static_cast<std::size_t>(r) * kVdpNametableRowByteStride);
        for (std::uint16_t c = 0U; c < 41U; ++c) {
            const std::uint16_t src_c = static_cast<std::uint16_t>((127U + c) % kMapW);
            const std::uint16_t expected = static_cast<std::uint16_t>((src_r << 8U) | src_c);
            CHECK(read_be16(vdp.vram, row_vram + c * 2U) == expected);
        }
        CHECK(read_be16(vdp.vram, row_vram + 41U * 2U) == 0xFFFFU);
    }
    CHECK(read_be16(vdp.vram, kPlaneAVramBase + 29U * kVdpNametableRowByteStride) == 0xFFFFU);
}

void test_stream_64x32_and_32x32_full_latch() {
    // 32x32 map: height == 32 && width <= 64 latches bit 1 and tiles horizontally & vertically across 64x32
    constexpr std::uint16_t kMapW = 32U;
    constexpr std::uint16_t kMapH = 32U;
    const auto tilemap = make_tilemap_bytes(kMapW, kMapH);

    oasis::genesis::VdpState vdp{};
    PlaneContextRecord ctx{};
    ctx.tile_data_ptr = 0x00FF3FACU;
    ctx.vram_base = kPlaneBVramBase;
    ctx.width_tiles = kMapW;
    ctx.height_tiles = kMapH;
    ctx.byte_stride_shift = compute_log2_byte_stride(kMapW);
    ctx.control_flags = kPlaneActiveBit;

    VisiblePlaneState state = make_visible_plane_state(ctx, 24, 16); // start_col=3, start_row=2
    const auto res = vdp_upload_visible_tilemap_plane(state, tilemap, vdp);
    CHECK(res.ok);
    CHECK(!res.filled_blank);
    CHECK((res.dims == VisibleStreamDimensions{64U, 32U}));
    CHECK(res.start_col == 3U);
    CHECK(res.start_row == 2U);
    CHECK(state.control_flags == (kPlaneActiveBit | kPlaneFull64x32LatchBit));
    CHECK(res.tiles_written == 64U * 32U);
    // Row start write (32) + col-64 wrap write per row (32) + row-32 wrap write (1) = 65 cmds
    CHECK(res.vdp_control_writes.size() == 65U);
    CHECK(res.vdp_control_writes.front() == 0x60000003U);
    CHECK(res.vdp_control_writes.back() == 0x60000003U);

    // Every 64x32 cell in VDP Plane B ($E000..$EFFF) matches (src_r, src_c) wrapped mod (32, 32)
    for (std::uint16_t r = 0U; r < 32U; ++r) {
        const std::uint16_t src_r = static_cast<std::uint16_t>((2U + r) % kMapH);
        const std::size_t row_vram = kPlaneBVramBase + (static_cast<std::size_t>(r) * kVdpNametableRowByteStride);
        for (std::uint16_t c = 0U; c < 64U; ++c) {
            const std::uint16_t src_c = static_cast<std::uint16_t>((3U + c) % kMapW);
            const std::uint16_t expected = static_cast<std::uint16_t>((src_r << 8U) | src_c);
            CHECK(read_be16(vdp.vram, row_vram + c * 2U) == expected);
        }
    }
}

void test_ram_window_roundtrips_and_guards() {
    std::array<std::uint8_t, 0x10000> ram{};
    oasis::genesis::VdpState vdp{};

    // Populate a 64x32 tilemap at $FF4000 inside the 64KB RAM window
    constexpr std::uint16_t kMapW = 64U;
    constexpr std::uint16_t kMapH = 32U;
    const auto tilemap = make_tilemap_bytes(kMapW, kMapH);
    std::copy(tilemap.begin(), tilemap.end(), ram.begin() + 0x4000U);

    // Set up Plane A at $FF1716 with cam_x=16, cam_y=24, preserve unrelated fields (+0x02, +0x26)
    VisiblePlaneState plane_a{};
    plane_a.cam_x = 16;  // start_col = 2
    plane_a.cam_y = 24;  // start_row = 3
    plane_a.vdp_hscroll = 0x0123U;
    plane_a.vdp_vscroll = 0x0045U;
    plane_a.tile_col_step = 7;
    plane_a.tile_row_step = -3;
    plane_a.tile_data_ptr = 0x00FF4000U;
    plane_a.vram_base = kPlaneAVramBase;
    plane_a.width_tiles = kMapW;
    plane_a.height_tiles = kMapH;
    plane_a.byte_stride_shift = compute_log2_byte_stride(kMapW);
    plane_a.control_flags = kPlaneActiveBit;
    store_visible_plane_state_to_ram(ram, kPlaneARamOffset, plane_a);
    ram[kPlaneARamOffset + 0x26U] = 0x0FU; // dirty_flags must be preserved

    // Set up inactive Plane B at $FF173E (bit 0 == 0, null tile_data_ptr)
    VisiblePlaneState plane_b{};
    plane_b.vram_base = kPlaneBVramBase;
    plane_b.control_flags = 0U;
    store_visible_plane_state_to_ram(ram, kPlaneBRamOffset, plane_b);

    // Execute Plane A via full 24-bit address ($00FF1716) and Plane B via 16-bit offset (0x173E)
    const auto res_a = vdp_upload_visible_tilemap_plane_from_ram(ram, 0x00FF1716U, vdp);
    const auto res_b = vdp_upload_visible_tilemap_plane_from_ram(ram, kPlaneBRamOffset, vdp);

    CHECK(res_a.ok);
    CHECK(!res_a.filled_blank);
    CHECK((res_a.dims == VisibleStreamDimensions{64U, 32U}));
    CHECK(read_be16(ram, kPlaneARamOffset + 0x08U) == 2U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x0AU) == 3U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x0CU) == 0U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x0EU) == 0U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x10U) == 0U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x1CU) == 0U);
    CHECK(read_be16(ram, kPlaneARamOffset + 0x1EU) == 0U);
    CHECK(ram[kPlaneARamOffset + 0x26U] == 0x0FU);
    CHECK(ram[kPlaneARamOffset + 0x27U] == (kPlaneActiveBit | kPlaneFull64x32LatchBit));
    CHECK(read_be16(vdp.vram, kPlaneAVramBase) == ((3U << 8U) | 2U));
    CHECK(read_be16(vdp.vram, kPlaneAVramBase + 31U * kVdpNametableRowByteStride + 63U * 2U) ==
          ((2U << 8U) | 1U));

    CHECK(res_b.ok);
    CHECK(res_b.filled_blank);
    CHECK(read_be16(vdp.vram, kPlaneBVramBase) == kVdpBlankTileWord);

    // Bounds guards: out-of-range plane offset, null/OOB tile_data_ptr, truncated payload
    CHECK(!vdp_upload_visible_tilemap_plane_from_ram(ram, 0xFFF0U, vdp).ok);
    write_be16(ram, kPlaneARamOffset + 0x16U, 0x0000U);
    write_be16(ram, kPlaneARamOffset + 0x18U, 0x0000U);
    CHECK(!vdp_upload_visible_tilemap_plane_from_ram(ram, kPlaneARamOffset, vdp).ok);

    std::array<std::uint8_t, 16> tiny_payload{};
    CHECK(!vdp_upload_visible_tilemap_plane(plane_a, tiny_payload, vdp).ok);
}

} // namespace

int main() {
    test_constants_and_vdp_commands();
    test_dimension_selection_and_bit1_latch();
    test_blank_tile_plane_clearer();
    test_stream_41x29_with_toroidal_wrap();
    test_stream_64x32_and_32x32_full_latch();
    test_ram_window_roundtrips_and_guards();

    std::cout << "[PASS] All M16.190 visible plane streamer tests succeeded!\n";
    return 0;
}
