#pragma once

#include <cstdint>
#include <span>

namespace oasis::render {

struct PaletteScalingContract {
    static constexpr unsigned kMaxIntensity = 7U;
    static constexpr unsigned kColorsPerPalette = 16U;
    static constexpr unsigned kTotalPalettes = 4U;
    static constexpr unsigned kTotalColors = 64U;
};

// Pure domain logic for Genesis palette intensity scaling (0x002FDA)
// Scales 16-bit Genesis color (0000_BBB0_GGG0_RRR0) by intensity (0..7).
uint16_t scale_genesis_color(uint16_t color, uint8_t intensity) noexcept;

// Scales an entire palette buffer of Genesis colors
void scale_palette(std::span<const uint16_t> src, std::span<uint16_t> dst, uint8_t intensity) noexcept;

} // namespace oasis::render
