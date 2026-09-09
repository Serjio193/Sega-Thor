// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0031B2(unsigned entry_pc) {
    if (entry_pc == 0x0031B2U) return 1U;
    return 0;
}

BlockExit execute_0x0031B2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031B2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031B2U) {
    // guest 0x0031B2 opcode 0x6A46 6A46 bpl.s loc_0031FA
    const auto opcode_0x0031B2 = fetch_checked(api, 0x6A46U);
    api.begin_instruction(opcode_0x0031B2);
    branch_condition(api, 10U, 0x0031FAU, -14);
    api.finish_instruction(opcode_0x0031B2);
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

unsigned instruction_count_from_0x0031C2(unsigned entry_pc) {
    if (entry_pc == 0x0031C2U) return 1U;
    return 0;
}

BlockExit execute_0x0031C2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031C2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031C2U) {
    // guest 0x0031C2 opcode 0x6B36 6B36 bmi.s loc_0031FA
    const auto opcode_0x0031C2 = fetch_checked(api, 0x6B36U);
    api.begin_instruction(opcode_0x0031C2);
    branch_condition(api, 11U, 0x0031FAU, -14);
    api.finish_instruction(opcode_0x0031C2);
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

unsigned instruction_count_from_0x0031CA(unsigned entry_pc) {
    if (entry_pc == 0x0031CAU) return 1U;
    return 0;
}

BlockExit execute_0x0031CA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031CAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031CAU) {
    // guest 0x0031CA opcode 0x6A14 6A14 bpl.s loc_0031E0
    const auto opcode_0x0031CA = fetch_checked(api, 0x6A14U);
    api.begin_instruction(opcode_0x0031CA);
    branch_condition(api, 10U, 0x0031E0U, -14);
    api.finish_instruction(opcode_0x0031CA);
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

unsigned instruction_count_from_0x0031CC(unsigned entry_pc) {
    if (entry_pc == 0x0031CCU) return 1U;
    return 0;
}

BlockExit execute_0x0031CC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031CCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031CCU) {
    // guest 0x0031CC opcode 0x4A39 4A39 00FF 0BFD tst.b ($00FF0BFD).L
    const auto opcode_0x0031CC = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x0031CC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BFDU);
    test_absolute_long(api, 0xFF0BFDU, 1U);
    api.finish_instruction(opcode_0x0031CC);
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

unsigned instruction_count_from_0x0031D2(unsigned entry_pc) {
    if (entry_pc == 0x0031D2U) return 1U;
    return 0;
}

BlockExit execute_0x0031D2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031D2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031D2U) {
    // guest 0x0031D2 opcode 0x670C 670C beq.s loc_0031E0
    const auto opcode_0x0031D2 = fetch_checked(api, 0x670CU);
    api.begin_instruction(opcode_0x0031D2);
    branch_condition(api, 7U, 0x0031E0U, -14);
    api.finish_instruction(opcode_0x0031D2);
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

unsigned instruction_count_from_0x0031E4(unsigned entry_pc) {
    if (entry_pc == 0x0031E4U) return 1U;
    return 0;
}

BlockExit execute_0x0031E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031E4U) {
    // guest 0x0031E4 opcode 0x43F9 43F9 00FF 1350 lea.l ($00FF1350).L,A1
    const auto opcode_0x0031E4 = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x0031E4);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1350U);
    lea_absolute_long(api, 0xFF1350U, 1U);
    api.finish_instruction(opcode_0x0031E4);
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

unsigned instruction_count_from_0x0031EE(unsigned entry_pc) {
    if (entry_pc == 0x0031EEU) return 1U;
    return 0;
}

BlockExit execute_0x0031EE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0031EEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0031EEU) {
    // guest 0x0031EE opcode 0x51C8 51C8 FFFC dbf D0,loc_0031EC
    const auto opcode_0x0031EE = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x0031EE);
    dbcc(api, 1U, 0U, 0x0031ECU, 0xFFFCU);
    api.finish_instruction(opcode_0x0031EE);
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

unsigned instruction_count_from_0x003248(unsigned entry_pc) {
    if (entry_pc == 0x003248U) return 1U;
    return 0;
}

BlockExit execute_0x003248(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003248U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003248U) {
    // guest 0x003248 opcode 0x0839 0839 0002 00FF 164D btst.b #$2,($00FF164D).L
    const auto opcode_0x003248 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x003248);
    (void)fetch_checked(api, 0x0002U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    bit_test_immediate_absolute_long(api, 2U, 0xFF164DU);
    api.finish_instruction(opcode_0x003248);
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

unsigned instruction_count_from_0x003260(unsigned entry_pc) {
    if (entry_pc == 0x003260U) return 1U;
    return 0;
}

BlockExit execute_0x003260(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003260U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003260U) {
    // guest 0x003260 opcode 0x4DF9 4DF9 0000 31FC lea.l ($000031FC).L,A6
    const auto opcode_0x003260 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x003260);
    (void)fetch_checked(api, 0x0000U);
    (void)fetch_checked(api, 0x31FCU);
    lea_absolute_long(api, 0x0031FCU, 6U);
    api.finish_instruction(opcode_0x003260);
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

unsigned instruction_count_from_0x00326A(unsigned entry_pc) {
    if (entry_pc == 0x00326AU) return 1U;
    return 0;
}

BlockExit execute_0x00326A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00326AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00326AU) {
    // guest 0x00326A opcode 0x41F9 41F9 0015 0000 lea.l ($00150000).L,A0
    const auto opcode_0x00326A = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x00326A);
    (void)fetch_checked(api, 0x0015U);
    (void)fetch_checked(api, 0x0000U);
    lea_absolute_long(api, 0x150000U, 0U);
    api.finish_instruction(opcode_0x00326A);
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

unsigned instruction_count_from_0x003270(unsigned entry_pc) {
    if (entry_pc == 0x003270U) return 1U;
    return 0;
}

BlockExit execute_0x003270(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003270U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003270U) {
    // guest 0x003270 opcode 0x43F9 43F9 00FF 2FA8 lea.l ($00FF2FA8).L,A1
    const auto opcode_0x003270 = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x003270);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x2FA8U);
    lea_absolute_long(api, 0xFF2FA8U, 1U);
    api.finish_instruction(opcode_0x003270);
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

unsigned instruction_count_from_0x003276(unsigned entry_pc) {
    if (entry_pc == 0x003276U) return 1U;
    return 0;
}

BlockExit execute_0x003276(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003276U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003276U) {
    // guest 0x003276 opcode 0x2449 2449 movea.l A1,A2
    const auto opcode_0x003276 = fetch_checked(api, 0x2449U);
    api.begin_instruction(opcode_0x003276);
    movea_l_address_to_address(api, 1U, 2U);
    api.finish_instruction(opcode_0x003276);
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

unsigned instruction_count_from_0x0032DC(unsigned entry_pc) {
    if (entry_pc == 0x0032DCU) return 1U;
    return 0;
}

BlockExit execute_0x0032DC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0032DCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0032DCU) {
    // guest 0x0032DC opcode 0x0839 0839 0001 00FF 164D btst.b #$1,($00FF164D).L
    const auto opcode_0x0032DC = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x0032DC);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164DU);
    api.finish_instruction(opcode_0x0032DC);
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

unsigned instruction_count_from_0x0032E4(unsigned entry_pc) {
    if (entry_pc == 0x0032E4U) return 1U;
    return 0;
}

BlockExit execute_0x0032E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0032E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0032E4U) {
    // guest 0x0032E4 opcode 0x66F6 66F6 bne.s loc_0032DC
    const auto opcode_0x0032E4 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x0032E4);
    branch_condition(api, 6U, 0x0032DCU, -14);
    api.finish_instruction(opcode_0x0032E4);
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

unsigned instruction_count_from_0x00376A(unsigned entry_pc) {
    if (entry_pc == 0x00376AU) return 1U;
    return 0;
}

BlockExit execute_0x00376A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00376AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00376AU) {
    // guest 0x00376A opcode 0x41F9 41F9 0015 457A lea.l ($0015457A).L,A0
    const auto opcode_0x00376A = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x00376A);
    (void)fetch_checked(api, 0x0015U);
    (void)fetch_checked(api, 0x457AU);
    lea_absolute_long(api, 0x15457AU, 0U);
    api.finish_instruction(opcode_0x00376A);
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

unsigned instruction_count_from_0x003770(unsigned entry_pc) {
    if (entry_pc == 0x003770U) return 1U;
    return 0;
}

BlockExit execute_0x003770(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003770U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003770U) {
    // guest 0x003770 opcode 0x43F9 43F9 00FF 2FA8 lea.l ($00FF2FA8).L,A1
    const auto opcode_0x003770 = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x003770);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x2FA8U);
    lea_absolute_long(api, 0xFF2FA8U, 1U);
    api.finish_instruction(opcode_0x003770);
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

unsigned instruction_count_from_0x00377E(unsigned entry_pc) {
    if (entry_pc == 0x00377EU) return 1U;
    return 0;
}

BlockExit execute_0x00377E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00377EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00377EU) {
    // guest 0x00377E opcode 0x41F9 41F9 0015 45C8 lea.l ($001545C8).L,A0
    const auto opcode_0x00377E = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x00377E);
    (void)fetch_checked(api, 0x0015U);
    (void)fetch_checked(api, 0x45C8U);
    lea_absolute_long(api, 0x1545C8U, 0U);
    api.finish_instruction(opcode_0x00377E);
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
