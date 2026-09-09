// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x060218(unsigned entry_pc) {
    if (entry_pc == 0x060218U) return 1U;
    return 0;
}

BlockExit execute_0x060218(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060218U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060218U) {
    // guest 0x060218 opcode 0x4DF9 4DF9 00FF 034A lea.l ($00FF034A).L,A6
    const auto opcode_0x060218 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060218);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x034AU);
    lea_absolute_long(api, 0xFF034AU, 6U);
    api.finish_instruction(opcode_0x060218);
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

unsigned instruction_count_from_0x060228(unsigned entry_pc) {
    if (entry_pc == 0x060228U) return 1U;
    return 0;
}

BlockExit execute_0x060228(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060228U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060228U) {
    // guest 0x060228 opcode 0x7E00 7E00 moveq #0,D7
    const auto opcode_0x060228 = fetch_checked(api, 0x7E00U);
    api.begin_instruction(opcode_0x060228);
    moveq_data(api, 0, 7U);
    api.finish_instruction(opcode_0x060228);
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

unsigned instruction_count_from_0x06022E(unsigned entry_pc) {
    if (entry_pc == 0x06022EU) return 1U;
    return 0;
}

BlockExit execute_0x06022E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06022EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06022EU) {
    // guest 0x06022E opcode 0x4DF9 4DF9 00FF 0414 lea.l ($00FF0414).L,A6
    const auto opcode_0x06022E = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x06022E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0414U);
    lea_absolute_long(api, 0xFF0414U, 6U);
    api.finish_instruction(opcode_0x06022E);
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

unsigned instruction_count_from_0x06023C(unsigned entry_pc) {
    if (entry_pc == 0x06023CU) return 1U;
    return 0;
}

BlockExit execute_0x06023C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06023CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06023CU) {
    // guest 0x06023C opcode 0x4DF9 4DF9 00FF 049E lea.l ($00FF049E).L,A6
    const auto opcode_0x06023C = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x06023C);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x049EU);
    lea_absolute_long(api, 0xFF049EU, 6U);
    api.finish_instruction(opcode_0x06023C);
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

unsigned instruction_count_from_0x06024A(unsigned entry_pc) {
    if (entry_pc == 0x06024AU) return 1U;
    return 0;
}

BlockExit execute_0x06024A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06024AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06024AU) {
    // guest 0x06024A opcode 0x4DF9 4DF9 00FF 0528 lea.l ($00FF0528).L,A6
    const auto opcode_0x06024A = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x06024A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0528U);
    lea_absolute_long(api, 0xFF0528U, 6U);
    api.finish_instruction(opcode_0x06024A);
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

unsigned instruction_count_from_0x06025A(unsigned entry_pc) {
    if (entry_pc == 0x06025AU) return 1U;
    return 0;
}

BlockExit execute_0x06025A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06025AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06025AU) {
    // guest 0x06025A opcode 0x4DF9 4DF9 00FF 0628 lea.l ($00FF0628).L,A6
    const auto opcode_0x06025A = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x06025A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0628U);
    lea_absolute_long(api, 0xFF0628U, 6U);
    api.finish_instruction(opcode_0x06025A);
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

unsigned instruction_count_from_0x060270(unsigned entry_pc) {
    if (entry_pc == 0x060270U) return 1U;
    return 0;
}

BlockExit execute_0x060270(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060270U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060270U) {
    // guest 0x060270 opcode 0x4DF9 4DF9 00FF 06F2 lea.l ($00FF06F2).L,A6
    const auto opcode_0x060270 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060270);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x06F2U);
    lea_absolute_long(api, 0xFF06F2U, 6U);
    api.finish_instruction(opcode_0x060270);
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

unsigned instruction_count_from_0x060296(unsigned entry_pc) {
    if (entry_pc == 0x060296U) return 1U;
    return 0;
}

BlockExit execute_0x060296(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060296U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060296U) {
    // guest 0x060296 opcode 0x6600 6600 FFF6 bne.w loc_06028E
    const auto opcode_0x060296 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x060296);
    branch_condition(api, 6U, 0x06028EU, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x060296);
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

unsigned instruction_count_from_0x06029A(unsigned entry_pc) {
    if (entry_pc == 0x06029AU) return 1U;
    return 0;
}

BlockExit execute_0x06029A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06029AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06029AU) {
    // guest 0x06029A opcode 0x4A39 4A39 00FF 0013 tst.b ($00FF0013).L
    const auto opcode_0x06029A = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x06029A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0013U);
    test_absolute_long(api, 0xFF0013U, 1U);
    api.finish_instruction(opcode_0x06029A);
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

unsigned instruction_count_from_0x0602A0(unsigned entry_pc) {
    if (entry_pc == 0x0602A0U) return 1U;
    return 0;
}

BlockExit execute_0x0602A0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602A0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602A0U) {
    // guest 0x0602A0 opcode 0x6700 6700 0010 beq.w loc_0602B2
    const auto opcode_0x0602A0 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0602A0);
    branch_condition(api, 7U, 0x0602B2U, 14, 0x0010U);
    api.finish_instruction(opcode_0x0602A0);
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

unsigned instruction_count_from_0x0602B8(unsigned entry_pc) {
    if (entry_pc == 0x0602B8U) return 1U;
    return 0;
}

BlockExit execute_0x0602B8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602B8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602B8U) {
    // guest 0x0602B8 opcode 0x6600 6600 0016 bne.w loc_0602D0
    const auto opcode_0x0602B8 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0602B8);
    branch_condition(api, 6U, 0x0602D0U, 14, 0x0016U);
    api.finish_instruction(opcode_0x0602B8);
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

unsigned instruction_count_from_0x0602C2(unsigned entry_pc) {
    if (entry_pc == 0x0602C2U) return 1U;
    return 0;
}

BlockExit execute_0x0602C2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602C2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602C2U) {
    // guest 0x0602C2 opcode 0x6600 6600 000C bne.w loc_0602D0
    const auto opcode_0x0602C2 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0602C2);
    branch_condition(api, 6U, 0x0602D0U, 14, 0x000CU);
    api.finish_instruction(opcode_0x0602C2);
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

unsigned instruction_count_from_0x0602C6(unsigned entry_pc) {
    if (entry_pc == 0x0602C6U) return 1U;
    return 0;
}

BlockExit execute_0x0602C6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602C6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602C6U) {
    // guest 0x0602C6 opcode 0x4DF9 4DF9 00FF 05B2 lea.l ($00FF05B2).L,A6
    const auto opcode_0x0602C6 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0602C6);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x05B2U);
    lea_absolute_long(api, 0xFF05B2U, 6U);
    api.finish_instruction(opcode_0x0602C6);
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

unsigned instruction_count_from_0x0602D8(unsigned entry_pc) {
    if (entry_pc == 0x0602D8U) return 1U;
    return 0;
}

BlockExit execute_0x0602D8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602D8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602D8U) {
    // guest 0x0602D8 opcode 0x6700 6700 0024 beq.w loc_0602FE
    const auto opcode_0x0602D8 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0602D8);
    branch_condition(api, 7U, 0x0602FEU, 14, 0x0024U);
    api.finish_instruction(opcode_0x0602D8);
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

unsigned instruction_count_from_0x0602E4(unsigned entry_pc) {
    if (entry_pc == 0x0602E4U) return 1U;
    return 0;
}

BlockExit execute_0x0602E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602E4U) {
    // guest 0x0602E4 opcode 0x6700 6700 0042 beq.w loc_060328
    const auto opcode_0x0602E4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0602E4);
    branch_condition(api, 7U, 0x060328U, 14, 0x0042U);
    api.finish_instruction(opcode_0x0602E4);
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

unsigned instruction_count_from_0x0602FE(unsigned entry_pc) {
    if (entry_pc == 0x0602FEU) return 1U;
    return 0;
}

BlockExit execute_0x0602FE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0602FEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0602FEU) {
    // guest 0x0602FE opcode 0x41F9 41F9 00FF 077C lea.l ($00FF077C).L,A0
    const auto opcode_0x0602FE = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x0602FE);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x077CU);
    lea_absolute_long(api, 0xFF077CU, 0U);
    api.finish_instruction(opcode_0x0602FE);
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

unsigned instruction_count_from_0x060328(unsigned entry_pc) {
    if (entry_pc == 0x060328U) return 1U;
    return 0;
}

BlockExit execute_0x060328(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060328U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060328U) {
    // guest 0x060328 opcode 0x41F9 41F9 00FF 077C lea.l ($00FF077C).L,A0
    const auto opcode_0x060328 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x060328);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x077CU);
    lea_absolute_long(api, 0xFF077CU, 0U);
    api.finish_instruction(opcode_0x060328);
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
