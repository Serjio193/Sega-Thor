#include "game/camera/camera_controller.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

using namespace thor::camera;

void test_camera_deadzone_no_movement() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;

    // Inside deadzone: rel_x = 150 in [120, 200], rel_y = 115 in [104, 128]
    update_camera_deadzone(cam, 250, 215, 0, false, false, false);
    assert(cam.x == 100);
    assert(cam.y == 100);
}

void test_camera_horizontal_smooth_panning() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;
    cam.scroll_bias_dir = 0;

    // Small rightward delta: rel_x = 205 -> d0 = +5 <= 32 -> speed clamped to +3
    update_camera_deadzone(cam, 305, 215, 0, false, false, false);
    assert(cam.x == 103);
    assert(cam.scroll_bias_dir == 0); // Moved right -> dir remains 0

    // Small leftward delta: rel_x = 115 -> d0 = -5 >= -32 -> speed clamped to -3
    cam.x = 100;
    update_camera_deadzone(cam, 215, 215, 0, false, false, false);
    assert(cam.x == 97);
    assert(cam.scroll_bias_dir == 0xFF); // Moved left -> dir set to 0xFF

    // Delta within +/-3 range: rel_x = 202 -> d0 = +2 <= 3 -> speed = +2
    cam.x = 100;
    update_camera_deadzone(cam, 302, 215, 0, false, false, false);
    assert(cam.x == 102);

    // Delta within -3 range: rel_x = 118 -> d0 = -2 >= -3 -> speed = -2
    cam.x = 100;
    update_camera_deadzone(cam, 218, 215, 0, false, false, false);
    assert(cam.x == 98);
}

void test_camera_horizontal_fast_catchup() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;

    // Large right delta: rel_x = 245 -> d0 = 45 > 32 -> speed = 45 - 30 = 15
    update_camera_deadzone(cam, 345, 215, 0, false, false, false);
    assert(cam.x == 115);

    // Large left delta: rel_x = 60 -> d0 = -60 < -32 -> speed = -60 + 30 = -30
    cam.x = 100;
    update_camera_deadzone(cam, 160, 215, 0, false, false, false);
    assert(cam.x == 70);
}

void test_camera_vertical_smooth_and_fast_catchup() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;

    // Small bottom delta: rel_y = 135 -> d0 = +7 <= 32 -> clamped to +3
    update_camera_deadzone(cam, 250, 235, 0, false, false, false);
    assert(cam.y == 103);

    // Large bottom delta: rel_y = 175 -> d0 = 47 > 32 -> speed = 47 - 30 = 17
    cam.y = 100;
    update_camera_deadzone(cam, 250, 275, 0, false, false, false);
    assert(cam.y == 117);

    // Small top delta: rel_y = 95 -> d0 = -9 >= -32 -> clamped to -3
    cam.y = 100;
    update_camera_deadzone(cam, 250, 195, 0, false, false, false);
    assert(cam.y == 97);

    // Large top delta: rel_y = 50 -> d0 = -54 < -32 -> d0 + 30 = -24, clamped >= -6 -> speed = -6
    cam.y = 100;
    update_camera_deadzone(cam, 250, 150, 0, false, false, false);
    assert(cam.y == 94);
}

void test_camera_instant_snap() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;

    // Instant snap overrides smooth clamping:
    // Left: rel_x = 60 -> d0 = -60 -> full step -60
    update_camera_deadzone(cam, 160, 215, 0, false, true, false);
    assert(cam.x == 40);

    // Right: rel_x = 245 -> d0 = 45 -> full step +45
    cam.x = 100;
    update_camera_deadzone(cam, 345, 215, 0, false, true, false);
    assert(cam.x == 145);

    // Top: rel_y = 50 -> d0 = -54 -> full step -54
    cam.y = 100;
    update_camera_deadzone(cam, 250, 150, 0, false, true, false);
    assert(cam.y == 46);

    // Bottom: rel_y = 175 -> d0 = +47 -> full step +47
    cam.y = 100;
    update_camera_deadzone(cam, 250, 275, 0, false, true, false);
    assert(cam.y == 147);
}

void test_camera_deadzone_bias_and_hud_slide() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;
    cam.scroll_bias_dir = 0;

    // rel_x <= 80 (rel_x = 75) when dir == 0 -> dir set to 0xFF
    update_camera_deadzone(cam, 175, 215, 0, false, false, false);
    assert(cam.scroll_bias_dir == 0xFF);

    // If instant snap active when rel_x <= 80 -> target = 0x0138
    cam.x = 100;
    cam.y = 100;
    cam.scroll_bias_dir = 0;
    update_camera_deadzone(cam, 175, 215, 0, false, true, false);
    assert(cam.scroll_bias_dir == 0xFF);
    assert(cam.scroll_bias_target == 0x0138);

    // rel_x >= 240 (rel_x = 245) when dir == 0xFF -> dir cleared to 0x00
    cam.x = 100;
    cam.y = 100;
    cam.scroll_bias_dir = 0xFF;
    update_camera_deadzone(cam, 345, 215, 0, false, false, false);
    assert(cam.scroll_bias_dir == 0x00);

    // If instant snap active when rel_x >= 240 -> target = 0x0020
    cam.x = 100;
    cam.y = 100;
    cam.scroll_bias_dir = 0xFF;
    update_camera_deadzone(cam, 345, 215, 0, false, true, false);
    assert(cam.scroll_bias_dir == 0x00);
    assert(cam.scroll_bias_target == 0x0020);
}

void test_camera_elevation_and_warp_active() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;

    // player_y = 215, player_elev_z = 20
    // Normal: effective_y = 215 - 20 = 195. rel_y = 195 - 100 = 95 (< 104) -> pans up
    update_camera_deadzone(cam, 250, 215, 20, false, false, false);
    assert(cam.y == 97);

    // Warp active: elevation subtraction skipped! effective_y = 215. rel_y = 215 - 100 = 115 (in deadzone)
    cam.y = 100;
    update_camera_deadzone(cam, 250, 215, 20, true, false, false);
    assert(cam.y == 100);
}

void test_camera_direct_follow_mode() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;
    cam.offset_x = 10;
    cam.offset_y = -5;

    // Direct follow mode locks:
    // cam.x = player_x + offset_x - 160 = 300 + 10 - 160 = 150
    // cam.y = (player_y - z) + offset_y - 120 = (250 - 10) - 5 - 120 = 115
    update_camera_deadzone(cam, 300, 250, 10, false, false, true);
    assert(cam.x == 150);
    assert(cam.y == 115);
}

void test_camera_screen_shake_oscillation() {
    CameraState cam{};
    cam.x = 100;
    cam.y = 100;
    cam.max_x = 500;
    cam.max_y = 500;

    // Timer = 4:
    // Decrement -> timer = 3 (odd) -> offset = +3. cam.y = 100 + 3 = 103.
    cam.shake_timer = 4;
    update_camera_deadzone(cam, 250, 215, 0, false, false, false);
    assert(cam.shake_timer == 3);
    assert(cam.y == 103);

    // Timer = 3:
    // Decrement -> timer = 2 (even) -> offset = -2. cam.y = 103 - 2 = 101.
    update_camera_deadzone(cam, 250, 215, 0, false, false, false);
    assert(cam.shake_timer == 2);
    assert(cam.y == 101);

    // Timer = 2:
    // Decrement -> timer = 1 (odd) -> offset = +1. cam.y = 101 + 1 = 102.
    update_camera_deadzone(cam, 250, 215, 0, false, false, false);
    assert(cam.shake_timer == 1);
    assert(cam.y == 102);

    // Timer = 1:
    // Decrement -> timer = 0 (even) -> offset = -0 = 0. cam.y = 102.
    update_camera_deadzone(cam, 250, 215, 0, false, false, false);
    assert(cam.shake_timer == 0);
    assert(cam.y == 102);

    // Shake clamping: cam.y cannot drop below 0
    cam.y = 1;
    cam.shake_timer = 11; // decrements to 10 (even) -> offset = -10 -> clamped to 0
    update_camera_deadzone(cam, 250, 115, 0, false, false, false);
    assert(cam.y == 0);

    // Shake clamping: cam.y cannot exceed max_y
    cam.y = cam.max_y;
    cam.shake_timer = 2; // decrements to 1 (odd) -> offset = +1 -> clamped to max_y
    update_camera_deadzone(cam, 250, static_cast<int16_t>(cam.max_y + 115), 0, false, false, false);
    assert(cam.y == cam.max_y);
}

void test_clamp_camera_viewport() {
    CameraState cam{};

    // Map 60 x 50 tiles (each tile 8px).
    // max_x = (60 - 40) * 8 = 160
    // max_y = (50 - 28) * 8 = 176
    cam.x = 200;
    cam.y = 300;
    clamp_camera_viewport(cam, 60, 50);
    assert(cam.x == 160);
    assert(cam.y == 176);

    // Negative positions clamped to 0
    cam.x = -50;
    cam.y = -20;
    clamp_camera_viewport(cam, 60, 50);
    assert(cam.x == 0);
    assert(cam.y == 0);

    // Map smaller than viewport clamped to 0
    cam.x = 50;
    cam.y = 50;
    clamp_camera_viewport(cam, 30, 20);
    assert(cam.x == 0);
    assert(cam.y == 0);
}

void test_raw_ram_window_roundtrip() {
    std::array<uint8_t, 0x10000> ram{};

    // Setup RAM registers
    // Player: X = 305, Y = 215, Z = 0
    ram[0x19F0] = 0x01; ram[0x19F1] = 0x31; // 305
    ram[0x19F4] = 0x00; ram[0x19F5] = 0xD7; // 215
    ram[0x19F8] = 0x00; ram[0x19F9] = 0x00; // 0

    // Flags: instant snap active, warp inactive, normal mode
    ram[0x17BB] = 0;
    ram[0x17BA] = 1;
    ram[0x16F1] = 0;

    // Bounds and offsets
    ram[0x177A] = 0x01; ram[0x177B] = 0xF4; // max_x = 500
    ram[0x177C] = 0x01; ram[0x177D] = 0xF4; // max_y = 500
    ram[0x177E] = 0x00; ram[0x177F] = 0x00; // offset_x = 0
    ram[0x1780] = 0x00; ram[0x1781] = 0x00; // offset_y = 0

    // Shake timer = 2
    ram[0x17C4] = 0x00; ram[0x17C5] = 0x02;

    // Scroll bias dir = 0, target = 0
    ram[0x1859] = 0;
    ram[0x185A] = 0; ram[0x185B] = 0;

    CameraState cam{};
    cam.x = 100;
    cam.y = 100;

    update_camera_from_ram(cam, ram);

    // Verify instant snap cleared
    assert(ram[0x17BA] == 0);

    // rel_x = 305 - 100 = 205 (d0 = +5). With instant snap, full step +5
    assert(cam.x == 105);

    // Shake timer decremented from 2 to 1 (odd) -> +1 offset to cam.y
    assert(cam.shake_timer == 1);
    assert(cam.y == 101);
    assert(ram[0x17C5] == 1);
}

} // namespace

int main() {
    test_camera_deadzone_no_movement();
    test_camera_horizontal_smooth_panning();
    test_camera_horizontal_fast_catchup();
    test_camera_vertical_smooth_and_fast_catchup();
    test_camera_instant_snap();
    test_camera_deadzone_bias_and_hud_slide();
    test_camera_elevation_and_warp_active();
    test_camera_direct_follow_mode();
    test_camera_screen_shake_oscillation();
    test_clamp_camera_viewport();
    test_raw_ram_window_roundtrip();

    std::cout << "[PASS] All M16.184 camera controller tests succeeded!\n";
    return 0;
}
