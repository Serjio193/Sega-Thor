// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x003784(unsigned entry_pc) {
    if (entry_pc == 0x003784U) return 1U;
    return 0;
}

BlockExit execute_0x003784(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003784U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003784U) {
    // guest 0x003784 opcode 0x43F9 43F9 00FF 2FA8 lea.l ($00FF2FA8).L,A1
    const auto opcode_0x003784 = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x003784);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x2FA8U);
    lea_absolute_long(api, 0xFF2FA8U, 1U);
    api.finish_instruction(opcode_0x003784);
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

unsigned instruction_count_from_0x003798(unsigned entry_pc) {
    if (entry_pc == 0x003798U) return 1U;
    return 0;
}

BlockExit execute_0x003798(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003798U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003798U) {
    // guest 0x003798 opcode 0x41F9 41F9 00FF 1840 lea.l ($00FF1840).L,A0
    const auto opcode_0x003798 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x003798);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1840U);
    lea_absolute_long(api, 0xFF1840U, 0U);
    api.finish_instruction(opcode_0x003798);
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

unsigned instruction_count_from_0x003806(unsigned entry_pc) {
    if (entry_pc == 0x003806U) return 1U;
    return 0;
}

BlockExit execute_0x003806(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003806U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003806U) {
    // guest 0x003806 opcode 0x66F6 66F6 bne.s loc_0037FE
    const auto opcode_0x003806 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003806);
    branch_condition(api, 6U, 0x0037FEU, -14);
    api.finish_instruction(opcode_0x003806);
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

unsigned instruction_count_from_0x003810(unsigned entry_pc) {
    if (entry_pc == 0x003810U) return 1U;
    return 0;
}

BlockExit execute_0x003810(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003810U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003810U) {
    // guest 0x003810 opcode 0x0839 0839 0001 00FF 164E btst.b #$1,($00FF164E).L
    const auto opcode_0x003810 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x003810);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164EU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164EU);
    api.finish_instruction(opcode_0x003810);
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

unsigned instruction_count_from_0x003828(unsigned entry_pc) {
    if (entry_pc == 0x003828U) return 1U;
    return 0;
}

BlockExit execute_0x003828(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003828U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003828U) {
    // guest 0x003828 opcode 0x6700 6700 00A6 beq.w loc_0038D0
    const auto opcode_0x003828 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x003828);
    branch_condition(api, 7U, 0x0038D0U, 14, 0x00A6U);
    api.finish_instruction(opcode_0x003828);
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

unsigned instruction_count_from_0x003838(unsigned entry_pc) {
    if (entry_pc == 0x003838U) return 1U;
    return 0;
}

BlockExit execute_0x003838(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003838U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003838U) {
    // guest 0x003838 opcode 0xD481 D481 add.l D1,D2
    const auto opcode_0x003838 = fetch_checked(api, 0xD481U);
    api.begin_instruction(opcode_0x003838);
    add_l_data_to_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x003838);
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

unsigned instruction_count_from_0x00383C(unsigned entry_pc) {
    if (entry_pc == 0x00383CU) return 1U;
    return 0;
}

BlockExit execute_0x00383C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00383CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00383CU) {
    // guest 0x00383C opcode 0x6700 6700 0086 beq.w loc_0038C4
    const auto opcode_0x00383C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x00383C);
    branch_condition(api, 7U, 0x0038C4U, 14, 0x0086U);
    api.finish_instruction(opcode_0x00383C);
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

unsigned instruction_count_from_0x003840(unsigned entry_pc) {
    if (entry_pc == 0x003840U) return 1U;
    return 0;
}

BlockExit execute_0x003840(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003840U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003840U) {
    // guest 0x003840 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x003840 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x003840);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x003840);
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

unsigned instruction_count_from_0x003844(unsigned entry_pc) {
    if (entry_pc == 0x003844U) return 1U;
    return 0;
}

BlockExit execute_0x003844(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003844U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003844U) {
    // guest 0x003844 opcode 0x0880 0880 0007 bclr.l #$7,D0
    const auto opcode_0x003844 = fetch_checked(api, 0x0880U);
    api.begin_instruction(opcode_0x003844);
    (void)fetch_checked(api, 0x0007U);
    bclr_l_data(api, 7U, 0U);
    api.finish_instruction(opcode_0x003844);
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

unsigned instruction_count_from_0x003848(unsigned entry_pc) {
    if (entry_pc == 0x003848U) return 1U;
    return 0;
}

BlockExit execute_0x003848(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003848U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003848U) {
    // guest 0x003848 opcode 0x6600 6600 003C bne.w loc_003886
    const auto opcode_0x003848 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x003848);
    branch_condition(api, 6U, 0x003886U, 14, 0x003CU);
    api.finish_instruction(opcode_0x003848);
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

unsigned instruction_count_from_0x003850(unsigned entry_pc) {
    if (entry_pc == 0x003850U) return 1U;
    return 0;
}

BlockExit execute_0x003850(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003850U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003850U) {
    // guest 0x003850 opcode 0x6600 6600 001A bne.w loc_00386C
    const auto opcode_0x003850 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x003850);
    branch_condition(api, 6U, 0x00386CU, 14, 0x001AU);
    api.finish_instruction(opcode_0x003850);
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

unsigned instruction_count_from_0x003858(unsigned entry_pc) {
    if (entry_pc == 0x003858U) return 1U;
    return 0;
}

BlockExit execute_0x003858(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003858U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003858U) {
    // guest 0x003858 opcode 0x6700 6700 0006 beq.w loc_003860
    const auto opcode_0x003858 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x003858);
    branch_condition(api, 7U, 0x003860U, 14, 0x0006U);
    api.finish_instruction(opcode_0x003858);
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

unsigned instruction_count_from_0x003864(unsigned entry_pc) {
    if (entry_pc == 0x003864U) return 1U;
    return 0;
}

BlockExit execute_0x003864(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003864U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003864U) {
    // guest 0x003864 opcode 0x51C8 51C8 FFFC dbf D0,loc_003862
    const auto opcode_0x003864 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x003864);
    dbcc(api, 1U, 0U, 0x003862U, 0xFFFCU);
    api.finish_instruction(opcode_0x003864);
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

unsigned instruction_count_from_0x003870(unsigned entry_pc) {
    if (entry_pc == 0x003870U) return 1U;
    return 0;
}

BlockExit execute_0x003870(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003870U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003870U) {
    // guest 0x003870 opcode 0x6700 6700 0006 beq.w loc_003878
    const auto opcode_0x003870 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x003870);
    branch_condition(api, 7U, 0x003878U, 14, 0x0006U);
    api.finish_instruction(opcode_0x003870);
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

unsigned instruction_count_from_0x00387E(unsigned entry_pc) {
    if (entry_pc == 0x00387EU) return 1U;
    return 0;
}

BlockExit execute_0x00387E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00387EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00387EU) {
    // guest 0x00387E opcode 0x51C8 51C8 FFFC dbf D0,loc_00387C
    const auto opcode_0x00387E = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x00387E);
    dbcc(api, 1U, 0U, 0x00387CU, 0xFFFCU);
    api.finish_instruction(opcode_0x00387E);
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

unsigned instruction_count_from_0x00389A(unsigned entry_pc) {
    if (entry_pc == 0x00389AU) return 1U;
    return 0;
}

BlockExit execute_0x00389A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00389AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00389AU) {
    // guest 0x00389A opcode 0x2449 2449 movea.l A1,A2
    const auto opcode_0x00389A = fetch_checked(api, 0x2449U);
    api.begin_instruction(opcode_0x00389A);
    movea_l_address_to_address(api, 1U, 2U);
    api.finish_instruction(opcode_0x00389A);
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

unsigned instruction_count_from_0x0038A6(unsigned entry_pc) {
    if (entry_pc == 0x0038A6U) return 1U;
    return 0;
}

BlockExit execute_0x0038A6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038A6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038A6U) {
    // guest 0x0038A6 opcode 0x0201 0201 00E0 andi.b #$E0,D1
    const auto opcode_0x0038A6 = fetch_checked(api, 0x0201U);
    api.begin_instruction(opcode_0x0038A6);
    (void)fetch_checked(api, 0x00E0U);
    andi_b_data(api, 224U, 1U);
    api.finish_instruction(opcode_0x0038A6);
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
