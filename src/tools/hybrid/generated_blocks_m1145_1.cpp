// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x00037E(unsigned entry_pc) {
    if (entry_pc == 0x00037EU) return 1U;
    return 0;
}

BlockExit execute_0x00037E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00037EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00037EU) {
    // guest 0x00037E opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x00037E = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x00037E);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x00037E);
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

unsigned instruction_count_from_0x0003A4(unsigned entry_pc) {
    if (entry_pc == 0x0003A4U) return 1U;
    return 0;
}

BlockExit execute_0x0003A4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003A4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003A4U) {
    // guest 0x0003A4 opcode 0x51C9 51C9 FFDA dbf D1,loc_000380
    const auto opcode_0x0003A4 = fetch_checked(api, 0x51C9U);
    api.begin_instruction(opcode_0x0003A4);
    dbcc(api, 1U, 1U, 0x000380U, 0xFFDAU);
    api.finish_instruction(opcode_0x0003A4);
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

unsigned instruction_count_from_0x0003AE(unsigned entry_pc) {
    if (entry_pc == 0x0003AEU) return 1U;
    return 0;
}

BlockExit execute_0x0003AE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003AEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003AEU) {
    // guest 0x0003AE opcode 0x6700 6700 0016 beq.w loc_0003C6
    const auto opcode_0x0003AE = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0003AE);
    branch_condition(api, 7U, 0x0003C6U, 14, 0x0016U);
    api.finish_instruction(opcode_0x0003AE);
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

unsigned instruction_count_from_0x0003CE(unsigned entry_pc) {
    if (entry_pc == 0x0003CEU) return 1U;
    return 0;
}

BlockExit execute_0x0003CE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003CEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003CEU) {
    // guest 0x0003CE opcode 0x6600 6600 0010 bne.w loc_0003E0
    const auto opcode_0x0003CE = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0003CE);
    branch_condition(api, 6U, 0x0003E0U, 14, 0x0010U);
    api.finish_instruction(opcode_0x0003CE);
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

unsigned instruction_count_from_0x0003E6(unsigned entry_pc) {
    if (entry_pc == 0x0003E6U) return 1U;
    return 0;
}

BlockExit execute_0x0003E6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003E6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003E6U) {
    // guest 0x0003E6 opcode 0x41F9 41F9 00FF 0BFE lea.l ($00FF0BFE).L,A0
    const auto opcode_0x0003E6 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x0003E6);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BFEU);
    lea_absolute_long(api, 0xFF0BFEU, 0U);
    api.finish_instruction(opcode_0x0003E6);
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

unsigned instruction_count_from_0x0003F6(unsigned entry_pc) {
    if (entry_pc == 0x0003F6U) return 1U;
    return 0;
}

BlockExit execute_0x0003F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003F6U) {
    // guest 0x0003F6 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0003F6 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0003F6);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0003F6);
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

unsigned instruction_count_from_0x00041C(unsigned entry_pc) {
    if (entry_pc == 0x00041CU) return 1U;
    return 0;
}

BlockExit execute_0x00041C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00041CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00041CU) {
    // guest 0x00041C opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x00041C = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x00041C);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x00041C);
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

unsigned instruction_count_from_0x000436(unsigned entry_pc) {
    if (entry_pc == 0x000436U) return 1U;
    return 0;
}

BlockExit execute_0x000436(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000436U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000436U) {
    // guest 0x000436 opcode 0x670E 670E beq.s loc_000446
    const auto opcode_0x000436 = fetch_checked(api, 0x670EU);
    api.begin_instruction(opcode_0x000436);
    branch_condition(api, 7U, 0x000446U, -14);
    api.finish_instruction(opcode_0x000436);
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

unsigned instruction_count_from_0x001F8A(unsigned entry_pc) {
    if (entry_pc == 0x001F8AU) return 1U;
    return 0;
}

BlockExit execute_0x001F8A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x001F8AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x001F8AU) {
    // guest 0x001F8A opcode 0x6700 6700 0186 beq.w loc_002112
    const auto opcode_0x001F8A = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x001F8A);
    branch_condition(api, 7U, 0x002112U, 14, 0x0186U);
    api.finish_instruction(opcode_0x001F8A);
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

unsigned instruction_count_from_0x00211A(unsigned entry_pc) {
    if (entry_pc == 0x00211AU) return 1U;
    return 0;
}

BlockExit execute_0x00211A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00211AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00211AU) {
    // guest 0x00211A opcode 0x6700 6700 0054 beq.w loc_002170
    const auto opcode_0x00211A = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x00211A);
    branch_condition(api, 7U, 0x002170U, 14, 0x0054U);
    api.finish_instruction(opcode_0x00211A);
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

unsigned instruction_count_from_0x00212A(unsigned entry_pc) {
    if (entry_pc == 0x00212AU) return 1U;
    return 0;
}

BlockExit execute_0x00212A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00212AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00212AU) {
    // guest 0x00212A opcode 0x6F00 6F00 0004 ble.w loc_002130
    const auto opcode_0x00212A = fetch_checked(api, 0x6F00U);
    api.begin_instruction(opcode_0x00212A);
    branch_condition(api, 15U, 0x002130U, 14, 0x0004U);
    api.finish_instruction(opcode_0x00212A);
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

unsigned instruction_count_from_0x002144(unsigned entry_pc) {
    if (entry_pc == 0x002144U) return 1U;
    return 0;
}

BlockExit execute_0x002144(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002144U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002144U) {
    // guest 0x002144 opcode 0x6700 6700 002A beq.w loc_002170
    const auto opcode_0x002144 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x002144);
    branch_condition(api, 7U, 0x002170U, 14, 0x002AU);
    api.finish_instruction(opcode_0x002144);
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

unsigned instruction_count_from_0x002160(unsigned entry_pc) {
    if (entry_pc == 0x002160U) return 1U;
    return 0;
}

BlockExit execute_0x002160(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002160U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002160U) {
    // guest 0x002160 opcode 0x6700 6700 0008 beq.w loc_00216A
    const auto opcode_0x002160 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x002160);
    branch_condition(api, 7U, 0x00216AU, 14, 0x0008U);
    api.finish_instruction(opcode_0x002160);
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

unsigned instruction_count_from_0x002178(unsigned entry_pc) {
    if (entry_pc == 0x002178U) return 1U;
    return 0;
}

BlockExit execute_0x002178(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002178U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002178U) {
    // guest 0x002178 opcode 0x6600 6600 0072 bne.w loc_0021EC
    const auto opcode_0x002178 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x002178);
    branch_condition(api, 6U, 0x0021ECU, 14, 0x0072U);
    api.finish_instruction(opcode_0x002178);
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

unsigned instruction_count_from_0x00217C(unsigned entry_pc) {
    if (entry_pc == 0x00217CU) return 1U;
    return 0;
}

BlockExit execute_0x00217C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00217CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00217CU) {
    // guest 0x00217C opcode 0x43F9 43F9 00FF 165C lea.l ($00FF165C).L,A1
    const auto opcode_0x00217C = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x00217C);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x165CU);
    lea_absolute_long(api, 0xFF165CU, 1U);
    api.finish_instruction(opcode_0x00217C);
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

unsigned instruction_count_from_0x002196(unsigned entry_pc) {
    if (entry_pc == 0x002196U) return 1U;
    return 0;
}

BlockExit execute_0x002196(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002196U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002196U) {
    // guest 0x002196 opcode 0x6610 6610 bne.s loc_0021A8
    const auto opcode_0x002196 = fetch_checked(api, 0x6610U);
    api.begin_instruction(opcode_0x002196);
    branch_condition(api, 6U, 0x0021A8U, -14);
    api.finish_instruction(opcode_0x002196);
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

unsigned instruction_count_from_0x0021A8(unsigned entry_pc) {
    if (entry_pc == 0x0021A8U) return 1U;
    return 0;
}

BlockExit execute_0x0021A8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021A8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021A8U) {
    // guest 0x0021A8 opcode 0x4A39 4A39 00FF 1997 tst.b ($00FF1997).L
    const auto opcode_0x0021A8 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x0021A8);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1997U);
    test_absolute_long(api, 0xFF1997U, 1U);
    api.finish_instruction(opcode_0x0021A8);
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
