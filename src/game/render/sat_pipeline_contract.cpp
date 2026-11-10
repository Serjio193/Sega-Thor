#include "game/render/sat_pipeline_contract.hpp"
#include "genesis/vdp_state.hpp"

#include <cstring>

namespace oasis::game::render {

SatPipelineContract::SatPipelineContract() noexcept {
    reset_frame();
}

void SatPipelineContract::reset_frame() noexcept {
    entries_.fill(MegaDriveSatEntry{});
    active_count_ = 0;
    dirty_ = false;
}

bool SatPipelineContract::emit_piece(int screen_x, int screen_y,
                                     std::uint8_t width_tiles, std::uint8_t height_tiles,
                                     std::uint16_t tile_index, std::uint8_t palette,
                                     bool priority, bool h_flip, bool v_flip) noexcept {
    if (active_count_ >= kMaxSprites) {
        return false; // Table capacity full
    }
    if (width_tiles < 1 || width_tiles > 4 || height_tiles < 1 || height_tiles > 4) {
        return false; // Invalid sprite dimensions
    }

    const int pixel_w = static_cast<int>(width_tiles) * 8;
    const int pixel_h = static_cast<int>(height_tiles) * 8;

    // Viewport visibility culling (Mega Drive 320x224 active area)
    if (screen_x + pixel_w <= 0 || screen_x >= 320 ||
        screen_y + pixel_h <= 0 || screen_y >= 224) {
        return false; // Clipped outside screen
    }

    const int vdp_x = screen_x + kCoordOffset;
    const int vdp_y = screen_y + kCoordOffset;

    const std::uint8_t size_byte = static_cast<std::uint8_t>(
        ((width_tiles - 1U) << 2U) | (height_tiles - 1U)
    );

    // Build attribute word: Priority (bit 15), Palette (14..13), VFlip (12), HFlip (11), Tile (10..0)
    std::uint16_t attr = tile_index & 0x07FFU;
    if (h_flip) attr |= 0x0800U;
    if (v_flip) attr |= 0x1000U;
    attr |= static_cast<std::uint16_t>((palette & 0x03U) << 13U);
    if (priority) attr |= 0x8000U;

    auto& entry = entries_[active_count_];
    entry.y_pos = static_cast<std::uint16_t>(vdp_y & 0x03FFU);
    entry.size = size_byte;
    entry.link = static_cast<std::uint8_t>(active_count_ + 1U); // Link to subsequent entry
    entry.attr = attr;
    entry.x_pos = static_cast<std::uint16_t>(vdp_x & 0x01FFU);

    active_count_++;
    dirty_ = true;
    return true;
}

bool SatPipelineContract::emit_rom_piece(int vdp_x, int vdp_y,
                                         std::uint8_t size_byte,
                                         std::uint16_t raw_attr) noexcept {
    if (active_count_ >= kMaxSprites) {
        return false;
    }
    const std::uint16_t wy = static_cast<std::uint16_t>(vdp_y) & 0xFFFFU;
    const std::uint16_t wx = static_cast<std::uint16_t>(vdp_x) & 0xFFFFU;
    if (static_cast<std::uint16_t>(wy - 0x0060U) > 0x0100U ||
        static_cast<std::uint16_t>(wx - 0x0060U) > 0x0160U) {
        return false;
    }
    auto& entry = entries_[active_count_];
    entry.y_pos = static_cast<std::uint16_t>(wy & 0x03FFU);
    entry.size = size_byte;
    entry.link = static_cast<std::uint8_t>(active_count_ + 1U);
    entry.attr = raw_attr;
    entry.x_pos = static_cast<std::uint16_t>(wx & 0x01FFU);
    active_count_++;
    dirty_ = true;
    return true;
}

bool SatPipelineContract::emit_shadow(int screen_x, int screen_y,
                                      std::uint16_t tile_index, std::uint8_t palette) noexcept {
    // Mega Drive Beyond Oasis oval shadow: 2x1 tiles (16x8 pixels), palette 3, low priority
    return emit_piece(screen_x, screen_y, 2, 1, tile_index, palette, false, false, false);
}

SatFrameSummary SatPipelineContract::finalize_frame() noexcept {
    if (active_count_ > 0) {
        // Terminate link list at the last active sprite
        entries_[active_count_ - 1U].link = 0U;
    }
    SatFrameSummary summary{};
    summary.active_count = active_count_;
    summary.buffer_bytes = static_cast<std::uint16_t>(active_count_ * sizeof(MegaDriveSatEntry));
    summary.is_dirty = dirty_;
    return summary;
}

void SatPipelineContract::commit_to_vdp(genesis::VdpState& vdp) const noexcept {
    const std::size_t sat_base = (static_cast<std::size_t>(vdp.registers[5] & 0x7FU)) << 9U;
    for (std::size_t i = 0; i < active_count_; ++i) {
        const auto& e = entries_[i];
        const std::size_t addr = sat_base + (i * 8U);
        if (addr + 8U <= vdp.vram.size()) {
            vdp.vram[addr + 0U] = static_cast<std::uint8_t>((e.y_pos >> 8U) & 0xFFU);
            vdp.vram[addr + 1U] = static_cast<std::uint8_t>(e.y_pos & 0xFFU);
            vdp.vram[addr + 2U] = e.size;
            vdp.vram[addr + 3U] = e.link;
            vdp.vram[addr + 4U] = static_cast<std::uint8_t>((e.attr >> 8U) & 0xFFU);
            vdp.vram[addr + 5U] = static_cast<std::uint8_t>(e.attr & 0xFFU);
            vdp.vram[addr + 6U] = static_cast<std::uint8_t>((e.x_pos >> 8U) & 0xFFU);
            vdp.vram[addr + 7U] = static_cast<std::uint8_t>(e.x_pos & 0xFFU);
        }
    }
}

std::vector<MegaDriveSatEntry> SatPipelineContract::active_chain() const {
    std::vector<MegaDriveSatEntry> chain;
    chain.reserve(active_count_);
    std::array<bool, kMaxSprites> visited{};
    std::uint8_t idx = 0;
    while (!visited[idx] && idx < active_count_) {
        visited[idx] = true;
        chain.push_back(entries_[idx]);
        if (entries_[idx].link == 0) break;
        idx = entries_[idx].link;
    }
    return chain;
}

std::span<const std::uint8_t> SatPipelineContract::raw_buffer() const noexcept {
    return std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(entries_.data()),
        kBufferSize
    );
}

std::span<const MegaDriveSatEntry> SatPipelineContract::entries() const noexcept {
    return std::span<const MegaDriveSatEntry>(entries_.data(), active_count_);
}

} // namespace oasis::game::render
