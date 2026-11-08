#pragma once

#include "genesis/vdp_state.hpp"

#include <cstdint>
#include <span>

namespace oasis::genesis {

struct RasterMetrics {
    int backdrop_pixels{0};
    int plane_b_pixels{0};
    int plane_a_pixels{0};
    int sprite_pixels{0};
    int window_pixels{0};
    int total_pixels{0};
};

// Renders the explicit VDP state into an RGB24 pixel buffer (width * height * 3 bytes).
// Pure native C++, completely decoupled from any emulator or window frontend.
void render_vdp_frame_rgb24(
    const VdpState& state,
    std::span<std::uint8_t> output_rgb24,
    RasterMetrics* metrics = nullptr
);

} // namespace oasis::genesis
