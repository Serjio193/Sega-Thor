#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace oasis::render {

struct TilemapPlaneUploadContract {
    static constexpr unsigned kColumnsPerScreenRow = 40U;
    static constexpr unsigned kWordsPerUpload = 41U;
    static constexpr unsigned kVdpDataPort = 0xC00000U;
    static constexpr unsigned kVdpControlPort = 0xC00004U;
    static constexpr uint16_t kVdpWriteCommandBase = 0x4000U;
};

struct PlaneUploadState {
    const uint16_t* tile_data{nullptr};
    uint16_t vram_base{0xC000U};
    int16_t scroll_counter{0};
    uint16_t vram_offset{0};
    uint16_t plane_width{0x80U};
    uint16_t plane_height{0x20U};
    uint16_t x_pos{0};
    uint16_t y_pos{0};
    uint16_t shift_factor{7U};
};

struct VdpWriteRecord {
    uint32_t port;
    uint16_t value;
};

// Pure domain logic for Mega Drive VDP Plane Nametable Row Streaming (0x002512)
// Advances plane coordinates and emits VDP command and data port writes.
std::vector<VdpWriteRecord> execute_plane_row_upload(PlaneUploadState& state, std::span<const uint16_t> source_tiles);

} // namespace oasis::render
