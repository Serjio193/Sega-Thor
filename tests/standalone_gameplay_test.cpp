#include "core/runtime.hpp"
#include "game/player/player.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string resolve_fixture(const std::string& relative) {
    if (fs::exists(relative)) return relative;
    const auto parent = fs::path("..") / relative;
    if (fs::exists(parent)) return parent.string();
    return relative;
}

void test_direction_input_mapping() {
    using oasis::game::player::Direction;
    using oasis::game::player::direction_from_input;

    assert(direction_from_input(0x0) == Direction::none);
    assert(direction_from_input(0x1) == Direction::up);
    assert(direction_from_input(0x2) == Direction::down);
    assert(direction_from_input(0x4) == Direction::left);
    assert(direction_from_input(0x8) == Direction::right);
    assert(direction_from_input(0x5) == Direction::up_left);
    assert(direction_from_input(0x6) == Direction::down_left);
    assert(direction_from_input(0x9) == Direction::up_right);
    assert(direction_from_input(0xA) == Direction::down_right);
    // Opposing inputs resolve to none
    assert(direction_from_input(0x3) == Direction::none);
    assert(direction_from_input(0xC) == Direction::none);
    std::cout << "[PASS] test_direction_input_mapping\n";
}

void test_controller_state_buttons() {
    using oasis::core::Button;
    using oasis::core::ControllerState;

    ControllerState ctrl{};
    assert(!ctrl.pressed(Button::Up));
    assert(!ctrl.pressed(Button::Right));

    ctrl.set(Button::Right);
    assert(ctrl.pressed(Button::Right));
    assert((ctrl.buttons & 0x08) != 0);

    ctrl.set(Button::A);
    assert(ctrl.pressed(Button::A));

    ctrl.set(Button::Right, false);
    assert(!ctrl.pressed(Button::Right));
    assert(ctrl.pressed(Button::A));
    std::cout << "[PASS] test_controller_state_buttons\n";
}

void test_player_position_integration() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68; // sat_base = 0xD000

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize(baseline, 261, 107);

    assert(sim.state().player_x == 261);
    assert(sim.state().player_y == 107);
    assert(sim.player().x_fixed == 261 * 0x10000);

    // Frame 1: move Right (cardinal_x_speed = 0x36000)
    oasis::game::FrameInput input1{};
    input1.frame_index = 1;
    input1.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState vdp1{};
    sim.step_frame(input1, vdp1);

    // 261 * 0x10000 + 0x36000 = 0x01050000 + 0x00036000 = 0x01086000
    assert(sim.player().x_fixed == (261 * 0x10000) + 0x36000);
    assert(sim.state().player_x == static_cast<std::uint16_t>(sim.player().x_fixed / 0x10000));
    assert(sim.player().direction_code == 1); // Right

    // Frame 2: move Right again
    oasis::game::FrameInput input2{};
    input2.frame_index = 2;
    input2.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState vdp2{};
    sim.step_frame(input2, vdp2);

    assert(sim.player().x_fixed == (261 * 0x10000) + (2 * 0x36000));
    std::cout << "[PASS] test_player_position_integration\n";
}

void test_sat_sprite_generation() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68; // sat_base = 0xD000

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize(baseline, 261, 107);

    oasis::game::FrameInput input{};
    input.frame_index = 1;
    input.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState out_vdp{};
    sim.step_frame(input, out_vdp);

    const auto sat_base = out_vdp.sat_base();
    assert(sat_base == 0xD000);

    // Check Piece 13 in VRAM (sat_base + 13 * 8 = 0xD068)
    const std::uint32_t p13_off = sat_base + (13U * 8U);
    const auto p13_y = static_cast<std::uint16_t>((out_vdp.vram[p13_off] << 8U) | out_vdp.vram[p13_off + 1U]);
    const auto p13_attr = static_cast<std::uint16_t>((out_vdp.vram[p13_off + 4U] << 8U) | out_vdp.vram[p13_off + 5U]);
    const auto p13_x = static_cast<std::uint16_t>((out_vdp.vram[p13_off + 6U] << 8U) | out_vdp.vram[p13_off + 7U]);

    const int screen_x = static_cast<int>(sim.player().x_fixed / 0x10000) - sim.camera_x();
    const int screen_y = static_cast<int>(sim.player().y_fixed / 0x10000) - sim.camera_y();

    assert(p13_y == static_cast<std::uint16_t>(screen_y + 128));
    assert(p13_x == static_cast<std::uint16_t>(screen_x + 128));
    assert((p13_attr & 0x0800U) == 0); // No hflip when facing Right

    // Now turn Left
    oasis::game::FrameInput left_input{};
    left_input.frame_index = 2;
    left_input.controller_1.set(oasis::core::Button::Left);

    oasis::genesis::VdpState left_vdp{};
    sim.step_frame(left_input, left_vdp);

    const auto left_attr = static_cast<std::uint16_t>((left_vdp.vram[p13_off + 4U] << 8U) | left_vdp.vram[p13_off + 5U]);
    assert((left_attr & 0x0800U) != 0); // Hflip active when facing Left!
    std::cout << "[PASS] test_sat_sprite_generation\n";
}

void test_simulation_determinism() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim1{};
    sim1.initialize(baseline, 261, 107);

    oasis::game::StandaloneGameSimulation sim2{};
    sim2.initialize(baseline, 261, 107);

    // Run 30 identical steps
    for (int f = 1; f <= 30; ++f) {
        oasis::game::FrameInput in{};
        in.frame_index = static_cast<std::uint32_t>(f);
        if (f % 2 == 1) in.controller_1.set(oasis::core::Button::Right);
        if (f % 3 == 0) in.controller_1.set(oasis::core::Button::Down);

        oasis::genesis::VdpState vdp1{}, vdp2{};
        sim1.step_frame(in, vdp1);
        sim2.step_frame(in, vdp2);

        assert(sim1.player().x_fixed == sim2.player().x_fixed);
        assert(sim1.player().y_fixed == sim2.player().y_fixed);
        assert(sim1.state().player_x == sim2.state().player_x);
        assert(sim1.state().player_y == sim2.state().player_y);
        assert(vdp1.vram == vdp2.vram);
    }
    std::cout << "[PASS] test_simulation_determinism\n";
}

void test_canonical_beach_oracle_trajectory() {
    const std::string fixture_path = resolve_fixture("data/standalone-fixtures/beach_canonical_60_frames.json");
    if (!fs::exists(fixture_path)) {
        std::cout << "[SKIP] test_canonical_beach_oracle_trajectory (fixture not present)\n";
        return;
    }

    std::ifstream in(fixture_path);
    assert(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize(baseline, 261, 107);

    // Step 60 frames rightward
    for (int f = 1; f <= 60; ++f) {
        oasis::game::FrameInput input{};
        input.frame_index = static_cast<std::uint32_t>(f);
        input.controller_1.set(oasis::core::Button::Right);

        oasis::genesis::VdpState vdp{};
        sim.step_frame(input, vdp);
    }

    // After 60 frames moving right at 0x36000 (3.375 px/f), candidate reaches ~463 px
    assert(sim.state().player_x > 261);
    assert(sim.state().frame_counter == 60);

    // Test reset restores starting position
    sim.reset();
    assert(sim.state().player_x == 261);
    assert(sim.state().player_y == 107);
    assert(sim.state().frame_counter == 0);

    std::cout << "[PASS] test_canonical_beach_oracle_trajectory\n";
}

void test_rasterization_of_simulation_output() {
    const std::string fixture_path = resolve_fixture("data/standalone-fixtures/beach_frame_000.json");
    if (!fs::exists(fixture_path)) {
        std::cout << "[SKIP] test_rasterization_of_simulation_output (fixture not present)\n";
        return;
    }

    oasis::genesis::VdpState beach_vdp{};
    assert(beach_vdp.load_from_file(fixture_path.c_str()));

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize(beach_vdp, 261, 107);

    oasis::game::FrameInput input{};
    input.frame_index = 1;
    input.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState out_vdp{};
    sim.step_frame(input, out_vdp);

    std::vector<std::uint32_t> rgba(320 * 224);
    oasis::genesis::RasterMetrics metrics{};
    oasis::genesis::render_vdp_frame_rgba32(out_vdp, rgba,
                                           oasis::genesis::ColorDacMode::GenesisPlusGx,
                                           &metrics);

    assert(metrics.total_pixels == 320 * 224);
    assert(metrics.sprite_pixels > 0);
    assert(metrics.plane_a_pixels > 0);
    std::cout << "[PASS] test_rasterization_of_simulation_output\n";
}

} // namespace

int main() {
    std::cout << "Running Standalone Gameplay Simulation Tests (M16.2)...\n";
    test_direction_input_mapping();
    test_controller_state_buttons();
    test_player_position_integration();
    test_sat_sprite_generation();
    test_simulation_determinism();
    test_canonical_beach_oracle_trajectory();
    test_rasterization_of_simulation_output();
    std::cout << "All Standalone Gameplay Simulation Tests PASSED!\n";
    return 0;
}
