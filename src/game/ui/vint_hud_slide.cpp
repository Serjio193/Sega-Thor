#include "game/ui/vint_hud_slide.hpp"

namespace oasis::game::ui {

std::int16_t step_hud_slide_offset(
    std::int16_t offset,
    bool reverse_dir) noexcept {
    if (!reverse_dir) {
        if (offset == kHudSlideFwdStop) {
            return offset;
        }
        offset = static_cast<std::int16_t>(offset + kHudSlideStepPixels);
        if (offset >= kHudSlideWrapLimit) {
            offset = 0;
        }
        return offset;
    }

    if (offset == kHudSlideRevStop) {
        return offset;
    }
    offset = static_cast<std::int16_t>(offset - kHudSlideStepPixels);
    if (offset < 0) {
        offset = kHudSlideWrapLimit;
    }
    return offset;
}

void step_vint_tail(VintTailState& state) noexcept {
    ++state.sound_ticks;
    state.hud_slide_offset = step_hud_slide_offset(
        state.hud_slide_offset,
        state.hud_slide_reverse);
}

} // namespace oasis::game::ui
