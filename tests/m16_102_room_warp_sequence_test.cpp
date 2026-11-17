#include "game/world/room_warp_sequence.hpp"

#include <cassert>

namespace {

using namespace oasis::game::world;

void test_6_flag_warp_lock_guard() {
    RoomWarpLockState st{};
    assert(can_run_room_update_hooks(st));
    assert(can_trigger_room_warp(st));

    st.flags_ff17b8 = 0x02U;
    assert(!can_run_room_update_hooks(st));
    assert(can_trigger_room_warp(st)); // bit 1 alone does not block warp

    st.flags_ff17b8 = 0x04U;
    assert(!can_trigger_room_warp(st));
    st.flags_ff17b8 = 0x00U;

    st.lock_ff17b9 = 1U;
    assert(!can_trigger_room_warp(st));
    st.lock_ff17b9 = 0U;

    st.lock_ff1987 = 1U;
    assert(!can_trigger_room_warp(st));
    st.lock_ff1987 = 0U;

    st.lock_ff1985 = 1U;
    assert(!can_trigger_room_warp(st));
    st.lock_ff1985 = 0U;

    st.lock_ff1983 = 1U;
    assert(!can_trigger_room_warp(st));
    st.lock_ff1983 = 0U;

    st.lock_ff1982 = 1U;
    assert(!can_trigger_room_warp(st));
}

void test_redirect_camera_checkpoint_and_flag_reset() {
    RoomWarpLockState st{};
    st.flags_ff17b8 = 0x02U;
    st.lock_ff17b9 = 1U;
    st.lock_ff1988 = 2U;
    st.lock_ff1858 = 3U;
    st.scroll_ff177e = 0x1234U;
    st.scroll_ff1780 = 0x5678U;
    st.global_flag_16f1 = 0x00U;

    // Room 0x0004 redirects to 0x0611; room_hdr_flag_19 bit 6 triggers checkpoint
    const auto p1 = execute_room_warp_sequence(st, {0x0004U, 256, 768, 0x40U});
    assert(p1.resolved_room_id == 0x0611U);
    assert(p1.camera_x == 96);
    assert(p1.camera_y == 656);
    assert(p1.save_checkpoint);
    assert(p1.sound_cmd == 2U);
    assert(p1.fade_speed == 4U);

    // Verify flag reset
    assert(st.flag_ff17ba == 0xFFU);
    assert(st.flags_ff17b8 == 0U && st.lock_ff17b9 == 0U);
    assert(st.lock_ff1988 == 0U && st.lock_ff1858 == 0U);
    assert(st.scroll_ff177e == 0U && st.scroll_ff1780 == 0U);
    assert(st.fade_speed_ff1896 == 4U);

    // Global flag $FF16F1 bit 6 also triggers checkpoint even if header bit 6 is clear
    st.global_flag_16f1 = 0x40U;
    const auto p2 = execute_room_warp_sequence(st, {0x0D00U, 160, 112, 0x00U});
    assert(p2.resolved_room_id == 0x0D00U);
    assert(p2.camera_x == 0 && p2.camera_y == 0);
    assert(p2.save_checkpoint);

    // Neither bit 6 set -> no checkpoint
    st.global_flag_16f1 = 0x00U;
    const auto p3 = execute_room_warp_sequence(st, {0x001BU, 256, 80, 0x00U});
    assert(!p3.save_checkpoint);
}

} // namespace

int main() {
    test_6_flag_warp_lock_guard();
    test_redirect_camera_checkpoint_and_flag_reset();
    return 0;
}
