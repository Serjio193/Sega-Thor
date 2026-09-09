// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x061A0E(unsigned entry_pc) {
    if (entry_pc == 0x061A0EU) return 1U;
    return 0;
}

BlockExit execute_0x061A0E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A0EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A0EU) {
    // guest 0x061A0E opcode 0x6600 6600 000A bne.w loc_061A1A
    const auto opcode_0x061A0E = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061A0E);
    branch_condition(api, 6U, 0x061A1AU, 14, 0x000AU);
    api.finish_instruction(opcode_0x061A0E);
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

unsigned instruction_count_from_0x061A20(unsigned entry_pc) {
    if (entry_pc == 0x061A20U) return 1U;
    return 0;
}

BlockExit execute_0x061A20(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A20U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A20U) {
    // guest 0x061A20 opcode 0xD040 D040 add.w D0,D0
    const auto opcode_0x061A20 = fetch_checked(api, 0xD040U);
    api.begin_instruction(opcode_0x061A20);
    add_w_data_to_data(api, 0U, 0U);
    api.finish_instruction(opcode_0x061A20);
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

unsigned instruction_count_from_0x061A40(unsigned entry_pc) {
    if (entry_pc == 0x061A40U) return 1U;
    return 0;
}

BlockExit execute_0x061A40(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A40U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A40U) {
    // guest 0x061A40 opcode 0x6200 6200 0006 bhi.w loc_061A48
    const auto opcode_0x061A40 = fetch_checked(api, 0x6200U);
    api.begin_instruction(opcode_0x061A40);
    branch_condition(api, 2U, 0x061A48U, 14, 0x0006U);
    api.finish_instruction(opcode_0x061A40);
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

unsigned instruction_count_from_0x061A5A(unsigned entry_pc) {
    if (entry_pc == 0x061A5AU) return 1U;
    return 0;
}

BlockExit execute_0x061A5A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A5AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A5AU) {
    // guest 0x061A5A opcode 0x6700 6700 004E beq.w loc_061AAA
    const auto opcode_0x061A5A = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061A5A);
    branch_condition(api, 7U, 0x061AAAU, 14, 0x004EU);
    api.finish_instruction(opcode_0x061A5A);
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

unsigned instruction_count_from_0x061A6C(unsigned entry_pc) {
    if (entry_pc == 0x061A6CU) return 1U;
    return 0;
}

BlockExit execute_0x061A6C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A6CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A6CU) {
    // guest 0x061A6C opcode 0x6600 6600 0034 bne.w loc_061AA2
    const auto opcode_0x061A6C = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061A6C);
    branch_condition(api, 6U, 0x061AA2U, 14, 0x0034U);
    api.finish_instruction(opcode_0x061A6C);
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

unsigned instruction_count_from_0x061A76(unsigned entry_pc) {
    if (entry_pc == 0x061A76U) return 1U;
    return 0;
}

BlockExit execute_0x061A76(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A76U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A76U) {
    // guest 0x061A76 opcode 0x6600 6600 000C bne.w loc_061A84
    const auto opcode_0x061A76 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061A76);
    branch_condition(api, 6U, 0x061A84U, 14, 0x000CU);
    api.finish_instruction(opcode_0x061A76);
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

unsigned instruction_count_from_0x061A80(unsigned entry_pc) {
    if (entry_pc == 0x061A80U) return 1U;
    return 0;
}

BlockExit execute_0x061A80(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A80U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A80U) {
    // guest 0x061A80 opcode 0x6600 6600 0014 bne.w loc_061A96
    const auto opcode_0x061A80 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061A80);
    branch_condition(api, 6U, 0x061A96U, 14, 0x0014U);
    api.finish_instruction(opcode_0x061A80);
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

unsigned instruction_count_from_0x061A8A(unsigned entry_pc) {
    if (entry_pc == 0x061A8AU) return 1U;
    return 0;
}

BlockExit execute_0x061A8A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061A8AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061A8AU) {
    // guest 0x061A8A opcode 0x6600 6600 000A bne.w loc_061A96
    const auto opcode_0x061A8A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061A8A);
    branch_condition(api, 6U, 0x061A96U, 14, 0x000AU);
    api.finish_instruction(opcode_0x061A8A);
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

unsigned instruction_count_from_0x061AB0(unsigned entry_pc) {
    if (entry_pc == 0x061AB0U) return 1U;
    return 0;
}

BlockExit execute_0x061AB0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061AB0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061AB0U) {
    // guest 0x061AB0 opcode 0x6700 6700 0008 beq.w loc_061ABA
    const auto opcode_0x061AB0 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061AB0);
    branch_condition(api, 7U, 0x061ABAU, 14, 0x0008U);
    api.finish_instruction(opcode_0x061AB0);
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

unsigned instruction_count_from_0x061AD0(unsigned entry_pc) {
    if (entry_pc == 0x061AD0U) return 1U;
    return 0;
}

BlockExit execute_0x061AD0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061AD0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061AD0U) {
    // guest 0x061AD0 opcode 0x6700 6700 0036 beq.w loc_061B08
    const auto opcode_0x061AD0 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061AD0);
    branch_condition(api, 7U, 0x061B08U, 14, 0x0036U);
    api.finish_instruction(opcode_0x061AD0);
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

unsigned instruction_count_from_0x061B0E(unsigned entry_pc) {
    if (entry_pc == 0x061B0EU) return 1U;
    return 0;
}

BlockExit execute_0x061B0E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061B0EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061B0EU) {
    // guest 0x061B0E opcode 0x6700 6700 0064 beq.w loc_061B74
    const auto opcode_0x061B0E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061B0E);
    branch_condition(api, 7U, 0x061B74U, 14, 0x0064U);
    api.finish_instruction(opcode_0x061B0E);
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

unsigned instruction_count_from_0x061B7A(unsigned entry_pc) {
    if (entry_pc == 0x061B7AU) return 1U;
    return 0;
}

BlockExit execute_0x061B7A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061B7AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061B7AU) {
    // guest 0x061B7A opcode 0x6600 6600 0172 bne.w loc_061CEE
    const auto opcode_0x061B7A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061B7A);
    branch_condition(api, 6U, 0x061CEEU, 14, 0x0172U);
    api.finish_instruction(opcode_0x061B7A);
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

unsigned instruction_count_from_0x061B84(unsigned entry_pc) {
    if (entry_pc == 0x061B84U) return 1U;
    return 0;
}

BlockExit execute_0x061B84(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061B84U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061B84U) {
    // guest 0x061B84 opcode 0x6600 6600 0258 bne.w loc_061DDE
    const auto opcode_0x061B84 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061B84);
    branch_condition(api, 6U, 0x061DDEU, 14, 0x0258U);
    api.finish_instruction(opcode_0x061B84);
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

unsigned instruction_count_from_0x061B8E(unsigned entry_pc) {
    if (entry_pc == 0x061B8EU) return 1U;
    return 0;
}

BlockExit execute_0x061B8E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061B8EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061B8EU) {
    // guest 0x061B8E opcode 0x6600 6600 01F6 bne.w loc_061D86
    const auto opcode_0x061B8E = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061B8E);
    branch_condition(api, 6U, 0x061D86U, 14, 0x01F6U);
    api.finish_instruction(opcode_0x061B8E);
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

unsigned instruction_count_from_0x061B98(unsigned entry_pc) {
    if (entry_pc == 0x061B98U) return 1U;
    return 0;
}

BlockExit execute_0x061B98(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061B98U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061B98U) {
    // guest 0x061B98 opcode 0x6600 6600 0346 bne.w loc_061EE0
    const auto opcode_0x061B98 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061B98);
    branch_condition(api, 6U, 0x061EE0U, 14, 0x0346U);
    api.finish_instruction(opcode_0x061B98);
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

unsigned instruction_count_from_0x061BA2(unsigned entry_pc) {
    if (entry_pc == 0x061BA2U) return 1U;
    return 0;
}

BlockExit execute_0x061BA2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061BA2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061BA2U) {
    // guest 0x061BA2 opcode 0x6600 6600 027C bne.w loc_061E20
    const auto opcode_0x061BA2 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061BA2);
    branch_condition(api, 6U, 0x061E20U, 14, 0x027CU);
    api.finish_instruction(opcode_0x061BA2);
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

unsigned instruction_count_from_0x061BBC(unsigned entry_pc) {
    if (entry_pc == 0x061BBCU) return 1U;
    return 0;
}

BlockExit execute_0x061BBC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061BBCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061BBCU) {
    // guest 0x061BBC opcode 0x6600 6600 00B6 bne.w loc_061C74
    const auto opcode_0x061BBC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061BBC);
    branch_condition(api, 6U, 0x061C74U, 14, 0x00B6U);
    api.finish_instruction(opcode_0x061BBC);
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

unsigned instruction_count_from_0x061BC4(unsigned entry_pc) {
    if (entry_pc == 0x061BC4U) return 1U;
    return 0;
}

BlockExit execute_0x061BC4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061BC4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061BC4U) {
    // guest 0x061BC4 opcode 0x6700 6700 0090 beq.w loc_061C56
    const auto opcode_0x061BC4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061BC4);
    branch_condition(api, 7U, 0x061C56U, 14, 0x0090U);
    api.finish_instruction(opcode_0x061BC4);
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
