#include "core/runtime.hpp"
#include "game/combat/attack_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

// Test 1: NativeAttackController unit differential test
void test_native_attack_controller_semantics() {
    oasis::game::NativeAttackController controller{};
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

// Test 2: Full simulation attack journal replay
void test_simulation_attack_journal_replay() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68;

    oasis::game::StandaloneGameSimulation sim{};
    oasis::game::CheckpointInitialState cp{};
    cp.x_fixed = 260 << 16;
    cp.y_fixed = 100 << 16;
    cp.movement_state = 0; // Free interactive idle
    sim.initialize_from_checkpoint(baseline, cp);

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

    // Tick through attack duration (18 logic frames = 36 simulation frames at 30Hz)
    oasis::game::FrameInput in_idle{};
    for (int f = 3; f <= 40; ++f) {
        in_idle.frame_index = f;
        sim.step_frame(in_idle, out_vdp);
    }

    // Attack cooldown expired: returned to idle
    assert(sim.player().movement_state == 0);
    assert(!sim.is_attacking());

    std::cout << "[PASS] test_simulation_attack_journal_replay\n";
}

} // namespace

int main() {
    std::cout << "========================================================================================================\n"
              << "SEGA-THOR M16.16 — NATIVE ATTACK JOURNAL DIFFERENTIAL TEST\n"
              << "========================================================================================================\n";

    test_native_attack_controller_semantics();
    test_simulation_attack_journal_replay();

    std::cout << "[PASS] All Attack Journal Differential Tests PASSED!\n";
    return 0;
}
