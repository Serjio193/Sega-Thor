#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::render {

// Exact 8-byte Mega Drive VDP Sprite Attribute Table entry.
struct MegaDriveSatEntry {
    std::uint16_t y_pos{};       // Screen Y + 128 (bits 9..0)
    std::uint8_t size{};         // ((width_tiles - 1) << 2) | (height_tiles - 1)
    std::uint8_t link{};         // Next sprite index in chain (0 terminates)
    std::uint16_t attr{};        // Priority (bit 15), Palette (14..13), VFlip (12), HFlip (11), Tile (10..0)
    std::uint16_t x_pos{};       // Screen X + 128 (bits 9..0)
};
static_assert(sizeof(MegaDriveSatEntry) == 8, "MegaDriveSatEntry must be exactly 8 bytes");

struct SatFrameSummary {
    std::uint16_t active_count{};
    std::uint16_t buffer_bytes{};
    bool is_dirty{};
};

class SatPipelineContract {
public:
    static constexpr std::size_t kMaxSprites = 80;
    static constexpr std::size_t kBufferSize = kMaxSprites * sizeof(MegaDriveSatEntry); // 640 bytes
    static constexpr int kCoordOffset = 128; // VDP coordinate base offset

    SatPipelineContract() noexcept;

    void reset_frame() noexcept;

    // Emits a single sprite piece into the SAT buffer.
    // Returns true if piece was visible and committed, false if clipped or table full.
    bool emit_piece(int screen_x, int screen_y,
                    std::uint8_t width_tiles, std::uint8_t height_tiles,
                    std::uint16_t tile_index, std::uint8_t palette,
                    bool priority, bool h_flip, bool v_flip) noexcept;

    // Emits an entity shadow piece at the specified base coordinates.
    bool emit_shadow(int screen_x, int screen_y,
                     std::uint16_t tile_index, std::uint8_t palette) noexcept;

    // Finalizes the frame: terminates link chain (link=0) and marks dirty for VDP DMA.
    SatFrameSummary finalize_frame() noexcept;

    [[nodiscard]] std::uint16_t active_count() const noexcept { return active_count_; }
    [[nodiscard]] std::span<const std::uint8_t> raw_buffer() const noexcept;
    [[nodiscard]] std::span<const MegaDriveSatEntry> entries() const noexcept;

private:
    std::array<MegaDriveSatEntry, kMaxSprites> entries_{};
    std::uint16_t active_count_{};
    bool dirty_{};
};

} // namespace oasis::game::render
