// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x03B7EA(unsigned entry_pc) {
    if (entry_pc == 0x03B7EAU) return 1U;
    return 0;
}

BlockExit execute_0x03B7EA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7EAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7EAU) {
    // guest 0x03B7EA opcode 0x3002 3002 move.w D2,D0
    const auto opcode_0x03B7EA = fetch_checked(api, 0x3002U);
    api.begin_instruction(opcode_0x03B7EA);
    move_w_data_to_data(api, 2U, 0U);
    api.finish_instruction(opcode_0x03B7EA);
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

unsigned instruction_count_from_0x03B7EC(unsigned entry_pc) {
    if (entry_pc == 0x03B7ECU) return 1U;
    return 0;
}

BlockExit execute_0x03B7EC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7ECU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7ECU) {
    // guest 0x03B7EC opcode 0x0240 0240 00E0 andi.w #$E0,D0
    const auto opcode_0x03B7EC = fetch_checked(api, 0x0240U);
    api.begin_instruction(opcode_0x03B7EC);
    (void)fetch_checked(api, 0x00E0U);
    andi_w_data(api, 224U, 0U);
    api.finish_instruction(opcode_0x03B7EC);
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

unsigned instruction_count_from_0x03B7F0(unsigned entry_pc) {
    if (entry_pc == 0x03B7F0U) return 1U;
    return 0;
}

BlockExit execute_0x03B7F0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7F0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7F0U) {
    // guest 0x03B7F0 opcode 0x3604 3604 move.w D4,D3
    const auto opcode_0x03B7F0 = fetch_checked(api, 0x3604U);
    api.begin_instruction(opcode_0x03B7F0);
    move_w_data_to_data(api, 4U, 3U);
    api.finish_instruction(opcode_0x03B7F0);
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

unsigned instruction_count_from_0x03B7F6(unsigned entry_pc) {
    if (entry_pc == 0x03B7F6U) return 1U;
    return 0;
}

BlockExit execute_0x03B7F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7F6U) {
    // guest 0x03B7F6 opcode 0x9640 9640 sub.w D0,D3
    const auto opcode_0x03B7F6 = fetch_checked(api, 0x9640U);
    api.begin_instruction(opcode_0x03B7F6);
    sub_w_data_to_data(api, 0U, 3U);
    api.finish_instruction(opcode_0x03B7F6);
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

unsigned instruction_count_from_0x03B7FE(unsigned entry_pc) {
    if (entry_pc == 0x03B7FEU) return 1U;
    return 0;
}

BlockExit execute_0x03B7FE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7FEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7FEU) {
    // guest 0x03B7FE opcode 0xD043 D043 add.w D3,D0
    const auto opcode_0x03B7FE = fetch_checked(api, 0xD043U);
    api.begin_instruction(opcode_0x03B7FE);
    add_w_data_to_data(api, 3U, 0U);
    api.finish_instruction(opcode_0x03B7FE);
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

unsigned instruction_count_from_0x03B800(unsigned entry_pc) {
    if (entry_pc == 0x03B800U) return 1U;
    return 0;
}

BlockExit execute_0x03B800(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B800U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B800U) {
    // guest 0x03B800 opcode 0x0240 0240 00E0 andi.w #$E0,D0
    const auto opcode_0x03B800 = fetch_checked(api, 0x0240U);
    api.begin_instruction(opcode_0x03B800);
    (void)fetch_checked(api, 0x00E0U);
    andi_w_data(api, 224U, 0U);
    api.finish_instruction(opcode_0x03B800);
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

unsigned instruction_count_from_0x03B804(unsigned entry_pc) {
    if (entry_pc == 0x03B804U) return 1U;
    return 0;
}

BlockExit execute_0x03B804(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B804U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B804U) {
    // guest 0x03B804 opcode 0x8240 8240 or.w D0,D1
    const auto opcode_0x03B804 = fetch_checked(api, 0x8240U);
    api.begin_instruction(opcode_0x03B804);
    or_w_data_to_data(api, 0U, 1U);
    api.finish_instruction(opcode_0x03B804);
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

unsigned instruction_count_from_0x03B806(unsigned entry_pc) {
    if (entry_pc == 0x03B806U) return 1U;
    return 0;
}

BlockExit execute_0x03B806(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B806U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B806U) {
    // guest 0x03B806 opcode 0x3002 3002 move.w D2,D0
    const auto opcode_0x03B806 = fetch_checked(api, 0x3002U);
    api.begin_instruction(opcode_0x03B806);
    move_w_data_to_data(api, 2U, 0U);
    api.finish_instruction(opcode_0x03B806);
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

unsigned instruction_count_from_0x03B808(unsigned entry_pc) {
    if (entry_pc == 0x03B808U) return 1U;
    return 0;
}

BlockExit execute_0x03B808(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B808U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B808U) {
    // guest 0x03B808 opcode 0x0240 0240 0E00 andi.w #$E00,D0
    const auto opcode_0x03B808 = fetch_checked(api, 0x0240U);
    api.begin_instruction(opcode_0x03B808);
    (void)fetch_checked(api, 0x0E00U);
    andi_w_data(api, 3584U, 0U);
    api.finish_instruction(opcode_0x03B808);
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

unsigned instruction_count_from_0x03B80C(unsigned entry_pc) {
    if (entry_pc == 0x03B80CU) return 1U;
    return 0;
}

BlockExit execute_0x03B80C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B80CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B80CU) {
    // guest 0x03B80C opcode 0x3604 3604 move.w D4,D3
    const auto opcode_0x03B80C = fetch_checked(api, 0x3604U);
    api.begin_instruction(opcode_0x03B80C);
    move_w_data_to_data(api, 4U, 3U);
    api.finish_instruction(opcode_0x03B80C);
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

unsigned instruction_count_from_0x03B812(unsigned entry_pc) {
    if (entry_pc == 0x03B812U) return 1U;
    return 0;
}

BlockExit execute_0x03B812(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B812U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B812U) {
    // guest 0x03B812 opcode 0x9640 9640 sub.w D0,D3
    const auto opcode_0x03B812 = fetch_checked(api, 0x9640U);
    api.begin_instruction(opcode_0x03B812);
    sub_w_data_to_data(api, 0U, 3U);
    api.finish_instruction(opcode_0x03B812);
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

unsigned instruction_count_from_0x03B81A(unsigned entry_pc) {
    if (entry_pc == 0x03B81AU) return 1U;
    return 0;
}

BlockExit execute_0x03B81A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B81AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B81AU) {
    // guest 0x03B81A opcode 0xD043 D043 add.w D3,D0
    const auto opcode_0x03B81A = fetch_checked(api, 0xD043U);
    api.begin_instruction(opcode_0x03B81A);
    add_w_data_to_data(api, 3U, 0U);
    api.finish_instruction(opcode_0x03B81A);
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

unsigned instruction_count_from_0x03B81C(unsigned entry_pc) {
    if (entry_pc == 0x03B81CU) return 1U;
    return 0;
}

BlockExit execute_0x03B81C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B81CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B81CU) {
    // guest 0x03B81C opcode 0x0240 0240 0E00 andi.w #$E00,D0
    const auto opcode_0x03B81C = fetch_checked(api, 0x0240U);
    api.begin_instruction(opcode_0x03B81C);
    (void)fetch_checked(api, 0x0E00U);
    andi_w_data(api, 3584U, 0U);
    api.finish_instruction(opcode_0x03B81C);
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

unsigned instruction_count_from_0x03B820(unsigned entry_pc) {
    if (entry_pc == 0x03B820U) return 1U;
    return 0;
}

BlockExit execute_0x03B820(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B820U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B820U) {
    // guest 0x03B820 opcode 0x8240 8240 or.w D0,D1
    const auto opcode_0x03B820 = fetch_checked(api, 0x8240U);
    api.begin_instruction(opcode_0x03B820);
    or_w_data_to_data(api, 0U, 1U);
    api.finish_instruction(opcode_0x03B820);
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

unsigned instruction_count_from_0x03B824(unsigned entry_pc) {
    if (entry_pc == 0x03B824U) return 1U;
    return 0;
}

BlockExit execute_0x03B824(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B824U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B824U) {
    // guest 0x03B824 opcode 0x51CF 51CF FFA6 dbf D7,loc_03B7CC
    const auto opcode_0x03B824 = fetch_checked(api, 0x51CFU);
    api.begin_instruction(opcode_0x03B824);
    dbcc(api, 1U, 7U, 0x03B7CCU, 0xFFA6U);
    api.finish_instruction(opcode_0x03B824);
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

unsigned instruction_count_from_0x06009A(unsigned entry_pc) {
    if (entry_pc == 0x06009AU) return 1U;
    return 0;
}

BlockExit execute_0x06009A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06009AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06009AU) {
    // guest 0x06009A opcode 0x4A39 4A39 00FF 0010 tst.b ($00FF0010).L
    const auto opcode_0x06009A = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x06009A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0010U);
    test_absolute_long(api, 0xFF0010U, 1U);
    api.finish_instruction(opcode_0x06009A);
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

unsigned instruction_count_from_0x0600A0(unsigned entry_pc) {
    if (entry_pc == 0x0600A0U) return 1U;
    return 0;
}

BlockExit execute_0x0600A0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0600A0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0600A0U) {
    // guest 0x0600A0 opcode 0x6700 6700 006E beq.w loc_060110
    const auto opcode_0x0600A0 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0600A0);
    branch_condition(api, 7U, 0x060110U, 14, 0x006EU);
    api.finish_instruction(opcode_0x0600A0);
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
