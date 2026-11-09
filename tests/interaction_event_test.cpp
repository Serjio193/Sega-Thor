#include "core/runtime.hpp"
#include "game/entity/native_entity.hpp"
#include "game/interaction/interaction_system.hpp"
#include "game/player/player.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
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

void test_interaction_oracle_fixture_parsing() {
    const std::string path = resolve_fixture("data/standalone-fixtures/beach_chest_interaction_event_oracle.json");
    if (!fs::exists(path)) {
        std::cout << "[SKIP] test_interaction_oracle_fixture_parsing (fixture not found)\n";
        return;
    }

    std::ifstream in(path);
    assert(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    assert(content.find("\"schema\": \"oasis.thor.interaction_event_oracle.v1\"") != std::string::npos);
    assert(content.find("\"status\": \"ORIGINAL_EXECUTION_CONFIRMED\"") != std::string::npos);
    assert(content.find("\"event_classification\": \"CHEST_OR_OBJECT_INTERACTION\"") != std::string::npos);

    // Count frame records in the 20-frame oracle window
    std::size_t frame_count = 0;
    std::size_t pos = 0;
    while ((pos = content.find("\"FRAME_NUMBER\":", pos)) != std::string::npos) {
        ++frame_count;
        pos += 15;
    }
    assert(frame_count == 20);

    // Verify trigger frame 567 signature
    assert(content.find("\"FRAME_NUMBER\": 567") != std::string::npos);
    assert(content.find("\"PLAYER_MOVEMENT_STATE\": 106") != std::string::npos); // 0x6A
    assert(content.find("\"INTERACTION_TIMER_1856\": 59") != std::string::npos);

    std::cout << "[PASS] test_interaction_oracle_fixture_parsing (20 frames confirmed)\n";
}

void test_interaction_system_negative_cases() {
    oasis::game::NativeInteractionSystem system{};
    oasis::core::ControllerState controller{};
    std::vector<oasis::game::NativeEntity> entities;

    oasis::game::NativeEntity chest{};
    chest.type_id = 0x0060;
    chest.world_x = 256;
    chest.world_y = 88;
    chest.state = 0;
    chest.tile_base = 0x04A8;
    chest.active = true;
    entities.push_back(chest);

    // Negative Proof 1: Outside interaction bounding box with Button A
    controller.set(oasis::core::Button::A);
    system.update(200, 200, controller, entities);
    assert(entities[0].state == 0);
    assert(system.interaction_timer() == 0);
    assert(system.event_flag() == 0);
    assert(system.sound_command() == 0);

    // Negative Proof 2: In bounding box (258, 100) but no button pressed
    controller = oasis::core::ControllerState{};
    system.update(258, 100, controller, entities);
    assert(entities[0].state == 0);
    assert(system.interaction_timer() == 0);
    assert(system.event_flag() == 0);

    // Negative Proof 3: In bounding box with attack button (Button B), not action
    controller.set(oasis::core::Button::B);
    system.update(258, 100, controller, entities);
    assert(entities[0].state == 0);
    assert(system.interaction_timer() == 0);
    assert(system.event_flag() == 0);

    std::cout << "[PASS] test_interaction_system_negative_cases\n";
}

void test_interaction_system_experimental_button_gate() {
    oasis::game::NativeInteractionSystem system{};
    // In original-parity mode, experimental button trigger is disabled
    system.set_experimental_button_trigger_enabled(false);
    system.set_trigger_on_proximity(false);

    oasis::core::ControllerState controller{};
    controller.set(oasis::core::Button::A); // Press Button A

    std::vector<oasis::game::NativeEntity> entities;
    oasis::game::NativeEntity chest{};
    chest.type_id = 0x0060;
    chest.world_x = 256;
    chest.world_y = 88;
    chest.state = 0;
    chest.tile_base = 0x04A8;
    chest.active = true;
    entities.push_back(chest);

    // Ali at (258, 100), in bounding box, Button A pressed
    system.update(258, 100, controller, entities);

    // Button trigger is disabled: chest remains closed
    assert(entities[0].state == 0);
    assert(system.interaction_timer() == 0);
    assert(system.event_flag() == 0);

    // Now enable proximity trigger (authentic ROM cutscene contact path)
    system.set_trigger_on_proximity(true);
    controller = oasis::core::ControllerState{}; // zero buttons pressed
    system.update(258, 100, controller, entities);

    // Authentic collision contact triggers opening
    assert(entities[0].state == 0x6A);
    assert(entities[0].tile_base == 0x04B0);
    assert(system.event_flag() == 0x10);
    assert(system.interaction_timer() == 60);

    std::cout << "[PASS] test_interaction_system_experimental_button_gate\n";
}

void test_interaction_system_proximity_cutscene_trigger() {
    oasis::game::NativeInteractionSystem system{};
    // In canonical opening cutscene (frame 567), Ali reaches (258, 104) with raw_input == 0.
    // The proximity contact flag ($6C(a6) in ROM routine 0x006338) triggers 0x0063DA directly.
    system.set_trigger_on_proximity(true);

    oasis::core::ControllerState controller{}; // No buttons pressed (current_input == 0)
    std::vector<oasis::game::NativeEntity> entities;

    oasis::game::NativeEntity chest{};
    chest.type_id = 0x0060;
    chest.world_x = 256;
    chest.world_y = 88;
    chest.state = 0;
    chest.tile_base = 0x04A8;
    chest.active = true;
    entities.push_back(chest);

    // Ali at (258, 100), in proximity of chest (256, 88), zero controller input
    system.update(258, 100, controller, entities);

    assert(entities[0].state == 0x6A);
    assert(entities[0].tile_base == 0x04B0);
    assert(system.event_flag() == 0x10);
    assert(system.interaction_timer() == 60);
    assert(system.sound_command() == 0x36);

    std::cout << "[PASS] test_interaction_system_proximity_cutscene_trigger (Frame 567 causality verified)\n";
}

void test_interaction_system_positive_trigger_and_countdown() {
    oasis::game::NativeInteractionSystem system{};
    oasis::core::ControllerState controller{};
    std::vector<oasis::game::NativeEntity> entities;

    oasis::game::NativeEntity chest{};
    chest.type_id = 0x0060;
    chest.world_x = 256;
    chest.world_y = 88;
    chest.state = 0;
    chest.tile_base = 0x04A8;
    chest.active = true;
    entities.push_back(chest);

    // Positive Trigger: Ali at (258, 100), Button A pressed
    // dx = |258 - 256| = 2 <= 20, dy = |100 - 88| = 12 <= 12
    controller.set(oasis::core::Button::A);
    system.update(258, 100, controller, entities);

    // Assert ROM disassembly confirmed state transition (routine 0x0063DA -> 0x006430)
    assert(entities[0].state == 0x6A);
    assert(entities[0].tile_base == 0x04B0);
    assert(system.event_flag() == 0x10);
    assert(system.interaction_timer() == 60);
    assert(system.sound_command() == 0x36);

    // Verify authentic countdown sequence
    controller = oasis::core::ControllerState{};
    for (std::uint16_t expected_timer = 59; expected_timer > 0; --expected_timer) {
        system.update(258, 100, controller, entities);
        assert(system.interaction_timer() == expected_timer);
        assert(system.sound_command() == 0); // Sound is one-shot
        assert(entities[0].state == 0x6A);   // Stays opened
    }

    // Next frame: timer reaches 0
    system.update(258, 100, controller, entities);
    assert(system.interaction_timer() == 0);

    std::cout << "[PASS] test_interaction_system_positive_trigger_and_countdown\n";
}

void test_full_simulation_chest_differential() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 258 << 16;
    cp.y_fixed = 100 << 16;
    sim.initialize_from_checkpoint(baseline, cp);

    assert(!sim.chest_opened());
    assert(sim.interaction_timer() == 0);
    assert(sim.event_flag() == 0);

    // Frame 1: Action Button C pressed at chest proximity
    oasis::game::FrameInput in_act{};
    in_act.frame_index = 1;
    in_act.controller_1.set(oasis::core::Button::C);
    oasis::genesis::VdpState out_vdp{};
    sim.step_frame(in_act, out_vdp);

    assert(sim.chest_opened());
    assert(sim.event_flag() == 0x10);
    assert(sim.interaction_timer() == 60);
    assert(sim.sound_command() == 0x36);

    // Step 5 frames and verify countdown in simulation
    oasis::game::FrameInput in_idle{};
    for (int f = 2; f <= 6; ++f) {
        in_idle.frame_index = f;
        sim.step_frame(in_idle, out_vdp);
    }
    assert(sim.interaction_timer() == 55);
    assert(sim.chest_opened());

    std::cout << "[PASS] test_full_simulation_chest_differential\n";
}

} // namespace

int main() {
    std::cout << "Running Native Interaction Event Oracle Tests (M16.14)...\n";
    test_interaction_oracle_fixture_parsing();
    test_interaction_system_negative_cases();
    test_interaction_system_experimental_button_gate();
    test_interaction_system_proximity_cutscene_trigger();
    test_interaction_system_positive_trigger_and_countdown();
    test_full_simulation_chest_differential();
    std::cout << "All Native Interaction Event Tests PASSED!\n";
    return 0;
}
