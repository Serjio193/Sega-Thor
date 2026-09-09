// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x06033C(unsigned entry_pc) {
    if (entry_pc == 0x06033CU) return 1U;
    return 0;
}

BlockExit execute_0x06033C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06033CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06033CU) {
    // guest 0x06033C opcode 0x51C8 51C8 FFFC dbf D0,loc_06033A
    const auto opcode_0x06033C = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x06033C);
    dbcc(api, 1U, 0U, 0x06033AU, 0xFFFCU);
    api.finish_instruction(opcode_0x06033C);
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

unsigned instruction_count_from_0x060434(unsigned entry_pc) {
    if (entry_pc == 0x060434U) return 1U;
    return 0;
}

BlockExit execute_0x060434(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060434U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060434U) {
    // guest 0x060434 opcode 0x4BF9 4BF9 00FF 001A lea.l ($00FF001A).L,A5
    const auto opcode_0x060434 = fetch_checked(api, 0x4BF9U);
    api.begin_instruction(opcode_0x060434);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x001AU);
    lea_absolute_long(api, 0xFF001AU, 5U);
    api.finish_instruction(opcode_0x060434);
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

unsigned instruction_count_from_0x060442(unsigned entry_pc) {
    if (entry_pc == 0x060442U) return 1U;
    return 0;
}

BlockExit execute_0x060442(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060442U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060442U) {
    // guest 0x060442 opcode 0x6600 6600 004C bne.w loc_060490
    const auto opcode_0x060442 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x060442);
    branch_condition(api, 6U, 0x060490U, 14, 0x004CU);
    api.finish_instruction(opcode_0x060442);
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

unsigned instruction_count_from_0x060448(unsigned entry_pc) {
    if (entry_pc == 0x060448U) return 1U;
    return 0;
}

BlockExit execute_0x060448(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060448U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060448U) {
    // guest 0x060448 opcode 0x6700 6700 009C beq.w loc_0604E6
    const auto opcode_0x060448 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060448);
    branch_condition(api, 7U, 0x0604E6U, 14, 0x009CU);
    api.finish_instruction(opcode_0x060448);
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

unsigned instruction_count_from_0x060494(unsigned entry_pc) {
    if (entry_pc == 0x060494U) return 1U;
    return 0;
}

BlockExit execute_0x060494(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060494U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060494U) {
    // guest 0x060494 opcode 0x6700 6700 06BA beq.w loc_060B50
    const auto opcode_0x060494 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060494);
    branch_condition(api, 7U, 0x060B50U, 14, 0x06BAU);
    api.finish_instruction(opcode_0x060494);
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

unsigned instruction_count_from_0x06049C(unsigned entry_pc) {
    if (entry_pc == 0x06049CU) return 1U;
    return 0;
}

BlockExit execute_0x06049C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06049CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06049CU) {
    // guest 0x06049C opcode 0x6700 6700 0C90 beq.w loc_06112E
    const auto opcode_0x06049C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06049C);
    branch_condition(api, 7U, 0x06112EU, 14, 0x0C90U);
    api.finish_instruction(opcode_0x06049C);
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

unsigned instruction_count_from_0x0604C8(unsigned entry_pc) {
    if (entry_pc == 0x0604C8U) return 1U;
    return 0;
}

BlockExit execute_0x0604C8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0604C8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0604C8U) {
    // guest 0x0604C8 opcode 0x4DF9 4DF9 00FF 06F2 lea.l ($00FF06F2).L,A6
    const auto opcode_0x0604C8 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0604C8);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x06F2U);
    lea_absolute_long(api, 0xFF06F2U, 6U);
    api.finish_instruction(opcode_0x0604C8);
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

unsigned instruction_count_from_0x060B60(unsigned entry_pc) {
    if (entry_pc == 0x060B60U) return 1U;
    return 0;
}

BlockExit execute_0x060B60(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060B60U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060B60U) {
    // guest 0x060B60 opcode 0x6700 6700 067E beq.w loc_0611E0
    const auto opcode_0x060B60 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060B60);
    branch_condition(api, 7U, 0x0611E0U, 14, 0x067EU);
    api.finish_instruction(opcode_0x060B60);
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

unsigned instruction_count_from_0x060B78(unsigned entry_pc) {
    if (entry_pc == 0x060B78U) return 1U;
    return 0;
}

BlockExit execute_0x060B78(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060B78U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060B78U) {
    // guest 0x060B78 opcode 0x6600 6600 FFF6 bne.w loc_060B70
    const auto opcode_0x060B78 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x060B78);
    branch_condition(api, 6U, 0x060B70U, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x060B78);
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

unsigned instruction_count_from_0x060BA0(unsigned entry_pc) {
    if (entry_pc == 0x060BA0U) return 1U;
    return 0;
}

BlockExit execute_0x060BA0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BA0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BA0U) {
    // guest 0x060BA0 opcode 0x6600 6600 FFF6 bne.w loc_060B98
    const auto opcode_0x060BA0 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x060BA0);
    branch_condition(api, 6U, 0x060B98U, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x060BA0);
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

unsigned instruction_count_from_0x060BAA(unsigned entry_pc) {
    if (entry_pc == 0x060BAAU) return 1U;
    return 0;
}

BlockExit execute_0x060BAA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BAAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BAAU) {
    // guest 0x060BAA opcode 0x6700 6700 0018 beq.w loc_060BC4
    const auto opcode_0x060BAA = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060BAA);
    branch_condition(api, 7U, 0x060BC4U, 14, 0x0018U);
    api.finish_instruction(opcode_0x060BAA);
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

unsigned instruction_count_from_0x060BB6(unsigned entry_pc) {
    if (entry_pc == 0x060BB6U) return 1U;
    return 0;
}

BlockExit execute_0x060BB6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BB6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BB6U) {
    // guest 0x060BB6 opcode 0x4E71 4E71 nop
    const auto opcode_0x060BB6 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x060BB6);

    api.finish_instruction(opcode_0x060BB6);
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

unsigned instruction_count_from_0x060BB8(unsigned entry_pc) {
    if (entry_pc == 0x060BB8U) return 1U;
    return 0;
}

BlockExit execute_0x060BB8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BB8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BB8U) {
    // guest 0x060BB8 opcode 0x4E71 4E71 nop
    const auto opcode_0x060BB8 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x060BB8);

    api.finish_instruction(opcode_0x060BB8);
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

unsigned instruction_count_from_0x060BBA(unsigned entry_pc) {
    if (entry_pc == 0x060BBAU) return 1U;
    return 0;
}

BlockExit execute_0x060BBA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BBAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BBAU) {
    // guest 0x060BBA opcode 0x4E71 4E71 nop
    const auto opcode_0x060BBA = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x060BBA);

    api.finish_instruction(opcode_0x060BBA);
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

unsigned instruction_count_from_0x060BBC(unsigned entry_pc) {
    if (entry_pc == 0x060BBCU) return 1U;
    return 0;
}

BlockExit execute_0x060BBC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BBCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BBCU) {
    // guest 0x060BBC opcode 0x4E71 4E71 nop
    const auto opcode_0x060BBC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x060BBC);

    api.finish_instruction(opcode_0x060BBC);
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

unsigned instruction_count_from_0x060BBE(unsigned entry_pc) {
    if (entry_pc == 0x060BBEU) return 1U;
    return 0;
}

BlockExit execute_0x060BBE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BBEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BBEU) {
    // guest 0x060BBE opcode 0x4E71 4E71 nop
    const auto opcode_0x060BBE = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x060BBE);

    api.finish_instruction(opcode_0x060BBE);
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

unsigned instruction_count_from_0x060BC0(unsigned entry_pc) {
    if (entry_pc == 0x060BC0U) return 1U;
    return 0;
}

BlockExit execute_0x060BC0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060BC0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060BC0U) {
    // guest 0x060BC0 opcode 0x4E71 4E71 nop
    const auto opcode_0x060BC0 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x060BC0);

    api.finish_instruction(opcode_0x060BC0);
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
