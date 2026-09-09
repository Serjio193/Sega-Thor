// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0029C4(unsigned entry_pc) {
    if (entry_pc == 0x0029C4U) return 1U;
    return 0;
}

BlockExit execute_0x0029C4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029C4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029C4U) {
    // guest 0x0029C4 opcode 0x5240 5240 addq.w #$1,D0
    const auto opcode_0x0029C4 = fetch_checked(api, 0x5240U);
    api.begin_instruction(opcode_0x0029C4);
    addq_w_data(api, 1U, 0U);
    api.finish_instruction(opcode_0x0029C4);
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

unsigned instruction_count_from_0x0029C6(unsigned entry_pc) {
    if (entry_pc == 0x0029C6U) return 1U;
    return 0;
}

BlockExit execute_0x0029C6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029C6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029C6U) {
    // guest 0x0029C6 opcode 0xD040 D040 add.w D0,D0
    const auto opcode_0x0029C6 = fetch_checked(api, 0xD040U);
    api.begin_instruction(opcode_0x0029C6);
    add_w_data_to_data(api, 0U, 0U);
    api.finish_instruction(opcode_0x0029C6);
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

unsigned instruction_count_from_0x0029CC(unsigned entry_pc) {
    if (entry_pc == 0x0029CCU) return 1U;
    return 0;
}

BlockExit execute_0x0029CC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029CCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029CCU) {
    // guest 0x0029CC opcode 0x4E71 4E71 nop
    const auto opcode_0x0029CC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029CC);

    api.finish_instruction(opcode_0x0029CC);
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

unsigned instruction_count_from_0x0029CE(unsigned entry_pc) {
    if (entry_pc == 0x0029CEU) return 1U;
    return 0;
}

BlockExit execute_0x0029CE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029CEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029CEU) {
    // guest 0x0029CE opcode 0x4E71 4E71 nop
    const auto opcode_0x0029CE = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029CE);

    api.finish_instruction(opcode_0x0029CE);
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

unsigned instruction_count_from_0x0029D0(unsigned entry_pc) {
    if (entry_pc == 0x0029D0U) return 1U;
    return 0;
}

BlockExit execute_0x0029D0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029D0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029D0U) {
    // guest 0x0029D0 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029D0 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029D0);

    api.finish_instruction(opcode_0x0029D0);
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

unsigned instruction_count_from_0x0029D2(unsigned entry_pc) {
    if (entry_pc == 0x0029D2U) return 1U;
    return 0;
}

BlockExit execute_0x0029D2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029D2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029D2U) {
    // guest 0x0029D2 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029D2 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029D2);

    api.finish_instruction(opcode_0x0029D2);
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

unsigned instruction_count_from_0x0029D6(unsigned entry_pc) {
    if (entry_pc == 0x0029D6U) return 1U;
    return 0;
}

BlockExit execute_0x0029D6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029D6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029D6U) {
    // guest 0x0029D6 opcode 0x0243 0243 0003 andi.w #$3,D3
    const auto opcode_0x0029D6 = fetch_checked(api, 0x0243U);
    api.begin_instruction(opcode_0x0029D6);
    (void)fetch_checked(api, 0x0003U);
    andi_w_data(api, 3U, 3U);
    api.finish_instruction(opcode_0x0029D6);
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

unsigned instruction_count_from_0x0029DA(unsigned entry_pc) {
    if (entry_pc == 0x0029DAU) return 1U;
    return 0;
}

BlockExit execute_0x0029DA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029DAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029DAU) {
    // guest 0x0029DA opcode 0x6700 6700 0004 beq.w loc_0029E0
    const auto opcode_0x0029DA = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0029DA);
    branch_condition(api, 7U, 0x0029E0U, 14, 0x0004U);
    api.finish_instruction(opcode_0x0029DA);
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

unsigned instruction_count_from_0x0029DE(unsigned entry_pc) {
    if (entry_pc == 0x0029DEU) return 1U;
    return 0;
}

BlockExit execute_0x0029DE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029DEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029DEU) {
    // guest 0x0029DE opcode 0x5240 5240 addq.w #$1,D0
    const auto opcode_0x0029DE = fetch_checked(api, 0x5240U);
    api.begin_instruction(opcode_0x0029DE);
    addq_w_data(api, 1U, 0U);
    api.finish_instruction(opcode_0x0029DE);
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

unsigned instruction_count_from_0x0029E0(unsigned entry_pc) {
    if (entry_pc == 0x0029E0U) return 1U;
    return 0;
}

BlockExit execute_0x0029E0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029E0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029E0U) {
    // guest 0x0029E0 opcode 0xD040 D040 add.w D0,D0
    const auto opcode_0x0029E0 = fetch_checked(api, 0xD040U);
    api.begin_instruction(opcode_0x0029E0);
    add_w_data_to_data(api, 0U, 0U);
    api.finish_instruction(opcode_0x0029E0);
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

unsigned instruction_count_from_0x0029E6(unsigned entry_pc) {
    if (entry_pc == 0x0029E6U) return 1U;
    return 0;
}

BlockExit execute_0x0029E6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029E6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029E6U) {
    // guest 0x0029E6 opcode 0x0203 0203 000C andi.b #$C,D3
    const auto opcode_0x0029E6 = fetch_checked(api, 0x0203U);
    api.begin_instruction(opcode_0x0029E6);
    (void)fetch_checked(api, 0x000CU);
    andi_b_data(api, 12U, 3U);
    api.finish_instruction(opcode_0x0029E6);
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

unsigned instruction_count_from_0x0029EA(unsigned entry_pc) {
    if (entry_pc == 0x0029EAU) return 1U;
    return 0;
}

BlockExit execute_0x0029EA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029EAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029EAU) {
    // guest 0x0029EA opcode 0x6700 6700 0004 beq.w loc_0029F0
    const auto opcode_0x0029EA = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0029EA);
    branch_condition(api, 7U, 0x0029F0U, 14, 0x0004U);
    api.finish_instruction(opcode_0x0029EA);
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

unsigned instruction_count_from_0x0029F0(unsigned entry_pc) {
    if (entry_pc == 0x0029F0U) return 1U;
    return 0;
}

BlockExit execute_0x0029F0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029F0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029F0U) {
    // guest 0x0029F0 opcode 0xD040 D040 add.w D0,D0
    const auto opcode_0x0029F0 = fetch_checked(api, 0xD040U);
    api.begin_instruction(opcode_0x0029F0);
    add_w_data_to_data(api, 0U, 0U);
    api.finish_instruction(opcode_0x0029F0);
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

unsigned instruction_count_from_0x0029F6(unsigned entry_pc) {
    if (entry_pc == 0x0029F6U) return 1U;
    return 0;
}

BlockExit execute_0x0029F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029F6U) {
    // guest 0x0029F6 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029F6 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029F6);

    api.finish_instruction(opcode_0x0029F6);
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

unsigned instruction_count_from_0x0029F8(unsigned entry_pc) {
    if (entry_pc == 0x0029F8U) return 1U;
    return 0;
}

BlockExit execute_0x0029F8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029F8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029F8U) {
    // guest 0x0029F8 opcode 0x4E71 4E71 nop
    const auto opcode_0x0029F8 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029F8);

    api.finish_instruction(opcode_0x0029F8);
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

unsigned instruction_count_from_0x0029FA(unsigned entry_pc) {
    if (entry_pc == 0x0029FAU) return 1U;
    return 0;
}

BlockExit execute_0x0029FA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029FAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029FAU) {
    // guest 0x0029FA opcode 0x4E71 4E71 nop
    const auto opcode_0x0029FA = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029FA);

    api.finish_instruction(opcode_0x0029FA);
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

unsigned instruction_count_from_0x0029FC(unsigned entry_pc) {
    if (entry_pc == 0x0029FCU) return 1U;
    return 0;
}

BlockExit execute_0x0029FC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0029FCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0029FCU) {
    // guest 0x0029FC opcode 0x4E71 4E71 nop
    const auto opcode_0x0029FC = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x0029FC);

    api.finish_instruction(opcode_0x0029FC);
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
