#include "game/ui/vint_hud_slide.hpp"

#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_constants() {
    assert(kSoundDriverTickAddr == 0x060000U);
    assert(kHudSlideFwdStop == 0x20);
    assert(kHudSlideRevStop == 0x138);
    assert(kHudSlideWrapLimit == 0x158);
    assert(kHudSlideStepPixels == 2);
}

void test_forward_and_reverse_slide() {
    // Forward (dir == 0): stops at 0x20
    assert(step_hud_slide_offset(0x20, false) == 0x20);

    // Forward wrap: 0x156 + 2 == 0x158 -> wraps to 0, then steps to 0x20
    assert(step_hud_slide_offset(0x156, false) == 0);
    std::int16_t off = 0;
    for (int i = 0; i < 16; ++i) {
        off = step_hud_slide_offset(off, false);
    }
    assert(off == 0x20);
    assert(step_hud_slide_offset(off, false) == 0x20);

    // Reverse (dir != 0): stops at 0x138
    assert(step_hud_slide_offset(0x138, true) == 0x138);

    // Reverse wrap: 0 - 2 < 0 -> wraps to 0x158, then steps down to 0x138 in 16 steps
    off = 0;
    off = step_hud_slide_offset(off, true);
    assert(off == 0x158);
    for (int i = 0; i < 16; ++i) {
        off = step_hud_slide_offset(off, true);
    }
    assert(off == 0x138);
    assert(step_hud_slide_offset(off, true) == 0x138);
}

void test_vint_tail_state() {
    VintTailState st{};
    st.hud_slide_offset = 0x20;
    st.hud_slide_reverse = true;
    step_vint_tail(st);
    assert(st.sound_ticks == 1U);
    assert(st.hud_slide_offset == 0x1E);
}

} // namespace

int main() {
    test_constants();
    test_forward_and_reverse_slide();
    test_vint_tail_state();
    return 0;
}
