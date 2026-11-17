#include "game/camera/camera_follow.hpp"

#include <cassert>

namespace {

using namespace oasis::game::camera;

void test_deadzone_and_smooth_catchup() {
    CameraFollowState st{};
    st.cam_x = 100;
    st.cam_y = 100;
    st.cam_max_x = 500;
    st.cam_max_y = 500;

    // Inside deadzone: rel_x = 150 in [120, 200], rel_y = 115 in [104, 128] -> no move
    st.player_x = 250;
    st.player_y = 215;
    step_camera_follow(st);
    assert(st.cam_x == 100 && st.cam_y == 100);

    // Small positive delta (rel_x = 210 -> dx = +10 <= 32) -> clamped to +3
    st.player_x = 310;
    step_camera_follow(st);
    assert(st.cam_x == 103);
    assert(st.hud_slide_dir_ff1859 == 0U);

    // Large negative Y catch-up (rel_y = 50 -> dy = -54 < -32 -> -54 + 30 = -24, clamped >= -6)
    st.player_y = 150;
    step_camera_follow(st);
    assert(st.cam_y == 94);
}

void test_hud_slide_trigger_and_instant_snap() {
    CameraFollowState st{};
    st.cam_x = 100;
    st.cam_y = 100;
    st.cam_max_x = 500;
    st.cam_max_y = 500;
    st.instant_snap_ff17ba = true;

    // rel_x = 70 <= 0x50 -> hud_slide_dir = 0xFF, offset snapped to 0x0138
    st.player_x = 170;
    st.player_y = 215;
    step_camera_follow(st);
    assert(st.hud_slide_dir_ff1859 == 0xFFU);
    assert(st.hud_slide_offset_ff185a == 0x0138U);

    // rel_x = 250 >= 0xF0 -> hud_slide_dir = 0, offset snapped to 0x0020
    st.player_x = st.cam_x + 0xF0;
    step_camera_follow(st);
    assert(st.hud_slide_dir_ff1859 == 0U);
    assert(st.hud_slide_offset_ff185a == 0x0020U);
}

void test_locked_center_and_screen_shake() {
    CameraFollowState st{};
    st.cam_max_x = 500;
    st.cam_max_y = 500;
    st.locked_center_mode = true;
    st.player_x = 300;
    st.player_y = 260;
    st.player_z = 20; // target_y = 240
    st.lock_offset_x = 10;
    st.lock_offset_y = -4;
    st.shake_timer_ff17c4 = 3U;

    // Locked center: (300 + 10 - 160 = 150, 240 - 4 - 120 = 116), + shake(+3) = 119
    step_camera_follow(st);
    assert(st.cam_x == 150);
    assert(st.cam_y == 119);
    assert(st.shake_timer_ff17c4 == 2U);

    // Next frame: shake(-2) -> 116 - 2 = 114
    step_camera_follow(st);
    assert(st.cam_y == 114);
    assert(st.shake_timer_ff17c4 == 1U);
}

} // namespace

int main() {
    test_deadzone_and_smooth_catchup();
    test_hud_slide_trigger_and_instant_snap();
    test_locked_center_and_screen_shake();
    return 0;
}
