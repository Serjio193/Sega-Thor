// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x060110(unsigned entry_pc) {
    if (entry_pc == 0x060110U) return 1U;
    return 0;
}

BlockExit execute_0x060110(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060110U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060110U) {
    // guest 0x060110 opcode 0x4A39 4A39 00FF 0011 tst.b ($00FF0011).L
    const auto opcode_0x060110 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x060110);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0011U);
    test_absolute_long(api, 0xFF0011U, 1U);
    api.finish_instruction(opcode_0x060110);
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

unsigned instruction_count_from_0x060116(unsigned entry_pc) {
    if (entry_pc == 0x060116U) return 1U;
    return 0;
}

BlockExit execute_0x060116(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060116U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060116U) {
    // guest 0x060116 opcode 0x6700 6700 0052 beq.w loc_06016A
    const auto opcode_0x060116 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060116);
    branch_condition(api, 7U, 0x06016AU, 14, 0x0052U);
    api.finish_instruction(opcode_0x060116);
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

unsigned instruction_count_from_0x06016A(unsigned entry_pc) {
    if (entry_pc == 0x06016AU) return 1U;
    return 0;
}

BlockExit execute_0x06016A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06016AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06016AU) {
    // guest 0x06016A opcode 0x4A39 4A39 00FF 0012 tst.b ($00FF0012).L
    const auto opcode_0x06016A = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x06016A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0012U);
    test_absolute_long(api, 0xFF0012U, 1U);
    api.finish_instruction(opcode_0x06016A);
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

unsigned instruction_count_from_0x060170(unsigned entry_pc) {
    if (entry_pc == 0x060170U) return 1U;
    return 0;
}

BlockExit execute_0x060170(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060170U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060170U) {
    // guest 0x060170 opcode 0x6700 6700 0010 beq.w loc_060182
    const auto opcode_0x060170 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060170);
    branch_condition(api, 7U, 0x060182U, 14, 0x0010U);
    api.finish_instruction(opcode_0x060170);
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

unsigned instruction_count_from_0x060182(unsigned entry_pc) {
    if (entry_pc == 0x060182U) return 1U;
    return 0;
}

BlockExit execute_0x060182(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060182U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060182U) {
    // guest 0x060182 opcode 0x4BF9 4BF9 00FF 001A lea.l ($00FF001A).L,A5
    const auto opcode_0x060182 = fetch_checked(api, 0x4BF9U);
    api.begin_instruction(opcode_0x060182);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x001AU);
    lea_absolute_long(api, 0xFF001AU, 5U);
    api.finish_instruction(opcode_0x060182);
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

unsigned instruction_count_from_0x060188(unsigned entry_pc) {
    if (entry_pc == 0x060188U) return 1U;
    return 0;
}

BlockExit execute_0x060188(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060188U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060188U) {
    // guest 0x060188 opcode 0x4A39 4A39 00FF 0014 tst.b ($00FF0014).L
    const auto opcode_0x060188 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x060188);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0014U);
    test_absolute_long(api, 0xFF0014U, 1U);
    api.finish_instruction(opcode_0x060188);
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

unsigned instruction_count_from_0x06018E(unsigned entry_pc) {
    if (entry_pc == 0x06018EU) return 1U;
    return 0;
}

BlockExit execute_0x06018E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06018EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06018EU) {
    // guest 0x06018E opcode 0x6700 6700 0044 beq.w loc_0601D4
    const auto opcode_0x06018E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06018E);
    branch_condition(api, 7U, 0x0601D4U, 14, 0x0044U);
    api.finish_instruction(opcode_0x06018E);
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

unsigned instruction_count_from_0x0601D4(unsigned entry_pc) {
    if (entry_pc == 0x0601D4U) return 1U;
    return 0;
}

BlockExit execute_0x0601D4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601D4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601D4U) {
    // guest 0x0601D4 opcode 0x49F9 49F9 00FF 077C lea.l ($00FF077C).L,A4
    const auto opcode_0x0601D4 = fetch_checked(api, 0x49F9U);
    api.begin_instruction(opcode_0x0601D4);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x077CU);
    lea_absolute_long(api, 0xFF077CU, 4U);
    api.finish_instruction(opcode_0x0601D4);
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

unsigned instruction_count_from_0x0601DC(unsigned entry_pc) {
    if (entry_pc == 0x0601DCU) return 1U;
    return 0;
}

BlockExit execute_0x0601DC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601DCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601DCU) {
    // guest 0x0601DC opcode 0x4DF9 4DF9 00FF 05B2 lea.l ($00FF05B2).L,A6
    const auto opcode_0x0601DC = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0601DC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x05B2U);
    lea_absolute_long(api, 0xFF05B2U, 6U);
    api.finish_instruction(opcode_0x0601DC);
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

unsigned instruction_count_from_0x0601E6(unsigned entry_pc) {
    if (entry_pc == 0x0601E6U) return 1U;
    return 0;
}

BlockExit execute_0x0601E6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601E6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601E6U) {
    // guest 0x0601E6 opcode 0x7E00 7E00 moveq #0,D7
    const auto opcode_0x0601E6 = fetch_checked(api, 0x7E00U);
    api.begin_instruction(opcode_0x0601E6);
    moveq_data(api, 0, 7U);
    api.finish_instruction(opcode_0x0601E6);
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

unsigned instruction_count_from_0x0601E8(unsigned entry_pc) {
    if (entry_pc == 0x0601E8U) return 1U;
    return 0;
}

BlockExit execute_0x0601E8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601E8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601E8U) {
    // guest 0x0601E8 opcode 0x4DF9 4DF9 00FF 0022 lea.l ($00FF0022).L,A6
    const auto opcode_0x0601E8 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0601E8);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0022U);
    lea_absolute_long(api, 0xFF0022U, 6U);
    api.finish_instruction(opcode_0x0601E8);
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

unsigned instruction_count_from_0x0601F2(unsigned entry_pc) {
    if (entry_pc == 0x0601F2U) return 1U;
    return 0;
}

BlockExit execute_0x0601F2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601F2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601F2U) {
    // guest 0x0601F2 opcode 0x5247 5247 addq.w #$1,D7
    const auto opcode_0x0601F2 = fetch_checked(api, 0x5247U);
    api.begin_instruction(opcode_0x0601F2);
    addq_w_data(api, 1U, 7U);
    api.finish_instruction(opcode_0x0601F2);
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

unsigned instruction_count_from_0x0601F4(unsigned entry_pc) {
    if (entry_pc == 0x0601F4U) return 1U;
    return 0;
}

BlockExit execute_0x0601F4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601F4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601F4U) {
    // guest 0x0601F4 opcode 0x4DF9 4DF9 00FF 00EC lea.l ($00FF00EC).L,A6
    const auto opcode_0x0601F4 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0601F4);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x00ECU);
    lea_absolute_long(api, 0xFF00ECU, 6U);
    api.finish_instruction(opcode_0x0601F4);
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

unsigned instruction_count_from_0x0601FE(unsigned entry_pc) {
    if (entry_pc == 0x0601FEU) return 1U;
    return 0;
}

BlockExit execute_0x0601FE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0601FEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0601FEU) {
    // guest 0x0601FE opcode 0x5247 5247 addq.w #$1,D7
    const auto opcode_0x0601FE = fetch_checked(api, 0x5247U);
    api.begin_instruction(opcode_0x0601FE);
    addq_w_data(api, 1U, 7U);
    api.finish_instruction(opcode_0x0601FE);
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

unsigned instruction_count_from_0x060200(unsigned entry_pc) {
    if (entry_pc == 0x060200U) return 1U;
    return 0;
}

BlockExit execute_0x060200(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060200U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060200U) {
    // guest 0x060200 opcode 0x4DF9 4DF9 00FF 01B6 lea.l ($00FF01B6).L,A6
    const auto opcode_0x060200 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060200);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x01B6U);
    lea_absolute_long(api, 0xFF01B6U, 6U);
    api.finish_instruction(opcode_0x060200);
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

unsigned instruction_count_from_0x06020C(unsigned entry_pc) {
    if (entry_pc == 0x06020CU) return 1U;
    return 0;
}

BlockExit execute_0x06020C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06020CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06020CU) {
    // guest 0x06020C opcode 0x4DF9 4DF9 00FF 0280 lea.l ($00FF0280).L,A6
    const auto opcode_0x06020C = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x06020C);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0280U);
    lea_absolute_long(api, 0xFF0280U, 6U);
    api.finish_instruction(opcode_0x06020C);
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

unsigned instruction_count_from_0x060216(unsigned entry_pc) {
    if (entry_pc == 0x060216U) return 1U;
    return 0;
}

BlockExit execute_0x060216(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060216U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060216U) {
    // guest 0x060216 opcode 0x5247 5247 addq.w #$1,D7
    const auto opcode_0x060216 = fetch_checked(api, 0x5247U);
    api.begin_instruction(opcode_0x060216);
    addq_w_data(api, 1U, 7U);
    api.finish_instruction(opcode_0x060216);
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
