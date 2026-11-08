#include "game/render/sprite_compositor.hpp"
#include "genesis/vdp_state.hpp"

#include <algorithm>

namespace oasis::game::render {

SpriteCompositor::SpriteCompositor() noexcept {
    reset_frame();
}

void SpriteCompositor::reset_frame() noexcept {
    records_.fill(MegaDriveSatEntry{});
    active_count_ = 0;
    dirty_ = false;
}

bool SpriteCompositor::compose_mode0(int world_x, int world_y,
                                    std::span<const SpritePiece> pieces,
                                    bool flip_h, bool flip_v) noexcept {
    const int screen_base_x = world_x - cam_x_;
    const int screen_base_y = world_y - cam_y_;

    bool all_emitted = true;
    for (const auto& piece : pieces) {
        const int px = flip_h ? (-piece.offset_x - (static_cast<int>(piece.width_tiles) * 8)) : piece.offset_x;
        const int py = flip_v ? (-piece.offset_y - (static_cast<int>(piece.height_tiles) * 8)) : piece.offset_y;

        SpritePiece transformed = piece;
        transformed.h_flip = piece.h_flip ^ flip_h;
        transformed.v_flip = piece.v_flip ^ flip_v;

        if (!emit_piece(transformed, screen_base_x + px, screen_base_y + py)) {
            all_emitted = false;
        }
    }
    return all_emitted;
}

bool SpriteCompositor::compose_mode1(int world_x, int world_y,
                                    std::span<const SpritePiece> pieces,
                                    bool flip_h, bool flip_v,
                                    bool absolute_coords) noexcept {
    const int screen_base_x = absolute_coords ? world_x : (world_x - cam_x_);
    const int screen_base_y = absolute_coords ? world_y : (world_y - cam_y_);

    bool all_emitted = true;
    for (const auto& piece : pieces) {
        const int px = flip_h ? (-piece.offset_x - (static_cast<int>(piece.width_tiles) * 8)) : piece.offset_x;
        const int py = flip_v ? (-piece.offset_y - (static_cast<int>(piece.height_tiles) * 8)) : piece.offset_y;

        SpritePiece transformed = piece;
        transformed.h_flip = piece.h_flip ^ flip_h;
        transformed.v_flip = piece.v_flip ^ flip_v;

        if (!emit_piece(transformed, screen_base_x + px, screen_base_y + py)) {
            all_emitted = false;
        }
    }
    return all_emitted;
}

bool SpriteCompositor::emit_piece(const SpritePiece& piece, int screen_x, int screen_y) noexcept {
    if (active_count_ >= kMaxSprites) return false;

    const int w_px = static_cast<int>(piece.width_tiles) * 8;
    const int h_px = static_cast<int>(piece.height_tiles) * 8;

    // Viewport clipping check
    if (screen_x + w_px <= 0 || screen_x >= kScreenWidth ||
        screen_y + h_px <= 0 || screen_y >= kScreenHeight) {
        return false;
    }

    auto& entry = records_[active_count_];
    entry.y_pos = static_cast<std::uint16_t>((screen_y + kCoordOffset) & 0x03FFU);
    entry.size = static_cast<std::uint8_t>(((piece.width_tiles - 1U) << 2U) | (piece.height_tiles - 1U));
    entry.link = static_cast<std::uint8_t>(active_count_ + 1U);

    std::uint16_t attr = piece.tile_index & 0x07FFU;
    if (piece.priority) attr |= 0x8000U;
    attr |= static_cast<std::uint16_t>((piece.palette & 0x03U) << 13U);
    if (piece.v_flip) attr |= 0x1000U;
    if (piece.h_flip) attr |= 0x0800U;
    entry.attr = attr;

    entry.x_pos = static_cast<std::uint16_t>((screen_x + kCoordOffset) & 0x01FFU);

    ++active_count_;
    dirty_ = true;
    return true;
}

bool SpriteCompositor::emit_shadow(int screen_x, int screen_y,
                                  std::uint16_t tile_index, std::uint8_t palette) noexcept {
    SpritePiece shadow_piece;
    shadow_piece.width_tiles = 2;
    shadow_piece.height_tiles = 1;
    shadow_piece.tile_index = tile_index;
    shadow_piece.palette = palette;
    shadow_piece.priority = false;
    return emit_piece(shadow_piece, screen_x, screen_y);
}

SatFrameSummary SpriteCompositor::finalize_frame() noexcept {
    if (active_count_ > 0) {
        records_[active_count_ - 1U].link = 0; // Terminate hardware link chain
    }
    const SatFrameSummary summary{
        .active_count = static_cast<std::uint16_t>(active_count_),
        .buffer_bytes = static_cast<std::uint16_t>(active_count_ * sizeof(MegaDriveSatEntry)),
        .is_dirty = dirty_,
    };
    dirty_ = false;
    return summary;
}

void SpriteCompositor::commit_to_vdp(genesis::VdpState& vdp) const noexcept {
    const std::size_t sat_base = (static_cast<std::size_t>(vdp.registers[5] & 0x7FU)) << 9U;
    for (std::size_t i = 0; i < active_count_; ++i) {
        const auto& e = records_[i];
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

std::vector<MegaDriveSatEntry> SpriteCompositor::active_chain() const {
    std::vector<MegaDriveSatEntry> chain;
    chain.reserve(active_count_);
    std::array<bool, kMaxSprites> visited{};
    std::uint8_t idx = 0;
    while (!visited[idx] && idx < active_count_) {
        visited[idx] = true;
        chain.push_back(records_[idx]);
        if (records_[idx].link == 0) break;
        idx = records_[idx].link;
    }
    return chain;
}

std::span<const MegaDriveSatEntry> SpriteCompositor::records() const noexcept {
    return std::span<const MegaDriveSatEntry>(records_.data(), active_count_);
}

} // namespace oasis::game::render
