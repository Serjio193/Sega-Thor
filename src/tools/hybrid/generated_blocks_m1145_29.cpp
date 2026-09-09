// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x062518(unsigned entry_pc) {
    if (entry_pc == 0x062518U) return 1U;
    return 0;
}

BlockExit execute_0x062518(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062518U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062518U) {
    // guest 0x062518 opcode 0x6600 6600 000C bne.w loc_062526
    const auto opcode_0x062518 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062518);
    branch_condition(api, 6U, 0x062526U, 14, 0x000CU);
    api.finish_instruction(opcode_0x062518);
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

unsigned instruction_count_from_0x062522(unsigned entry_pc) {
    if (entry_pc == 0x062522U) return 1U;
    return 0;
}

BlockExit execute_0x062522(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062522U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062522U) {
    // guest 0x062522 opcode 0x6600 6600 000C bne.w loc_062530
    const auto opcode_0x062522 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062522);
    branch_condition(api, 6U, 0x062530U, 14, 0x000CU);
    api.finish_instruction(opcode_0x062522);
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

unsigned instruction_count_from_0x062552(unsigned entry_pc) {
    if (entry_pc == 0x062552U) return 1U;
    return 0;
}

BlockExit execute_0x062552(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062552U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062552U) {
    // guest 0x062552 opcode 0x6700 6700 0008 beq.w loc_06255C
    const auto opcode_0x062552 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062552);
    branch_condition(api, 7U, 0x06255CU, 14, 0x0008U);
    api.finish_instruction(opcode_0x062552);
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

unsigned instruction_count_from_0x062562(unsigned entry_pc) {
    if (entry_pc == 0x062562U) return 1U;
    return 0;
}

BlockExit execute_0x062562(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062562U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062562U) {
    // guest 0x062562 opcode 0x6700 6700 0012 beq.w loc_062576
    const auto opcode_0x062562 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062562);
    branch_condition(api, 7U, 0x062576U, 14, 0x0012U);
    api.finish_instruction(opcode_0x062562);
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

unsigned instruction_count_from_0x062580(unsigned entry_pc) {
    if (entry_pc == 0x062580U) return 1U;
    return 0;
}

BlockExit execute_0x062580(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062580U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062580U) {
    // guest 0x062580 opcode 0x6700 6700 0036 beq.w loc_0625B8
    const auto opcode_0x062580 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062580);
    branch_condition(api, 7U, 0x0625B8U, 14, 0x0036U);
    api.finish_instruction(opcode_0x062580);
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

unsigned instruction_count_from_0x0625BE(unsigned entry_pc) {
    if (entry_pc == 0x0625BEU) return 1U;
    return 0;
}

BlockExit execute_0x0625BE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0625BEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0625BEU) {
    // guest 0x0625BE opcode 0x6700 6700 0064 beq.w loc_062624
    const auto opcode_0x0625BE = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0625BE);
    branch_condition(api, 7U, 0x062624U, 14, 0x0064U);
    api.finish_instruction(opcode_0x0625BE);
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

unsigned instruction_count_from_0x06262A(unsigned entry_pc) {
    if (entry_pc == 0x06262AU) return 1U;
    return 0;
}

BlockExit execute_0x06262A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06262AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06262AU) {
    // guest 0x06262A opcode 0x6700 6700 0008 beq.w loc_062634
    const auto opcode_0x06262A = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06262A);
    branch_condition(api, 7U, 0x062634U, 14, 0x0008U);
    api.finish_instruction(opcode_0x06262A);
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

unsigned instruction_count_from_0x06264C(unsigned entry_pc) {
    if (entry_pc == 0x06264CU) return 1U;
    return 0;
}

BlockExit execute_0x06264C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06264CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06264CU) {
    // guest 0x06264C opcode 0x6C00 6C00 0006 bge.w loc_062654
    const auto opcode_0x06264C = fetch_checked(api, 0x6C00U);
    api.begin_instruction(opcode_0x06264C);
    branch_condition(api, 12U, 0x062654U, 14, 0x0006U);
    api.finish_instruction(opcode_0x06264C);
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

unsigned instruction_count_from_0x062658(unsigned entry_pc) {
    if (entry_pc == 0x062658U) return 1U;
    return 0;
}

BlockExit execute_0x062658(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062658U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062658U) {
    // guest 0x062658 opcode 0x6F00 6F00 0006 ble.w loc_062660
    const auto opcode_0x062658 = fetch_checked(api, 0x6F00U);
    api.begin_instruction(opcode_0x062658);
    branch_condition(api, 15U, 0x062660U, 14, 0x0006U);
    api.finish_instruction(opcode_0x062658);
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

unsigned instruction_count_from_0x062664(unsigned entry_pc) {
    if (entry_pc == 0x062664U) return 1U;
    return 0;
}

BlockExit execute_0x062664(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062664U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062664U) {
    // guest 0x062664 opcode 0x6700 6700 002C beq.w loc_062692
    const auto opcode_0x062664 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062664);
    branch_condition(api, 7U, 0x062692U, 14, 0x002CU);
    api.finish_instruction(opcode_0x062664);
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

unsigned instruction_count_from_0x062670(unsigned entry_pc) {
    if (entry_pc == 0x062670U) return 1U;
    return 0;
}

BlockExit execute_0x062670(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062670U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062670U) {
    // guest 0x062670 opcode 0x6700 6700 0022 beq.w loc_062694
    const auto opcode_0x062670 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062670);
    branch_condition(api, 7U, 0x062694U, 14, 0x0022U);
    api.finish_instruction(opcode_0x062670);
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

unsigned instruction_count_from_0x06267A(unsigned entry_pc) {
    if (entry_pc == 0x06267AU) return 1U;
    return 0;
}

BlockExit execute_0x06267A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06267AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06267AU) {
    // guest 0x06267A opcode 0x6600 6600 0016 bne.w loc_062692
    const auto opcode_0x06267A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06267A);
    branch_condition(api, 6U, 0x062692U, 14, 0x0016U);
    api.finish_instruction(opcode_0x06267A);
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

unsigned instruction_count_from_0x0626C0(unsigned entry_pc) {
    if (entry_pc == 0x0626C0U) return 1U;
    return 0;
}

BlockExit execute_0x0626C0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0626C0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0626C0U) {
    // guest 0x0626C0 opcode 0x6600 6600 000C bne.w loc_0626CE
    const auto opcode_0x0626C0 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0626C0);
    branch_condition(api, 6U, 0x0626CEU, 14, 0x000CU);
    api.finish_instruction(opcode_0x0626C0);
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

unsigned instruction_count_from_0x0626D2(unsigned entry_pc) {
    if (entry_pc == 0x0626D2U) return 1U;
    return 0;
}

BlockExit execute_0x0626D2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0626D2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0626D2U) {
    // guest 0x0626D2 opcode 0x6700 6700 0004 beq.w loc_0626D8
    const auto opcode_0x0626D2 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0626D2);
    branch_condition(api, 7U, 0x0626D8U, 14, 0x0004U);
    api.finish_instruction(opcode_0x0626D2);
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

unsigned instruction_count_from_0x062726(unsigned entry_pc) {
    if (entry_pc == 0x062726U) return 1U;
    return 0;
}

BlockExit execute_0x062726(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062726U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062726U) {
    // guest 0x062726 opcode 0x6700 6700 0008 beq.w loc_062730
    const auto opcode_0x062726 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062726);
    branch_condition(api, 7U, 0x062730U, 14, 0x0008U);
    api.finish_instruction(opcode_0x062726);
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

unsigned instruction_count_from_0x062740(unsigned entry_pc) {
    if (entry_pc == 0x062740U) return 1U;
    return 0;
}

BlockExit execute_0x062740(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062740U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062740U) {
    // guest 0x062740 opcode 0x6600 6600 003C bne.w loc_06277E
    const auto opcode_0x062740 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062740);
    branch_condition(api, 6U, 0x06277EU, 14, 0x003CU);
    api.finish_instruction(opcode_0x062740);
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

unsigned instruction_count_from_0x06274A(unsigned entry_pc) {
    if (entry_pc == 0x06274AU) return 1U;
    return 0;
}

BlockExit execute_0x06274A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06274AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06274AU) {
    // guest 0x06274A opcode 0x6600 6600 0032 bne.w loc_06277E
    const auto opcode_0x06274A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06274A);
    branch_condition(api, 6U, 0x06277EU, 14, 0x0032U);
    api.finish_instruction(opcode_0x06274A);
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

unsigned instruction_count_from_0x062758(unsigned entry_pc) {
    if (entry_pc == 0x062758U) return 1U;
    return 0;
}

BlockExit execute_0x062758(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062758U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062758U) {
    // guest 0x062758 opcode 0x6700 6700 000A beq.w loc_062764
    const auto opcode_0x062758 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062758);
    branch_condition(api, 7U, 0x062764U, 14, 0x000AU);
    api.finish_instruction(opcode_0x062758);
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
