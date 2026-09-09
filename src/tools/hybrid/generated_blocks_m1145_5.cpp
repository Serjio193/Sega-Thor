// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x002A00(unsigned entry_pc) {
    if (entry_pc == 0x002A00U) return 1U;
    return 0;
}

BlockExit execute_0x002A00(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A00U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A00U) {
    // guest 0x002A00 opcode 0x0243 0243 0003 andi.w #$3,D3
    const auto opcode_0x002A00 = fetch_checked(api, 0x0243U);
    api.begin_instruction(opcode_0x002A00);
    (void)fetch_checked(api, 0x0003U);
    andi_w_data(api, 3U, 3U);
    api.finish_instruction(opcode_0x002A00);
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

unsigned instruction_count_from_0x002A04(unsigned entry_pc) {
    if (entry_pc == 0x002A04U) return 1U;
    return 0;
}

BlockExit execute_0x002A04(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A04U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A04U) {
    // guest 0x002A04 opcode 0x6700 6700 0004 beq.w loc_002A0A
    const auto opcode_0x002A04 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x002A04);
    branch_condition(api, 7U, 0x002A0AU, 14, 0x0004U);
    api.finish_instruction(opcode_0x002A04);
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

unsigned instruction_count_from_0x002A08(unsigned entry_pc) {
    if (entry_pc == 0x002A08U) return 1U;
    return 0;
}

BlockExit execute_0x002A08(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A08U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A08U) {
    // guest 0x002A08 opcode 0x5240 5240 addq.w #$1,D0
    const auto opcode_0x002A08 = fetch_checked(api, 0x5240U);
    api.begin_instruction(opcode_0x002A08);
    addq_w_data(api, 1U, 0U);
    api.finish_instruction(opcode_0x002A08);
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

unsigned instruction_count_from_0x002A10(unsigned entry_pc) {
    if (entry_pc == 0x002A10U) return 1U;
    return 0;
}

BlockExit execute_0x002A10(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A10U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A10U) {
    // guest 0x002A10 opcode 0x6600 6600 00BC bne.w loc_002ACE
    const auto opcode_0x002A10 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x002A10);
    branch_condition(api, 6U, 0x002ACEU, 14, 0x00BCU);
    api.finish_instruction(opcode_0x002A10);
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

unsigned instruction_count_from_0x002A1A(unsigned entry_pc) {
    if (entry_pc == 0x002A1AU) return 1U;
    return 0;
}

BlockExit execute_0x002A1A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A1AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A1AU) {
    // guest 0x002A1A opcode 0x4E71 4E71 nop
    const auto opcode_0x002A1A = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A1A);

    api.finish_instruction(opcode_0x002A1A);
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

unsigned instruction_count_from_0x002A1C(unsigned entry_pc) {
    if (entry_pc == 0x002A1CU) return 1U;
    return 0;
}

BlockExit execute_0x002A1C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A1CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A1CU) {
    // guest 0x002A1C opcode 0x4E71 4E71 nop
    const auto opcode_0x002A1C = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A1C);

    api.finish_instruction(opcode_0x002A1C);
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

unsigned instruction_count_from_0x002A1E(unsigned entry_pc) {
    if (entry_pc == 0x002A1EU) return 1U;
    return 0;
}

BlockExit execute_0x002A1E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A1EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A1EU) {
    // guest 0x002A1E opcode 0x4E71 4E71 nop
    const auto opcode_0x002A1E = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A1E);

    api.finish_instruction(opcode_0x002A1E);
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

unsigned instruction_count_from_0x002A20(unsigned entry_pc) {
    if (entry_pc == 0x002A20U) return 1U;
    return 0;
}

BlockExit execute_0x002A20(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A20U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A20U) {
    // guest 0x002A20 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A20 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A20);

    api.finish_instruction(opcode_0x002A20);
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

unsigned instruction_count_from_0x002A22(unsigned entry_pc) {
    if (entry_pc == 0x002A22U) return 1U;
    return 0;
}

BlockExit execute_0x002A22(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A22U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A22U) {
    // guest 0x002A22 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A22 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A22);

    api.finish_instruction(opcode_0x002A22);
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

unsigned instruction_count_from_0x002A24(unsigned entry_pc) {
    if (entry_pc == 0x002A24U) return 1U;
    return 0;
}

BlockExit execute_0x002A24(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A24U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A24U) {
    // guest 0x002A24 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A24 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A24);

    api.finish_instruction(opcode_0x002A24);
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

unsigned instruction_count_from_0x002A26(unsigned entry_pc) {
    if (entry_pc == 0x002A26U) return 1U;
    return 0;
}

BlockExit execute_0x002A26(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A26U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A26U) {
    // guest 0x002A26 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A26 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A26);

    api.finish_instruction(opcode_0x002A26);
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

unsigned instruction_count_from_0x002A28(unsigned entry_pc) {
    if (entry_pc == 0x002A28U) return 1U;
    return 0;
}

BlockExit execute_0x002A28(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A28U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A28U) {
    // guest 0x002A28 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A28 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A28);

    api.finish_instruction(opcode_0x002A28);
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

unsigned instruction_count_from_0x002A30(unsigned entry_pc) {
    if (entry_pc == 0x002A30U) return 1U;
    return 0;
}

BlockExit execute_0x002A30(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A30U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A30U) {
    // guest 0x002A30 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A30 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A30);

    api.finish_instruction(opcode_0x002A30);
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

unsigned instruction_count_from_0x002A32(unsigned entry_pc) {
    if (entry_pc == 0x002A32U) return 1U;
    return 0;
}

BlockExit execute_0x002A32(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A32U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A32U) {
    // guest 0x002A32 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A32 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A32);

    api.finish_instruction(opcode_0x002A32);
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

unsigned instruction_count_from_0x002A34(unsigned entry_pc) {
    if (entry_pc == 0x002A34U) return 1U;
    return 0;
}

BlockExit execute_0x002A34(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A34U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A34U) {
    // guest 0x002A34 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A34 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A34);

    api.finish_instruction(opcode_0x002A34);
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

unsigned instruction_count_from_0x002A36(unsigned entry_pc) {
    if (entry_pc == 0x002A36U) return 1U;
    return 0;
}

BlockExit execute_0x002A36(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A36U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A36U) {
    // guest 0x002A36 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A36 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A36);

    api.finish_instruction(opcode_0x002A36);
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

unsigned instruction_count_from_0x002A38(unsigned entry_pc) {
    if (entry_pc == 0x002A38U) return 1U;
    return 0;
}

BlockExit execute_0x002A38(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A38U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A38U) {
    // guest 0x002A38 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A38 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A38);

    api.finish_instruction(opcode_0x002A38);
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

unsigned instruction_count_from_0x002A3A(unsigned entry_pc) {
    if (entry_pc == 0x002A3AU) return 1U;
    return 0;
}

BlockExit execute_0x002A3A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A3AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A3AU) {
    // guest 0x002A3A opcode 0x4E71 4E71 nop
    const auto opcode_0x002A3A = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A3A);

    api.finish_instruction(opcode_0x002A3A);
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
