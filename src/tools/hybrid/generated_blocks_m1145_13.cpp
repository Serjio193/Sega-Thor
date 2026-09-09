// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0039D2(unsigned entry_pc) {
    if (entry_pc == 0x0039D2U) return 1U;
    return 0;
}

BlockExit execute_0x0039D2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039D2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039D2U) {
    // guest 0x0039D2 opcode 0x6500 6500 000E bcs.w loc_0039E2
    const auto opcode_0x0039D2 = fetch_checked(api, 0x6500U);
    api.begin_instruction(opcode_0x0039D2);
    branch_condition(api, 5U, 0x0039E2U, 14, 0x000EU);
    api.finish_instruction(opcode_0x0039D2);
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

unsigned instruction_count_from_0x0039E2(unsigned entry_pc) {
    if (entry_pc == 0x0039E2U) return 1U;
    return 0;
}

BlockExit execute_0x0039E2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039E2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039E2U) {
    // guest 0x0039E2 opcode 0x4242 4242 clr.w D2
    const auto opcode_0x0039E2 = fetch_checked(api, 0x4242U);
    api.begin_instruction(opcode_0x0039E2);
    clear_w_data_register(api, 2U);
    api.finish_instruction(opcode_0x0039E2);
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

unsigned instruction_count_from_0x0039E4(unsigned entry_pc) {
    if (entry_pc == 0x0039E4U) return 1U;
    return 0;
}

BlockExit execute_0x0039E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039E4U) {
    // guest 0x0039E4 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039E4 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039E4);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039E4);
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

unsigned instruction_count_from_0x0039E6(unsigned entry_pc) {
    if (entry_pc == 0x0039E6U) return 1U;
    return 0;
}

BlockExit execute_0x0039E6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039E6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039E6U) {
    // guest 0x0039E6 opcode 0x6B00 6B00 012C bmi.w loc_003B14
    const auto opcode_0x0039E6 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039E6);
    branch_condition(api, 11U, 0x003B14U, 14, 0x012CU);
    api.finish_instruction(opcode_0x0039E6);
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

unsigned instruction_count_from_0x0039EE(unsigned entry_pc) {
    if (entry_pc == 0x0039EEU) return 1U;
    return 0;
}

BlockExit execute_0x0039EE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039EEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039EEU) {
    // guest 0x0039EE opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039EE = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039EE);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039EE);
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

unsigned instruction_count_from_0x0039F0(unsigned entry_pc) {
    if (entry_pc == 0x0039F0U) return 1U;
    return 0;
}

BlockExit execute_0x0039F0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039F0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039F0U) {
    // guest 0x0039F0 opcode 0x6B00 6B00 0130 bmi.w loc_003B22
    const auto opcode_0x0039F0 = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039F0);
    branch_condition(api, 11U, 0x003B22U, 14, 0x0130U);
    api.finish_instruction(opcode_0x0039F0);
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

unsigned instruction_count_from_0x0039F8(unsigned entry_pc) {
    if (entry_pc == 0x0039F8U) return 1U;
    return 0;
}

BlockExit execute_0x0039F8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039F8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039F8U) {
    // guest 0x0039F8 opcode 0x5306 5306 subq.b #$1,D6
    const auto opcode_0x0039F8 = fetch_checked(api, 0x5306U);
    api.begin_instruction(opcode_0x0039F8);
    subq_b_data(api, 1U, 6U);
    api.finish_instruction(opcode_0x0039F8);
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

unsigned instruction_count_from_0x0039FA(unsigned entry_pc) {
    if (entry_pc == 0x0039FAU) return 1U;
    return 0;
}

BlockExit execute_0x0039FA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0039FAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0039FAU) {
    // guest 0x0039FA opcode 0x6B00 6B00 0134 bmi.w loc_003B30
    const auto opcode_0x0039FA = fetch_checked(api, 0x6B00U);
    api.begin_instruction(opcode_0x0039FA);
    branch_condition(api, 11U, 0x003B30U, 14, 0x0134U);
    api.finish_instruction(opcode_0x0039FA);
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

unsigned instruction_count_from_0x003A06(unsigned entry_pc) {
    if (entry_pc == 0x003A06U) return 1U;
    return 0;
}

BlockExit execute_0x003A06(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003A06U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003A06U) {
    // guest 0x003A06 opcode 0x2449 2449 movea.l A1,A2
    const auto opcode_0x003A06 = fetch_checked(api, 0x2449U);
    api.begin_instruction(opcode_0x003A06);
    movea_l_address_to_address(api, 1U, 2U);
    api.finish_instruction(opcode_0x003A06);
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

unsigned instruction_count_from_0x003A0A(unsigned entry_pc) {
    if (entry_pc == 0x003A0AU) return 1U;
    return 0;
}

BlockExit execute_0x003A0A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003A0AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003A0AU) {
    // guest 0x003A0A opcode 0x5342 5342 subq.w #$1,D2
    const auto opcode_0x003A0A = fetch_checked(api, 0x5342U);
    api.begin_instruction(opcode_0x003A0A);
    subq_w_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x003A0A);
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

unsigned instruction_count_from_0x003A18(unsigned entry_pc) {
    if (entry_pc == 0x003A18U) return 1U;
    return 0;
}

BlockExit execute_0x003A18(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003A18U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003A18U) {
    // guest 0x003A18 opcode 0x6600 6600 FEBA bne.w loc_0038D4
    const auto opcode_0x003A18 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x003A18);
    branch_condition(api, 6U, 0x0038D4U, 14, 0xFEBAU);
    api.finish_instruction(opcode_0x003A18);
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

unsigned instruction_count_from_0x00D998(unsigned entry_pc) {
    if (entry_pc == 0x00D998U) return 1U;
    return 0;
}

BlockExit execute_0x00D998(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00D998U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00D998U) {
    // guest 0x00D998 opcode 0xDE86 DE86 add.l D6,D7
    const auto opcode_0x00D998 = fetch_checked(api, 0xDE86U);
    api.begin_instruction(opcode_0x00D998);
    add_l_data_to_data(api, 6U, 7U);
    api.finish_instruction(opcode_0x00D998);
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

unsigned instruction_count_from_0x00D99A(unsigned entry_pc) {
    if (entry_pc == 0x00D99AU) return 1U;
    return 0;
}

BlockExit execute_0x00D99A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00D99AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00D99AU) {
    // guest 0x00D99A opcode 0x51CA 51CA FFEE dbf D2,loc_00D98A
    const auto opcode_0x00D99A = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x00D99A);
    dbcc(api, 1U, 2U, 0x00D98AU, 0xFFEEU);
    api.finish_instruction(opcode_0x00D99A);
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

unsigned instruction_count_from_0x03A750(unsigned entry_pc) {
    if (entry_pc == 0x03A750U) return 1U;
    return 0;
}

BlockExit execute_0x03A750(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A750U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A750U) {
    // guest 0x03A750 opcode 0x0839 0839 0002 00FF 164D btst.b #$2,($00FF164D).L
    const auto opcode_0x03A750 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x03A750);
    (void)fetch_checked(api, 0x0002U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    bit_test_immediate_absolute_long(api, 2U, 0xFF164DU);
    api.finish_instruction(opcode_0x03A750);
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

unsigned instruction_count_from_0x03A782(unsigned entry_pc) {
    if (entry_pc == 0x03A782U) return 1U;
    return 0;
}

BlockExit execute_0x03A782(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A782U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A782U) {
    // guest 0x03A782 opcode 0x0839 0839 0001 00FF 164D btst.b #$1,($00FF164D).L
    const auto opcode_0x03A782 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x03A782);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164DU);
    api.finish_instruction(opcode_0x03A782);
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

} // namespace oasis::hybrid::generated

// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

unsigned instruction_count_from_0x03A78A(unsigned entry_pc) {
    if (entry_pc == 0x03A78AU) return 1U;
    return 0;
}

BlockExit execute_0x03A78A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A78AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A78AU) {
    // guest 0x03A78A opcode 0x66F6 66F6 bne.s loc_03A782
    const auto opcode_0x03A78A = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x03A78A);
    branch_condition(api, 6U, 0x03A782U, -14);
    api.finish_instruction(opcode_0x03A78A);
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

unsigned instruction_count_from_0x03A7CE(unsigned entry_pc) {
    if (entry_pc == 0x03A7CEU) return 1U;
    return 0;
}

BlockExit execute_0x03A7CE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7CEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7CEU) {
    // guest 0x03A7CE opcode 0x6600 6600 FFC4 bne.w loc_03A794
    const auto opcode_0x03A7CE = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A7CE);
    branch_condition(api, 6U, 0x03A794U, 14, 0xFFC4U);
    api.finish_instruction(opcode_0x03A7CE);
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
