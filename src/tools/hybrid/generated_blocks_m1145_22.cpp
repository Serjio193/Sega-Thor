// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x06127A(unsigned entry_pc) {
    if (entry_pc == 0x06127AU) return 1U;
    return 0;
}

BlockExit execute_0x06127A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06127AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06127AU) {
    // guest 0x06127A opcode 0x41F9 41F9 00FF 0628 lea.l ($00FF0628).L,A0
    const auto opcode_0x06127A = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x06127A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0628U);
    lea_absolute_long(api, 0xFF0628U, 0U);
    api.finish_instruction(opcode_0x06127A);
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

unsigned instruction_count_from_0x061286(unsigned entry_pc) {
    if (entry_pc == 0x061286U) return 1U;
    return 0;
}

BlockExit execute_0x061286(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061286U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061286U) {
    // guest 0x061286 opcode 0x41F9 41F9 00FF 06F2 lea.l ($00FF06F2).L,A0
    const auto opcode_0x061286 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x061286);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x06F2U);
    lea_absolute_long(api, 0xFF06F2U, 0U);
    api.finish_instruction(opcode_0x061286);
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

unsigned instruction_count_from_0x0612A6(unsigned entry_pc) {
    if (entry_pc == 0x0612A6U) return 1U;
    return 0;
}

BlockExit execute_0x0612A6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612A6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612A6U) {
    // guest 0x0612A6 opcode 0x6600 6600 FFF6 bne.w loc_06129E
    const auto opcode_0x0612A6 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0612A6);
    branch_condition(api, 6U, 0x06129EU, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x0612A6);
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

unsigned instruction_count_from_0x0612B0(unsigned entry_pc) {
    if (entry_pc == 0x0612B0U) return 1U;
    return 0;
}

BlockExit execute_0x0612B0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612B0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612B0U) {
    // guest 0x0612B0 opcode 0x6B00 6B00 FFF8 bmi.w loc_0612AA
    const auto opcode_0x0612B0 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0612B0);
    branch_condition(api, 11U, 0x0612AAU, 14, 0xFFF8U);
    api.finish_instruction(opcode_0x0612B0);
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

unsigned instruction_count_from_0x0612BC(unsigned entry_pc) {
    if (entry_pc == 0x0612BCU) return 1U;
    return 0;
}

BlockExit execute_0x0612BC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612BCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612BCU) {
    // guest 0x0612BC opcode 0x4E71 4E71 nop
    const auto opcode_0x0612BC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612BC);

    api.finish_instruction(opcode_0x0612BC);
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

unsigned instruction_count_from_0x0612BE(unsigned entry_pc) {
    if (entry_pc == 0x0612BEU) return 1U;
    return 0;
}

BlockExit execute_0x0612BE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612BEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612BEU) {
    // guest 0x0612BE opcode 0x4E71 4E71 nop
    const auto opcode_0x0612BE = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612BE);

    api.finish_instruction(opcode_0x0612BE);
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

unsigned instruction_count_from_0x0612C0(unsigned entry_pc) {
    if (entry_pc == 0x0612C0U) return 1U;
    return 0;
}

BlockExit execute_0x0612C0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612C0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612C0U) {
    // guest 0x0612C0 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612C0 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612C0);

    api.finish_instruction(opcode_0x0612C0);
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

unsigned instruction_count_from_0x0612C2(unsigned entry_pc) {
    if (entry_pc == 0x0612C2U) return 1U;
    return 0;
}

BlockExit execute_0x0612C2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612C2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612C2U) {
    // guest 0x0612C2 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612C2 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612C2);

    api.finish_instruction(opcode_0x0612C2);
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

unsigned instruction_count_from_0x0612C4(unsigned entry_pc) {
    if (entry_pc == 0x0612C4U) return 1U;
    return 0;
}

BlockExit execute_0x0612C4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612C4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612C4U) {
    // guest 0x0612C4 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612C4 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612C4);

    api.finish_instruction(opcode_0x0612C4);
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

unsigned instruction_count_from_0x0612CC(unsigned entry_pc) {
    if (entry_pc == 0x0612CCU) return 1U;
    return 0;
}

BlockExit execute_0x0612CC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612CCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612CCU) {
    // guest 0x0612CC opcode 0x6B00 6B00 FFF8 bmi.w loc_0612C6
    const auto opcode_0x0612CC = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0612CC);
    branch_condition(api, 11U, 0x0612C6U, 14, 0xFFF8U);
    api.finish_instruction(opcode_0x0612CC);
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

unsigned instruction_count_from_0x0612D8(unsigned entry_pc) {
    if (entry_pc == 0x0612D8U) return 1U;
    return 0;
}

BlockExit execute_0x0612D8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612D8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612D8U) {
    // guest 0x0612D8 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612D8 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612D8);

    api.finish_instruction(opcode_0x0612D8);
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

unsigned instruction_count_from_0x0612DA(unsigned entry_pc) {
    if (entry_pc == 0x0612DAU) return 1U;
    return 0;
}

BlockExit execute_0x0612DA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612DAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612DAU) {
    // guest 0x0612DA opcode 0x4E71 4E71 nop
    const auto opcode_0x0612DA = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612DA);

    api.finish_instruction(opcode_0x0612DA);
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

unsigned instruction_count_from_0x0612DC(unsigned entry_pc) {
    if (entry_pc == 0x0612DCU) return 1U;
    return 0;
}

BlockExit execute_0x0612DC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612DCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612DCU) {
    // guest 0x0612DC opcode 0x4E71 4E71 nop
    const auto opcode_0x0612DC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612DC);

    api.finish_instruction(opcode_0x0612DC);
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

unsigned instruction_count_from_0x0612DE(unsigned entry_pc) {
    if (entry_pc == 0x0612DEU) return 1U;
    return 0;
}

BlockExit execute_0x0612DE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612DEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612DEU) {
    // guest 0x0612DE opcode 0x4E71 4E71 nop
    const auto opcode_0x0612DE = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612DE);

    api.finish_instruction(opcode_0x0612DE);
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

unsigned instruction_count_from_0x0612E0(unsigned entry_pc) {
    if (entry_pc == 0x0612E0U) return 1U;
    return 0;
}

BlockExit execute_0x0612E0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612E0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612E0U) {
    // guest 0x0612E0 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612E0 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612E0);

    api.finish_instruction(opcode_0x0612E0);
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

unsigned instruction_count_from_0x0612E8(unsigned entry_pc) {
    if (entry_pc == 0x0612E8U) return 1U;
    return 0;
}

BlockExit execute_0x0612E8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612E8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612E8U) {
    // guest 0x0612E8 opcode 0x6B00 6B00 FFF8 bmi.w loc_0612E2
    const auto opcode_0x0612E8 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0612E8);
    branch_condition(api, 11U, 0x0612E2U, 14, 0xFFF8U);
    api.finish_instruction(opcode_0x0612E8);
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

unsigned instruction_count_from_0x0612F4(unsigned entry_pc) {
    if (entry_pc == 0x0612F4U) return 1U;
    return 0;
}

BlockExit execute_0x0612F4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612F4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612F4U) {
    // guest 0x0612F4 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612F4 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612F4);

    api.finish_instruction(opcode_0x0612F4);
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
