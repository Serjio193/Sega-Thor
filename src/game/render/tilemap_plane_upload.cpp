#include "game/render/tilemap_plane_upload.hpp"

namespace oasis::render {

std::vector<VdpWriteRecord> execute_plane_row_upload(PlaneUploadState& state, std::span<const uint16_t> source_tiles) {
    std::vector<VdpWriteRecord> writes;
    writes.reserve(TilemapPlaneUploadContract::kWordsPerRowUpload + 2U);

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
    for (unsigned loop = 0; loop < TilemapPlaneUploadContract::kWordsPerRowUpload; ++loop) {
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

std::vector<VdpWriteRecord> execute_plane_column_upload(PlaneUploadState& state, std::span<const uint16_t> source_tiles) {
    std::vector<VdpWriteRecord> writes;
    writes.reserve(TilemapPlaneUploadContract::kWordsPerColumnUpload + 4U);

    // Set VDP autoincrement = 128 (0x8F80)
    writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, TilemapPlaneUploadContract::kVdpAutoincrement128});

    uint16_t d1 = state.vram_offset;
    uint16_t d2_y = state.y_pos;
    uint16_t d3_h = state.plane_height;
    uint16_t d4_w = state.plane_width;
    uint16_t d5 = state.vram_base;
    int16_t  d6 = state.scroll_counter;
    uint16_t d7_x = state.x_pos;

    uint32_t src_offset = 0U;

    if (d6 >= 0) { // Scroll Right
        d6 -= 1;
        state.scroll_counter = d6;

        const uint32_t d0 = static_cast<uint32_t>(d2_y) << state.shift_factor;
        d7_x += 1U;
        if (d7_x >= d4_w) d7_x -= d4_w;
        state.x_pos = d7_x;

        d7_x += 0x0028U; // +40 tiles
        while (d7_x >= d4_w) {
            d7_x -= d4_w;
        }

        src_offset = d0 + (static_cast<uint32_t>(d7_x) * 2U);
        uint16_t d4 = d1;
        d1 = (d1 & 0x007EU) + 2U;
        d1 &= 0x007EU;
        d4 &= 0x0F80U;
        state.vram_offset = d1 + d4;
        d1 = ((d1 + 0x0050U) & 0x007EU) + d4;
    } else { // Scroll Left
        d6 += 1;
        state.scroll_counter = d6;

        const uint32_t d0 = static_cast<uint32_t>(d2_y) << state.shift_factor;
        if (d7_x == 0U) d7_x = d4_w - 1U;
        else d7_x -= 1U;
        state.x_pos = d7_x;

        src_offset = d0 + (static_cast<uint32_t>(d7_x) * 2U);
        uint16_t d4 = d1;
        d1 = (d1 & 0x007EU) - 2U;
        d1 &= 0x007EU;
        d4 &= 0x0F80U;
        d1 += d4;
        state.vram_offset = d1;
    }

    uint16_t d4_row = (state.vram_offset & 0x0F80U) >> 7U;
    uint16_t d6_cmd = d5;
    d5 = static_cast<uint16_t>((d5 << 2U) | (d5 >> 14U)) & 0x0003U;
    d6_cmd = (d6_cmd & 0x3FFFU) | TilemapPlaneUploadContract::kVdpWriteCommandBase;
    const uint16_t initial_d0 = d6_cmd + d1;

    writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, initial_d0});
    writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, d5});

    // 29-word column streaming loop
    const uint32_t bytes_per_row = static_cast<uint32_t>(d4_w) * 2U;
    const uint32_t wrap_bytes = static_cast<uint32_t>(d3_h) << state.shift_factor;
    uint32_t curr_src_byte = src_offset;

    for (unsigned loop = 0; loop < TilemapPlaneUploadContract::kWordsPerColumnUpload; ++loop) {
        const uint32_t curr_tile_idx = curr_src_byte >> 1U;
        const uint16_t tile = (curr_tile_idx < source_tiles.size()) ? source_tiles[curr_tile_idx] : 0U;
        writes.push_back({TilemapPlaneUploadContract::kVdpDataPort, tile});

        curr_src_byte += bytes_per_row;
        d2_y += 1U;
        if (d2_y >= d3_h) {
            d2_y = 0U;
            curr_src_byte -= wrap_bytes;
        }

        d4_row += 1U;
        if (d4_row >= 0x0020U) {
            d4_row -= 0x0020U;
            d1 -= 0x1000U;
            const uint16_t wrapped_cmd = d6_cmd + d1;
            writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, wrapped_cmd});
            writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, d5});
        }
    }

    // Restore VDP autoincrement = 2 (0x8F02)
    writes.push_back({TilemapPlaneUploadContract::kVdpControlPort, TilemapPlaneUploadContract::kVdpAutoincrement2});

    return writes;
}

} // namespace oasis::render
