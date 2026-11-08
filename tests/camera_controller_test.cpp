#include "game/camera/camera_controller.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <iostream>

namespace {

void test_camera_initial_state() {
    using namespace oasis::game::camera;

    CameraController camera{};
    // Authentic RAM $FF1716 / $FF171A beach initial coordinates
    assert(camera.camera_x() == 96);
    assert(camera.camera_y() == 3);
    assert(camera.state().plane_a_hscroll == 0);
    assert(camera.state().plane_b_hscroll == 0);
    assert(camera.state().plane_a_vscroll == 139);
    assert(camera.state().plane_b_vscroll == 0);
    assert(camera.tracking_enabled());
    assert(!camera.state().moved_this_frame);

    std::cout << "[PASS] test_camera_initial_state\n";
}

void test_camera_deadzone() {
    using namespace oasis::game::camera;

    CameraController camera{};
    // Initial camera at (96, 3). Ali at (261, 107) -> screen_x = 165 (in [120..200]), screen_y = 104 (in [104..128])
    camera.update(261, 107, 0);

    assert(!camera.state().moved_this_frame);
    assert(camera.camera_x() == 96);
    assert(camera.camera_y() == 3);

    std::cout << "[PASS] test_camera_deadzone\n";
}

void test_camera_tracking_right_and_smoothing() {
    using namespace oasis::game::camera;

    CameraController camera{};
    camera.set_camera_position(96, 3);

    // Player moves right: X = 300 -> screen_x = 300 - 96 = 204 > 200 (delta = +4)
    // Smoothing limits delta to +3
    camera.update(300, 107, 0);

    assert(camera.state().moved_this_frame);
    assert(camera.camera_x() == 99); // 96 + 3 = 99

    // Next frame: player at 304 -> screen_x = 304 - 99 = 205 > 200
    camera.update(304, 107, 0);
    assert(camera.camera_x() == 102); // 99 + 3 = 102

    std::cout << "[PASS] test_camera_tracking_right_and_smoothing\n";
}

void test_camera_clamping() {
    using namespace oasis::game::camera;

    CameraConfig cfg{};
    cfg.min_camera_x = 0;
    cfg.max_camera_x = 192;
    cfg.smooth_tracking = false; // Disable smoothing to test clamp directly
    CameraController camera{cfg};

    // Move player very far right
    camera.update(1000, 115, 0);
    assert(camera.camera_x() == 192); // Clamped at max 192 ($FF177A)

    // Move player very far left
    camera.update(-500, 115, 0);
    assert(camera.camera_x() == 0); // Clamped at min 0

    std::cout << "[PASS] test_camera_clamping\n";
}

void test_world_to_screen_mapping() {
    using namespace oasis::game::camera;

    CameraController camera{};
    camera.set_camera_position(96, 3);

    assert(camera.world_to_screen_x(261) == 165);
    assert(camera.world_to_screen_y(107) == 104);

    assert(camera.screen_to_world_x(165) == 261);
    assert(camera.screen_to_world_y(104) == 107);

    // With altitude
    assert(camera.world_to_screen_y(107, 10) == 94);

    std::cout << "[PASS] test_world_to_screen_mapping\n";
}

void test_apply_to_vdp() {
    using namespace oasis::game::camera;

    oasis::genesis::VdpState vdp{};
    vdp.registers[13] = 0x37; // Base address = 0xDC00
    vdp.vram.fill(0);
    vdp.vsram.fill(0);

    CameraController camera{};
    camera.set_camera_position(106, 13); // total_dx = +10, total_dy = +10
    camera.set_camera_position(106, 13); // step 2 to advance 2-frame latch
    camera.apply_to_vdp(vdp);

    // HScroll table at 0xDC00
    const auto h_a = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(vdp.vram[0xDC00]) << 8U) | vdp.vram[0xDC01]);
    const auto h_b = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(vdp.vram[0xDC02]) << 8U) | vdp.vram[0xDC03]);

    assert(h_a == static_cast<std::int16_t>((-10) & 0x01FF));
    assert(h_b == static_cast<std::int16_t>((-5) & 0x01FF));

    // VSRAM at offset 0 and 2
    const auto v_a = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(vdp.vsram[0]) << 8U) | vdp.vsram[1]);
    const auto v_b = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(vdp.vsram[2]) << 8U) | vdp.vsram[3]);

    assert(v_a == (139 + 10));
    assert(v_b == (0 + 5));

    std::cout << "[PASS] test_apply_to_vdp\n";
}

} // namespace

int main() {
    std::cout << "Running Camera Controller Tests...\n";
    test_camera_initial_state();
    test_camera_deadzone();
    test_camera_tracking_right_and_smoothing();
    test_camera_clamping();
    test_world_to_screen_mapping();
    test_apply_to_vdp();
    std::cout << "All Camera Controller Tests PASSED!\n";
    return 0;
}
