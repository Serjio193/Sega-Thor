#include "game/render/palette_scaler.hpp"

namespace oasis::render {

uint16_t scale_genesis_color(uint16_t color, uint8_t intensity) noexcept {
    const uint32_t factor = intensity & 0x07U;
    const uint32_t r = ((color & 0x000EU) * factor) / 7U;
    const uint32_t g = (((color & 0x00E0U) * factor) / 7U) & 0x00E0U;
    const uint32_t b = (((color & 0x0E00U) * factor) / 7U) & 0x0E00U;
    return static_cast<uint16_t>((r & 0x000EU) | g | b);
}

void scale_palette(std::span<const uint16_t> src, std::span<uint16_t> dst, uint8_t intensity) noexcept {
    const auto count = src.size() < dst.size() ? src.size() : dst.size();
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = scale_genesis_color(src[i], intensity);
    }
}

} // namespace oasis::render
