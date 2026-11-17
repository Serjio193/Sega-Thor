#include "game/entities/entity_anim_sequencer.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::entities;

void test_keyframe_sequencer_and_wrap() {
    constexpr std::array<AnimKeyframe4, 3> kScript = {{
        {0x0010U, 0x0004U}, // dur = (4>>1)-1 = 1
        {0x0012U, 0x0002U}, // dur = (2>>1)-1 = 0
        {0x0014U, 0x8004U}, // last_frame, dur = (4>>1)-1 = 1
    }};

    assert(!kScript[0].is_last_frame());
    assert(kScript[2].is_last_frame());

    EntityAnimState st{};
    st.combat_flags_37 = kAnimFlagSwingHitOnce; // bit 3 should be cleared on keyframe load

    // Tick 1: loads keyframe 0 (frame 0x10, timer_2e = 1, cursor = 4)
    assert(step_anim_tick(kScript, st));
    assert(st.frame_id_32 == 0x0010U);
    assert(st.timer_2e == 1U);
    assert(st.cursor_bytes == 4U);
    assert((st.combat_flags_37 & kAnimFlagSwingHitOnce) == 0U);
    assert((st.combat_flags_37 & kAnimFlagLastFrame) == 0U);

    // Tick 2: decrements timer_2e 1 -> 0
    assert(!step_anim_tick(kScript, st));
    assert(st.timer_2e == 0U);
    assert((st.combat_flags_37 & kAnimFlagAnimCompleted) == 0U);

    // Tick 3: loads keyframe 1 (frame 0x12, timer_2e = 0, cursor = 8)
    assert(step_anim_tick(kScript, st));
    assert(st.frame_id_32 == 0x0012U);
    assert(st.timer_2e == 0U);
    assert(st.cursor_bytes == 8U);

    // Tick 4: loads keyframe 2 (frame 0x14, last_frame, timer_2e = 1, cursor = 12)
    assert(step_anim_tick(kScript, st));
    assert(st.frame_id_32 == 0x0014U);
    assert(st.timer_2e == 1U);
    assert((st.combat_flags_37 & kAnimFlagLastFrame) != 0U);
    assert((st.combat_flags_37 & kAnimFlagAnimCompleted) == 0U);

    // Tick 5: timer_2e 1 -> 0 on last_frame -> sets anim_completed (bit 1)
    assert(!step_anim_tick(kScript, st));
    assert(st.timer_2e == 0U);
    assert((st.combat_flags_37 & kAnimFlagAnimCompleted) != 0U);

    // Tick 6: wraps back to keyframe 0 and clears last_frame bit 2
    assert(step_anim_tick(kScript, st));
    assert(st.frame_id_32 == 0x0010U);
    assert(st.cursor_bytes == 4U);
    assert((st.combat_flags_37 & kAnimFlagLastFrame) == 0U);
}

void test_immediate_zero_duration_last_frame() {
    constexpr std::array<AnimKeyframe4, 1> kSingle = {{{0x0020U, 0x8002U}}};
    EntityAnimState st{};
    assert(step_anim_keyframe(kSingle, st));
    assert(st.timer_2e == 0U);
    assert((st.combat_flags_37 & kAnimFlagLastFrame) != 0U);
    assert((st.combat_flags_37 & kAnimFlagAnimCompleted) != 0U);
}

} // namespace

int main() {
    test_keyframe_sequencer_and_wrap();
    test_immediate_zero_duration_last_frame();
    return 0;
}
