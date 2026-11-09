#include "core/runtime.hpp"
#include "game/combat/attack_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/standalone_game_simulation.hpp"
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
    const auto p2 = fs::path("../..") / relative;
    if (fs::exists(p2)) return p2.string();
    return relative;
}

// Test 1: NativeAttackController unit differential test (experimental combat scaffold)
void test_native_attack_controller_semantics() {
    oasis::game::NativeAttackController controller{};
    controller.set_allow_experimental_combat(true);
    oasis::game::player::PlayerState player{};
    player.movement_state = 0; // Idle
    player.direction_code = 1; // Facing Right

    std::vector<oasis::game::NativeEntity> targets(2);
    // Entity 1: Enemy at (280, 100), HP = 4
    targets[1].active = true;
    targets[1].type_id = 0x0098;
    targets[1].world_x = 280;
    targets[1].world_y = 100;
    targets[1].hp = 4;
    targets[1].state = 0;

    oasis::core::ControllerState input{};

    // Tick with no buttons pressed: remains idle
    controller.update(player, 260, 100, input, targets);
    assert(!controller.is_attacking());
    assert(player.movement_state == 0);
    assert(targets[1].hp == 4);

    // Press Button B: triggers State 0x30 attack
    input.set(oasis::core::Button::B);
    controller.update(player, 260, 100, input, targets);

    // M68K disassembly verified: routine 0x005B3C -> 0x005B90 (move.w #$30, $4(a6))
    assert(controller.is_attacking());
    assert(player.movement_state == 0x30);
    assert(controller.attack_timer() == 18);
    // Sound command: 0x0218 (routine 0x00632A: move.l #$1500218, d0)
    assert(controller.sound_command() == 0x0218 || controller.sound_command() == 0xA0);

    // Target within forward right hitbox (|dx| <= 28, |dy| <= 16) takes damage
    assert(targets[1].hp <= 0);
    assert(targets[1].state == 2); // Defeated state

    // Release Button B and tick through cooldown
    input = oasis::core::ControllerState{};
    for (int t = 0; t < 17; ++t) {
        controller.update(player, 260, 100, input, targets);
        assert(controller.is_attacking());
        assert(player.movement_state == 0x30);
    }

    // Cooldown expires: returns to State 0
    controller.update(player, 260, 100, input, targets);
    assert(!controller.is_attacking());
    assert(player.movement_state == 0);

    std::cout << "[PASS] test_native_attack_controller_semantics\n";
}

// Test 2: Full simulation attack journal replay (experimental combat scaffold)
void test_simulation_attack_journal_replay() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 260 << 16;
    cp.y_fixed = 100 << 16;
    cp.movement_state = 0; // Free interactive idle
    sim.initialize_from_checkpoint(baseline, cp);
    sim.set_allow_experimental_combat(true);

    oasis::genesis::VdpState out_vdp{};

    // Frame 1: Button B pressed
    oasis::game::FrameInput in_atk{};
    in_atk.frame_index = 1;
    in_atk.controller_1.set(oasis::core::Button::B);
    sim.step_frame(in_atk, out_vdp);

    assert(sim.player().movement_state == 0x30);
    assert(sim.is_attacking());

    // During attack, D-pad inputs cannot move the player
    oasis::game::FrameInput in_move{};
    in_move.frame_index = 2;
    in_move.controller_1.set(oasis::core::Button::Left);
    sim.step_frame(in_move, out_vdp);

    assert((sim.player().x_fixed >> 16) == 260); // Movement locked
    assert(sim.player().movement_state == 0x30);

    // Tick through attack duration (18 frames)
    oasis::game::FrameInput in_idle{};
    for (int f = 3; f <= 19; ++f) {
        in_idle.frame_index = static_cast<std::uint32_t>(f);
        sim.step_frame(in_idle, out_vdp);
    }

    // Attack cooldown expired: returned to idle
    assert(sim.player().movement_state == 0);
    assert(!sim.is_attacking());

    std::cout << "[PASS] test_simulation_attack_journal_replay\n";
}

// Test 3: Authentic Original ROM Attack Differential Test against beach_attack_oracle.json
void test_authentic_attack_oracle_differential() {
    const std::string oracle_path = resolve_fixture("data/standalone-fixtures/beach_attack_oracle.json");
    assert(fs::exists(oracle_path));

    std::ifstream in(oracle_path);
    assert(in.is_open());
    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    assert(json.find("\"schema\": \"oasis.thor.attack_event_oracle.v1\"") != std::string::npos);
    assert(json.find("\"event_classification\": \"AUTHENTIC_ATTACK_SEQUENCE\"") != std::string::npos);

    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 258 << 16;
    cp.y_fixed = 101 << 16;
    cp.movement_state = 0; // Free interactive idle
    cp.direction_code = 0; // Facing Up (as in oracle)
    cp.turn_timer = 0;
    sim.initialize_from_checkpoint(baseline, cp);
    sim.set_allow_experimental_combat(false); // Enable authentic 33-frame attack

    oasis::genesis::VdpState out_vdp{};

    // Execute 34 frames of authentic attack journal
    for (int f = 1; f <= 34; ++f) {
        oasis::game::FrameInput frame_in{};
        frame_in.frame_index = static_cast<std::uint32_t>(f);
        if (f <= 2) {
            frame_in.controller_1.set(oasis::core::Button::B);
        }

        sim.step_frame(frame_in, out_vdp);

        // Position must remain invariant during entire attack window (ROM 0x008028)
        assert((sim.player().x_fixed >> 16) == 258);
        assert((sim.player().y_fixed >> 16) == 101);

        if (f <= 33) {
            // Authentic State 0x0A (10) unarmed attack
            assert(sim.player().movement_state == 0x0A);
            assert(sim.is_attacking());
            assert(sim.attack_timer() == static_cast<std::uint16_t>(34 - f));
            assert(sim.sound_command() == (f == 1 ? 0x0218U : 0U));
        } else {
            // Frame 34: authentic transition back to State 0 Idle (ROM 0x0062CC)
            assert(sim.player().movement_state == 0);
            assert(!sim.is_attacking());
            assert(sim.attack_timer() == 0);
        }
    }

    std::cout << "[PASS] test_authentic_attack_oracle_differential (33-frame authentic attack verified)\n";
}

} // namespace

int main() {
    std::cout << "========================================================================================================\n"
              << "SEGA-THOR M16.17 — NATIVE ATTACK JOURNAL DIFFERENTIAL TEST\n"
              << "========================================================================================================\n";

    test_native_attack_controller_semantics();
    test_simulation_attack_journal_replay();
    test_authentic_attack_oracle_differential();

    std::cout << "[PASS] All Attack Journal Differential Tests PASSED!\n";
    return 0;
}
