// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x061054(unsigned entry_pc) {
    if (entry_pc == 0x061054U) return 1U;
    return 0;
}

BlockExit execute_0x061054(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061054U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061054U) {
    // guest 0x061054 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x061054 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x061054);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x061054);
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

unsigned instruction_count_from_0x0610BC(unsigned entry_pc) {
    if (entry_pc == 0x0610BCU) return 1U;
    return 0;
}

BlockExit execute_0x0610BC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0610BCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0610BCU) {
    // guest 0x0610BC opcode 0x6600 6600 0008 bne.w loc_0610C6
    const auto opcode_0x0610BC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0610BC);
    branch_condition(api, 6U, 0x0610C6U, 14, 0x0008U);
    api.finish_instruction(opcode_0x0610BC);
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

unsigned instruction_count_from_0x0610C8(unsigned entry_pc) {
    if (entry_pc == 0x0610C8U) return 1U;
    return 0;
}

BlockExit execute_0x0610C8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0610C8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0610C8U) {
    // guest 0x0610C8 opcode 0xD481 D481 add.l D1,D2
    const auto opcode_0x0610C8 = fetch_checked(api, 0xD481U);
    api.begin_instruction(opcode_0x0610C8);
    add_l_data_to_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x0610C8);
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

unsigned instruction_count_from_0x0610D2(unsigned entry_pc) {
    if (entry_pc == 0x0610D2U) return 1U;
    return 0;
}

BlockExit execute_0x0610D2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0610D2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0610D2U) {
    // guest 0x0610D2 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0610D2 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0610D2);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0610D2);
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

unsigned instruction_count_from_0x0610E0(unsigned entry_pc) {
    if (entry_pc == 0x0610E0U) return 1U;
    return 0;
}

BlockExit execute_0x0610E0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0610E0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0610E0U) {
    // guest 0x0610E0 opcode 0x6700 6700 0008 beq.w loc_0610EA
    const auto opcode_0x0610E0 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0610E0);
    branch_condition(api, 7U, 0x0610EAU, 14, 0x0008U);
    api.finish_instruction(opcode_0x0610E0);
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

unsigned instruction_count_from_0x0610EA(unsigned entry_pc) {
    if (entry_pc == 0x0610EAU) return 1U;
    return 0;
}

BlockExit execute_0x0610EA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0610EAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0610EAU) {
    // guest 0x0610EA opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0610EA = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0610EA);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0610EA);
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

unsigned instruction_count_from_0x061122(unsigned entry_pc) {
    if (entry_pc == 0x061122U) return 1U;
    return 0;
}

BlockExit execute_0x061122(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061122U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061122U) {
    // guest 0x061122 opcode 0x6600 6600 0008 bne.w loc_06112C
    const auto opcode_0x061122 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061122);
    branch_condition(api, 6U, 0x06112CU, 14, 0x0008U);
    api.finish_instruction(opcode_0x061122);
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

unsigned instruction_count_from_0x06112E(unsigned entry_pc) {
    if (entry_pc == 0x06112EU) return 1U;
    return 0;
}

BlockExit execute_0x06112E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06112EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06112EU) {
    // guest 0x06112E opcode 0x4A39 4A39 00FF 0012 tst.b ($00FF0012).L
    const auto opcode_0x06112E = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x06112E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0012U);
    test_absolute_long(api, 0xFF0012U, 1U);
    api.finish_instruction(opcode_0x06112E);
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

unsigned instruction_count_from_0x061134(unsigned entry_pc) {
    if (entry_pc == 0x061134U) return 1U;
    return 0;
}

BlockExit execute_0x061134(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061134U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061134U) {
    // guest 0x061134 opcode 0x6600 6600 006C bne.w loc_0611A2
    const auto opcode_0x061134 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061134);
    branch_condition(api, 6U, 0x0611A2U, 14, 0x006CU);
    api.finish_instruction(opcode_0x061134);
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

unsigned instruction_count_from_0x061138(unsigned entry_pc) {
    if (entry_pc == 0x061138U) return 1U;
    return 0;
}

BlockExit execute_0x061138(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061138U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061138U) {
    // guest 0x061138 opcode 0x4DF9 4DF9 00FF 05B2 lea.l ($00FF05B2).L,A6
    const auto opcode_0x061138 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x061138);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x05B2U);
    lea_absolute_long(api, 0xFF05B2U, 6U);
    api.finish_instruction(opcode_0x061138);
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

unsigned instruction_count_from_0x061142(unsigned entry_pc) {
    if (entry_pc == 0x061142U) return 1U;
    return 0;
}

BlockExit execute_0x061142(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061142U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061142U) {
    // guest 0x061142 opcode 0x6600 6600 0030 bne.w loc_061174
    const auto opcode_0x061142 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061142);
    branch_condition(api, 6U, 0x061174U, 14, 0x0030U);
    api.finish_instruction(opcode_0x061142);
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

unsigned instruction_count_from_0x061146(unsigned entry_pc) {
    if (entry_pc == 0x061146U) return 1U;
    return 0;
}

BlockExit execute_0x061146(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061146U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061146U) {
    // guest 0x061146 opcode 0x4A39 4A39 00FF 0013 tst.b ($00FF0013).L
    const auto opcode_0x061146 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x061146);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0013U);
    test_absolute_long(api, 0xFF0013U, 1U);
    api.finish_instruction(opcode_0x061146);
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

unsigned instruction_count_from_0x06114C(unsigned entry_pc) {
    if (entry_pc == 0x06114CU) return 1U;
    return 0;
}

BlockExit execute_0x06114C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06114CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06114CU) {
    // guest 0x06114C opcode 0x6600 6600 0026 bne.w loc_061174
    const auto opcode_0x06114C = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06114C);
    branch_condition(api, 6U, 0x061174U, 14, 0x0026U);
    api.finish_instruction(opcode_0x06114C);
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

unsigned instruction_count_from_0x0611D6(unsigned entry_pc) {
    if (entry_pc == 0x0611D6U) return 1U;
    return 0;
}

BlockExit execute_0x0611D6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0611D6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0611D6U) {
    // guest 0x0611D6 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0611D6 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0611D6);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0611D6);
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

unsigned instruction_count_from_0x061204(unsigned entry_pc) {
    if (entry_pc == 0x061204U) return 1U;
    return 0;
}

BlockExit execute_0x061204(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061204U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061204U) {
    // guest 0x061204 opcode 0x6600 6600 FFF6 bne.w loc_0611FC
    const auto opcode_0x061204 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061204);
    branch_condition(api, 6U, 0x0611FCU, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x061204);
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

unsigned instruction_count_from_0x061258(unsigned entry_pc) {
    if (entry_pc == 0x061258U) return 1U;
    return 0;
}

BlockExit execute_0x061258(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061258U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061258U) {
    // guest 0x061258 opcode 0x4BF9 4BF9 00FF 001A lea.l ($00FF001A).L,A5
    const auto opcode_0x061258 = fetch_checked(api, 0x4BF9U);
    api.begin_instruction(opcode_0x061258);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x001AU);
    lea_absolute_long(api, 0xFF001AU, 5U);
    api.finish_instruction(opcode_0x061258);
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

unsigned instruction_count_from_0x061268(unsigned entry_pc) {
    if (entry_pc == 0x061268U) return 1U;
    return 0;
}

BlockExit execute_0x061268(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061268U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061268U) {
    // guest 0x061268 opcode 0x51C8 51C8 FFFC dbf D0,loc_061266
    const auto opcode_0x061268 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x061268);
    dbcc(api, 1U, 0U, 0x061266U, 0xFFFCU);
    api.finish_instruction(opcode_0x061268);
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
