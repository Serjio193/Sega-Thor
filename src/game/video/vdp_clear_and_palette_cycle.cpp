#include "game/video/vdp_clear_and_palette_cycle.hpp"

namespace oasis::game::video {

std::uint8_t step_armlet_glow(
    std::uint8_t glow,
    bool spirit_active,
    std::uint8_t timer_ff199e,
    std::uint8_t pulse_ff199f) noexcept {
    if (spirit_active) {
        if (glow < 3U) {
            return static_cast<std::uint8_t>(glow + 1U);
        }
        if (timer_ff199e >= kArmletPulseThreshold && (pulse_ff199f & 1U) == 0U) {
            return 4U;
        }
        return 3U;
    }

    if (glow >= 4U) {
        return 2U;
    }
    if (glow > 0U) {
        return static_cast<std::uint8_t>(glow - 1U);
    }
    return 0U;
}

bool PaletteCycler::step_frame(std::span<std::uint16_t, 64> cram_shadow) noexcept {
    if (!enabled) {
        return false;
    }

    bool cram_dirty = false;
    for (auto& entry : entries) {
        if (entry.keyframes.empty()) {
            continue;
        }
        if (entry.timer > 1U) {
            --entry.timer;
            continue;
        }

        std::size_t next_cursor = entry.cursor + 1U;
        if (next_cursor >= entry.keyframes.size() ||
            entry.keyframes[next_cursor].duration == 0U) {
            next_cursor = 0U;
        }
        entry.cursor = next_cursor;
        const auto& kf = entry.keyframes[entry.cursor];
        entry.timer = kf.duration;

        const std::size_t cram_idx = static_cast<std::size_t>(entry.cram_offset / 2U);
        if (cram_idx < cram_shadow.size()) {
            cram_shadow[cram_idx] = kf.bgr333;
            cram_dirty = true;
        }
    }

    return cram_dirty;
}

} // namespace oasis::game::video
