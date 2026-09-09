// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x003990(unsigned entry_pc) {
    if (entry_pc == 0x003990U) return 1U;
    return 0;
}

BlockExit execute_0x003990(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003990U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003990U) {
    // guest 0x003990 opcode 0x3602 3602 move.w D2,D3
    const auto opcode_0x003990 = fetch_checked(api, 0x3602U);
    api.begin_instruction(opcode_0x003990);
    move_w_data_to_data(api, 2U, 3U);
    api.finish_instruction(opcode_0x003990);
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

unsigned instruction_count_from_0x003994(unsigned entry_pc) {
    if (entry_pc == 0x003994U) return 1U;
    return 0;
}

BlockExit execute_0x003994(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003994U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003994U) {
    // guest 0x003994 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003994 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003994);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003994);
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

unsigned instruction_count_from_0x003996(unsigned entry_pc) {
    if (entry_pc == 0x003996U) return 1U;
    return 0;
}

BlockExit execute_0x003996(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003996U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003996U) {
    // guest 0x003996 opcode 0x6B00 6B00 0136 bmi.w loc_003ACE
    const auto opcode_0x003996 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003996);
    branch_condition(api, 11U, 0x003ACEU, 14, 0x0136U);
    api.finish_instruction(opcode_0x003996);
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

unsigned instruction_count_from_0x00399C(unsigned entry_pc) {
    if (entry_pc == 0x00399CU) return 1U;
    return 0;
}

BlockExit execute_0x00399C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00399CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00399CU) {
    // guest 0x00399C opcode 0x6500 6500 0068 bcs.w loc_003A06
    const auto opcode_0x00399C = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x00399C);
    branch_condition(api, 5U, 0x003A06U, 14, 0x0068U);
    api.finish_instruction(opcode_0x00399C);
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

unsigned instruction_count_from_0x0039A0(unsigned entry_pc) {
    if (entry_pc == 0x0039A0U) return 1U;
    return 0;
}

BlockExit execute_0x0039A0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039A0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039A0U) {
    // guest 0x0039A0 opcode 0x5242 5242 addq.w #$1,D2
    const auto opcode_0x0039A0 = fetch_checked(api, 0x5242U);
    api.begin_instruction(opcode_0x0039A0);
    addq_w_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x0039A0);
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

unsigned instruction_count_from_0x0039A2(unsigned entry_pc) {
    if (entry_pc == 0x0039A2U) return 1U;
    return 0;
}

BlockExit execute_0x0039A2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039A2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039A2U) {
    // guest 0x0039A2 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039A2 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039A2);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039A2);
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

unsigned instruction_count_from_0x0039A4(unsigned entry_pc) {
    if (entry_pc == 0x0039A4U) return 1U;
    return 0;
}

BlockExit execute_0x0039A4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039A4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039A4U) {
    // guest 0x0039A4 opcode 0x6B00 6B00 0136 bmi.w loc_003ADC
    const auto opcode_0x0039A4 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039A4);
    branch_condition(api, 11U, 0x003ADCU, 14, 0x0136U);
    api.finish_instruction(opcode_0x0039A4);
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

unsigned instruction_count_from_0x0039AA(unsigned entry_pc) {
    if (entry_pc == 0x0039AAU) return 1U;
    return 0;
}

BlockExit execute_0x0039AA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039AAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039AAU) {
    // guest 0x0039AA opcode 0x6500 6500 005A bcs.w loc_003A06
    const auto opcode_0x0039AA = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x0039AA);
    branch_condition(api, 5U, 0x003A06U, 14, 0x005AU);
    api.finish_instruction(opcode_0x0039AA);
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

unsigned instruction_count_from_0x0039AE(unsigned entry_pc) {
    if (entry_pc == 0x0039AEU) return 1U;
    return 0;
}

BlockExit execute_0x0039AE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039AEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039AEU) {
    // guest 0x0039AE opcode 0x5242 5242 addq.w #$1,D2
    const auto opcode_0x0039AE = fetch_checked(api, 0x5242U);
    api.begin_instruction(opcode_0x0039AE);
    addq_w_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x0039AE);
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

unsigned instruction_count_from_0x0039B0(unsigned entry_pc) {
    if (entry_pc == 0x0039B0U) return 1U;
    return 0;
}

BlockExit execute_0x0039B0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039B0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039B0U) {
    // guest 0x0039B0 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039B0 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039B0);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039B0);
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

unsigned instruction_count_from_0x0039B2(unsigned entry_pc) {
    if (entry_pc == 0x0039B2U) return 1U;
    return 0;
}

BlockExit execute_0x0039B2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039B2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039B2U) {
    // guest 0x0039B2 opcode 0x6B00 6B00 0136 bmi.w loc_003AEA
    const auto opcode_0x0039B2 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039B2);
    branch_condition(api, 11U, 0x003AEAU, 14, 0x0136U);
    api.finish_instruction(opcode_0x0039B2);
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

unsigned instruction_count_from_0x0039B8(unsigned entry_pc) {
    if (entry_pc == 0x0039B8U) return 1U;
    return 0;
}

BlockExit execute_0x0039B8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039B8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039B8U) {
    // guest 0x0039B8 opcode 0x6500 6500 004C bcs.w loc_003A06
    const auto opcode_0x0039B8 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x0039B8);
    branch_condition(api, 5U, 0x003A06U, 14, 0x004CU);
    api.finish_instruction(opcode_0x0039B8);
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

unsigned instruction_count_from_0x0039BC(unsigned entry_pc) {
    if (entry_pc == 0x0039BCU) return 1U;
    return 0;
}

BlockExit execute_0x0039BC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039BCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039BCU) {
    // guest 0x0039BC opcode 0x5242 5242 addq.w #$1,D2
    const auto opcode_0x0039BC = fetch_checked(api, 0x5242U);
    api.begin_instruction(opcode_0x0039BC);
    addq_w_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x0039BC);
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

unsigned instruction_count_from_0x0039BE(unsigned entry_pc) {
    if (entry_pc == 0x0039BEU) return 1U;
    return 0;
}

BlockExit execute_0x0039BE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039BEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039BEU) {
    // guest 0x0039BE opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039BE = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039BE);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039BE);
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

unsigned instruction_count_from_0x0039C0(unsigned entry_pc) {
    if (entry_pc == 0x0039C0U) return 1U;
    return 0;
}

BlockExit execute_0x0039C0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039C0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039C0U) {
    // guest 0x0039C0 opcode 0x6B00 6B00 0136 bmi.w loc_003AF8
    const auto opcode_0x0039C0 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039C0);
    branch_condition(api, 11U, 0x003AF8U, 14, 0x0136U);
    api.finish_instruction(opcode_0x0039C0);
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

unsigned instruction_count_from_0x0039C6(unsigned entry_pc) {
    if (entry_pc == 0x0039C6U) return 1U;
    return 0;
}

BlockExit execute_0x0039C6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039C6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039C6U) {
    // guest 0x0039C6 opcode 0x6500 6500 003E bcs.w loc_003A06
    const auto opcode_0x0039C6 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x0039C6);
    branch_condition(api, 5U, 0x003A06U, 14, 0x003EU);
    api.finish_instruction(opcode_0x0039C6);
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

unsigned instruction_count_from_0x0039CA(unsigned entry_pc) {
    if (entry_pc == 0x0039CAU) return 1U;
    return 0;
}

BlockExit execute_0x0039CA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039CAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039CAU) {
    // guest 0x0039CA opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039CA = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039CA);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039CA);
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

unsigned instruction_count_from_0x0039CC(unsigned entry_pc) {
    if (entry_pc == 0x0039CCU) return 1U;
    return 0;
}

BlockExit execute_0x0039CC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039CCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039CCU) {
    // guest 0x0039CC opcode 0x6B00 6B00 0138 bmi.w loc_003B06
    const auto opcode_0x0039CC = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039CC);
    branch_condition(api, 11U, 0x003B06U, 14, 0x0138U);
    api.finish_instruction(opcode_0x0039CC);
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

}
 // namespace oasis::hybrid::generated
