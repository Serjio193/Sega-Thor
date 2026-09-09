// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0612F6(unsigned entry_pc) {
    if (entry_pc == 0x0612F6U) return 1U;
    return 0;
}

BlockExit execute_0x0612F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612F6U) {
    // guest 0x0612F6 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612F6 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612F6);

    api.finish_instruction(opcode_0x0612F6);
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

unsigned instruction_count_from_0x0612F8(unsigned entry_pc) {
    if (entry_pc == 0x0612F8U) return 1U;
    return 0;
}

BlockExit execute_0x0612F8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612F8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612F8U) {
    // guest 0x0612F8 opcode 0x4E71 4E71 nop
    const auto opcode_0x0612F8 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612F8);

    api.finish_instruction(opcode_0x0612F8);
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

unsigned instruction_count_from_0x0612FA(unsigned entry_pc) {
    if (entry_pc == 0x0612FAU) return 1U;
    return 0;
}

BlockExit execute_0x0612FA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612FAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612FAU) {
    // guest 0x0612FA opcode 0x4E71 4E71 nop
    const auto opcode_0x0612FA = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612FA);

    api.finish_instruction(opcode_0x0612FA);
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

unsigned instruction_count_from_0x0612FC(unsigned entry_pc) {
    if (entry_pc == 0x0612FCU) return 1U;
    return 0;
}

BlockExit execute_0x0612FC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0612FCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0612FCU) {
    // guest 0x0612FC opcode 0x4E71 4E71 nop
    const auto opcode_0x0612FC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0612FC);

    api.finish_instruction(opcode_0x0612FC);
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

unsigned instruction_count_from_0x061304(unsigned entry_pc) {
    if (entry_pc == 0x061304U) return 1U;
    return 0;
}

BlockExit execute_0x061304(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061304U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061304U) {
    // guest 0x061304 opcode 0x6B00 6B00 FFF8 bmi.w loc_0612FE
    const auto opcode_0x061304 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x061304);
    branch_condition(api, 11U, 0x0612FEU, 14, 0xFFF8U);
    api.finish_instruction(opcode_0x061304);
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

unsigned instruction_count_from_0x061310(unsigned entry_pc) {
    if (entry_pc == 0x061310U) return 1U;
    return 0;
}

BlockExit execute_0x061310(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061310U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061310U) {
    // guest 0x061310 opcode 0x4E71 4E71 nop
    const auto opcode_0x061310 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061310);

    api.finish_instruction(opcode_0x061310);
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

unsigned instruction_count_from_0x061312(unsigned entry_pc) {
    if (entry_pc == 0x061312U) return 1U;
    return 0;
}

BlockExit execute_0x061312(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061312U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061312U) {
    // guest 0x061312 opcode 0x4E71 4E71 nop
    const auto opcode_0x061312 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061312);

    api.finish_instruction(opcode_0x061312);
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

unsigned instruction_count_from_0x061314(unsigned entry_pc) {
    if (entry_pc == 0x061314U) return 1U;
    return 0;
}

BlockExit execute_0x061314(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061314U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061314U) {
    // guest 0x061314 opcode 0x4E71 4E71 nop
    const auto opcode_0x061314 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061314);

    api.finish_instruction(opcode_0x061314);
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

unsigned instruction_count_from_0x061316(unsigned entry_pc) {
    if (entry_pc == 0x061316U) return 1U;
    return 0;
}

BlockExit execute_0x061316(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061316U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061316U) {
    // guest 0x061316 opcode 0x4E71 4E71 nop
    const auto opcode_0x061316 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061316);

    api.finish_instruction(opcode_0x061316);
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

unsigned instruction_count_from_0x061318(unsigned entry_pc) {
    if (entry_pc == 0x061318U) return 1U;
    return 0;
}

BlockExit execute_0x061318(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061318U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061318U) {
    // guest 0x061318 opcode 0x4E71 4E71 nop
    const auto opcode_0x061318 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061318);

    api.finish_instruction(opcode_0x061318);
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

unsigned instruction_count_from_0x06134A(unsigned entry_pc) {
    if (entry_pc == 0x06134AU) return 1U;
    return 0;
}

BlockExit execute_0x06134A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06134AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06134AU) {
    // guest 0x06134A opcode 0x6600 6600 FFF6 bne.w loc_061342
    const auto opcode_0x06134A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06134A);
    branch_condition(api, 6U, 0x061342U, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x06134A);
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

unsigned instruction_count_from_0x06134E(unsigned entry_pc) {
    if (entry_pc == 0x06134EU) return 1U;
    return 0;
}

BlockExit execute_0x06134E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06134EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06134EU) {
    // guest 0x06134E opcode 0x43F9 43F9 0006 2E38 lea.l ($00062E38).L,A1
    const auto opcode_0x06134E = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x06134E);
    (void)fetch_checked(api, 0x0006U);
    (void)fetch_checked(api, 0x2E38U);
    lea_absolute_long(api, 0x062E38U, 1U);
    api.finish_instruction(opcode_0x06134E);
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

unsigned instruction_count_from_0x061374(unsigned entry_pc) {
    if (entry_pc == 0x061374U) return 1U;
    return 0;
}

BlockExit execute_0x061374(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061374U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061374U) {
    // guest 0x061374 opcode 0x4E71 4E71 nop
    const auto opcode_0x061374 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061374);

    api.finish_instruction(opcode_0x061374);
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

unsigned instruction_count_from_0x061376(unsigned entry_pc) {
    if (entry_pc == 0x061376U) return 1U;
    return 0;
}

BlockExit execute_0x061376(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061376U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061376U) {
    // guest 0x061376 opcode 0x4E71 4E71 nop
    const auto opcode_0x061376 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061376);

    api.finish_instruction(opcode_0x061376);
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

unsigned instruction_count_from_0x061378(unsigned entry_pc) {
    if (entry_pc == 0x061378U) return 1U;
    return 0;
}

BlockExit execute_0x061378(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061378U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061378U) {
    // guest 0x061378 opcode 0x4E71 4E71 nop
    const auto opcode_0x061378 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x061378);

    api.finish_instruction(opcode_0x061378);
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

unsigned instruction_count_from_0x06137A(unsigned entry_pc) {
    if (entry_pc == 0x06137AU) return 1U;
    return 0;
}

BlockExit execute_0x06137A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06137AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06137AU) {
    // guest 0x06137A opcode 0x4E71 4E71 nop
    const auto opcode_0x06137A = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x06137A);

    api.finish_instruction(opcode_0x06137A);
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

unsigned instruction_count_from_0x06137C(unsigned entry_pc) {
    if (entry_pc == 0x06137CU) return 1U;
    return 0;
}

BlockExit execute_0x06137C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06137CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06137CU) {
    // guest 0x06137C opcode 0x4E71 4E71 nop
    const auto opcode_0x06137C = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x06137C);

    api.finish_instruction(opcode_0x06137C);
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

unsigned instruction_count_from_0x0613B8(unsigned entry_pc) {
    if (entry_pc == 0x0613B8U) return 1U;
    return 0;
}

BlockExit execute_0x0613B8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0613B8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0613B8U) {
    // guest 0x0613B8 opcode 0x6700 6700 003C beq.w loc_0613F6
    const auto opcode_0x0613B8 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0613B8);
    branch_condition(api, 7U, 0x0613F6U, 14, 0x003CU);
    api.finish_instruction(opcode_0x0613B8);
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
