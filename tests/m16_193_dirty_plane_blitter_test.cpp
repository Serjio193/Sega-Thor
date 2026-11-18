#include "game/render/dirty_plane_blitter.hpp"
#include "../include/game/render/dirty_plane_blitter.hpp"
#include "game/render/dual_plane_loader.hpp"
#include "game/render/scroll_integrator.hpp"
#include "game/render/tilemap_rect_ops.hpp"
#include "game/render/vdp_scroll_builder.hpp"
#include "game/render/visible_plane_streamer.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

using namespace thor::render;

void require_check(bool condition, const char* expr, int line) {
    if (!condition) {
        std::cerr << "[FAIL] M16.193 check failed at line " << line << ": " << expr << '\n';
        std::abort();
    }
}

#define CHECK(expr) require_check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
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

[[nodiscard]] std::vector<std::uint8_t> make_pattern_tilemap(
    std::uint16_t width_tiles,
    std::uint16_t height_tiles,
    std::uint16_t tag_base = 0x0000U)
{
    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(width_tiles) * height_tiles * 2U, 0U);
    for (std::uint16_t r = 0U; r < height_tiles; ++r) {
        for (std::uint16_t c = 0U; c < width_tiles; ++c) {
            const auto word = static_cast<std::uint16_t>(
                tag_base | ((r & 0x3FU) << 7U) | (c & 0x7FU));
            const std::size_t off =
                (static_cast<std::size_t>(r) * width_tiles + c) * 2U;
            write_be16(bytes, off, word);
        }
    }
    return bytes;
}

void test_constants_and_cross_header_compat() {
    static_assert(kVdpSyncDirtyPlaneRectAddr == 0x00DC26U);
    static_assert(kVdpSyncDirtyPlaneRectClippedAddr == 0x00DC30U);
    static_assert(kVdpSyncDirtyPlaneRectClippedRtsAddr == 0x00DD12U);
    static_assert(kVdpSyncDirtyPlaneRectUnclippedAddr == 0x00DD14U);
    static_assert(kDirtyPlaneBlitterEndAddr == 0x00DDA2U);
    static_assert(kDirtyPlaneBlitterByteCount == 380U);
    static_assert(kDirtyPlaneBlitterEndAddr - kVdpSyncDirtyPlaneRectAddr ==
                  kDirtyPlaneBlitterByteCount);

    static_assert(kDirtyClipMaxRelCol == 41);
    static_assert(kDirtyClipMaxRelRow == 29);
    static_assert(kDirtyClipMaxCols == 42U);
    static_assert(kDirtyClipMaxRows == 30U);
    static_assert(kDirtyPlaneUnclippedModeBit == 0x02U);
    static_assert(kDirtyVdpColMask == 0x003FU);
    static_assert(kDirtyVdpRowMask == 0x001FU);

    static_assert(oasis::game::render::kDirtyPlaneBlitterEndAddr == kDirtyPlaneBlitterEndAddr);
    static_assert(sizeof(oasis::game::render::DirtyPlaneTransferPlan) ==
                  sizeof(DirtyPlaneTransferPlan));

    static_assert(compute_dirty_vram_write_cmd(0xC000U, 0U, 0U) == 0x40000003U);
    static_assert(compute_dirty_vram_write_cmd(0xE000U, 0U, 0U) == 0x60000003U);
    static_assert(compute_dirty_vram_write_cmd(0xC000U, 63U, 31U) == 0x4FFE0003U);
    static_assert(compute_dirty_vram_write_cmd(0xE000U, 10U, 5U) == 0x62940003U);
    static_assert(compute_dirty_vram_write_cmd(0xC000U, 5U, 2U) ==
                  compute_vram_write_cmd(0xC10AU));
}

void test_bit0_ignored_and_42x30_clipping_bounds() {
    VisiblePlaneState state{};
    state.start_col = 10U;
    state.start_row = 20U;
    state.vram_ring_offset = 0U;
    state.vram_base = kPlaneAVramBase;
    state.width_tiles = 128U;
    state.height_tiles = 64U;
    state.byte_stride_shift = 8U; // 256 bytes/row
    state.control_flags = 0U;     // Bit 0 == 0, Bit 1 == 0 -> still runs clipped mode!

    // Offscreen left: rel_x = -1, width = 1 (w_m1 = 0) -> culled
    const auto left_out = plan_dirty_plane_rect(state, 9, 20, 0, 0);
    CHECK(left_out.valid && left_out.culled);

    // Partial left edge: rel_x = -1, width = 2 (w_m1 = 1) -> 1 col at rel_x=0, skip 2 bytes
    const auto left_in = plan_dirty_plane_rect(state, 9, 20, 1, 0);
    CHECK(left_in.valid && !left_in.culled && !left_in.unclipped_mode);
    CHECK(left_in.clipped_rel_x == 0 && left_in.cols == 1U && left_in.source_byte_offset == 2U);

    // Right boundary: dbra +1 allows rel_x == 41 (0x29), culls rel_x == 42
    const auto right_edge = plan_dirty_plane_rect(state, 51, 20, 4, 0); // rel_x=41
    CHECK(right_edge.valid && !right_edge.culled);
    CHECK(right_edge.clipped_rel_x == 41 && right_edge.cols == 1U);
    const auto right_out = plan_dirty_plane_rect(state, 52, 20, 0, 0); // rel_x=42
    CHECK(right_out.valid && right_out.culled);

    // Full horizontal span at rel_x=0 clamps width_minus_1 to 41 -> 42 columns
    const auto full_w = plan_dirty_plane_rect(state, 10, 20, 49, 0);
    CHECK(full_w.valid && !full_w.culled && full_w.cols == 42U);

    // Offscreen top & partial top edge
    const auto top_out = plan_dirty_plane_rect(state, 10, 19, 0, 0);
    CHECK(top_out.valid && top_out.culled);
    const auto top_in = plan_dirty_plane_rect(state, 10, 19, 0, 1);
    CHECK(top_in.valid && !top_in.culled);
    CHECK(top_in.clipped_rel_y == 0 && top_in.rows == 1U && top_in.source_byte_offset == 256U);

    // Bottom boundary: dbra +1 allows rel_y == 29 (0x1D), culls rel_y == 30
    const auto bot_edge = plan_dirty_plane_rect(state, 10, 49, 0, 4); // rel_y=29
    CHECK(bot_edge.valid && !bot_edge.culled);
    CHECK(bot_edge.clipped_rel_y == 29 && bot_edge.rows == 1U);
    const auto bot_out = plan_dirty_plane_rect(state, 10, 50, 0, 0); // rel_y=30
    CHECK(bot_out.valid && bot_out.culled);

    // Combined 4-sided clip: (-3, -2) 60x50 -> (0, 0) 42x30, skip 3*2 + (2 << 8) = 518 bytes
    const auto four_side = plan_dirty_plane_rect(state, 7, 18, 59, 49);
    CHECK(four_side.valid && !four_side.culled);
    CHECK(four_side.cols == 42U && four_side.rows == 30U);
    CHECK(four_side.source_byte_offset == 518U);

    // Culled blit executes cleanly with empty source and never mutates VRAM
    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0x5AU);
    const auto cull_res = vdp_sync_dirty_plane_rect(state, {}, 52, 20, 3, 3, vdp);
    CHECK(cull_res.ok && cull_res.culled && cull_res.tiles_written == 0U);
    CHECK(cull_res.vdp_control_writes.empty());
    CHECK(vdp.vram[kPlaneAVramBase] == 0x5AU);

    // Non-culled blit with bit 0 == 0 still writes to VRAM
    const auto map = make_pattern_tilemap(128U, 64U, 0x8000U);
    const auto active0_res = vdp_sync_dirty_plane_rect_from_tilemap(
        state, map, 10, 20, 1, 0, vdp);
    CHECK(active0_res.ok && !active0_res.culled && active0_res.tiles_written == 2U);
    CHECK(read_be16(vdp.vram, kPlaneAVramBase) == read_be16(map, (20U << 8U) + 20U));
}

void test_clipped_streaming_and_vdp_ring_wrapping() {
    constexpr std::uint16_t kMapW = 128U;
    constexpr std::uint16_t kMapH = 64U;
    const auto map = make_pattern_tilemap(kMapW, kMapH, 0x4000U);

    VisiblePlaneState state{};
    state.start_col = 10U;
    state.start_row = 5U;
    // VDP ring origin at col=62, row=30 -> +$0C = (30 << 7) | (62 << 1) = 0x0F7C
    state.vram_ring_offset = static_cast<std::uint16_t>((30U << 7U) | (62U << 1U));
    state.vram_base = kPlaneAVramBase;
    state.width_tiles = kMapW;
    state.height_tiles = kMapH;
    state.byte_stride_shift = 8U;
    state.control_flags = kPlaneActiveBit;

    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0xEEU);

    // Target (8, 4) size 6x4 (w_m1=5, h_m1=3): rel=(-2, -1) -> clipped (0, 0) size 4x3
    // VDP start: col=62, row=30 -> wraps cols 62,63,0,1 and rows 30,31,0!
    const auto res = vdp_sync_dirty_plane_rect_from_tilemap(state, map, 8, 4, 5, 3, vdp);
    CHECK(res.ok && !res.culled && !res.unclipped_mode);
    CHECK(res.plan.cols == 4U && res.plan.rows == 3U);
    CHECK(res.plan.vdp_start_col == 62U && res.plan.vdp_start_row == 30U);
    CHECK(res.tiles_written == 12U);
    // 3 rows * (row-start cmd + col-64 wrap cmd) = 6 commands (no extra row-32 cmd!)
    CHECK(res.vdp_control_writes.size() == 6U);
    CHECK(res.vdp_control_writes[0] == compute_dirty_vram_write_cmd(0xC000U, 62U, 30U));
    CHECK(res.vdp_control_writes[1] == compute_dirty_vram_write_cmd(0xC000U, 0U, 30U));
    CHECK(res.vdp_control_writes[2] == compute_dirty_vram_write_cmd(0xC000U, 62U, 31U));
    CHECK(res.vdp_control_writes[3] == compute_dirty_vram_write_cmd(0xC000U, 0U, 31U));
    CHECK(res.vdp_control_writes[4] == compute_dirty_vram_write_cmd(0xC000U, 62U, 0U));
    CHECK(res.vdp_control_writes[5] == compute_dirty_vram_write_cmd(0xC000U, 0U, 0U));

    for (std::uint16_t r = 0U; r < 3U; ++r) {
        const std::uint16_t src_r = static_cast<std::uint16_t>(5U + r);
        const std::uint16_t vdp_r = static_cast<std::uint16_t>((30U + r) & 31U);
        for (std::uint16_t c = 0U; c < 4U; ++c) {
            const std::uint16_t src_c = static_cast<std::uint16_t>(10U + c);
            const std::uint16_t vdp_c = static_cast<std::uint16_t>((62U + c) & 63U);
            const std::uint16_t expected =
                read_be16(map, (static_cast<std::size_t>(src_r) << 8U) + src_c * 2U);
            const std::size_t vram_off =
                kPlaneAVramBase + (static_cast<std::size_t>(vdp_r) << 7U) + vdp_c * 2U;
            CHECK(read_be16(vdp.vram, vram_off) == expected);
        }
        // Adjacent untouched cells on same wrapped row remain 0xEEEE
        const std::size_t before_off =
            kPlaneAVramBase + (static_cast<std::size_t>(vdp_r) << 7U) + 61U * 2U;
        const std::size_t after_off =
            kPlaneAVramBase + (static_cast<std::size_t>(vdp_r) << 7U) + 2U * 2U;
        CHECK(read_be16(vdp.vram, before_off) == 0xEEEEU);
        CHECK(read_be16(vdp.vram, after_off) == 0xEEEEU);
    }
    CHECK(read_be16(vdp.vram, kPlaneBVramBase) == 0xEEEEU);

    // Final tile landing on col 63 still emits trailing col-64 wrap VDP command
    state.vram_ring_offset = static_cast<std::uint16_t>(62U << 1U); // col=62, row=0
    const auto edge_res = vdp_sync_dirty_plane_rect_from_tilemap(state, map, 10, 5, 1, 0, vdp);
    CHECK(edge_res.ok && edge_res.tiles_written == 2U);
    CHECK(edge_res.vdp_control_writes.size() == 2U);
    CHECK(edge_res.vdp_control_writes[0] == compute_dirty_vram_write_cmd(0xC000U, 62U, 0U));
    CHECK(edge_res.vdp_control_writes[1] == compute_dirty_vram_write_cmd(0xC000U, 0U, 0U));
}

void test_unclipped_mode_and_stride_shift_variants() {
    // Unclipped mode (bit 1 = 1, bit 0 = 0): ignores +$0C(a3), +$24(a3), and viewport bounds.
    constexpr std::uint16_t kMapW = 64U;
    constexpr std::uint16_t kMapH = 32U;
    const auto map = make_pattern_tilemap(kMapW, kMapH, 0xA000U);

    VisiblePlaneState state{};
    state.start_col = 5U;
    state.start_row = 3U;
    state.vram_ring_offset = 0x0F7EU; // Must be ignored in unclipped mode
    state.vram_base = kPlaneBVramBase;
    state.width_tiles = kMapW;
    state.height_tiles = kMapH;
    state.byte_stride_shift = 0U; // Must be ignored in unclipped mode (uses 2 * width_tiles)
    state.control_flags = kDirtyPlaneUnclippedModeBit;

    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0x33U);

    // target_x=3 (-2 mod 64 -> col 62), target_y=33 (+30 mod 32 -> row 30), size 4x3
    // Note: if clipped mode ran, rel_x=-2 and rel_y=30 would cull this rect!
    const std::size_t off_3_2 = (2U * kMapW + 3U) * 2U;
    const auto res_unclipped = vdp_sync_dirty_plane_rect(
        state, std::span<const std::uint8_t>(map).subspan(off_3_2), 3, 33, 3, 2, vdp);
    CHECK(res_unclipped.ok && res_unclipped.unclipped_mode && !res_unclipped.culled);
    CHECK(res_unclipped.plan.vdp_start_col == 62U);
    CHECK(res_unclipped.plan.vdp_start_row == 30U);
    CHECK(res_unclipped.tiles_written == 12U);
    CHECK(res_unclipped.vdp_control_writes.size() == 6U);
    CHECK(res_unclipped.vdp_control_writes[0] == 0x6F7C0003U);
    CHECK(res_unclipped.vdp_control_writes[1] == 0x6F000003U);
    CHECK(res_unclipped.vdp_control_writes[4] == 0x607C0003U);
    CHECK(res_unclipped.vdp_control_writes[5] == 0x60000003U);

    for (std::uint16_t r = 0U; r < 3U; ++r) {
        const std::uint16_t vdp_r = static_cast<std::uint16_t>((30U + r) & 31U);
        for (std::uint16_t c = 0U; c < 4U; ++c) {
            const std::uint16_t vdp_c = static_cast<std::uint16_t>((62U + c) & 63U);
            const std::uint16_t expected = read_be16(
                map, (static_cast<std::size_t>(2U + r) * kMapW + (3U + c)) * 2U);
            CHECK(read_be16(vdp.vram, kPlaneBVramBase + (vdp_r << 7U) + vdp_c * 2U) == expected);
        }
    }
    CHECK(read_be16(vdp.vram, kPlaneAVramBase) == 0x3333U);

    // Verify byte_stride_shift = 6 (32-tile width) top-edge skip
    const auto map32 = make_pattern_tilemap(32U, 32U, 0x2000U);
    VisiblePlaneState state32{};
    state32.start_col = 2U;
    state32.start_row = 4U;
    state32.vram_base = kPlaneAVramBase;
    state32.width_tiles = 32U;
    state32.height_tiles = 32U;
    state32.byte_stride_shift = 6U; // 64 bytes/row
    state32.control_flags = kPlaneActiveBit;
    const auto res32 = vdp_sync_dirty_plane_rect_from_tilemap(state32, map32, 1, 2, 2, 3, vdp);
    CHECK(res32.ok && res32.plan.cols == 2U && res32.plan.rows == 2U);
    CHECK(res32.plan.source_byte_offset == (1U * 2U) + (2U << 6U));
    CHECK(read_be16(vdp.vram, kPlaneAVramBase) == read_be16(map32, (4U << 6U) + 2U * 2U));
}

void test_guards_and_m16_192_ram_pipeline() {
    oasis::genesis::VdpState vdp{};
    vdp.vram.fill(0x77U);

    VisiblePlaneState state{};
    state.vram_base = kPlaneAVramBase;
    state.width_tiles = 64U;
    state.height_tiles = 32U;
    state.byte_stride_shift = 7U;
    state.control_flags = kPlaneActiveBit;

    const std::array<std::uint8_t, 4> tiny{0x12U, 0x34U, 0x56U, 0x78U};
    CHECK(!vdp_sync_dirty_plane_rect(state, tiny, 0, 0, -1, 0, vdp).ok);
    CHECK(!vdp_sync_dirty_plane_rect(state, tiny, 0, 0, 0, -1, vdp).ok);
    CHECK(!vdp_sync_dirty_plane_rect(state, tiny, 0, 0, 2, 0, vdp).ok); // Too short -> atomic
    CHECK(read_be16(vdp.vram, kPlaneAVramBase) == 0x7777U);

    state.byte_stride_shift = 16U;
    state.start_row = 2U;
    CHECK(!plan_dirty_plane_rect(state, 0, 0, 0, 3).valid);
    state.byte_stride_shift = 7U;
    state.width_tiles = 0U;
    CHECK(!vdp_sync_dirty_plane_rect(state, tiny, 0, 2, 0, 0, vdp).ok);

    // Full M16.192 -> M16.193 RAM integration (0x00DB64 -> 0x00DBC0 -> 0x00DC26)
    std::array<std::uint8_t, 0x10000> ram{};
    constexpr std::uint32_t kMapRamAddr = 0x00FF4000U;
    constexpr std::uint32_t kAuxRamAddr = 0x00FF8000U;
    const auto map = make_pattern_tilemap(64U, 64U, 0xC000U);
    std::copy(map.begin(), map.end(), ram.begin() + 0x4000U);

    VisiblePlaneState ram_plane{};
    ram_plane.start_col = 12U;
    ram_plane.start_row = 8U;
    ram_plane.vram_ring_offset = static_cast<std::uint16_t>((4U << 7U) | (10U << 1U));
    ram_plane.tile_data_ptr = kMapRamAddr;
    ram_plane.vram_base = kPlaneAVramBase;
    ram_plane.width_tiles = 64U;
    ram_plane.height_tiles = 64U;
    ram_plane.byte_stride_shift = 7U;
    ram_plane.control_flags = kPlaneActiveBit;
    store_visible_plane_state_to_ram(ram, kPlaneARamOffset, ram_plane);

    write_be32(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x00U, kAuxRamAddr);
    write_be16(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x04U, 64U);
    write_be16(ram, TilemapRectOpsConfig::kPlaneAAuxRamOffset + 0x08U, 6U);

    // Copy 3x2 rect (2..4, 1..2) -> dst (11, 8): rel=(-1, 0) -> clips to (0, 0) 2x2 at VDP (10, 4)
    DirtyPlaneBlitResult sync_res{};
    const std::size_t copied = tilemap_copy_rect_word_and_sync_from_ram(
        ram, false, TilemapRect{2, 1, 4, 2}, TilemapPoint{11, 8},
        [&](const TilemapRectSyncRequest& req) {
            sync_res = vdp_sync_dirty_plane_rect_from_ram(ram, req, vdp);
        });
    CHECK(copied == 6U);
    CHECK(sync_res.ok && !sync_res.culled);
    CHECK(sync_res.plan.cols == 2U && sync_res.plan.rows == 2U);
    CHECK(sync_res.plan.vdp_start_col == 10U && sync_res.plan.vdp_start_row == 4U);
    CHECK(sync_res.tiles_written == 4U);
    CHECK(read_be16(vdp.vram, kPlaneAVramBase + (4U << 7U) + 10U * 2U) ==
          read_be16(map, (1U << 7U) + 3U * 2U));
    CHECK(read_be16(vdp.vram, kPlaneAVramBase + (5U << 7U) + 11U * 2U) ==
          read_be16(map, (2U << 7U) + 4U * 2U));

    // Direct from_ram & invalid pointer guards
    const auto direct_ram_res = vdp_sync_dirty_plane_rect_from_ram(
        ram, 0x00FF1716U, kMapRamAddr + (8U << 7U) + 12U * 2U, 12, 8, 1, 0, vdp);
    CHECK(direct_ram_res.ok && direct_ram_res.tiles_written == 2U);
    CHECK(!vdp_sync_dirty_plane_rect_from_ram(ram, 0xFFF0U, kMapRamAddr, 0, 0, 0, 0, vdp).ok);
    CHECK(!vdp_sync_dirty_plane_rect_from_ram(ram, kPlaneARamOffset, 0U, 0, 0, 0, 0, vdp).ok);
}

} // namespace

int main() {
    test_constants_and_cross_header_compat();
    test_bit0_ignored_and_42x30_clipping_bounds();
    test_clipped_streaming_and_vdp_ring_wrapping();
    test_unclipped_mode_and_stride_shift_variants();
    test_guards_and_m16_192_ram_pipeline();

    std::cout << "[PASS] All M16.193 dirty plane blitter tests succeeded!\n";
    return 0;
}
