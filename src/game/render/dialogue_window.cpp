#include "game/render/dialogue_window.hpp"

#include <algorithm>

namespace oasis::game::render {
namespace {

// 5x7 font glyph for clean text rendering into RGB24
struct FontGlyph {
    char ch{};
    std::uint8_t cols[5]{};
};

constexpr FontGlyph kFont[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
    {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
    {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
    {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
    {'W', {0x7F, 0x20, 0x18, 0x20, 0x7F}},
    {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
    {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
    {'a', {0x20, 0x54, 0x54, 0x54, 0x78}},
    {'b', {0x7F, 0x48, 0x44, 0x44, 0x38}},
    {'c', {0x38, 0x44, 0x44, 0x44, 0x20}},
    {'d', {0x38, 0x44, 0x44, 0x48, 0x7F}},
    {'e', {0x38, 0x54, 0x54, 0x54, 0x18}},
    {'f', {0x08, 0x7E, 0x09, 0x01, 0x02}},
    {'g', {0x0C, 0x52, 0x52, 0x52, 0x3E}},
    {'h', {0x7F, 0x08, 0x04, 0x04, 0x78}},
    {'i', {0x00, 0x44, 0x7D, 0x40, 0x00}},
    {'j', {0x20, 0x40, 0x44, 0x3D, 0x00}},
    {'k', {0x7F, 0x10, 0x28, 0x44, 0x00}},
    {'l', {0x00, 0x41, 0x7F, 0x40, 0x00}},
    {'m', {0x7C, 0x04, 0x18, 0x04, 0x78}},
    {'n', {0x7C, 0x08, 0x04, 0x04, 0x78}},
    {'o', {0x38, 0x44, 0x44, 0x44, 0x38}},
    {'p', {0x7C, 0x14, 0x14, 0x14, 0x08}},
    {'q', {0x08, 0x14, 0x14, 0x18, 0x7C}},
    {'r', {0x7C, 0x08, 0x04, 0x04, 0x08}},
    {'s', {0x48, 0x54, 0x54, 0x54, 0x20}},
    {'t', {0x04, 0x3F, 0x44, 0x40, 0x20}},
    {'u', {0x3C, 0x40, 0x40, 0x20, 0x7C}},
    {'v', {0x1C, 0x20, 0x40, 0x20, 0x1C}},
    {'w', {0x3C, 0x40, 0x30, 0x40, 0x3C}},
    {'x', {0x44, 0x28, 0x10, 0x28, 0x44}},
    {'y', {0x0C, 0x50, 0x50, 0x50, 0x3C}},
    {'z', {0x44, 0x64, 0x54, 0x4C, 0x44}},
    {',', {0x00, 0x50, 0x30, 0x00, 0x00}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {'?', {0x02, 0x01, 0x51, 0x09, 0x06}},
    {'!', {0x00, 0x00, 0x5F, 0x00, 0x00}},
    {'\'', {0x00, 0x07, 0x03, 0x00, 0x00}}
};

const FontGlyph* find_glyph(char c) noexcept {
    for (const auto& g : kFont) {
        if (g.ch == c) return &g;
    }
    return nullptr;
}

} // namespace

DialogueWindow::DialogueWindow() noexcept {
    reset();
}

void DialogueWindow::reset() noexcept {
    is_open_ = false;
    sequence_ = {};
    current_page_index_ = 0;
    frame_counter_ = 0;
}

void DialogueWindow::open(event::DialogueSequence sequence) noexcept {
    sequence_ = std::move(sequence);
    current_page_index_ = 0;
    frame_counter_ = 0;
    is_open_ = !sequence_.pages.empty();
}

void DialogueWindow::close() noexcept {
    is_open_ = false;
    sequence_ = {};
    current_page_index_ = 0;
}

bool DialogueWindow::advance_page() noexcept {
    if (!is_open_) return false;
    if (current_page_index_ + 1U < sequence_.pages.size()) {
        ++current_page_index_;
        return true;
    }
    close();
    return false;
}

void DialogueWindow::step_frame() noexcept {
    if (is_open_) {
        ++frame_counter_;
    }
}

void DialogueWindow::render_to_vdp(genesis::VdpState& vdp) const noexcept {
    if (!is_open_ || sequence_.pages.empty()) return;

    // Set background color of dialogue box in Plane A nametable
    // Plane A base is typically 0xC000
    const std::uint16_t plane_a_base = 0xC000U;
    const auto& page = sequence_.pages[current_page_index_];

    // Mark border and text lines into nametable
    const int start_x = config_.start_tile_x;
    const int start_y = config_.start_tile_y;

    for (int y = 0; y < config_.height_tiles; ++y) {
        for (int x = 0; x < config_.width_tiles; ++x) {
            const int tx = start_x + x;
            const int ty = start_y + y;
            if (tx < 64 && ty < 32) {
                const std::size_t addr = plane_a_base + static_cast<std::size_t>((ty * 64 + tx) * 2);
                if (addr + 1U < vdp.vram.size()) {
                    // Pattern: border or solid dialogue interior tile
                    const std::uint16_t tile = config_.border_pattern_base;
                    vdp.vram[addr] = static_cast<std::uint8_t>((tile >> 8U) & 0xFFU);
                    vdp.vram[addr + 1U] = static_cast<std::uint8_t>(tile & 0xFFU);
                }
            }
        }
    }
}

void DialogueWindow::render_to_rgb(std::span<std::uint8_t> rgb_buffer) const noexcept {
    if (!is_open_ || sequence_.pages.empty() || rgb_buffer.size() < 320U * 224U * 3U) return;

    const auto& page = sequence_.pages[current_page_index_];
    const int box_x = 16;
    const int box_y = 156;
    const int box_w = 288;
    const int box_h = 56;

    // Draw dark background box with border
    for (int y = box_y; y < box_y + box_h; ++y) {
        for (int x = box_x; x < box_x + box_w; ++x) {
            const std::size_t idx = static_cast<std::size_t>((y * 320 + x) * 3);
            const bool is_border = (x == box_x || x == box_x + box_w - 1 ||
                                    y == box_y || y == box_y + box_h - 1);
            if (is_border) {
                rgb_buffer[idx] = 220;
                rgb_buffer[idx + 1] = 220;
                rgb_buffer[idx + 2] = 220;
            } else {
                // Dim background
                rgb_buffer[idx] = 16;
                rgb_buffer[idx + 1] = 20;
                rgb_buffer[idx + 2] = 36;
            }
        }
    }

    // Render text lines
    int text_y = box_y + 8;
    for (const auto& line : page.lines) {
        int text_x = box_x + 12;
        for (char c : line) {
            const auto* g = find_glyph(c);
            if (g != nullptr) {
                for (int col = 0; col < 5; ++col) {
                    const std::uint8_t col_bits = g->cols[col];
                    for (int row = 0; row < 7; ++row) {
                        if ((col_bits & (1 << row)) != 0) {
                            const int px = text_x + col;
                            const int py = text_y + row;
                            if (px >= 0 && px < 320 && py >= 0 && py < 224) {
                                const std::size_t idx = static_cast<std::size_t>((py * 320 + px) * 3);
                                rgb_buffer[idx] = 255;
                                rgb_buffer[idx + 1] = 255;
                                rgb_buffer[idx + 2] = 255;
                            }
                        }
                    }
                }
            }
            text_x += 6;
        }
        text_y += 12;
    }
}

} // namespace oasis::game::render
