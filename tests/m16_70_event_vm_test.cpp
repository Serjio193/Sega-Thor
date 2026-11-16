#include "game/events/event_vm.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

std::vector<std::uint8_t> make_op12(
    std::uint8_t sub_action,
    std::uint8_t cmp_or_neg,
    std::uint16_t imm16,
    std::int16_t rel16) {
    return {
        0x12U, 0x00U, sub_action, cmp_or_neg,
        static_cast<std::uint8_t>((imm16 >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(imm16 & 0xFFU),
        static_cast<std::uint8_t>((static_cast<std::uint16_t>(rel16) >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(static_cast<std::uint16_t>(rel16) & 0xFFU),
    };
}

} // namespace

int main() {
    using oasis::game::events::evaluate_op12_comparator;
    using oasis::game::events::evaluate_op12_condition;
    using oasis::game::events::EventVmInterpreter;
    using oasis::game::events::lookup_event_vm_opcode_target;

    // 1. All 31 opcode targets (0x00..0x1E) at 0x00F7D0.
    assert(lookup_event_vm_opcode_target(0x00U) == 0x0000F91CU);
    assert(lookup_event_vm_opcode_target(0x12U) == 0x0000FAB2U);
    assert(lookup_event_vm_opcode_target(0x1EU) == 0x0000F854U);
    assert(lookup_event_vm_opcode_target(0x1FU) == 0U);

    // 2. All 6 comparators (0, 2, 4, 6, 8, 10).
    assert(evaluate_op12_comparator(0U, 5U, 5U) && !evaluate_op12_comparator(0U, 5U, 6U));
    assert(evaluate_op12_comparator(2U, 5U, 6U) && !evaluate_op12_comparator(2U, 5U, 5U));
    assert(evaluate_op12_comparator(4U, 6U, 5U) && !evaluate_op12_comparator(4U, 5U, 5U));
    assert(evaluate_op12_comparator(6U, 5U, 5U) && !evaluate_op12_comparator(6U, 4U, 5U));
    assert(evaluate_op12_comparator(8U, 5U, 5U) && !evaluate_op12_comparator(8U, 6U, 5U));
    assert(evaluate_op12_comparator(10U, 4U, 5U) && !evaluate_op12_comparator(10U, 5U, 5U));

    // 3. All 12 Opcode 0x12 sub-actions (0x00..0x0B).
    EventVmInterpreter vm;
    auto& st = vm.state();
    st.entities[0] = {0x0048U, 0U}; // non-spirit
    st.entities[1] = {0x0018U, 1U}; // spirit (exempt)
    st.global_flags[0] = 0x08U;     // bit 3
    st.chest_flags[0] = 0x02U;      // bit 1
    st.secondary_flags[0] = 0x04U;  // bit 2
    st.player_hp = 50U; st.player_max_hp = 200U; // 25%
    st.player_sp = 60U; st.player_max_sp = 100U; // 60%
    st.active_spirit_id = 0x001AU;
    st.player_x = 120U; st.player_y = 240U;
    st.spirit_x = 140U; st.spirit_y = 260U;

    assert(evaluate_op12_condition(st, make_op12(0x00U, 0U, 1U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x01U, 0U, 3U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x02U, 0U, 1U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x03U, 0U, 2U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x04U, 0U, 0x0048U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x05U, 0U, 25U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x06U, 6U, 60U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x07U, 0U, 0x001AU, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x08U, 0U, 120U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x09U, 0U, 240U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x0AU, 0U, 140U, 16), 0U));
    assert(evaluate_op12_condition(st, make_op12(0x0BU, 0U, 260U, 16), 0U));

    // 4. Script execution: SetEventVar (0x1C), Op12 branch (+12), WaitFrames (0x08), End (0x00).
    std::vector<std::uint8_t> script = {
        0x1CU, 0x00U, 0x00U, 0x42U,                         // pc=0: event_var = 0x0042
        0x12U, 0x00U, 0x01U, 0x00U, 0x00U, 0x03U, 0x00U, 0x0CU, // pc=4: if global bit 3 -> +12 (pc=16)
        0x1CU, 0x00U, 0x00U, 0x99U,                         // pc=12: skipped
        0x08U, 0x02U,                                       // pc=16: wait 2 frames -> pc=18
        0x1BU, 0x00U, 0x00U, 0x80U, 0x03U, 0x99U,           // pc=18: play sound 0x800399 -> pc=24
        0x00U, 0x00U                                        // pc=24: halt
    };
    vm.reset(0U);
    vm.state().global_flags[0] = 0x08U;
    vm.step_frame(script); // Executes 0x1C, 0x12 (->16), 0x08 (wait=2, yields)
    assert(vm.state().event_var == 0x0042U);
    assert(vm.state().pc == 18U);
    assert(vm.state().wait_frames == 2U);

    vm.step_frame(script); // wait_frames 2 -> 1 (yields)
    assert(vm.state().wait_frames == 1U);
    assert(vm.state().sound_commands.empty());

    vm.step_frame(script); // wait_frames 1 -> 0, executes 0x1B and 0x00
    assert(vm.state().sound_commands.size() == 1U);
    assert(vm.state().sound_commands[0] == 0x00800399U);
    assert(vm.state().halted);

    return 0;
}
