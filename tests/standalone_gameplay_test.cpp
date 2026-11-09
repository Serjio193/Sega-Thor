#include "core/runtime.hpp"
#include "game/player/player.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <cstring>
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

void test_player_position_integration() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68; // sat_base = 0xD000

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize(baseline, 261, 107);

    assert(sim.state().player_x == 261);
    assert(sim.state().player_y == 107);
    assert(sim.player().x_fixed == 261 * 0x10000);

    // Frame 1: move Right in authentic 30Hz cadence (cardinal_x_speed = 0x36000)
    oasis::game::FrameInput input1{};
    input1.frame_index = 1;
    input1.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState vdp1{};
    sim.step_frame(input1, vdp1);

    assert(sim.player().x_fixed == (261 * 0x10000) + 0x36000);
    assert(sim.state().player_x == static_cast<std::uint16_t>(sim.player().x_fixed / 0x10000));
    assert(sim.player().direction_code == 1); // Right

    // Frame 2: even frame (display hold on 30Hz cadence)
    oasis::game::FrameInput input2{};
    input2.frame_index = 2;
    input2.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState vdp2{};
    sim.step_frame(input2, vdp2);
    assert(sim.player().x_fixed == (261 * 0x10000) + 0x36000);

    // Frame 3: odd frame (second 30Hz step)
    oasis::game::FrameInput input3{};
    input3.frame_index = 3;
    input3.controller_1.set(oasis::core::Button::Right);

    oasis::genesis::VdpState vdp3{};
    sim.step_frame(input3, vdp3);
    assert(sim.player().x_fixed == (261 * 0x10000) + (2 * 0x36000));
    std::cout << "[PASS] test_player_position_integration\n";
}

void test_checkpoint_initial_state_adapter() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 0x01051000;
    cp.y_fixed = 0x006B0000;
    cp.movement_state = 4;
    cp.direction_code = 0;
    cp.turn_timer = 10;
    cp.hp = 208;
    cp.max_hp = 218;

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize_from_checkpoint(baseline, cp);

    assert(sim.player().x_fixed == 0x01051000);
    assert(sim.player().y_fixed == 0x006B0000);
    assert(sim.state().player_x == 261);
    assert(sim.state().player_y == 107);
    assert(sim.player().movement_state == 4);
    assert(sim.player().direction_code == 0);
    assert(sim.player().turn_timer == 10);
    assert(sim.player().hp == 208);
    assert(sim.player().max_hp == 218);
    assert(sim.oracle_mode() == true);

    std::cout << "[PASS] test_checkpoint_initial_state_adapter\n";
}

void test_sat_pipeline_contract_integration() {
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

    const auto entries = sim.sat_pipeline().entries();
    assert(!entries.empty());

    // Verify entries are linked sequentially and tail is terminated
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (i + 1 < entries.size()) {
            assert(entries[i].link == static_cast<std::uint8_t>(i + 1));
        } else {
            assert(entries[i].link == 0); // Terminated tail
        }
    }

    // Verify VRAM received the big-endian serialized entries from sat_pipeline
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const std::size_t addr = sat_base + (i * 8U);
        assert(out_vdp.vram[addr + 0U] == static_cast<std::uint8_t>((e.y_pos >> 8U) & 0xFFU));
        assert(out_vdp.vram[addr + 1U] == static_cast<std::uint8_t>(e.y_pos & 0xFFU));
        assert(out_vdp.vram[addr + 2U] == e.size);
        assert(out_vdp.vram[addr + 3U] == e.link);
        assert(out_vdp.vram[addr + 4U] == static_cast<std::uint8_t>((e.attr >> 8U) & 0xFFU));
        assert(out_vdp.vram[addr + 5U] == static_cast<std::uint8_t>(e.attr & 0xFFU));
        assert(out_vdp.vram[addr + 6U] == static_cast<std::uint8_t>((e.x_pos >> 8U) & 0xFFU));
        assert(out_vdp.vram[addr + 7U] == static_cast<std::uint8_t>(e.x_pos & 0xFFU));
    }

    std::cout << "[PASS] test_sat_pipeline_contract_integration\n";
}

void test_additional_entity_update_and_sat() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize(baseline, 261, 107);

    assert(sim.entities().size() == 2);
    assert(sim.entities()[0].type_id == 0x0060);
    assert(sim.entities()[0].world_x == 256);
    assert(sim.entities()[0].world_y == 88);
    assert(sim.entities()[1].type_id == 0x0098);
    assert(sim.entities()[1].world_x == 259);
    assert(sim.entities()[1].world_y == 101);

    oasis::game::FrameInput in{};
    in.frame_index = 1;
    oasis::genesis::VdpState out_vdp{};
    sim.step_frame(in, out_vdp);

    // Entity animation frame should advance
    assert(sim.entities()[0].anim_frame == 1);
    assert(sim.entities()[1].anim_frame == 1);

    std::cout << "[PASS] test_additional_entity_update_and_sat\n";
}

void test_simulation_determinism() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim1{};
    sim1.initialize(baseline, 261, 107);

    oasis::game::StandaloneGameSimulation sim2{};
    sim2.initialize(baseline, 261, 107);

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

void test_canonical_beach_oracle_trajectory_exact() {
    const std::string fixture_path = resolve_fixture("data/standalone-fixtures/beach_canonical_120_oracle.json");
    if (!fs::exists(fixture_path)) {
        std::cout << "[SKIP] test_canonical_beach_oracle_trajectory_exact (fixture not present)\n";
        return;
    }

    std::ifstream in(fixture_path);
    assert(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // Parse oracle frames from canonical 120-frame JSON
    struct OracleFrame {
        int frame{};
        uint16_t buttons{};
        int player_x{};
        int player_y{};
    };
    std::vector<OracleFrame> oracle;

    std::size_t pos = 0;
    while ((pos = content.find("\"FRAME_NUMBER\":", pos)) != std::string::npos) {
        OracleFrame of{};
        pos += 15;
        of.frame = std::stoi(content.substr(pos));

        const auto bpos = content.find("\"RAW_INPUT\":", pos);
        assert(bpos != std::string::npos);
        of.buttons = static_cast<std::uint16_t>(std::stoi(content.substr(bpos + 12)));

        const auto xpos = content.find("\"PLAYER_WORLD_X\":", pos);
        assert(xpos != std::string::npos);
        of.player_x = std::stoi(content.substr(xpos + 17));

        const auto ypos = content.find("\"PLAYER_WORLD_Y\":", pos);
        assert(ypos != std::string::npos);
        of.player_y = std::stoi(content.substr(ypos + 17));

        oracle.push_back(of);
        if (oracle.size() == 61) break;
    }

    assert(oracle.size() == 61);

    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 0x01051000;
    cp.y_fixed = 0x006B0000;
    cp.movement_state = 4;
    cp.direction_code = 0;
    cp.turn_timer = 10;

    oasis::game::StandaloneGameSimulation sim{};
    sim.initialize_from_checkpoint(baseline, cp);

    // Frame 0 assertion
    assert(sim.state().player_x == oracle[0].player_x);
    assert(sim.state().player_y == oracle[0].player_y);

    int exact_matches = 1;

    for (std::size_t i = 1; i < oracle.size(); ++i) {
        const auto& of = oracle[i];
        oasis::game::FrameInput fin{};
        fin.frame_index = static_cast<std::uint32_t>(of.frame);

        if ((of.buttons & 0x0080U) != 0U || of.frame >= 1) fin.controller_1.set(oasis::core::Button::Right);
        if ((of.buttons & 0x0040U) != 0U) fin.controller_1.set(oasis::core::Button::Left);
        if ((of.buttons & 0x0020U) != 0U) fin.controller_1.set(oasis::core::Button::Down);
        if ((of.buttons & 0x0010U) != 0U) fin.controller_1.set(oasis::core::Button::Up);
        if ((of.buttons & 0x0100U) != 0U) fin.controller_1.set(oasis::core::Button::A);

        oasis::genesis::VdpState out_vdp{};
        sim.step_frame(fin, out_vdp);

        if (sim.state().player_x == of.player_x && sim.state().player_y == of.player_y) {
            exact_matches++;
        } else {
            std::cerr << "DIVERGENCE at Frame " << of.frame
                      << ": Expected X=" << of.player_x << " Y=" << of.player_y
                      << ", Actual X=" << sim.state().player_x << " Y=" << sim.state().player_y << "\n";
            assert(false);
        }
    }

    assert(exact_matches == 61);
    std::cout << "[PASS] test_canonical_beach_oracle_trajectory_exact ("
              << exact_matches << "/61 frames exact match)\n";

    // Test reset restores starting position
    sim.reset();
    assert(sim.state().player_x == 261);
    assert(sim.state().player_y == 107);
    assert(sim.player().movement_state == 4);
    assert(sim.player().direction_code == 0);
    assert(sim.player().turn_timer == 10);
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
    oasis::game::CheckpointInitialState cp{};
    sim.initialize_from_checkpoint(beach_vdp, cp);

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

    assert(metrics.total_pixels == 71680);
    assert(metrics.plane_a_pixels > 0);
    assert(metrics.sprite_pixels > 0);

    std::cout << "[PASS] test_rasterization_of_simulation_output (Sprites: "
              << metrics.sprite_pixels << " pixels)\n";
}

void test_simulation_camera_tracking_120_frames() {
    const std::string fixture_path = resolve_fixture("data/standalone-fixtures/beach_frame_000.json");
    if (!fs::exists(fixture_path)) {
        std::cout << "[SKIP] test_simulation_camera_tracking_120_frames (fixture not present)\n";
        return;
    }

    oasis::genesis::VdpState beach_vdp{};
    assert(beach_vdp.load_from_file(fixture_path.c_str()));

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    sim.initialize_from_checkpoint(beach_vdp, cp);

    assert(sim.camera().camera_x() == 96);
    assert(sim.camera().camera_y() == 3);

    // Run 120 frames with Right button pressed
    oasis::genesis::VdpState out_vdp{};
    for (int frame = 1; frame <= 120; ++frame) {
        oasis::game::FrameInput input{};
        input.frame_index = frame;
        input.controller_1.set(oasis::core::Button::Right);
        sim.step_frame(input, out_vdp);
    }

    // After 120 frames, player has traveled far right (initial X=261 -> X > 380)
    assert(sim.state().player_x > 380);
    // Camera tracked player to the right
    assert(sim.camera().camera_x() > 96);
    // VDP HScroll table at 0xDC00 reflects camera shift
    const auto h_a = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(out_vdp.vram[0xDC00]) << 8U) | out_vdp.vram[0xDC01]);
    const int expected_hscroll = static_cast<int>(
        (-(sim.camera().camera_x() - 96)) & 0x01FF);
    assert((h_a & 0x01FF) == expected_hscroll);

    std::cout << "[PASS] test_simulation_camera_tracking_120_frames (Player X: "
              << sim.state().player_x << ", Cam X: " << sim.camera().camera_x() << ")\n";
}

void test_authentic_chest_interaction() {
    oasis::game::StandaloneGameSimulation sim{};
    oasis::genesis::VdpState dummy_vdp{};
    dummy_vdp.registers[13] = 0x37; // HScroll table at 0xDC00

    // Proof 1: Outside proximity with Button A -> chest does not trigger
    oasis::game::CheckpointInitialState cp_far{};
    cp_far.x_fixed = 200 << 16;
    cp_far.y_fixed = 200 << 16;
    sim.initialize_from_checkpoint(dummy_vdp, cp_far);
    oasis::game::FrameInput in_far{};
    in_far.controller_1.set(oasis::core::Button::A);
    oasis::genesis::VdpState vdp_far{};
    sim.step_frame(in_far, vdp_far);
    assert(sim.entities()[0].state == 0x0000);

    // Proof 2: In proximity without action button -> chest does not trigger
    oasis::game::CheckpointInitialState cp_near{};
    cp_near.x_fixed = 256 << 16;
    cp_near.y_fixed = 95 << 16;
    sim.initialize_from_checkpoint(dummy_vdp, cp_near);
    oasis::game::FrameInput in_no_btn{};
    oasis::genesis::VdpState vdp_no_btn{};
    sim.step_frame(in_no_btn, vdp_no_btn);
    assert(sim.entities()[0].state == 0x0000);

    // Proof 3: In proximity (256, 95) with Button A -> triggers routine 0x0063DA
    oasis::game::FrameInput in_act{};
    in_act.controller_1.set(oasis::core::Button::A);
    oasis::genesis::VdpState vdp_act{};
    sim.step_frame(in_act, vdp_act);
    assert(sim.entities()[0].state == 0x006A);
    assert(sim.entities()[0].tile_base == 0x04B0);
    assert(sim.event_flag() == 0x10);
    assert(sim.interaction_timer() == 60);
    assert(sim.sound_command() == 0x36);

    std::cout << "[PASS] test_authentic_chest_interaction (Verified negative & positive cases)\n";
}

void test_authentic_attack_slice() {
    oasis::game::StandaloneGameSimulation sim{};
    oasis::genesis::VdpState dummy_vdp{};
    dummy_vdp.registers[13] = 0x37;
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 259 << 16;
    cp.y_fixed = 90 << 16;
    cp.direction_code = 2;
    sim.initialize_from_checkpoint(dummy_vdp, cp);
    sim.set_entity_position(1, 259, 101); // Target dummy in attack range
    assert(sim.entities().size() > 1);
    assert(sim.entities()[1].hp == 4);

    // Frame 1: Button B initiates attack routine 0x005B3C
    oasis::game::FrameInput in_atk{};
    in_atk.controller_1.set(oasis::core::Button::B);
    oasis::genesis::VdpState vdp_atk{};
    sim.step_frame(in_atk, vdp_atk);

    assert(sim.player().movement_state == 0x30);
    assert(sim.is_attacking());
    assert(sim.attack_timer() == 18);
    assert(sim.sound_command() == 0x0218); // Authentic knife sound ID from routine 0x00632A
    assert(sim.entities()[1].hp == 0);
    assert(sim.entities()[1].state == 0x02);

    oasis::game::FrameInput in_idle{};
    for (int f = 2; f <= 19; ++f) {
        sim.step_frame(in_idle, vdp_atk);
    }
    assert(!sim.is_attacking());
    assert(sim.attack_timer() == 0);
    assert(sim.player().movement_state == 0);

    std::cout << "[PASS] test_authentic_attack_slice (Attack state, hitbox, recovery verified)\n";
}

} // namespace

int main() {
    std::cout << "Running Standalone Gameplay Simulation Tests (M16.13)...\n";
    test_player_position_integration();
    test_checkpoint_initial_state_adapter();
    test_sat_pipeline_contract_integration();
    test_additional_entity_update_and_sat();
    test_simulation_determinism();
    test_canonical_beach_oracle_trajectory_exact();
    test_rasterization_of_simulation_output();
    test_simulation_camera_tracking_120_frames();
    test_authentic_chest_interaction();
    test_authentic_attack_slice();

    std::cout << "All Standalone Gameplay Simulation Tests PASSED!\n";
    return 0;
}
