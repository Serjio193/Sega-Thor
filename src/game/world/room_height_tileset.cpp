#include "game/world/room_height_tileset.hpp"

#include <algorithm>

namespace oasis::game::world {

HeightBandTables build_height_band_tables(
    const std::array<std::int8_t, kHeightBandCount>& band_bytes) noexcept {
    HeightBandTables out{};
    std::int16_t accum = 0;
    for (std::size_t i = 0U; i < kHeightBandCount; ++i) {
        const std::int16_t step =
            static_cast<std::int16_t>(static_cast<std::int32_t>(band_bytes[i]) * 8);
        out.band_steps_4w[i] = step;
        out.height_lut_8w[i * 2U] = accum;
        out.height_lut_8w[i * 2U + 1U] = static_cast<std::int16_t>(accum + (step >> 1));
        accum = static_cast<std::int16_t>(accum + step);
    }
    out.height_lut_8w[7] = static_cast<std::int16_t>(kHeightLutSentinel);
    return out;
}

QuantizedCameraPos quantize_camera_pos(
    std::int32_t x,
    std::int32_t y) noexcept {
    return {
        std::max<std::int32_t>(0, x & ~7),
        std::max<std::int32_t>(0, y & ~7),
    };
}

void flush_vscroll_to_vdp_buffer(
    std::uint16_t vdp_vscroll,
    bool is_plane_b,
    bool per_col_mode,
    std::span<std::uint16_t, 40> vscroll_col_buf,
    std::array<std::uint16_t, 2>& vscroll_full_buf) noexcept {
    const std::size_t lane = is_plane_b ? 1U : 0U;
    if (per_col_mode) {
        for (std::size_t i = lane; i < vscroll_col_buf.size(); i += 2U) {
            vscroll_col_buf[i] = vdp_vscroll;
        }
    } else {
        vscroll_full_buf[lane] = vdp_vscroll;
    }
}

} // namespace oasis::game::world
