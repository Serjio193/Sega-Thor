// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0623B0(unsigned entry_pc) {
    if (entry_pc == 0x0623B0U) return 1U;
    return 0;
}

BlockExit execute_0x0623B0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623B0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623B0U) {
    // guest 0x0623B0 opcode 0x6600 6600 000C bne.w loc_0623BE
    const auto opcode_0x0623B0 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0623B0);
    branch_condition(api, 6U, 0x0623BEU, 14, 0x000CU);
    api.finish_instruction(opcode_0x0623B0);
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

unsigned instruction_count_from_0x0623BA(unsigned entry_pc) {
    if (entry_pc == 0x0623BAU) return 1U;
    return 0;
}

BlockExit execute_0x0623BA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623BAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623BAU) {
    // guest 0x0623BA opcode 0x6600 6600 002A bne.w loc_0623E6
    const auto opcode_0x0623BA = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0623BA);
    branch_condition(api, 6U, 0x0623E6U, 14, 0x002AU);
    api.finish_instruction(opcode_0x0623BA);
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

unsigned instruction_count_from_0x0623C8(unsigned entry_pc) {
    if (entry_pc == 0x0623C8U) return 1U;
    return 0;
}

BlockExit execute_0x0623C8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623C8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623C8U) {
    // guest 0x0623C8 opcode 0x6700 6700 0006 beq.w loc_0623D0
    const auto opcode_0x0623C8 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0623C8);
    branch_condition(api, 7U, 0x0623D0U, 14, 0x0006U);
    api.finish_instruction(opcode_0x0623C8);
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

unsigned instruction_count_from_0x0623D4(unsigned entry_pc) {
    if (entry_pc == 0x0623D4U) return 1U;
    return 0;
}

BlockExit execute_0x0623D4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623D4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623D4U) {
    // guest 0x0623D4 opcode 0x6700 6700 0006 beq.w loc_0623DC
    const auto opcode_0x0623D4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0623D4);
    branch_condition(api, 7U, 0x0623DCU, 14, 0x0006U);
    api.finish_instruction(opcode_0x0623D4);
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

unsigned instruction_count_from_0x0623E2(unsigned entry_pc) {
    if (entry_pc == 0x0623E2U) return 1U;
    return 0;
}

BlockExit execute_0x0623E2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623E2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623E2U) {
    // guest 0x0623E2 opcode 0x6700 6700 0004 beq.w loc_0623E8
    const auto opcode_0x0623E2 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0623E2);
    branch_condition(api, 7U, 0x0623E8U, 14, 0x0004U);
    api.finish_instruction(opcode_0x0623E2);
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

unsigned instruction_count_from_0x0623EC(unsigned entry_pc) {
    if (entry_pc == 0x0623ECU) return 1U;
    return 0;
}

BlockExit execute_0x0623EC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623ECU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623ECU) {
    // guest 0x0623EC opcode 0x6600 6600 0052 bne.w loc_062440
    const auto opcode_0x0623EC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0623EC);
    branch_condition(api, 6U, 0x062440U, 14, 0x0052U);
    api.finish_instruction(opcode_0x0623EC);
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

unsigned instruction_count_from_0x0623F6(unsigned entry_pc) {
    if (entry_pc == 0x0623F6U) return 1U;
    return 0;
}

BlockExit execute_0x0623F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623F6U) {
    // guest 0x0623F6 opcode 0x6600 6600 0048 bne.w loc_062440
    const auto opcode_0x0623F6 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0623F6);
    branch_condition(api, 6U, 0x062440U, 14, 0x0048U);
    api.finish_instruction(opcode_0x0623F6);
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

unsigned instruction_count_from_0x062406(unsigned entry_pc) {
    if (entry_pc == 0x062406U) return 1U;
    return 0;
}

BlockExit execute_0x062406(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062406U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062406U) {
    // guest 0x062406 opcode 0x6600 6600 002E bne.w loc_062436
    const auto opcode_0x062406 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062406);
    branch_condition(api, 6U, 0x062436U, 14, 0x002EU);
    api.finish_instruction(opcode_0x062406);
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

unsigned instruction_count_from_0x062410(unsigned entry_pc) {
    if (entry_pc == 0x062410U) return 1U;
    return 0;
}

BlockExit execute_0x062410(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062410U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062410U) {
    // guest 0x062410 opcode 0x6600 6600 000C bne.w loc_06241E
    const auto opcode_0x062410 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062410);
    branch_condition(api, 6U, 0x06241EU, 14, 0x000CU);
    api.finish_instruction(opcode_0x062410);
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

unsigned instruction_count_from_0x062444(unsigned entry_pc) {
    if (entry_pc == 0x062444U) return 1U;
    return 0;
}

BlockExit execute_0x062444(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062444U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062444U) {
    // guest 0x062444 opcode 0x6600 6600 0106 bne.w loc_06254C
    const auto opcode_0x062444 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062444);
    branch_condition(api, 6U, 0x06254CU, 14, 0x0106U);
    api.finish_instruction(opcode_0x062444);
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

unsigned instruction_count_from_0x06245A(unsigned entry_pc) {
    if (entry_pc == 0x06245AU) return 1U;
    return 0;
}

BlockExit execute_0x06245A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06245AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06245AU) {
    // guest 0x06245A opcode 0x6400 6400 0412 bcc.w loc_06286E
    const auto opcode_0x06245A = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x06245A);
    branch_condition(api, 4U, 0x06286EU, 14, 0x0412U);
    api.finish_instruction(opcode_0x06245A);
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

unsigned instruction_count_from_0x062460(unsigned entry_pc) {
    if (entry_pc == 0x062460U) return 1U;
    return 0;
}

BlockExit execute_0x062460(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062460U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062460U) {
    // guest 0x062460 opcode 0x6600 6600 000A bne.w loc_06246C
    const auto opcode_0x062460 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062460);
    branch_condition(api, 6U, 0x06246CU, 14, 0x000AU);
    api.finish_instruction(opcode_0x062460);
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

unsigned instruction_count_from_0x062470(unsigned entry_pc) {
    if (entry_pc == 0x062470U) return 1U;
    return 0;
}

BlockExit execute_0x062470(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062470U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062470U) {
    // guest 0x062470 opcode 0x6600 6600 0096 bne.w loc_062508
    const auto opcode_0x062470 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062470);
    branch_condition(api, 6U, 0x062508U, 14, 0x0096U);
    api.finish_instruction(opcode_0x062470);
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

unsigned instruction_count_from_0x062492(unsigned entry_pc) {
    if (entry_pc == 0x062492U) return 1U;
    return 0;
}

BlockExit execute_0x062492(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062492U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062492U) {
    // guest 0x062492 opcode 0x6600 6600 000C bne.w loc_0624A0
    const auto opcode_0x062492 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062492);
    branch_condition(api, 6U, 0x0624A0U, 14, 0x000CU);
    api.finish_instruction(opcode_0x062492);
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

unsigned instruction_count_from_0x0624AA(unsigned entry_pc) {
    if (entry_pc == 0x0624AAU) return 1U;
    return 0;
}

BlockExit execute_0x0624AA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0624AAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0624AAU) {
    // guest 0x0624AA opcode 0x6600 6600 0024 bne.w loc_0624D0
    const auto opcode_0x0624AA = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0624AA);
    branch_condition(api, 6U, 0x0624D0U, 14, 0x0024U);
    api.finish_instruction(opcode_0x0624AA);
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

unsigned instruction_count_from_0x0624B4(unsigned entry_pc) {
    if (entry_pc == 0x0624B4U) return 1U;
    return 0;
}

BlockExit execute_0x0624B4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0624B4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0624B4U) {
    // guest 0x0624B4 opcode 0x6700 6700 001A beq.w loc_0624D0
    const auto opcode_0x0624B4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0624B4);
    branch_condition(api, 7U, 0x0624D0U, 14, 0x001AU);
    api.finish_instruction(opcode_0x0624B4);
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

unsigned instruction_count_from_0x0624F6(unsigned entry_pc) {
    if (entry_pc == 0x0624F6U) return 1U;
    return 0;
}

BlockExit execute_0x0624F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0624F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0624F6U) {
    // guest 0x0624F6 opcode 0x6200 6200 0008 bhi.w loc_062500
    const auto opcode_0x0624F6 = fetch_checked(api, 0x6200U);
    api.begin_instruction(opcode_0x0624F6);
    branch_condition(api, 2U, 0x062500U, 14, 0x0008U);
    api.finish_instruction(opcode_0x0624F6);
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

unsigned instruction_count_from_0x06250E(unsigned entry_pc) {
    if (entry_pc == 0x06250EU) return 1U;
    return 0;
}

BlockExit execute_0x06250E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06250EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06250EU) {
    // guest 0x06250E opcode 0x6600 6600 0026 bne.w loc_062536
    const auto opcode_0x06250E = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06250E);
    branch_condition(api, 6U, 0x062536U, 14, 0x0026U);
    api.finish_instruction(opcode_0x06250E);
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
