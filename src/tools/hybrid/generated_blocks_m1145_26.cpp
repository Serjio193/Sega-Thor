// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x061C5A(unsigned entry_pc) {
    if (entry_pc == 0x061C5AU) return 1U;
    return 0;
}

BlockExit execute_0x061C5A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061C5AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061C5AU) {
    // guest 0x061C5A opcode 0x0607 0607 00A4 addi.b #$A4,D7
    const auto opcode_0x061C5A = fetch_checked(api, 0x0607U);
    api.begin_instruction(opcode_0x061C5A);
    (void)fetch_checked(api, 0x00A4U);
    addi_b_data(api, 164U, 7U);
    api.finish_instruction(opcode_0x061C5A);
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

unsigned instruction_count_from_0x061C70(unsigned entry_pc) {
    if (entry_pc == 0x061C70U) return 1U;
    return 0;
}

BlockExit execute_0x061C70(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061C70U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061C70U) {
    // guest 0x061C70 opcode 0x0407 0407 00A0 subi.b #$A0,D7
    const auto opcode_0x061C70 = fetch_checked(api, 0x0407U);
    api.begin_instruction(opcode_0x061C70);
    (void)fetch_checked(api, 0x00A0U);
    subi_b_data(api, 160U, 7U);
    api.finish_instruction(opcode_0x061C70);
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

unsigned instruction_count_from_0x061C86(unsigned entry_pc) {
    if (entry_pc == 0x061C86U) return 1U;
    return 0;
}

BlockExit execute_0x061C86(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061C86U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061C86U) {
    // guest 0x061C86 opcode 0x6700 6700 0040 beq.w loc_061CC8
    const auto opcode_0x061C86 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061C86);
    branch_condition(api, 7U, 0x061CC8U, 14, 0x0040U);
    api.finish_instruction(opcode_0x061C86);
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

unsigned instruction_count_from_0x061C98(unsigned entry_pc) {
    if (entry_pc == 0x061C98U) return 1U;
    return 0;
}

BlockExit execute_0x061C98(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061C98U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061C98U) {
    // guest 0x061C98 opcode 0x6600 6600 002E bne.w loc_061CC8
    const auto opcode_0x061C98 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061C98);
    branch_condition(api, 6U, 0x061CC8U, 14, 0x002EU);
    api.finish_instruction(opcode_0x061C98);
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

unsigned instruction_count_from_0x061CA2(unsigned entry_pc) {
    if (entry_pc == 0x061CA2U) return 1U;
    return 0;
}

BlockExit execute_0x061CA2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061CA2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061CA2U) {
    // guest 0x061CA2 opcode 0x6700 6700 0014 beq.w loc_061CB8
    const auto opcode_0x061CA2 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061CA2);
    branch_condition(api, 7U, 0x061CB8U, 14, 0x0014U);
    api.finish_instruction(opcode_0x061CA2);
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

unsigned instruction_count_from_0x061CBE(unsigned entry_pc) {
    if (entry_pc == 0x061CBEU) return 1U;
    return 0;
}

BlockExit execute_0x061CBE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061CBEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061CBEU) {
    // guest 0x061CBE opcode 0x6600 6600 0008 bne.w loc_061CC8
    const auto opcode_0x061CBE = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061CBE);
    branch_condition(api, 6U, 0x061CC8U, 14, 0x0008U);
    api.finish_instruction(opcode_0x061CBE);
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

unsigned instruction_count_from_0x061CE4(unsigned entry_pc) {
    if (entry_pc == 0x061CE4U) return 1U;
    return 0;
}

BlockExit execute_0x061CE4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061CE4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061CE4U) {
    // guest 0x061CE4 opcode 0x51C8 51C8 FFF4 dbf D0,loc_061CDA
    const auto opcode_0x061CE4 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x061CE4);
    dbcc(api, 1U, 0U, 0x061CDAU, 0xFFF4U);
    api.finish_instruction(opcode_0x061CE4);
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

unsigned instruction_count_from_0x061CF4(unsigned entry_pc) {
    if (entry_pc == 0x061CF4U) return 1U;
    return 0;
}

BlockExit execute_0x061CF4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061CF4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061CF4U) {
    // guest 0x061CF4 opcode 0x6600 6600 0036 bne.w loc_061D2C
    const auto opcode_0x061CF4 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061CF4);
    branch_condition(api, 6U, 0x061D2CU, 14, 0x0036U);
    api.finish_instruction(opcode_0x061CF4);
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

unsigned instruction_count_from_0x061CFE(unsigned entry_pc) {
    if (entry_pc == 0x061CFEU) return 1U;
    return 0;
}

BlockExit execute_0x061CFE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061CFEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061CFEU) {
    // guest 0x061CFE opcode 0x6600 6600 002C bne.w loc_061D2C
    const auto opcode_0x061CFE = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061CFE);
    branch_condition(api, 6U, 0x061D2CU, 14, 0x002CU);
    api.finish_instruction(opcode_0x061CFE);
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

unsigned instruction_count_from_0x061D08(unsigned entry_pc) {
    if (entry_pc == 0x061D08U) return 1U;
    return 0;
}

BlockExit execute_0x061D08(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061D08U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061D08U) {
    // guest 0x061D08 opcode 0x6700 6700 001E beq.w loc_061D28
    const auto opcode_0x061D08 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061D08);
    branch_condition(api, 7U, 0x061D28U, 14, 0x001EU);
    api.finish_instruction(opcode_0x061D08);
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

unsigned instruction_count_from_0x061D48(unsigned entry_pc) {
    if (entry_pc == 0x061D48U) return 1U;
    return 0;
}

BlockExit execute_0x061D48(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061D48U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061D48U) {
    // guest 0x061D48 opcode 0x51CD 51CD FFFC dbf D5,loc_061D46
    const auto opcode_0x061D48 = fetch_checked(api, 0x51CDU);
    api.begin_instruction(opcode_0x061D48);
    dbcc(api, 1U, 5U, 0x061D46U, 0xFFFCU);
    api.finish_instruction(opcode_0x061D48);
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

unsigned instruction_count_from_0x061D52(unsigned entry_pc) {
    if (entry_pc == 0x061D52U) return 1U;
    return 0;
}

BlockExit execute_0x061D52(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061D52U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061D52U) {
    // guest 0x061D52 opcode 0x6600 6600 FE2A bne.w loc_061B7E
    const auto opcode_0x061D52 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061D52);
    branch_condition(api, 6U, 0x061B7EU, 14, 0xFE2AU);
    api.finish_instruction(opcode_0x061D52);
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

unsigned instruction_count_from_0x061D5C(unsigned entry_pc) {
    if (entry_pc == 0x061D5CU) return 1U;
    return 0;
}

BlockExit execute_0x061D5C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061D5CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061D5CU) {
    // guest 0x061D5C opcode 0x6600 6600 FE20 bne.w loc_061B7E
    const auto opcode_0x061D5C = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061D5C);
    branch_condition(api, 6U, 0x061B7EU, 14, 0xFE20U);
    api.finish_instruction(opcode_0x061D5C);
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

unsigned instruction_count_from_0x061D72(unsigned entry_pc) {
    if (entry_pc == 0x061D72U) return 1U;
    return 0;
}

BlockExit execute_0x061D72(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061D72U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061D72U) {
    // guest 0x061D72 opcode 0x51CD 51CD FFFC dbf D5,loc_061D70
    const auto opcode_0x061D72 = fetch_checked(api, 0x51CDU);
    api.begin_instruction(opcode_0x061D72);
    dbcc(api, 1U, 5U, 0x061D70U, 0xFFFCU);
    api.finish_instruction(opcode_0x061D72);
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

unsigned instruction_count_from_0x061E26(unsigned entry_pc) {
    if (entry_pc == 0x061E26U) return 1U;
    return 0;
}

BlockExit execute_0x061E26(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061E26U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061E26U) {
    // guest 0x061E26 opcode 0x6600 6600 FD7E bne.w loc_061BA6
    const auto opcode_0x061E26 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061E26);
    branch_condition(api, 6U, 0x061BA6U, 14, 0xFD7EU);
    api.finish_instruction(opcode_0x061E26);
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

unsigned instruction_count_from_0x061E4E(unsigned entry_pc) {
    if (entry_pc == 0x061E4EU) return 1U;
    return 0;
}

BlockExit execute_0x061E4E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061E4EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061E4EU) {
    // guest 0x061E4E opcode 0x6700 6700 008E beq.w loc_061EDE
    const auto opcode_0x061E4E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061E4E);
    branch_condition(api, 7U, 0x061EDEU, 14, 0x008EU);
    api.finish_instruction(opcode_0x061E4E);
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

unsigned instruction_count_from_0x061EFA(unsigned entry_pc) {
    if (entry_pc == 0x061EFAU) return 1U;
    return 0;
}

BlockExit execute_0x061EFA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061EFAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061EFAU) {
    // guest 0x061EFA opcode 0x6600 6600 0058 bne.w loc_061F54
    const auto opcode_0x061EFA = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061EFA);
    branch_condition(api, 6U, 0x061F54U, 14, 0x0058U);
    api.finish_instruction(opcode_0x061EFA);
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

unsigned instruction_count_from_0x061F20(unsigned entry_pc) {
    if (entry_pc == 0x061F20U) return 1U;
    return 0;
}

BlockExit execute_0x061F20(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061F20U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061F20U) {
    // guest 0x061F20 opcode 0x6700 6700 002A beq.w loc_061F4C
    const auto opcode_0x061F20 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061F20);
    branch_condition(api, 7U, 0x061F4CU, 14, 0x002AU);
    api.finish_instruction(opcode_0x061F20);
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
