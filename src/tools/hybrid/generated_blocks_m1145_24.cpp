// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x061406(unsigned entry_pc) {
    if (entry_pc == 0x061406U) return 1U;
    return 0;
}

BlockExit execute_0x061406(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061406U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061406U) {
    // guest 0x061406 opcode 0x6700 6700 0010 beq.w loc_061418
    const auto opcode_0x061406 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061406);
    branch_condition(api, 7U, 0x061418U, 14, 0x0010U);
    api.finish_instruction(opcode_0x061406);
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

unsigned instruction_count_from_0x061422(unsigned entry_pc) {
    if (entry_pc == 0x061422U) return 1U;
    return 0;
}

BlockExit execute_0x061422(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061422U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061422U) {
    // guest 0x061422 opcode 0x6700 6700 0010 beq.w loc_061434
    const auto opcode_0x061422 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061422);
    branch_condition(api, 7U, 0x061434U, 14, 0x0010U);
    api.finish_instruction(opcode_0x061422);
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

unsigned instruction_count_from_0x06148C(unsigned entry_pc) {
    if (entry_pc == 0x06148CU) return 1U;
    return 0;
}

BlockExit execute_0x06148C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06148CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06148CU) {
    // guest 0x06148C opcode 0x6700 6700 001A beq.w loc_0614A8
    const auto opcode_0x06148C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06148C);
    branch_condition(api, 7U, 0x0614A8U, 14, 0x001AU);
    api.finish_instruction(opcode_0x06148C);
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

unsigned instruction_count_from_0x0614B2(unsigned entry_pc) {
    if (entry_pc == 0x0614B2U) return 1U;
    return 0;
}

BlockExit execute_0x0614B2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0614B2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0614B2U) {
    // guest 0x0614B2 opcode 0x6700 6700 00CE beq.w loc_061582
    const auto opcode_0x0614B2 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0614B2);
    branch_condition(api, 7U, 0x061582U, 14, 0x00CEU);
    api.finish_instruction(opcode_0x0614B2);
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

unsigned instruction_count_from_0x061934(unsigned entry_pc) {
    if (entry_pc == 0x061934U) return 1U;
    return 0;
}

BlockExit execute_0x061934(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061934U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061934U) {
    // guest 0x061934 opcode 0x0807 0807 001F btst.l #$1F,D7
    const auto opcode_0x061934 = fetch_checked(api, 0x0807U);
    api.begin_instruction(opcode_0x061934);
    (void)fetch_checked(api, 0x001FU);
    bit_test_immediate_data(api, 31U, 7U);
    api.finish_instruction(opcode_0x061934);
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

unsigned instruction_count_from_0x061942(unsigned entry_pc) {
    if (entry_pc == 0x061942U) return 1U;
    return 0;
}

BlockExit execute_0x061942(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061942U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061942U) {
    // guest 0x061942 opcode 0x6600 6600 0010 bne.w loc_061954
    const auto opcode_0x061942 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061942);
    branch_condition(api, 6U, 0x061954U, 14, 0x0010U);
    api.finish_instruction(opcode_0x061942);
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

unsigned instruction_count_from_0x061950(unsigned entry_pc) {
    if (entry_pc == 0x061950U) return 1U;
    return 0;
}

BlockExit execute_0x061950(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061950U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061950U) {
    // guest 0x061950 opcode 0x6700 6700 0008 beq.w loc_06195A
    const auto opcode_0x061950 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061950);
    branch_condition(api, 7U, 0x06195AU, 14, 0x0008U);
    api.finish_instruction(opcode_0x061950);
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

unsigned instruction_count_from_0x06195E(unsigned entry_pc) {
    if (entry_pc == 0x06195EU) return 1U;
    return 0;
}

BlockExit execute_0x06195E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06195EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06195EU) {
    // guest 0x06195E opcode 0x6700 6700 0004 beq.w loc_061964
    const auto opcode_0x06195E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06195E);
    branch_condition(api, 7U, 0x061964U, 14, 0x0004U);
    api.finish_instruction(opcode_0x06195E);
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

unsigned instruction_count_from_0x06196C(unsigned entry_pc) {
    if (entry_pc == 0x06196CU) return 1U;
    return 0;
}

BlockExit execute_0x06196C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06196CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06196CU) {
    // guest 0x06196C opcode 0x6600 6600 0048 bne.w loc_0619B6
    const auto opcode_0x06196C = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06196C);
    branch_condition(api, 6U, 0x0619B6U, 14, 0x0048U);
    api.finish_instruction(opcode_0x06196C);
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

unsigned instruction_count_from_0x061976(unsigned entry_pc) {
    if (entry_pc == 0x061976U) return 1U;
    return 0;
}

BlockExit execute_0x061976(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061976U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061976U) {
    // guest 0x061976 opcode 0x6600 6600 003E bne.w loc_0619B6
    const auto opcode_0x061976 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061976);
    branch_condition(api, 6U, 0x0619B6U, 14, 0x003EU);
    api.finish_instruction(opcode_0x061976);
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

unsigned instruction_count_from_0x061986(unsigned entry_pc) {
    if (entry_pc == 0x061986U) return 1U;
    return 0;
}

BlockExit execute_0x061986(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061986U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061986U) {
    // guest 0x061986 opcode 0x6600 6600 0016 bne.w loc_06199E
    const auto opcode_0x061986 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061986);
    branch_condition(api, 6U, 0x06199EU, 14, 0x0016U);
    api.finish_instruction(opcode_0x061986);
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

unsigned instruction_count_from_0x061990(unsigned entry_pc) {
    if (entry_pc == 0x061990U) return 1U;
    return 0;
}

BlockExit execute_0x061990(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061990U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061990U) {
    // guest 0x061990 opcode 0x6600 6600 0024 bne.w loc_0619B6
    const auto opcode_0x061990 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061990);
    branch_condition(api, 6U, 0x0619B6U, 14, 0x0024U);
    api.finish_instruction(opcode_0x061990);
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

unsigned instruction_count_from_0x0619BA(unsigned entry_pc) {
    if (entry_pc == 0x0619BAU) return 1U;
    return 0;
}

BlockExit execute_0x0619BA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0619BAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0619BAU) {
    // guest 0x0619BA opcode 0x6600 6600 0106 bne.w loc_061AC2
    const auto opcode_0x0619BA = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0619BA);
    branch_condition(api, 6U, 0x061AC2U, 14, 0x0106U);
    api.finish_instruction(opcode_0x0619BA);
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

unsigned instruction_count_from_0x0619D0(unsigned entry_pc) {
    if (entry_pc == 0x0619D0U) return 1U;
    return 0;
}

BlockExit execute_0x0619D0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0619D0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0619D0U) {
    // guest 0x0619D0 opcode 0x6400 6400 0584 bcc.w loc_061F56
    const auto opcode_0x0619D0 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x0619D0);
    branch_condition(api, 4U, 0x061F56U, 14, 0x0584U);
    api.finish_instruction(opcode_0x0619D0);
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

unsigned instruction_count_from_0x0619D6(unsigned entry_pc) {
    if (entry_pc == 0x0619D6U) return 1U;
    return 0;
}

BlockExit execute_0x0619D6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0619D6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0619D6U) {
    // guest 0x0619D6 opcode 0x6600 6600 000A bne.w loc_0619E2
    const auto opcode_0x0619D6 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0619D6);
    branch_condition(api, 6U, 0x0619E2U, 14, 0x000AU);
    api.finish_instruction(opcode_0x0619D6);
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

unsigned instruction_count_from_0x0619E6(unsigned entry_pc) {
    if (entry_pc == 0x0619E6U) return 1U;
    return 0;
}

BlockExit execute_0x0619E6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0619E6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0619E6U) {
    // guest 0x0619E6 opcode 0x6600 6600 007E bne.w loc_061A66
    const auto opcode_0x0619E6 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0619E6);
    branch_condition(api, 6U, 0x061A66U, 14, 0x007EU);
    api.finish_instruction(opcode_0x0619E6);
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

unsigned instruction_count_from_0x0619F0(unsigned entry_pc) {
    if (entry_pc == 0x0619F0U) return 1U;
    return 0;
}

BlockExit execute_0x0619F0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0619F0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0619F0U) {
    // guest 0x0619F0 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0619F0 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0619F0);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0619F0);
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

unsigned instruction_count_from_0x061A04(unsigned entry_pc) {
    if (entry_pc == 0x061A04U) return 1U;
    return 0;
}

BlockExit execute_0x061A04(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A04U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A04U) {
    // guest 0x061A04 opcode 0x6600 6600 000C bne.w loc_061A12
    const auto opcode_0x061A04 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061A04);
    branch_condition(api, 6U, 0x061A12U, 14, 0x000CU);
    api.finish_instruction(opcode_0x061A04);
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
