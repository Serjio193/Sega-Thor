// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x062768(unsigned entry_pc) {
    if (entry_pc == 0x062768U) return 1U;
    return 0;
}

BlockExit execute_0x062768(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062768U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062768U) {
    // guest 0x062768 opcode 0x6500 6500 0004 bcs.w loc_06276E
    const auto opcode_0x062768 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x062768);
    branch_condition(api, 5U, 0x06276EU, 14, 0x0004U);
    api.finish_instruction(opcode_0x062768);
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

unsigned instruction_count_from_0x06278E(unsigned entry_pc) {
    if (entry_pc == 0x06278EU) return 1U;
    return 0;
}

BlockExit execute_0x06278E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06278EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06278EU) {
    // guest 0x06278E opcode 0x6700 6700 0028 beq.w loc_0627B8
    const auto opcode_0x06278E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06278E);
    branch_condition(api, 7U, 0x0627B8U, 14, 0x0028U);
    api.finish_instruction(opcode_0x06278E);
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

unsigned instruction_count_from_0x06279C(unsigned entry_pc) {
    if (entry_pc == 0x06279CU) return 1U;
    return 0;
}

BlockExit execute_0x06279C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06279CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06279CU) {
    // guest 0x06279C opcode 0x6400 6400 0006 bcc.w loc_0627A4
    const auto opcode_0x06279C = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x06279C);
    branch_condition(api, 4U, 0x0627A4U, 14, 0x0006U);
    api.finish_instruction(opcode_0x06279C);
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

unsigned instruction_count_from_0x0627AC(unsigned entry_pc) {
    if (entry_pc == 0x0627ACU) return 1U;
    return 0;
}

BlockExit execute_0x0627AC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0627ACU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0627ACU) {
    // guest 0x0627AC opcode 0x6600 6600 0080 bne.w loc_06282E
    const auto opcode_0x0627AC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0627AC);
    branch_condition(api, 6U, 0x06282EU, 14, 0x0080U);
    api.finish_instruction(opcode_0x0627AC);
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

unsigned instruction_count_from_0x0627BE(unsigned entry_pc) {
    if (entry_pc == 0x0627BEU) return 1U;
    return 0;
}

BlockExit execute_0x0627BE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0627BEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0627BEU) {
    // guest 0x0627BE opcode 0x6700 6700 002E beq.w loc_0627EE
    const auto opcode_0x0627BE = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0627BE);
    branch_condition(api, 7U, 0x0627EEU, 14, 0x002EU);
    api.finish_instruction(opcode_0x0627BE);
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

unsigned instruction_count_from_0x0627D0(unsigned entry_pc) {
    if (entry_pc == 0x0627D0U) return 1U;
    return 0;
}

BlockExit execute_0x0627D0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0627D0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0627D0U) {
    // guest 0x0627D0 opcode 0x6500 6500 0008 bcs.w loc_0627DA
    const auto opcode_0x0627D0 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x0627D0);
    branch_condition(api, 5U, 0x0627DAU, 14, 0x0008U);
    api.finish_instruction(opcode_0x0627D0);
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

unsigned instruction_count_from_0x0627D6(unsigned entry_pc) {
    if (entry_pc == 0x0627D6U) return 1U;
    return 0;
}

BlockExit execute_0x0627D6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0627D6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0627D6U) {
    // guest 0x0627D6 opcode 0x6400 6400 0004 bcc.w loc_0627DC
    const auto opcode_0x0627D6 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x0627D6);
    branch_condition(api, 4U, 0x0627DCU, 14, 0x0004U);
    api.finish_instruction(opcode_0x0627D6);
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

unsigned instruction_count_from_0x0627E2(unsigned entry_pc) {
    if (entry_pc == 0x0627E2U) return 1U;
    return 0;
}

BlockExit execute_0x0627E2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0627E2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0627E2U) {
    // guest 0x0627E2 opcode 0x6600 6600 004A bne.w loc_06282E
    const auto opcode_0x0627E2 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0627E2);
    branch_condition(api, 6U, 0x06282EU, 14, 0x004AU);
    api.finish_instruction(opcode_0x0627E2);
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

unsigned instruction_count_from_0x0627F4(unsigned entry_pc) {
    if (entry_pc == 0x0627F4U) return 1U;
    return 0;
}

BlockExit execute_0x0627F4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0627F4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0627F4U) {
    // guest 0x0627F4 opcode 0x6700 6700 0024 beq.w loc_06281A
    const auto opcode_0x0627F4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0627F4);
    branch_condition(api, 7U, 0x06281AU, 14, 0x0024U);
    api.finish_instruction(opcode_0x0627F4);
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

unsigned instruction_count_from_0x062802(unsigned entry_pc) {
    if (entry_pc == 0x062802U) return 1U;
    return 0;
}

BlockExit execute_0x062802(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062802U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062802U) {
    // guest 0x062802 opcode 0x6400 6400 0004 bcc.w loc_062808
    const auto opcode_0x062802 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x062802);
    branch_condition(api, 4U, 0x062808U, 14, 0x0004U);
    api.finish_instruction(opcode_0x062802);
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

unsigned instruction_count_from_0x06280E(unsigned entry_pc) {
    if (entry_pc == 0x06280EU) return 1U;
    return 0;
}

BlockExit execute_0x06280E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06280EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06280EU) {
    // guest 0x06280E opcode 0x6600 6600 001E bne.w loc_06282E
    const auto opcode_0x06280E = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x06280E);
    branch_condition(api, 6U, 0x06282EU, 14, 0x001EU);
    api.finish_instruction(opcode_0x06280E);
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

unsigned instruction_count_from_0x062824(unsigned entry_pc) {
    if (entry_pc == 0x062824U) return 1U;
    return 0;
}

BlockExit execute_0x062824(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062824U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062824U) {
    // guest 0x062824 opcode 0x6400 6400 0004 bcc.w loc_06282A
    const auto opcode_0x062824 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x062824);
    branch_condition(api, 4U, 0x06282AU, 14, 0x0004U);
    api.finish_instruction(opcode_0x062824);
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

unsigned instruction_count_from_0x062828(unsigned entry_pc) {
    if (entry_pc == 0x062828U) return 1U;
    return 0;
}

BlockExit execute_0x062828(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062828U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062828U) {
    // guest 0x062828 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x062828 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x062828);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x062828);
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

unsigned instruction_count_from_0x06282E(unsigned entry_pc) {
    if (entry_pc == 0x06282EU) return 1U;
    return 0;
}

BlockExit execute_0x06282E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06282EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06282EU) {
    // guest 0x06282E opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x06282E = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x06282E);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x06282E);
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

unsigned instruction_count_from_0x062846(unsigned entry_pc) {
    if (entry_pc == 0x062846U) return 1U;
    return 0;
}

BlockExit execute_0x062846(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062846U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062846U) {
    // guest 0x062846 opcode 0x6600 6600 0012 bne.w loc_06285A
    const auto opcode_0x062846 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062846);
    branch_condition(api, 6U, 0x06285AU, 14, 0x0012U);
    api.finish_instruction(opcode_0x062846);
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

unsigned instruction_count_from_0x062850(unsigned entry_pc) {
    if (entry_pc == 0x062850U) return 1U;
    return 0;
}

BlockExit execute_0x062850(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062850U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062850U) {
    // guest 0x062850 opcode 0x6700 6700 0008 beq.w loc_06285A
    const auto opcode_0x062850 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062850);
    branch_condition(api, 7U, 0x06285AU, 14, 0x0008U);
    api.finish_instruction(opcode_0x062850);
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

unsigned instruction_count_from_0x062882(unsigned entry_pc) {
    if (entry_pc == 0x062882U) return 1U;
    return 0;
}

BlockExit execute_0x062882(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062882U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062882U) {
    // guest 0x062882 opcode 0x6600 6600 000A bne.w loc_06288E
    const auto opcode_0x062882 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062882);
    branch_condition(api, 6U, 0x06288EU, 14, 0x000AU);
    api.finish_instruction(opcode_0x062882);
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

unsigned instruction_count_from_0x062900(unsigned entry_pc) {
    if (entry_pc == 0x062900U) return 1U;
    return 0;
}

BlockExit execute_0x062900(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062900U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062900U) {
    // guest 0x062900 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x062900 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x062900);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x062900);
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
