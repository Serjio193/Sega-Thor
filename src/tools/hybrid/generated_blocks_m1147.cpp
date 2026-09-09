// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

unsigned instruction_count_from_0x00026A(unsigned entry_pc) {
    if (entry_pc == 0x00026AU) return 1U;
    return 0;
}

BlockExit execute_0x00026A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00026AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00026AU) {
    // guest 0x00026A opcode 0x2D00 2D00 move.l D0,-(A6)
    const auto opcode_0x00026A = fetch_checked(api, 0x2D00U);
    api.begin_instruction(opcode_0x00026A);
    move_l_data_to_predecrement_address(api, 0U, 6U);
    api.finish_instruction(opcode_0x00026A);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

unsigned instruction_count_from_0x003A0C(unsigned entry_pc) {
    if (entry_pc == 0x003A0CU) return 1U;
    return 0;
}

BlockExit execute_0x003A0C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003A0CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003A0CU) {
    // guest 0x003A0C opcode 0x12DA 12DA move.b (A2)+,(A1)+
    const auto opcode_0x003A0C = fetch_checked(api, 0x12DAU);
    api.begin_instruction(opcode_0x003A0C);
    move_b_postincrement_to_postincrement(api, 2U, 1U);
    api.finish_instruction(opcode_0x003A0C);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

unsigned instruction_count_from_0x00389E(unsigned entry_pc) {
    if (entry_pc == 0x00389EU) return 1U;
    return 0;
}

BlockExit execute_0x00389E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00389EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00389EU) {
    // guest 0x00389E opcode 0x12DA 12DA move.b (A2)+,(A1)+
    const auto opcode_0x00389E = fetch_checked(api, 0x12DAU);
    api.begin_instruction(opcode_0x00389E);
    move_b_postincrement_to_postincrement(api, 2U, 1U);
    api.finish_instruction(opcode_0x00389E);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

unsigned instruction_count_from_0x0003F0(unsigned entry_pc) {
    if (entry_pc == 0x0003F0U) return 1U;
    return 0;
}

BlockExit execute_0x0003F0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003F0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003F0U) {
    // guest 0x0003F0 opcode 0x4258 4258 clr.w (A0)+
    const auto opcode_0x0003F0 = fetch_checked(api, 0x4258U);
    api.begin_instruction(opcode_0x0003F0);
    clear_w_postincrement(api, 0U);
    api.finish_instruction(opcode_0x0003F0);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

unsigned instruction_count_from_0x06193C(unsigned entry_pc) {
    if (entry_pc == 0x06193CU) return 1U;
    return 0;
}

BlockExit execute_0x06193C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06193CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06193CU) {
    // guest 0x06193C opcode 0x082D 082D 0000 0000 btst.b #$0,0(A5)
    const auto opcode_0x06193C = fetch_checked(api, 0x082DU);
    api.begin_instruction(opcode_0x06193C);
    (void)fetch_checked(api, 0x0000U);
    (void)fetch_checked(api, 0x0000U);
    bit_test_immediate_displacement_address(api, 0U, 5U, 0);
    api.finish_instruction(opcode_0x06193C);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

unsigned instruction_count_from_0x061954(unsigned entry_pc) {
    if (entry_pc == 0x061954U) return 1U;
    return 0;
}

BlockExit execute_0x061954(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061954U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061954U) {
    // guest 0x061954 opcode 0x18FC 18FC 00FF move.b #$FF,(A4)+
    const auto opcode_0x061954 = fetch_checked(api, 0x18FCU);
    api.begin_instruction(opcode_0x061954);
    (void)fetch_checked(api, 0x00FFU);
    move_b_immediate_to_postincrement(api, 255U, 4U);
    api.finish_instruction(opcode_0x061954);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

unsigned instruction_count_from_0x061266(unsigned entry_pc) {
    if (entry_pc == 0x061266U) return 1U;
    return 0;
}

BlockExit execute_0x061266(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061266U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061266U) {
    // guest 0x061266 opcode 0x421D 421D clr.b (A5)+
    const auto opcode_0x061266 = fetch_checked(api, 0x421DU);
    api.begin_instruction(opcode_0x061266);
    clear_b_postincrement(api, 5U);
    api.finish_instruction(opcode_0x061266);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    return {api.reg(16), BlockExitReason::NORMAL_EXIT, instructions_executed};
}

} // namespace oasis::hybrid::generated
