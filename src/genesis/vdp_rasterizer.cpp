#include "genesis/vdp_rasterizer.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace oasis::genesis {
namespace {

constexpr std::uint8_t expand_3bit(std::uint8_t value) noexcept {
    return static_cast<std::uint8_t>((static_cast<unsigned>(value) * 255U + 3U) / 7U);
}

void get_cram_rgb(const VdpState& state, std::uint8_t cram_index, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) noexcept {
    const std::size_t offset = static_cast<std::size_t>(cram_index & 0x3FU) * 2U;
    const std::uint16_t word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(state.cram[offset]) << 8U) | state.cram[offset + 1U]);
    r = expand_3bit(static_cast<std::uint8_t>((word >> 1U) & 0x07U));
    g = expand_3bit(static_cast<std::uint8_t>((word >> 5U) & 0x07U));
    b = expand_3bit(static_cast<std::uint8_t>((word >> 9U) & 0x07U));
}

int signed10(std::uint16_t val) noexcept {
    const int v = static_cast<int>(val & 0x3FFU);
    return (v & 0x200) ? (v - 1024) : v;
}

int signed16(std::uint16_t val) noexcept {
    return static_cast<int>(static_cast<std::int16_t>(val));
}

void get_hscroll(const VdpState& state, int scanline, int& hscroll_a, int& hscroll_b) noexcept {
    const int mode = state.hscroll_mode();
    std::uint32_t addr = state.hscroll_base();
    if (mode == 1) {
        addr += static_cast<std::uint32_t>((scanline & 7) * 4);
    } else if (mode == 2) {
        addr += static_cast<std::uint32_t>((scanline & ~7) * 4);
    } else if (mode == 3) {
        addr += static_cast<std::uint32_t>(scanline * 4);
    }
    addr &= 0xFFFFU;
    const std::uint16_t wa = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(state.vram[addr]) << 8U) | state.vram[addr + 1U]);
    const std::uint16_t wb = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(state.vram[addr + 2U]) << 8U) | state.vram[addr + 3U]);
    hscroll_a = signed16(wa);
    hscroll_b = signed16(wb);
}

void get_vscroll(const VdpState& state, int screen_x, int& vscroll_a, int& vscroll_b) noexcept {
    const int mode = state.vscroll_mode();
    std::size_t offset = 0;
    if (mode != 0) {
        offset = static_cast<std::size_t>((screen_x / 16) * 4);
    }
    if (offset + 3U < state.vsram.size()) {
        const std::uint16_t wa = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(state.vsram[offset]) << 8U) | state.vsram[offset + 1U]);
        const std::uint16_t wb = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(state.vsram[offset + 2U]) << 8U) | state.vsram[offset + 3U]);
        vscroll_a = signed10(wa);
        vscroll_b = signed10(wb);
    } else {
        vscroll_a = 0;
        vscroll_b = 0;
    }
}

struct PixelSample {
    bool priority{false};
    std::uint8_t palette_line{0};
    std::uint8_t color_index{0};
};

PixelSample sample_plane(const VdpState& state, std::uint32_t plane_base,
                         int cell_x, int cell_y, int fine_x, int fine_y,
                         int plane_w_cells, int plane_h_cells) noexcept {
    cell_x = (cell_x % plane_w_cells + plane_w_cells) % plane_w_cells;
    cell_y = (cell_y % plane_h_cells + plane_h_cells) % plane_h_cells;

    const std::uint32_t cell_addr = (plane_base + static_cast<std::uint32_t>(cell_y * plane_w_cells + cell_x) * 2U) & 0xFFFFU;
    const std::uint16_t word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(state.vram[cell_addr]) << 8U) | state.vram[cell_addr + 1U]);

    const bool priority = (word & 0x8000U) != 0U;
    const auto palette = static_cast<std::uint8_t>((word >> 13U) & 0x03U);
    const bool vflip = (word & 0x1000U) != 0U;
    const bool hflip = (word & 0x0800U) != 0U;
    const auto tile_idx = static_cast<std::uint32_t>(word & 0x07FFU);

    const int fx = hflip ? (7 - fine_x) : fine_x;
    const int fy = vflip ? (7 - fine_y) : fine_y;

    const std::uint32_t byte_addr = ((tile_idx * 32U) + static_cast<std::uint32_t>(fy * 4 + (fx / 2))) & 0xFFFFU;
    const std::uint8_t byte = state.vram[byte_addr];
    const auto color = static_cast<std::uint8_t>((fx & 1) ? (byte & 0x0FU) : ((byte >> 4U) & 0x0FU));

    return {priority, palette, color};
}

struct SpriteInfo {
    int x{0};
    int y{0};
    int w_cells{0};
    int h_cells{0};
    bool priority{false};
    std::uint8_t palette{0};
    bool vflip{false};
    bool hflip{false};
    std::uint32_t base_tile{0};
};

std::vector<SpriteInfo> decode_sat(const VdpState& state) {
    std::vector<SpriteInfo> sprites;
    sprites.reserve(80);
    std::array<bool, 80> visited{};
    std::uint8_t current_idx = 0;

    const std::uint32_t sat_base = state.sat_base();

    while (!visited[current_idx] && current_idx < 80) {
        visited[current_idx] = true;
        const std::uint32_t addr = (sat_base + static_cast<std::uint32_t>(current_idx) * 8U) & 0xFFFFU;

        const auto y_raw = static_cast<int>(
            ((static_cast<std::uint16_t>(state.vram[addr] & 0x03U) << 8U) | state.vram[addr + 1U]));
        const std::uint8_t size_byte = state.vram[addr + 2U];
        const auto link = static_cast<std::uint8_t>(state.vram[addr + 3U] & 0x7FU);
        const auto attr_word = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(state.vram[addr + 4U]) << 8U) | state.vram[addr + 5U]);
        const auto x_raw = static_cast<int>(
            ((static_cast<std::uint16_t>(state.vram[addr + 6U] & 0x03U) << 8U) | state.vram[addr + 7U]));

        const int h_cells = static_cast<int>(size_byte & 0x03U) + 1;
        const int w_cells = static_cast<int>((size_byte >> 2U) & 0x03U) + 1;

        sprites.push_back({
            x_raw - 128,
            y_raw - 128,
            w_cells,
            h_cells,
            (attr_word & 0x8000U) != 0U,
            static_cast<std::uint8_t>((attr_word >> 13U) & 0x03U),
            (attr_word & 0x1000U) != 0U,
            (attr_word & 0x0800U) != 0U,
            static_cast<std::uint32_t>(attr_word & 0x07FFU)
        });

        if (link == 0) break;
        current_idx = link;
    }
    return sprites;
}

bool is_in_window(const VdpState& state, int screen_x, int screen_y, int width) noexcept {
    const std::uint8_t reg17 = state.registers[17];
    const std::uint8_t reg18 = state.registers[18];
    const int win_x_boundary = static_cast<int>(reg17 & 0x1FU) * 16;
    const int win_y_boundary = static_cast<int>(reg18 & 0x1FU) * 8;
    const bool right = (reg17 & 0x80U) != 0U;
    const bool down = (reg18 & 0x80U) != 0U;

    bool h_match = false;
    if (win_x_boundary > 0) {
        h_match = right ? (screen_x >= win_x_boundary) : (screen_x < win_x_boundary);
    }
    bool v_match = false;
    if (win_y_boundary > 0) {
        v_match = down ? (screen_y >= win_y_boundary) : (screen_y < win_y_boundary);
    }
    return h_match || v_match;
}

} // namespace

void render_vdp_frame_rgb24(
    const VdpState& state,
    std::span<std::uint8_t> output_rgb24,
    RasterMetrics* metrics
) {
    const int width = state.display_width();
    const int height = state.display_height();
    const std::size_t required_bytes = static_cast<std::size_t>(width * height * 3);

    if (output_rgb24.size() < required_bytes) {
        throw std::runtime_error("output buffer is smaller than display frame");
    }

    if (metrics) {
        *metrics = {};
        metrics->total_pixels = width * height;
    }

    const auto sprites = decode_sat(state);
    const int plane_w = state.plane_width_cells();
    const int plane_h = state.plane_height_cells();
    const std::uint32_t plane_a_base = state.plane_a_base();
    const std::uint32_t plane_b_base = state.plane_b_base();
    const std::uint32_t window_base = state.window_base();
    const std::uint8_t backdrop_idx = state.backdrop_cram_index();
    const int max_sprites_per_line = state.is_h40() ? 20 : 16;

    std::vector<const SpriteInfo*> line_sprites;
    line_sprites.reserve(max_sprites_per_line);

    for (int y = 0; y < height; ++y) {
        line_sprites.clear();
        for (const auto& spr : sprites) {
            if (spr.y <= y && y < spr.y + spr.h_cells * 8) {
                line_sprites.push_back(&spr);
                if (static_cast<int>(line_sprites.size()) >= max_sprites_per_line) {
                    break;
                }
            }
        }

        int hscroll_a = 0;
        int hscroll_b = 0;
        get_hscroll(state, y, hscroll_a, hscroll_b);

        for (int x = 0; x < width; ++x) {
            int vscroll_a = 0;
            int vscroll_b = 0;
            get_vscroll(state, x, vscroll_a, vscroll_b);

            // Plane B
            const int bx = ((x - hscroll_b) % (plane_w * 8) + (plane_w * 8)) % (plane_w * 8);
            const int by = ((y + vscroll_b) % (plane_h * 8) + (plane_h * 8)) % (plane_h * 8);
            const auto sample_b = sample_plane(state, plane_b_base, bx / 8, by / 8, bx % 8, by % 8, plane_w, plane_h);

            // Plane A / Window
            PixelSample sample_a{};
            bool is_window = is_in_window(state, x, y, width);
            if (is_window) {
                const int win_w_cells = state.is_h40() ? 64 : 32;
                sample_a = sample_plane(state, window_base, x / 8, y / 8, x % 8, y % 8, win_w_cells, 32);
            } else {
                const int ax = ((x - hscroll_a) % (plane_w * 8) + (plane_w * 8)) % (plane_w * 8);
                const int ay = ((y + vscroll_a) % (plane_h * 8) + (plane_h * 8)) % (plane_h * 8);
                sample_a = sample_plane(state, plane_a_base, ax / 8, ay / 8, ax % 8, ay % 8, plane_w, plane_h);
            }

            // Sprite pixel
            PixelSample sample_s{};
            for (const auto* spr : line_sprites) {
                if (spr->x <= x && x < spr->x + spr->w_cells * 8) {
                    int spr_x = x - spr->x;
                    int spr_y = y - spr->y;
                    if (spr->hflip) spr_x = spr->w_cells * 8 - 1 - spr_x;
                    if (spr->vflip) spr_y = spr->h_cells * 8 - 1 - spr_y;

                    const int col = spr_x / 8;
                    const int row = spr_y / 8;
                    const auto tile_idx = static_cast<std::uint32_t>(spr->base_tile + col * spr->h_cells + row) & 0x07FFU;
                    const int fx = spr_x % 8;
                    const int fy = spr_y % 8;

                    const std::uint32_t byte_addr = ((tile_idx * 32U) + static_cast<std::uint32_t>(fy * 4 + (fx / 2))) & 0xFFFFU;
                    const std::uint8_t byte = state.vram[byte_addr];
                    const auto color = static_cast<std::uint8_t>((fx & 1) ? (byte & 0x0FU) : ((byte >> 4U) & 0x0FU));

                    if (color != 0) {
                        sample_s = {spr->priority, spr->palette, color};
                        break;
                    }
                }
            }

            // Priority composition
            std::uint8_t winner_cram = backdrop_idx;
            enum class Layer { BACKDROP, PLANE_B, PLANE_A, SPRITE, WINDOW };
            Layer winner_layer = Layer::BACKDROP;

            if (sample_b.color_index != 0) {
                winner_layer = Layer::PLANE_B;
                winner_cram = static_cast<std::uint8_t>(sample_b.palette_line * 16U + sample_b.color_index);
            }
            if (sample_a.color_index != 0) {
                winner_layer = is_window ? Layer::WINDOW : Layer::PLANE_A;
                winner_cram = static_cast<std::uint8_t>(sample_a.palette_line * 16U + sample_a.color_index);
            }
            if (sample_s.color_index != 0) {
                winner_layer = Layer::SPRITE;
                winner_cram = static_cast<std::uint8_t>(sample_s.palette_line * 16U + sample_s.color_index);
            }
            if (sample_b.priority && sample_b.color_index != 0) {
                winner_layer = Layer::PLANE_B;
                winner_cram = static_cast<std::uint8_t>(sample_b.palette_line * 16U + sample_b.color_index);
            }
            if (sample_a.priority && sample_a.color_index != 0) {
                winner_layer = is_window ? Layer::WINDOW : Layer::PLANE_A;
                winner_cram = static_cast<std::uint8_t>(sample_a.palette_line * 16U + sample_a.color_index);
            }
            if (sample_s.priority && sample_s.color_index != 0) {
                winner_layer = Layer::SPRITE;
                winner_cram = static_cast<std::uint8_t>(sample_s.palette_line * 16U + sample_s.color_index);
            }

            if (metrics) {
                switch (winner_layer) {
                case Layer::BACKDROP: metrics->backdrop_pixels++; break;
                case Layer::PLANE_B:  metrics->plane_b_pixels++; break;
                case Layer::PLANE_A:  metrics->plane_a_pixels++; break;
                case Layer::SPRITE:   metrics->sprite_pixels++; break;
                case Layer::WINDOW:   metrics->window_pixels++; break;
                }
            }

            const std::size_t out_idx = static_cast<std::size_t>(y * width + x) * 3U;
            get_cram_rgb(state, winner_cram, output_rgb24[out_idx], output_rgb24[out_idx + 1U], output_rgb24[out_idx + 2U]);
        }
    }
}

} // namespace oasis::genesis
