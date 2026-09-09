// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x00238C(unsigned entry_pc) {
    if (entry_pc == 0x00238CU) return 1U;
    return 0;
}

BlockExit execute_0x00238C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00238CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00238CU) {
    // guest 0x00238C opcode 0x4A39 4A39 00FF 1859 tst.b ($00FF1859).L
    const auto opcode_0x00238C = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x00238C);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1859U);
    test_absolute_long(api, 0xFF1859U, 1U);
    api.finish_instruction(opcode_0x00238C);
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

unsigned instruction_count_from_0x002392(unsigned entry_pc) {
    if (entry_pc == 0x002392U) return 1U;
    return 0;
}

BlockExit execute_0x002392(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002392U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002392U) {
    // guest 0x002392 opcode 0x6600 6600 001A bne.w loc_0023AE
    const auto opcode_0x002392 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x002392);
    branch_condition(api, 6U, 0x0023AEU, 14, 0x001AU);
    api.finish_instruction(opcode_0x002392);
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

unsigned instruction_count_from_0x00239A(unsigned entry_pc) {
    if (entry_pc == 0x00239AU) return 1U;
    return 0;
}

BlockExit execute_0x00239A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00239AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00239AU) {
    // guest 0x00239A opcode 0x6700 6700 002A beq.w loc_0023C6
    const auto opcode_0x00239A = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x00239A);
    branch_condition(api, 7U, 0x0023C6U, 14, 0x002AU);
    api.finish_instruction(opcode_0x00239A);
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

unsigned instruction_count_from_0x0023A4(unsigned entry_pc) {
    if (entry_pc == 0x0023A4U) return 1U;
    return 0;
}

BlockExit execute_0x0023A4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0023A4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0023A4U) {
    // guest 0x0023A4 opcode 0x6500 6500 001A bcs.w loc_0023C0
    const auto opcode_0x0023A4 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x0023A4);
    branch_condition(api, 5U, 0x0023C0U, 14, 0x001AU);
    api.finish_instruction(opcode_0x0023A4);
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

unsigned instruction_count_from_0x0027BC(unsigned entry_pc) {
    if (entry_pc == 0x0027BCU) return 1U;
    return 0;
}

BlockExit execute_0x0027BC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0027BCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0027BCU) {
    // guest 0x0027BC opcode 0x66F6 66F6 bne.s loc_0027B4
    const auto opcode_0x0027BC = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x0027BC);
    branch_condition(api, 6U, 0x0027B4U, -14);
    api.finish_instruction(opcode_0x0027BC);
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

unsigned instruction_count_from_0x002812(unsigned entry_pc) {
    if (entry_pc == 0x002812U) return 1U;
    return 0;
}

BlockExit execute_0x002812(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002812U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002812U) {
    // guest 0x002812 opcode 0x66F6 66F6 bne.s loc_00280A
    const auto opcode_0x002812 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x002812);
    branch_condition(api, 6U, 0x00280AU, -14);
    api.finish_instruction(opcode_0x002812);
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

unsigned instruction_count_from_0x0029A2(unsigned entry_pc) {
    if (entry_pc == 0x0029A2U) return 1U;
    return 0;
}

BlockExit execute_0x0029A2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029A2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029A2U) {
    // guest 0x0029A2 opcode 0x66F6 66F6 bne.s loc_00299A
    const auto opcode_0x0029A2 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x0029A2);
    branch_condition(api, 6U, 0x00299AU, -14);
    api.finish_instruction(opcode_0x0029A2);
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

unsigned instruction_count_from_0x0029A8(unsigned entry_pc) {
    if (entry_pc == 0x0029A8U) return 1U;
    return 0;
}

BlockExit execute_0x0029A8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029A8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029A8U) {
    // guest 0x0029A8 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0029A8 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0029A8);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0029A8);
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

unsigned instruction_count_from_0x0029AA(unsigned entry_pc) {
    if (entry_pc == 0x0029AAU) return 1U;
    return 0;
}

BlockExit execute_0x0029AA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029AAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029AAU) {
    // guest 0x0029AA opcode 0x4E71 4E71 nop
    const auto opcode_0x0029AA = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029AA);

    api.finish_instruction(opcode_0x0029AA);
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

unsigned instruction_count_from_0x0029AC(unsigned entry_pc) {
    if (entry_pc == 0x0029ACU) return 1U;
    return 0;
}

BlockExit execute_0x0029AC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029ACU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029ACU) {
    // guest 0x0029AC opcode 0x4E71 4E71 nop
    const auto opcode_0x0029AC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029AC);

    api.finish_instruction(opcode_0x0029AC);
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

unsigned instruction_count_from_0x0029AE(unsigned entry_pc) {
    if (entry_pc == 0x0029AEU) return 1U;
    return 0;
}

BlockExit execute_0x0029AE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029AEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029AEU) {
    // guest 0x0029AE opcode 0x4E71 4E71 nop
    const auto opcode_0x0029AE = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029AE);

    api.finish_instruction(opcode_0x0029AE);
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

unsigned instruction_count_from_0x0029B0(unsigned entry_pc) {
    if (entry_pc == 0x0029B0U) return 1U;
    return 0;
}

BlockExit execute_0x0029B0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029B0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029B0U) {
    // guest 0x0029B0 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029B0 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029B0);

    api.finish_instruction(opcode_0x0029B0);
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

unsigned instruction_count_from_0x0029B2(unsigned entry_pc) {
    if (entry_pc == 0x0029B2U) return 1U;
    return 0;
}

BlockExit execute_0x0029B2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029B2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029B2U) {
    // guest 0x0029B2 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029B2 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029B2);

    api.finish_instruction(opcode_0x0029B2);
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

unsigned instruction_count_from_0x0029B4(unsigned entry_pc) {
    if (entry_pc == 0x0029B4U) return 1U;
    return 0;
}

BlockExit execute_0x0029B4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029B4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029B4U) {
    // guest 0x0029B4 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029B4 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029B4);

    api.finish_instruction(opcode_0x0029B4);
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

unsigned instruction_count_from_0x0029B6(unsigned entry_pc) {
    if (entry_pc == 0x0029B6U) return 1U;
    return 0;
}

BlockExit execute_0x0029B6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029B6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029B6U) {
    // guest 0x0029B6 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029B6 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029B6);

    api.finish_instruction(opcode_0x0029B6);
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

unsigned instruction_count_from_0x0029BC(unsigned entry_pc) {
    if (entry_pc == 0x0029BCU) return 1U;
    return 0;
}

BlockExit execute_0x0029BC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029BCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029BCU) {
    // guest 0x0029BC opcode 0x0203 0203 000C andi.b #$C,D3
    const auto opcode_0x0029BC = fetch_checked(api, 0x0203U);
    api.begin_instruction(opcode_0x0029BC);
    (void)fetch_checked(api, 0x000CU);
    andi_b_data(api, 12U, 3U);
    api.finish_instruction(opcode_0x0029BC);
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

unsigned instruction_count_from_0x0029C0(unsigned entry_pc) {
    if (entry_pc == 0x0029C0U) return 1U;
    return 0;
}

BlockExit execute_0x0029C0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029C0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029C0U) {
    // guest 0x0029C0 opcode 0x6700 6700 0004 beq.w loc_0029C6
    const auto opcode_0x0029C0 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0029C0);
    branch_condition(api, 7U, 0x0029C6U, 14, 0x0004U);
    api.finish_instruction(opcode_0x0029C0);
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
