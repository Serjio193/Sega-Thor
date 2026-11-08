#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace oasis::game::render {

void draw_debug_text(std::span<std::uint32_t> pixels,
                     std::size_t width,
                     std::size_t height,
                     int x,
                     int y,
                     std::string_view text,
                     std::uint32_t color = 0x00FFFFFFU,
                     std::uint32_t bg_color = 0x00000000U) noexcept;

void fill_debug_rect(std::span<std::uint32_t> pixels,
                     std::size_t width,
                     std::size_t height,
                     int x,
                     int y,
                     int rect_w,
                     int rect_h,
                     std::uint32_t color) noexcept;

} // namespace oasis::game::render
