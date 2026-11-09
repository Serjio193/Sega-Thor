#include "core/runtime.hpp"
#include "game/entity/native_entity.hpp"
#include "game/standalone_game_simulation.hpp"
#include "genesis/vdp_state.hpp"

#include <cassert>
#include <iostream>

namespace {

void test_live_player_interactive_journal() {
    oasis::genesis::VdpState baseline{};
    baseline.registers[5] = 0x68; // sat_base = 0xD000

    oasis::game::StandaloneGameSimulation sim{};
    // Initialize in interactive free-play mode (movement_state = 0)
    sim.initialize(baseline, 261, 107);
    assert(sim.player().movement_state == 0);
    assert(!sim.is_attacking());
    assert(!sim.chest_opened());

    const std::int32_t init_x = sim.player().x_fixed;
    const std::int32_t init_y = sim.player().y_fixed;

    // Step 1: Feed 10 frames of moving Right (5 logic steps in 30Hz cadence)
    for (int f = 1; f <= 10; ++f) {
        oasis::game::FrameInput in{};
        in.frame_index = f;
        in.controller_1.set(oasis::core::Button::Right);
        oasis::genesis::VdpState out_vdp{};
        sim.step_frame(in, out_vdp);
    }

    // In 30Hz cadence, 5 logic updates occurred (frames 1, 3, 5, 7, 9)
    const std::int32_t expected_x = init_x + 5 * 0x36000;
    assert(sim.player().x_fixed == expected_x);
    assert(sim.player().direction_code == 1); // Facing right
    assert(sim.player().movement_state == 2);  // Moving state

    // Step 2: Feed 4 idle frames (2 logic steps) to stop
    for (int f = 11; f <= 14; ++f) {
        oasis::game::FrameInput in{};
        in.frame_index = f;
        oasis::genesis::VdpState out_vdp{};
        sim.step_frame(in, out_vdp);
    }
    assert(sim.player().movement_state == 0); // Decelerated to idle
    assert(sim.walk_tick() == 0);

    // Step 3: Feed 6 frames of moving Down (3 logic steps)
    for (int f = 15; f <= 20; ++f) {
        oasis::game::FrameInput in{};
        in.frame_index = f;
        in.controller_1.set(oasis::core::Button::Down);
        oasis::genesis::VdpState out_vdp{};
        sim.step_frame(in, out_vdp);
    }
    const std::int32_t expected_y = init_y + 3 * 0x30000;
    assert(sim.player().y_fixed == expected_y);
    assert(sim.player().direction_code == 2); // Facing down

    // Step 4: Native Dagger Attack on Button B
    oasis::game::FrameInput in_atk{};
    in_atk.frame_index = 21;
    in_atk.controller_1.set(oasis::core::Button::B);
    oasis::genesis::VdpState vdp_atk{};
    sim.step_frame(in_atk, vdp_atk);

    assert(sim.player().movement_state == 0x30); // Attack state
    assert(sim.is_attacking());
    assert(sim.attack_timer() == 18);
    assert(sim.sound_command() == 0x0218); // M16.16: Authentic dagger swing sound (routine 0x00632A)

    // Attack cooldown over 18 frames
    for (int f = 22; f <= 39; ++f) {
        oasis::game::FrameInput in_idle{};
        in_idle.frame_index = f;
        sim.step_frame(in_idle, vdp_atk);
    }
    assert(!sim.is_attacking());
    assert(sim.attack_timer() == 0);
    assert(sim.player().movement_state == 0);

    // Step 5: Chest interaction with Action Button A
    sim.set_player_position(258, 100); // In proximity of chest (256, 88)
    oasis::game::FrameInput in_chest{};
    in_chest.frame_index = 40;
    in_chest.controller_1.set(oasis::core::Button::A);
    sim.step_frame(in_chest, vdp_atk);

    assert(sim.chest_opened());
    assert(sim.event_flag() == 0x10);
    assert(sim.interaction_timer() == 60);
    assert(sim.sound_command() == 0x36); // Chest discovery sound

    std::cout << "[PASS] test_live_player_interactive_journal (Live simulation verified)\n";
}

} // namespace

int main() {
    std::cout << "Running Live Player Journal Validation Tests (M16.15)...\n";
    test_live_player_interactive_journal();
    std::cout << "All Live Player Journal Tests PASSED!\n";
    return 0;
}
