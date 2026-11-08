#include "game/camera/camera_controller.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <iostream>

namespace {

void test_camera_initial_state() {
    using namespace oasis::game::camera;

    CameraController camera{};
    assert(camera.camera_x() == 125);
    assert(camera.camera_y() == 46);
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

    CameraConfig cfg{};
    cfg.min_camera_y = 0;
    CameraController camera{cfg};
    // At camera=(125, 46), screen center is (125+160=285, 46+112=158)
    // Screen X = player_x - 125.
    // For deadzone [120..200]: player_x in [245..325]
    // Initial player Ali is at X=261 -> screen_x = 261 - 125 = 136 (inside deadzone)
    // But if we test horizontal deadzone at Y=110 (screen_y = 64):
    camera.set_camera_position(125, 0); // Y=0 -> screen_y = 107 (inside [104..128])
    camera.update(261, 110, 0);

    // Player at X=261 -> screen_x = 261 - 125 = 136 (inside [120..200])
    // Player at Y=110 -> screen_y = 110 - 0 = 110 (inside [104..128])
    assert(!camera.state().moved_this_frame);
    assert(camera.camera_x() == 125);
    assert(camera.camera_y() == 0);

    std::cout << "[PASS] test_camera_deadzone\n";
}

void test_camera_tracking_right_and_smoothing() {
    using namespace oasis::game::camera;

    CameraController camera{};
    camera.set_camera_position(125, 46);

    // Player moves far to the right: X = 350 -> screen_x = 350 - 125 = 225 > 200 (delta = +25)
    // Smoothing limits delta to +3
    camera.update(350, 150, 0);

    assert(camera.state().moved_this_frame);
    assert(camera.camera_x() == 128); // 125 + 3 = 128
    // Scroll shifts left: hscroll_a = -(128 - 125) = -3 = 509 & 0x1FF
    assert(camera.state().plane_a_hscroll == static_cast<int>((-3) & 0x01FF));

    // Next frame: player still at 350 -> screen_x = 350 - 128 = 222 > 200
    camera.update(350, 150, 0);
    assert(camera.camera_x() == 131); // 128 + 3 = 131

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
    assert(camera.camera_x() == 192); // Clamped at max 192

    // Move player very far left
    camera.update(-500, 115, 0);
    assert(camera.camera_x() == 0); // Clamped at min 0

    std::cout << "[PASS] test_camera_clamping\n";
}

void test_world_to_screen_mapping() {
    using namespace oasis::game::camera;

    CameraController camera{};
    camera.set_camera_position(125, 46);

    assert(camera.world_to_screen_x(261) == 136);
    assert(camera.world_to_screen_y(107) == 61);

    assert(camera.screen_to_world_x(136) == 261);
    assert(camera.screen_to_world_y(61) == 107);

    // With altitude
    assert(camera.world_to_screen_y(107, 10) == 51);

    std::cout << "[PASS] test_world_to_screen_mapping\n";
}

void test_apply_to_vdp() {
    using namespace oasis::game::camera;

    oasis::genesis::VdpState vdp{};
    vdp.registers[13] = 0x37; // Base address = 0xDC00
    vdp.vram.fill(0);
    vdp.vsram.fill(0);

    CameraController camera{};
    camera.set_camera_position(135, 56); // total_dx = +10, total_dy = +10
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
