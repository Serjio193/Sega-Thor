#pragma once

#include "genesis/vdp_state.hpp"

#include <cstdint>
#include <span>

namespace oasis::genesis {

enum class ColorDacMode {
    ModelFullRange,  // Scaled to 0..255 via (v * 255 + 3) / 7 (M12 model / M16.0 default)
    GenesisPlusGx    // Hardware DAC step 0x22 (v * 34: 0, 34, 68, ..., 238)
};

struct RasterMetrics {
    int backdrop_pixels{0};
    int plane_b_pixels{0};
    int plane_a_pixels{0};
    int sprite_pixels{0};
    int window_pixels{0};
    int total_pixels{0};
};

// Renders explicit VDP state into an RGB24 pixel buffer (width * height * 3 bytes).
// Pure native C++, completely decoupled from any emulator or window frontend.
void render_vdp_frame_rgb24(
    const VdpState& state,
    std::span<std::uint8_t> output_rgb24,
    ColorDacMode dac_mode = ColorDacMode::ModelFullRange,
    RasterMetrics* metrics = nullptr
);

// Renders explicit VDP state into an RGBA32 / XRGB32 pixel buffer (width * height * 4 bytes).
// Format is 0x00RRGGBB suitable for direct presentation to NativeWindow or other backends.
void render_vdp_frame_rgba32(
    const VdpState& state,
    std::span<std::uint32_t> output_rgba32,
    ColorDacMode dac_mode = ColorDacMode::GenesisPlusGx,
    RasterMetrics* metrics = nullptr
);

// Renders explicit VDP state into raw logical VDP pixel indices (0..63 in CRAM).
// Pure native hardware representation independent of DAC or RGB color conversions.
void render_vdp_frame_indices(
    const VdpState& state,
    std::span<std::uint8_t> output_indices,
    RasterMetrics* metrics = nullptr
);

} // namespace oasis::genesis
