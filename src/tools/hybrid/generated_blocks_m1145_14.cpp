// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x03A7D2(unsigned entry_pc) {
    if (entry_pc == 0x03A7D2U) return 1U;
    return 0;
}

BlockExit execute_0x03A7D2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7D2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7D2U) {
    // guest 0x03A7D2 opcode 0x47F9 47F9 0017 0000 lea.l ($00170000).L,A3
    const auto opcode_0x03A7D2 = fetch_checked(api, 0x47F9U);
    api.begin_instruction(opcode_0x03A7D2);
    (void)fetch_checked(api, 0x0017U);
    (void)fetch_checked(api, 0x0000U);
    lea_absolute_long(api, 0x170000U, 3U);
    api.finish_instruction(opcode_0x03A7D2);
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

unsigned instruction_count_from_0x03A7DA(unsigned entry_pc) {
    if (entry_pc == 0x03A7DAU) return 1U;
    return 0;
}

BlockExit execute_0x03A7DA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7DAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7DAU) {
    // guest 0x03A7DA opcode 0x4BF9 4BF9 0003 B94E lea.l ($0003B94E).L,A5
    const auto opcode_0x03A7DA = fetch_checked(api, 0x4BF9U);
    api.begin_instruction(opcode_0x03A7DA);
    (void)fetch_checked(api, 0x0003U);
    (void)fetch_checked(api, 0xB94EU);
    lea_absolute_long(api, 0x03B94EU, 5U);
    api.finish_instruction(opcode_0x03A7DA);
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

unsigned instruction_count_from_0x03A7E4(unsigned entry_pc) {
    if (entry_pc == 0x03A7E4U) return 1U;
    return 0;
}

BlockExit execute_0x03A7E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7E4U) {
    // guest 0x03A7E4 opcode 0x0839 0839 0001 00FF 164E btst.b #$1,($00FF164E).L
    const auto opcode_0x03A7E4 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x03A7E4);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164EU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164EU);
    api.finish_instruction(opcode_0x03A7E4);
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

unsigned instruction_count_from_0x03A7EC(unsigned entry_pc) {
    if (entry_pc == 0x03A7ECU) return 1U;
    return 0;
}

BlockExit execute_0x03A7EC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7ECU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7ECU) {
    // guest 0x03A7EC opcode 0x6600 6600 FFF6 bne.w loc_03A7E4
    const auto opcode_0x03A7EC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A7EC);
    branch_condition(api, 6U, 0x03A7E4U, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x03A7EC);
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

unsigned instruction_count_from_0x03A7F0(unsigned entry_pc) {
    if (entry_pc == 0x03A7F0U) return 1U;
    return 0;
}

BlockExit execute_0x03A7F0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7F0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7F0U) {
    // guest 0x03A7F0 opcode 0x41F9 41F9 0016 943C lea.l ($0016943C).L,A0
    const auto opcode_0x03A7F0 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x03A7F0);
    (void)fetch_checked(api, 0x0016U);
    (void)fetch_checked(api, 0x943CU);
    lea_absolute_long(api, 0x16943CU, 0U);
    api.finish_instruction(opcode_0x03A7F0);
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

unsigned instruction_count_from_0x03A7F6(unsigned entry_pc) {
    if (entry_pc == 0x03A7F6U) return 1U;
    return 0;
}

BlockExit execute_0x03A7F6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7F6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7F6U) {
    // guest 0x03A7F6 opcode 0x43F9 43F9 00FF 316C lea.l ($00FF316C).L,A1
    const auto opcode_0x03A7F6 = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x03A7F6);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x316CU);
    lea_absolute_long(api, 0xFF316CU, 1U);
    api.finish_instruction(opcode_0x03A7F6);
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

unsigned instruction_count_from_0x03A7FC(unsigned entry_pc) {
    if (entry_pc == 0x03A7FCU) return 1U;
    return 0;
}

BlockExit execute_0x03A7FC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7FCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7FCU) {
    // guest 0x03A7FC opcode 0x2449 2449 movea.l A1,A2
    const auto opcode_0x03A7FC = fetch_checked(api, 0x2449U);
    api.begin_instruction(opcode_0x03A7FC);
    movea_l_address_to_address(api, 1U, 2U);
    api.finish_instruction(opcode_0x03A7FC);
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

unsigned instruction_count_from_0x03A854(unsigned entry_pc) {
    if (entry_pc == 0x03A854U) return 1U;
    return 0;
}

BlockExit execute_0x03A854(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A854U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A854U) {
    // guest 0x03A854 opcode 0x4DF9 4DF9 0003 BD86 lea.l ($0003BD86).L,A6
    const auto opcode_0x03A854 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x03A854);
    (void)fetch_checked(api, 0x0003U);
    (void)fetch_checked(api, 0xBD86U);
    lea_absolute_long(api, 0x03BD86U, 6U);
    api.finish_instruction(opcode_0x03A854);
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

unsigned instruction_count_from_0x03A876(unsigned entry_pc) {
    if (entry_pc == 0x03A876U) return 1U;
    return 0;
}

BlockExit execute_0x03A876(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A876U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A876U) {
    // guest 0x03A876 opcode 0x6A00 6A00 FFCA bpl.w loc_03A842
    const auto opcode_0x03A876 = fetch_checked(api, 0x6A00U);
    api.begin_instruction(opcode_0x03A876);
    branch_condition(api, 10U, 0x03A842U, 14, 0xFFCAU);
    api.finish_instruction(opcode_0x03A876);
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

unsigned instruction_count_from_0x03A892(unsigned entry_pc) {
    if (entry_pc == 0x03A892U) return 1U;
    return 0;
}

BlockExit execute_0x03A892(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A892U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A892U) {
    // guest 0x03A892 opcode 0x6600 6600 0010 bne.w loc_03A8A4
    const auto opcode_0x03A892 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A892);
    branch_condition(api, 6U, 0x03A8A4U, 14, 0x0010U);
    api.finish_instruction(opcode_0x03A892);
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

unsigned instruction_count_from_0x03A8C0(unsigned entry_pc) {
    if (entry_pc == 0x03A8C0U) return 1U;
    return 0;
}

BlockExit execute_0x03A8C0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A8C0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A8C0U) {
    // guest 0x03A8C0 opcode 0x6600 6600 FFCA bne.w loc_03A88C
    const auto opcode_0x03A8C0 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A8C0);
    branch_condition(api, 6U, 0x03A88CU, 14, 0xFFCAU);
    api.finish_instruction(opcode_0x03A8C0);
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

unsigned instruction_count_from_0x03A912(unsigned entry_pc) {
    if (entry_pc == 0x03A912U) return 1U;
    return 0;
}

BlockExit execute_0x03A912(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A912U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A912U) {
    // guest 0x03A912 opcode 0x6600 6600 017E bne.w loc_03AA92
    const auto opcode_0x03A912 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A912);
    branch_condition(api, 6U, 0x03AA92U, 14, 0x017EU);
    api.finish_instruction(opcode_0x03A912);
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

unsigned instruction_count_from_0x03A924(unsigned entry_pc) {
    if (entry_pc == 0x03A924U) return 1U;
    return 0;
}

BlockExit execute_0x03A924(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A924U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A924U) {
    // guest 0x03A924 opcode 0x6600 6600 0060 bne.w loc_03A986
    const auto opcode_0x03A924 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A924);
    branch_condition(api, 6U, 0x03A986U, 14, 0x0060U);
    api.finish_instruction(opcode_0x03A924);
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

unsigned instruction_count_from_0x03B1D4(unsigned entry_pc) {
    if (entry_pc == 0x03B1D4U) return 1U;
    return 0;
}

BlockExit execute_0x03B1D4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B1D4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B1D4U) {
    // guest 0x03B1D4 opcode 0x4DF9 4DF9 00FF AFCE lea.l ($00FFAFCE).L,A6
    const auto opcode_0x03B1D4 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x03B1D4);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0xAFCEU);
    lea_absolute_long(api, 0xFFAFCEU, 6U);
    api.finish_instruction(opcode_0x03B1D4);
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

unsigned instruction_count_from_0x03B1E0(unsigned entry_pc) {
    if (entry_pc == 0x03B1E0U) return 1U;
    return 0;
}

BlockExit execute_0x03B1E0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B1E0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B1E0U) {
    // guest 0x03B1E0 opcode 0x6B00 6B00 0038 bmi.w loc_03B21A
    const auto opcode_0x03B1E0 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x03B1E0);
    branch_condition(api, 11U, 0x03B21AU, 14, 0x0038U);
    api.finish_instruction(opcode_0x03B1E0);
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

unsigned instruction_count_from_0x03B21E(unsigned entry_pc) {
    if (entry_pc == 0x03B21EU) return 1U;
    return 0;
}

BlockExit execute_0x03B21E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B21EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B21EU) {
    // guest 0x03B21E opcode 0x0839 0839 0001 00FF 164E btst.b #$1,($00FF164E).L
    const auto opcode_0x03B21E = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x03B21E);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164EU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164EU);
    api.finish_instruction(opcode_0x03B21E);
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

unsigned instruction_count_from_0x03B226(unsigned entry_pc) {
    if (entry_pc == 0x03B226U) return 1U;
    return 0;
}

BlockExit execute_0x03B226(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B226U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B226U) {
    // guest 0x03B226 opcode 0x6600 6600 FFF6 bne.w loc_03B21E
    const auto opcode_0x03B226 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03B226);
    branch_condition(api, 6U, 0x03B21EU, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x03B226);
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
