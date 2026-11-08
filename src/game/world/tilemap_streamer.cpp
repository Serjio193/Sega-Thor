#include "game/world/tilemap_streamer.hpp"

#include <fstream>

namespace oasis::game::world {

bool TilemapStreamer::initialize(int initial_cam_x,
                                 int initial_cam_y,
                                 std::uint16_t initial_nt_off,
                                 std::span<const std::uint16_t> map_data) {
    if (!map_data.empty()) {
        map_.assign(map_data.begin(), map_data.end());
    }
    reset(initial_cam_x, initial_cam_y, initial_nt_off);
    return !map_.empty();
}

bool TilemapStreamer::load_map_from_file(const char* file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) return false;
    map_.resize(kMapTotalWords);
    std::vector<std::uint8_t> raw(kMapTotalWords * 2U);
    in.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()));
    if (in.gcount() != static_cast<std::streamsize>(raw.size())) {
        map_.clear();
        return false;
    }
    for (std::size_t i = 0; i < kMapTotalWords; ++i) {
        map_[i] = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(raw[i * 2U]) << 8U) |
            static_cast<std::uint16_t>(raw[i * 2U + 1U]));
    }
    return true;
}

void TilemapStreamer::reset(int cam_x, int cam_y, std::uint16_t initial_nt_off) {
    cur_col_idx_ = cam_x / 8;
    cur_row_idx_ = cam_y / 8;
    nt_off_ = initial_nt_off;
    prev_cam_x_ = cam_x;
    prev_cam_y_ = cam_y;
    frame_counter_ = 0;
    pending_commands_.clear();
}

void TilemapStreamer::step_frame(int cam_x, int cam_y, genesis::VdpState& vdp) {
    ++frame_counter_;

    if (map_.empty()) return;

    const int target_col = cam_x / 8;
    const int target_row = cam_y / 8;

    // Authentic Beyond Oasis dispatch (ROM 0x00D188):
    // When camera advances past 8-pixel tile boundaries, queue streaming
    // with deterministic 2-frame pipeline delay (+2 VDP frames = 1 30Hz loop tick).
    if (frame_counter_ > 1) {
        const int prev_col = prev_cam_x_ / 8;
        const int prev_row = prev_cam_y_ / 8;
        if (target_col != prev_col || target_row != prev_row) {
            StreamCommand cmd{};
            cmd.target_frame = frame_counter_ + 2;
            cmd.target_col = target_col;
            cmd.target_row = target_row;
            pending_commands_.push_back(cmd);
        }
    }

    prev_cam_x_ = cam_x;
    prev_cam_y_ = cam_y;

    // Process all pending stream commands scheduled for current frame
    while (!pending_commands_.empty() && pending_commands_.front().target_frame <= frame_counter_) {
        const auto cmd = pending_commands_.front();
        pending_commands_.pop_front();

        while (cur_col_idx_ < cmd.target_col) {
            stream_single_column(vdp);
        }
        while (cur_row_idx_ < cmd.target_row) {
            stream_single_row(vdp);
        }
    }
}

void TilemapStreamer::stream_single_column(genesis::VdpState& vdp) {
    // ROM routine 0x0023CC: advance column in map and name table offset
    ++cur_col_idx_;
    cur_col_idx_ %= static_cast<int>(kMapWidth);

    // Update horizontal component of name table offset (0x2438: d1 = ((d1 & 0x7E) + 2) & 0x7E)
    const std::uint16_t row_part = nt_off_ & 0x0F80U;
    const std::uint16_t col_part = ((nt_off_ & 0x007EU) + 2U) & 0x007EU;
    nt_off_ = row_part | col_part;

    // Fetch column on the leading edge (0x2420: fetch_col = (col_idx + 40) % 64)
    const int fetch_col = (cur_col_idx_ + 40) % static_cast<int>(kMapWidth);

    // Target VRAM column (0x2450: vram_col = ((col_part + 0x50) & 0x7E) / 2)
    const int vram_col = static_cast<int>(((col_part + 0x0050U) & 0x007EU) / 2U);
    const int vram_start_row = static_cast<int>((nt_off_ >> 7U) % 32U);

    // Write 29 rows (0x24C8: d7 = 0x1C)
    for (int r = 0; r < 29; ++r) {
        const int map_r = (cur_row_idx_ + r) % static_cast<int>(kMapHeight);
        const auto word = map_[static_cast<std::size_t>(map_r) * kMapWidth + static_cast<std::size_t>(fetch_col)];
        const int vr = (vram_start_row + r) % 32;
        const std::size_t vram_addr = kPlaneVramBase + (static_cast<std::size_t>(vr) * 64U + static_cast<std::size_t>(vram_col)) * 2U;
        if (vram_addr + 1U < vdp.vram.size()) {
            vdp.vram[vram_addr] = static_cast<std::uint8_t>((word >> 8U) & 0xFFU);
            vdp.vram[vram_addr + 1U] = static_cast<std::uint8_t>(word & 0xFFU);
        }
    }
}

void TilemapStreamer::stream_single_row(genesis::VdpState& vdp) {
    // ROM routine 0x002512: advance row in map and name table offset
    ++cur_row_idx_;
    cur_row_idx_ %= static_cast<int>(kMapHeight);

    // Update vertical component of name table offset (nt_off = (nt_off + 128) & 0xFFF)
    nt_off_ = (nt_off_ + 128U) & 0x0FFFU;

    // Fetch row on the leading bottom edge (0x2558: fetch_row = (row_idx + 28) % 64)
    const int fetch_row = (cur_row_idx_ + 28) % static_cast<int>(kMapHeight);

    // Target VRAM row (0x2588: vram_row = ((nt_off >> 7) + 28) % 32)
    const int vram_row = static_cast<int>(((nt_off_ >> 7U) + 28U) % 32U);
    const int vram_start_col = static_cast<int>((nt_off_ & 0x007EU) / 2U);

    // Write 41 words horizontally (0x25FC: d7 = 0x28)
    for (int c = 0; c < 41; ++c) {
        const int map_c = (cur_col_idx_ + c) % static_cast<int>(kMapWidth);
        const auto word = map_[static_cast<std::size_t>(fetch_row) * kMapWidth + static_cast<std::size_t>(map_c)];
        const int vc = (vram_start_col + c) % 64;
        const std::size_t vram_addr = kPlaneVramBase + (static_cast<std::size_t>(vram_row) * 64U + static_cast<std::size_t>(vc)) * 2U;
        if (vram_addr + 1U < vdp.vram.size()) {
            vdp.vram[vram_addr] = static_cast<std::uint8_t>((word >> 8U) & 0xFFU);
            vdp.vram[vram_addr + 1U] = static_cast<std::uint8_t>(word & 0xFFU);
        }
    }
}

} // namespace oasis::game::world
