// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x000214(unsigned entry_pc) {
    if (entry_pc == 0x000214U) return 1U;
    return 0;
}

BlockExit execute_0x000214(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000214U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000214U) {
    // guest 0x000214 opcode 0x6606 6606 bne.s loc_00021C
    const auto opcode_0x000214 = fetch_checked(api, 0x6606U);
    api.begin_instruction(opcode_0x000214);
    branch_condition(api, 6U, 0x00021CU, -14);
    api.finish_instruction(opcode_0x000214);
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

unsigned instruction_count_from_0x00021C(unsigned entry_pc) {
    if (entry_pc == 0x00021CU) return 1U;
    return 0;
}

BlockExit execute_0x00021C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00021CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00021CU) {
    // guest 0x00021C opcode 0x667C 667C bne.s loc_00029A
    const auto opcode_0x00021C = fetch_checked(api, 0x667CU);
    api.begin_instruction(opcode_0x00021C);
    branch_condition(api, 6U, 0x00029AU, -14);
    api.finish_instruction(opcode_0x00021C);
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

unsigned instruction_count_from_0x000232(unsigned entry_pc) {
    if (entry_pc == 0x000232U) return 1U;
    return 0;
}

BlockExit execute_0x000232(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000232U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000232U) {
    // guest 0x000232 opcode 0x6708 6708 beq.s loc_00023C
    const auto opcode_0x000232 = fetch_checked(api, 0x6708U);
    api.begin_instruction(opcode_0x000232);
    branch_condition(api, 7U, 0x00023CU, -14);
    api.finish_instruction(opcode_0x000232);
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

unsigned instruction_count_from_0x00023E(unsigned entry_pc) {
    if (entry_pc == 0x00023EU) return 1U;
    return 0;
}

BlockExit execute_0x00023E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00023EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00023EU) {
    // guest 0x00023E opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x00023E = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x00023E);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x00023E);
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

unsigned instruction_count_from_0x00024C(unsigned entry_pc) {
    if (entry_pc == 0x00024CU) return 1U;
    return 0;
}

BlockExit execute_0x00024C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00024CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00024CU) {
    // guest 0x00024C opcode 0x51C9 51C9 FFF8 dbf D1,loc_000246
    const auto opcode_0x00024C = fetch_checked(api, 0x51C9U);
    api.begin_instruction(opcode_0x00024C);
    dbcc(api, 1U, 1U, 0x000246U, 0xFFF8U);
    api.finish_instruction(opcode_0x00024C);
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

unsigned instruction_count_from_0x00025A(unsigned entry_pc) {
    if (entry_pc == 0x00025AU) return 1U;
    return 0;
}

BlockExit execute_0x00025A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00025AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00025AU) {
    // guest 0x00025A opcode 0x66FC 66FC bne.s loc_000258
    const auto opcode_0x00025A = fetch_checked(api, 0x66FCU);
    api.begin_instruction(opcode_0x00025A);
    branch_condition(api, 6U, 0x000258U, -14);
    api.finish_instruction(opcode_0x00025A);
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

unsigned instruction_count_from_0x000260(unsigned entry_pc) {
    if (entry_pc == 0x000260U) return 1U;
    return 0;
}

BlockExit execute_0x000260(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000260U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000260U) {
    // guest 0x000260 opcode 0x51CA 51CA FFFC dbf D2,loc_00025E
    const auto opcode_0x000260 = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x000260);
    dbcc(api, 1U, 2U, 0x00025EU, 0xFFFCU);
    api.finish_instruction(opcode_0x000260);
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

unsigned instruction_count_from_0x00026C(unsigned entry_pc) {
    if (entry_pc == 0x00026CU) return 1U;
    return 0;
}

BlockExit execute_0x00026C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00026CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00026CU) {
    // guest 0x00026C opcode 0x51CE 51CE FFFC dbf D6,loc_00026A
    const auto opcode_0x00026C = fetch_checked(api, 0x51CEU);
    api.begin_instruction(opcode_0x00026C);
    dbcc(api, 1U, 6U, 0x00026AU, 0xFFFCU);
    api.finish_instruction(opcode_0x00026C);
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

unsigned instruction_count_from_0x000278(unsigned entry_pc) {
    if (entry_pc == 0x000278U) return 1U;
    return 0;
}

BlockExit execute_0x000278(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000278U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000278U) {
    // guest 0x000278 opcode 0x51CB 51CB FFFC dbf D3,loc_000276
    const auto opcode_0x000278 = fetch_checked(api, 0x51CBU);
    api.begin_instruction(opcode_0x000278);
    dbcc(api, 1U, 3U, 0x000276U, 0xFFFCU);
    api.finish_instruction(opcode_0x000278);
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

unsigned instruction_count_from_0x000282(unsigned entry_pc) {
    if (entry_pc == 0x000282U) return 1U;
    return 0;
}

BlockExit execute_0x000282(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000282U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000282U) {
    // guest 0x000282 opcode 0x51CC 51CC FFFC dbf D4,loc_000280
    const auto opcode_0x000282 = fetch_checked(api, 0x51CCU);
    api.begin_instruction(opcode_0x000282);
    dbcc(api, 1U, 4U, 0x000280U, 0xFFFCU);
    api.finish_instruction(opcode_0x000282);
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

unsigned instruction_count_from_0x00028C(unsigned entry_pc) {
    if (entry_pc == 0x00028CU) return 1U;
    return 0;
}

BlockExit execute_0x00028C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00028CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00028CU) {
    // guest 0x00028C opcode 0x51CD 51CD FFFA dbf D5,loc_000288
    const auto opcode_0x00028C = fetch_checked(api, 0x51CDU);
    api.begin_instruction(opcode_0x00028C);
    dbcc(api, 1U, 5U, 0x000288U, 0xFFFAU);
    api.finish_instruction(opcode_0x00028C);
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

unsigned instruction_count_from_0x000318(unsigned entry_pc) {
    if (entry_pc == 0x000318U) return 1U;
    return 0;
}

BlockExit execute_0x000318(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000318U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000318U) {
    // guest 0x000318 opcode 0x66F4 66F4 bne.s loc_00030E
    const auto opcode_0x000318 = fetch_checked(api, 0x66F4U);
    api.begin_instruction(opcode_0x000318);
    branch_condition(api, 6U, 0x00030EU, -14);
    api.finish_instruction(opcode_0x000318);
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

unsigned instruction_count_from_0x00034E(unsigned entry_pc) {
    if (entry_pc == 0x00034EU) return 1U;
    return 0;
}

BlockExit execute_0x00034E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00034EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00034EU) {
    // guest 0x00034E opcode 0x4A79 4A79 0000 018E tst.w ($0000018E).L
    const auto opcode_0x00034E = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x00034E);
    (void)fetch_checked(api, 0x0000U);
    (void)fetch_checked(api, 0x018EU);
    test_absolute_long(api, 0x00018EU, 2U);
    api.finish_instruction(opcode_0x00034E);
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

unsigned instruction_count_from_0x000354(unsigned entry_pc) {
    if (entry_pc == 0x000354U) return 1U;
    return 0;
}

BlockExit execute_0x000354(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000354U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000354U) {
    // guest 0x000354 opcode 0x6700 6700 0070 beq.w loc_0003C6
    const auto opcode_0x000354 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x000354);
    branch_condition(api, 7U, 0x0003C6U, 14, 0x0070U);
    api.finish_instruction(opcode_0x000354);
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

unsigned instruction_count_from_0x000360(unsigned entry_pc) {
    if (entry_pc == 0x000360U) return 1U;
    return 0;
}

BlockExit execute_0x000360(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000360U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000360U) {
    // guest 0x000360 opcode 0x6600 6600 0064 bne.w loc_0003C6
    const auto opcode_0x000360 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x000360);
    branch_condition(api, 6U, 0x0003C6U, 14, 0x0064U);
    api.finish_instruction(opcode_0x000360);
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

unsigned instruction_count_from_0x000364(unsigned entry_pc) {
    if (entry_pc == 0x000364U) return 1U;
    return 0;
}

BlockExit execute_0x000364(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000364U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000364U) {
    // guest 0x000364 opcode 0x41F9 41F9 0000 01A4 lea.l ($000001A4).L,A0
    const auto opcode_0x000364 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x000364);
    (void)fetch_checked(api, 0x0000U);
    (void)fetch_checked(api, 0x01A4U);
    lea_absolute_long(api, 0x0001A4U, 0U);
    api.finish_instruction(opcode_0x000364);
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

unsigned instruction_count_from_0x00037A(unsigned entry_pc) {
    if (entry_pc == 0x00037AU) return 1U;
    return 0;
}

BlockExit execute_0x00037A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00037AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00037AU) {
    // guest 0x00037A opcode 0x5342 5342 subq.w #$1,D2
    const auto opcode_0x00037A = fetch_checked(api, 0x5342U);
    api.begin_instruction(opcode_0x00037A);
    subq_w_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x00037A);
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
