// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"

namespace oasis::hybrid::generated {

void execute_0x002D66(BasicBlockApi& api) {
    // guest 0x002D66 opcode 0x48E7 48E7 0110 movem.l D7/A3,-(A7)
    const auto opcode_0x002D66 = fetch_checked(api, 0x48E7U);
    api.begin_instruction(opcode_0x002D66);
    (void)fetch_checked(api, 0x0110U);
    movem_l_predecrement(api, 0x0880U);
    api.finish_instruction(opcode_0x002D66);
    api.add_cycles(112);
    api.skip_bus_refresh();
    // guest 0x002D6A opcode 0x4247 4247 clr.w D7
    const auto opcode_0x002D6A = fetch_checked(api, 0x4247U);
    api.begin_instruction(opcode_0x002D6A);
    clear_w_data_register(api, 7U);
    api.finish_instruction(opcode_0x002D6A);
    // guest 0x002D6C opcode 0x1E1E 1E1E move.b (A6)+,D7
    const auto opcode_0x002D6C = fetch_checked(api, 0x1E1EU);
    api.begin_instruction(opcode_0x002D6C);
    move_b_postincrement_to_data_register(api, 6U, 7U);
    api.finish_instruction(opcode_0x002D6C);
    // guest 0x002D6E opcode 0x47F9 47F9 00FF 134C lea.l ($00FF134C).L,A3
    const auto opcode_0x002D6E = fetch_checked(api, 0x47F9U);
    api.begin_instruction(opcode_0x002D6E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x134CU);
    lea_absolute_long(api, 0xFF134CU, 3U);
    api.finish_instruction(opcode_0x002D6E);
    // guest 0x002D74 opcode 0xD6C7 D6C7 adda.w D7,A3
    const auto opcode_0x002D74 = fetch_checked(api, 0xD6C7U);
    api.begin_instruction(opcode_0x002D74);
    adda_w_data_to_address(api, 7U, 3U);
    api.finish_instruction(opcode_0x002D74);
    // guest 0x002D76 opcode 0x1E1E 1E1E move.b (A6)+,D7
    const auto opcode_0x002D76 = fetch_checked(api, 0x1E1EU);
    api.begin_instruction(opcode_0x002D76);
    move_b_postincrement_to_data_register(api, 6U, 7U);
    api.finish_instruction(opcode_0x002D76);
    // guest 0x002D78 opcode 0x36DE 36DE move.w (A6)+,(A3)+
    const auto opcode_0x002D78 = fetch_checked(api, 0x36DEU);
    api.begin_instruction(opcode_0x002D78);
    move_w_postincrement_to_postincrement(api, 6U, 3U);
    api.finish_instruction(opcode_0x002D78);
}

void execute_0x0604BC(BasicBlockApi& api) {
    // guest 0x0604BC opcode 0x4DF9 4DF9 00FF 0628 lea.l ($00FF0628).L,A6
    const auto opcode_0x0604BC = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0604BC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0628U);
    lea_absolute_long(api, 0xFF0628U, 6U);
    api.finish_instruction(opcode_0x0604BC);
}

void execute_0x061032(BasicBlockApi& api) {
    // guest 0x061032 opcode 0xD481 D481 add.l D1,D2
    const auto opcode_0x061032 = fetch_checked(api, 0xD481U);
    api.begin_instruction(opcode_0x061032);
    add_l_data_to_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x061032);
}

} // namespace oasis::hybrid::generated
