// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x00391C(unsigned entry_pc) {
    if (entry_pc == 0x00391CU) return 1U;
    return 0;
}

BlockExit execute_0x00391C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00391CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00391CU) {
    // guest 0x00391C opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x00391C = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x00391C);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x00391C);
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

unsigned instruction_count_from_0x00391E(unsigned entry_pc) {
    if (entry_pc == 0x00391EU) return 1U;
    return 0;
}

BlockExit execute_0x00391E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00391EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00391EU) {
    // guest 0x00391E opcode 0x6B00 6B00 015A bmi.w loc_003A7A
    const auto opcode_0x00391E = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x00391E);
    branch_condition(api, 11U, 0x003A7AU, 14, 0x015AU);
    api.finish_instruction(opcode_0x00391E);
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

unsigned instruction_count_from_0x00392E(unsigned entry_pc) {
    if (entry_pc == 0x00392EU) return 1U;
    return 0;
}

BlockExit execute_0x00392E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00392EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00392EU) {
    // guest 0x00392E opcode 0x6200 6200 0060 bhi.w loc_003990
    const auto opcode_0x00392E = fetch_checked(api, 0x6200U);
    api.begin_instruction(opcode_0x00392E);
    branch_condition(api, 2U, 0x003990U, 14, 0x0060U);
    api.finish_instruction(opcode_0x00392E);
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

unsigned instruction_count_from_0x003932(unsigned entry_pc) {
    if (entry_pc == 0x003932U) return 1U;
    return 0;
}

BlockExit execute_0x003932(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003932U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003932U) {
    // guest 0x003932 opcode 0x6500 6500 00E2 bcs.w loc_003A16
    const auto opcode_0x003932 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x003932);
    branch_condition(api, 5U, 0x003A16U, 14, 0x00E2U);
    api.finish_instruction(opcode_0x003932);
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

unsigned instruction_count_from_0x003936(unsigned entry_pc) {
    if (entry_pc == 0x003936U) return 1U;
    return 0;
}

BlockExit execute_0x003936(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003936U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003936U) {
    // guest 0x003936 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003936 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003936);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003936);
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

unsigned instruction_count_from_0x003938(unsigned entry_pc) {
    if (entry_pc == 0x003938U) return 1U;
    return 0;
}

BlockExit execute_0x003938(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003938U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003938U) {
    // guest 0x003938 opcode 0x6B00 6B00 014E bmi.w loc_003A88
    const auto opcode_0x003938 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003938);
    branch_condition(api, 11U, 0x003A88U, 14, 0x014EU);
    api.finish_instruction(opcode_0x003938);
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

unsigned instruction_count_from_0x003940(unsigned entry_pc) {
    if (entry_pc == 0x003940U) return 1U;
    return 0;
}

BlockExit execute_0x003940(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003940U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003940U) {
    // guest 0x003940 opcode 0x4242 4242 clr.w D2
    const auto opcode_0x003940 = fetch_checked(api, 0x4242U);
    api.begin_instruction(opcode_0x003940);
    clear_w_data_register(api, 2U);
    api.finish_instruction(opcode_0x003940);
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

unsigned instruction_count_from_0x003942(unsigned entry_pc) {
    if (entry_pc == 0x003942U) return 1U;
    return 0;
}

BlockExit execute_0x003942(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003942U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003942U) {
    // guest 0x003942 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003942 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003942);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003942);
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

unsigned instruction_count_from_0x003944(unsigned entry_pc) {
    if (entry_pc == 0x003944U) return 1U;
    return 0;
}

BlockExit execute_0x003944(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003944U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003944U) {
    // guest 0x003944 opcode 0x6B00 6B00 0150 bmi.w loc_003A96
    const auto opcode_0x003944 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003944);
    branch_condition(api, 11U, 0x003A96U, 14, 0x0150U);
    api.finish_instruction(opcode_0x003944);
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

unsigned instruction_count_from_0x00394C(unsigned entry_pc) {
    if (entry_pc == 0x00394CU) return 1U;
    return 0;
}

BlockExit execute_0x00394C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00394CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00394CU) {
    // guest 0x00394C opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x00394C = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x00394C);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x00394C);
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

unsigned instruction_count_from_0x00394E(unsigned entry_pc) {
    if (entry_pc == 0x00394EU) return 1U;
    return 0;
}

BlockExit execute_0x00394E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00394EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00394EU) {
    // guest 0x00394E opcode 0x6B00 6B00 0154 bmi.w loc_003AA4
    const auto opcode_0x00394E = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x00394E);
    branch_condition(api, 11U, 0x003AA4U, 14, 0x0154U);
    api.finish_instruction(opcode_0x00394E);
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

unsigned instruction_count_from_0x003956(unsigned entry_pc) {
    if (entry_pc == 0x003956U) return 1U;
    return 0;
}

BlockExit execute_0x003956(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003956U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003956U) {
    // guest 0x003956 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003956 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003956);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003956);
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

unsigned instruction_count_from_0x003958(unsigned entry_pc) {
    if (entry_pc == 0x003958U) return 1U;
    return 0;
}

BlockExit execute_0x003958(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003958U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003958U) {
    // guest 0x003958 opcode 0x6B00 6B00 0158 bmi.w loc_003AB2
    const auto opcode_0x003958 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003958);
    branch_condition(api, 11U, 0x003AB2U, 14, 0x0158U);
    api.finish_instruction(opcode_0x003958);
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

unsigned instruction_count_from_0x003960(unsigned entry_pc) {
    if (entry_pc == 0x003960U) return 1U;
    return 0;
}

BlockExit execute_0x003960(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003960U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003960U) {
    // guest 0x003960 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x003960 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x003960);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x003960);
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

unsigned instruction_count_from_0x003962(unsigned entry_pc) {
    if (entry_pc == 0x003962U) return 1U;
    return 0;
}

BlockExit execute_0x003962(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003962U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003962U) {
    // guest 0x003962 opcode 0x6B00 6B00 015C bmi.w loc_003AC0
    const auto opcode_0x003962 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x003962);
    branch_condition(api, 11U, 0x003AC0U, 14, 0x015CU);
    api.finish_instruction(opcode_0x003962);
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

unsigned instruction_count_from_0x00396E(unsigned entry_pc) {
    if (entry_pc == 0x00396EU) return 1U;
    return 0;
}

BlockExit execute_0x00396E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00396EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00396EU) {
    // guest 0x00396E opcode 0x6700 6700 0006 beq.w loc_003976
    const auto opcode_0x00396E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x00396E);
    branch_condition(api, 7U, 0x003976U, 14, 0x0006U);
    api.finish_instruction(opcode_0x00396E);
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

unsigned instruction_count_from_0x00397E(unsigned entry_pc) {
    if (entry_pc == 0x00397EU) return 1U;
    return 0;
}

BlockExit execute_0x00397E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00397EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00397EU) {
    // guest 0x00397E opcode 0x51CA 51CA FFFC dbf D2,loc_00397C
    const auto opcode_0x00397E = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x00397E);
    dbcc(api, 1U, 2U, 0x00397CU, 0xFFFCU);
    api.finish_instruction(opcode_0x00397E);
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

unsigned instruction_count_from_0x00398C(unsigned entry_pc) {
    if (entry_pc == 0x00398CU) return 1U;
    return 0;
}

BlockExit execute_0x00398C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00398CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00398CU) {
    // guest 0x00398C opcode 0x4242 4242 clr.w D2
    const auto opcode_0x00398C = fetch_checked(api, 0x4242U);
    api.begin_instruction(opcode_0x00398C);
    clear_w_data_register(api, 2U);
    api.finish_instruction(opcode_0x00398C);
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
