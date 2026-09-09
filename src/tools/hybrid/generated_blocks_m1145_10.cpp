// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0038AE(unsigned entry_pc) {
    if (entry_pc == 0x0038AEU) return 1U;
    return 0;
}

BlockExit execute_0x0038AE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038AEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038AEU) {
    // guest 0x0038AE opcode 0x6600 6600 FF8A bne.w loc_00383A
    const auto opcode_0x0038AE = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0038AE);
    branch_condition(api, 6U, 0x00383AU, 14, 0xFF8AU);
    api.finish_instruction(opcode_0x0038AE);
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

unsigned instruction_count_from_0x0038B4(unsigned entry_pc) {
    if (entry_pc == 0x0038B4U) return 1U;
    return 0;
}

BlockExit execute_0x0038B4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038B4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038B4U) {
    // guest 0x0038B4 opcode 0x6700 6700 000E beq.w loc_0038C4
    const auto opcode_0x0038B4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0038B4);
    branch_condition(api, 7U, 0x0038C4U, 14, 0x000EU);
    api.finish_instruction(opcode_0x0038B4);
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

unsigned instruction_count_from_0x0038C6(unsigned entry_pc) {
    if (entry_pc == 0x0038C6U) return 1U;
    return 0;
}

BlockExit execute_0x0038C6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038C6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038C6U) {
    // guest 0x0038C6 opcode 0x6600 6600 FF64 bne.w loc_00382C
    const auto opcode_0x0038C6 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0038C6);
    branch_condition(api, 6U, 0x00382CU, 14, 0xFF64U);
    api.finish_instruction(opcode_0x0038C6);
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

unsigned instruction_count_from_0x0038DA(unsigned entry_pc) {
    if (entry_pc == 0x0038DAU) return 1U;
    return 0;
}

BlockExit execute_0x0038DA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038DAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038DAU) {
    // guest 0x0038DA opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0038DA = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0038DA);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0038DA);
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

unsigned instruction_count_from_0x0038DC(unsigned entry_pc) {
    if (entry_pc == 0x0038DCU) return 1U;
    return 0;
}

BlockExit execute_0x0038DC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038DCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038DCU) {
    // guest 0x0038DC opcode 0x6B00 6B00 0148 bmi.w loc_003A26
    const auto opcode_0x0038DC = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0038DC);
    branch_condition(api, 11U, 0x003A26U, 14, 0x0148U);
    api.finish_instruction(opcode_0x0038DC);
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

unsigned instruction_count_from_0x0038E2(unsigned entry_pc) {
    if (entry_pc == 0x0038E2U) return 1U;
    return 0;
}

BlockExit execute_0x0038E2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038E2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038E2U) {
    // guest 0x0038E2 opcode 0x6400 6400 00A2 bcc.w loc_003986
    const auto opcode_0x0038E2 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x0038E2);
    branch_condition(api, 4U, 0x003986U, 14, 0x00A2U);
    api.finish_instruction(opcode_0x0038E2);
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

unsigned instruction_count_from_0x0038E6(unsigned entry_pc) {
    if (entry_pc == 0x0038E6U) return 1U;
    return 0;
}

BlockExit execute_0x0038E6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038E6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038E6U) {
    // guest 0x0038E6 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0038E6 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0038E6);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0038E6);
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

unsigned instruction_count_from_0x0038E8(unsigned entry_pc) {
    if (entry_pc == 0x0038E8U) return 1U;
    return 0;
}

BlockExit execute_0x0038E8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038E8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038E8U) {
    // guest 0x0038E8 opcode 0x6B00 6B00 014A bmi.w loc_003A34
    const auto opcode_0x0038E8 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0038E8);
    branch_condition(api, 11U, 0x003A34U, 14, 0x014AU);
    api.finish_instruction(opcode_0x0038E8);
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

unsigned instruction_count_from_0x0038EE(unsigned entry_pc) {
    if (entry_pc == 0x0038EEU) return 1U;
    return 0;
}

BlockExit execute_0x0038EE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038EEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038EEU) {
    // guest 0x0038EE opcode 0x6400 6400 009C bcc.w loc_00398C
    const auto opcode_0x0038EE = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x0038EE);
    branch_condition(api, 4U, 0x00398CU, 14, 0x009CU);
    api.finish_instruction(opcode_0x0038EE);
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

unsigned instruction_count_from_0x0038F2(unsigned entry_pc) {
    if (entry_pc == 0x0038F2U) return 1U;
    return 0;
}

BlockExit execute_0x0038F2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038F2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038F2U) {
    // guest 0x0038F2 opcode 0x4242 4242 clr.w D2
    const auto opcode_0x0038F2 = fetch_checked(api, 0x4242U);
    api.begin_instruction(opcode_0x0038F2);
    clear_w_data_register(api, 2U);
    api.finish_instruction(opcode_0x0038F2);
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

unsigned instruction_count_from_0x0038F4(unsigned entry_pc) {
    if (entry_pc == 0x0038F4U) return 1U;
    return 0;
}

BlockExit execute_0x0038F4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038F4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038F4U) {
    // guest 0x0038F4 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0038F4 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0038F4);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0038F4);
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

unsigned instruction_count_from_0x0038F6(unsigned entry_pc) {
    if (entry_pc == 0x0038F6U) return 1U;
    return 0;
}

BlockExit execute_0x0038F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038F6U) {
    // guest 0x0038F6 opcode 0x6B00 6B00 014A bmi.w loc_003A42
    const auto opcode_0x0038F6 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0038F6);
    branch_condition(api, 11U, 0x003A42U, 14, 0x014AU);
    api.finish_instruction(opcode_0x0038F6);
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

unsigned instruction_count_from_0x0038FE(unsigned entry_pc) {
    if (entry_pc == 0x0038FEU) return 1U;
    return 0;
}

BlockExit execute_0x0038FE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038FEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038FEU) {
    // guest 0x0038FE opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0038FE = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0038FE);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0038FE);
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

unsigned instruction_count_from_0x003900(unsigned entry_pc) {
    if (entry_pc == 0x003900U) return 1U;
    return 0;
}

BlockExit execute_0x003900(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003900U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003900U) {
    // guest 0x003900 opcode 0x6B00 6B00 014E bmi.w loc_003A50
    const auto opcode_0x003900 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003900);
    branch_condition(api, 11U, 0x003A50U, 14, 0x014EU);
    api.finish_instruction(opcode_0x003900);
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

unsigned instruction_count_from_0x003908(unsigned entry_pc) {
    if (entry_pc == 0x003908U) return 1U;
    return 0;
}

BlockExit execute_0x003908(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003908U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003908U) {
    // guest 0x003908 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003908 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003908);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003908);
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

unsigned instruction_count_from_0x00390A(unsigned entry_pc) {
    if (entry_pc == 0x00390AU) return 1U;
    return 0;
}

BlockExit execute_0x00390A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00390AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00390AU) {
    // guest 0x00390A opcode 0x6B00 6B00 0152 bmi.w loc_003A5E
    const auto opcode_0x00390A = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x00390A);
    branch_condition(api, 11U, 0x003A5EU, 14, 0x0152U);
    api.finish_instruction(opcode_0x00390A);
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

unsigned instruction_count_from_0x003912(unsigned entry_pc) {
    if (entry_pc == 0x003912U) return 1U;
    return 0;
}

BlockExit execute_0x003912(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003912U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003912U) {
    // guest 0x003912 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003912 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003912);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003912);
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

unsigned instruction_count_from_0x003914(unsigned entry_pc) {
    if (entry_pc == 0x003914U) return 1U;
    return 0;
}

BlockExit execute_0x003914(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003914U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003914U) {
    // guest 0x003914 opcode 0x6B00 6B00 0156 bmi.w loc_003A6C
    const auto opcode_0x003914 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003914);
    branch_condition(api, 11U, 0x003A6CU, 14, 0x0156U);
    api.finish_instruction(opcode_0x003914);
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
