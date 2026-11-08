#include "game/render/tilemap_plane_upload.hpp"

namespace oasis::render {

std::vector<VdpWriteRecord> execute_plane_row_upload(PlaneUploadState& state, std::span<const uint16_t> source_tiles) {
    std::vector<VdpWriteRecord> writes;
    writes.reserve(TilemapPlaneUploadContract::kWordsPerUpload + 2U);

    uint16_t d1 = state.vram_offset;
    uint16_t d2 = state.x_pos;
    uint16_t d3 = state.plane_width;
    uint16_t d4 = state.plane_height;
    uint16_t d5 = state.vram_base;
    int16_t  d6 = state.scroll_counter;
    uint16_t d7 = state.y_pos;

    uint32_t src_offset = 0U;

    if (d6 >= 0) { // Scroll Up
        d6 -= 1;
        state.scroll_counter = d6;
        const uint32_t d0 = static_cast<uint32_t>(d2) * 2U;
        d7 += 1U;
        if (d7 >= d4) d7 -= d4;
        state.y_pos = d7;
        d7 += 0x001CU;
        if (d7 >= d4) d7 -= d4;
        src_offset = ((static_cast<uint32_t>(d7) << state.shift_factor) + d0);
        d4 = d1;
        d1 = (d1 & 0x0F80U) + 0x0080U;
        d4 &= 0x007EU;
        d1 &= 0x0F80U;
        d7 = d1 + d4;
        state.vram_offset = d7;
        d1 = (d1 + 0x0E00U) & 0x0F80U;
        d1 += d4;
    } else { // Scroll Down
        d6 += 1;
        state.scroll_counter = d6;
        const uint32_t d0 = static_cast<uint32_t>(d2) * 2U;
        if (d7 == 0U) d7 = d4 - 1U;
        else d7 -= 1U;
        state.y_pos = d7;
        src_offset = ((static_cast<uint32_t>(d7) << state.shift_factor) + d0);
        d4 = d1;
        d1 = (d1 & 0x0F80U) - 0x0080U;
        d4 &= 0x007EU;
        d1 = (d1 & 0x0F80U) + d4;
        state.vram_offset = d1;
    }

    d4 >>= 1U;
    uint16_t d6_cmd = d5;
    d5 = static_cast<uint16_t>((d5 << 2U) | (d5 >> 14U)) & 0x0003U;
    d6_cmd = (d6_cmd & 0x3FFFU) | TilemapPlaneUploadContract::kVdpWriteCommandBase;
    const uint16_t initial_d0 = d6_cmd + d1;

    // VDP control port writes
    writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, initial_d0});
    writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, d5});

    // 41-word tile upload loop
    const uint32_t tile_idx_start = (src_offset >> 1U);
    for (unsigned loop = 0; loop < TilemapPlaneUploadContract::kWordsPerUpload; ++loop) {
        const uint32_t curr_idx = tile_idx_start + loop;
        const uint16_t tile = (curr_idx < source_tiles.size()) ? source_tiles[curr_idx] : 0U;
        writes.push_back({TilemapPlaneUploadContract::kVdpDataPort, tile});

        d2 += 1U;
        if (d2 >= d3) {
            d2 -= d3;
        }

        d1 += 2U;
        d4 += 1U;
        if (d4 >= 0x0040U) {
            d4 -= 0x0040U;
            d1 -= 0x0080U;
            const uint16_t wrapped_cmd = d6_cmd + d1;
            writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, wrapped_cmd});
            writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, d5});
        }
    }

    return writes;
}

} // namespace oasis::render
