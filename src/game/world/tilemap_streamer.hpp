#pragma once

#include "genesis/vdp_state.hpp"

#include <cstdint>
#include <deque>
#include <span>
#include <vector>

namespace oasis::game::world {

/// Authentic Beyond Oasis Plane A Tilemap Streamer.
/// Reconstructs the 68000 routines at 0x002084, 0x0023CC (column streaming),
/// 0x002512 (row streaming), and 0x00D188 (camera tracking & dispatch).
/// Accurately manages the 2-frame presentation pipeline delay between
/// camera tile boundary crossing and VRAM name table commit.
class TilemapStreamer {
public:
    static constexpr std::size_t kMapWidth = 64;
    static constexpr std::size_t kMapHeight = 64;
    static constexpr std::size_t kMapTotalWords = kMapWidth * kMapHeight; // 4096 words = 8192 bytes
    static constexpr std::size_t kPlaneVramBase = 0xC000;
    static constexpr std::size_t kPlaneVramSize = 4096;

    TilemapStreamer() = default;

    /// Initialize streamer with camera position, initial name table offset, and map words.
    bool initialize(int initial_cam_x,
                    int initial_cam_y,
                    std::uint16_t initial_nt_off,
                    std::span<const std::uint16_t> map_data);

    /// Load screen map words (4096 words) from a binary file.
    bool load_map_from_file(const char* file_path);

    /// Reset streaming state to checkpoint coordinates.
    void reset(int cam_x, int cam_y, std::uint16_t initial_nt_off = 2176);

    /// Step simulation frame: evaluates camera movement, queues authentic
    /// delayed stream commands, and applies VRAM updates into vdp.
    void step_frame(int cam_x, int cam_y, genesis::VdpState& vdp);

    [[nodiscard]] int current_col_index() const noexcept { return cur_col_idx_; }
    [[nodiscard]] int current_row_index() const noexcept { return cur_row_idx_; }
    [[nodiscard]] std::uint16_t name_table_offset() const noexcept { return nt_off_; }

private:
    struct StreamCommand {
        int target_frame{0};
        int target_col{0};
        int target_row{0};
    };

    void stream_single_column(genesis::VdpState& vdp);
    void stream_single_row(genesis::VdpState& vdp);

    std::vector<std::uint16_t> map_{};
    int cur_col_idx_{12};
    int cur_row_idx_{0};
    std::uint16_t nt_off_{2176}; // 17 * 128 (row 17 in 64x32 Plane A table)
    int prev_cam_x_{96};
    int prev_cam_y_{3};
    int frame_counter_{0};
    std::deque<StreamCommand> pending_commands_{};
};

} // namespace oasis::game::world
