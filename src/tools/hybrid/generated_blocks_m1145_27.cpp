// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x061F2C(unsigned entry_pc) {
    if (entry_pc == 0x061F2CU) return 1U;
    return 0;
}

BlockExit execute_0x061F2C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061F2CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061F2CU) {
    // guest 0x061F2C opcode 0x6700 6700 000A beq.w loc_061F38
    const auto opcode_0x061F2C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061F2C);
    branch_condition(api, 7U, 0x061F38U, 14, 0x000AU);
    api.finish_instruction(opcode_0x061F2C);
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

unsigned instruction_count_from_0x061F3C(unsigned entry_pc) {
    if (entry_pc == 0x061F3CU) return 1U;
    return 0;
}

BlockExit execute_0x061F3C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061F3CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061F3CU) {
    // guest 0x061F3C opcode 0x6700 6700 0006 beq.w loc_061F44
    const auto opcode_0x061F3C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x061F3C);
    branch_condition(api, 7U, 0x061F44U, 14, 0x0006U);
    api.finish_instruction(opcode_0x061F3C);
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

unsigned instruction_count_from_0x061F50(unsigned entry_pc) {
    if (entry_pc == 0x061F50U) return 1U;
    return 0;
}

BlockExit execute_0x061F50(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061F50U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061F50U) {
    // guest 0x061F50 opcode 0x51CB 51CB FFC8 dbf D3,loc_061F1A
    const auto opcode_0x061F50 = fetch_checked(api, 0x51CBU);
    api.begin_instruction(opcode_0x061F50);
    dbcc(api, 1U, 3U, 0x061F1AU, 0xFFC8U);
    api.finish_instruction(opcode_0x061F50);
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

unsigned instruction_count_from_0x061F6A(unsigned entry_pc) {
    if (entry_pc == 0x061F6AU) return 1U;
    return 0;
}

BlockExit execute_0x061F6A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061F6AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061F6AU) {
    // guest 0x061F6A opcode 0x6600 6600 000A bne.w loc_061F76
    const auto opcode_0x061F6A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061F6A);
    branch_condition(api, 6U, 0x061F76U, 14, 0x000AU);
    api.finish_instruction(opcode_0x061F6A);
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

unsigned instruction_count_from_0x062048(unsigned entry_pc) {
    if (entry_pc == 0x062048U) return 1U;
    return 0;
}

BlockExit execute_0x062048(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062048U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062048U) {
    // guest 0x062048 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x062048 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x062048);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x062048);
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

unsigned instruction_count_from_0x06204C(unsigned entry_pc) {
    if (entry_pc == 0x06204CU) return 1U;
    return 0;
}

BlockExit execute_0x06204C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06204CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06204CU) {
    // guest 0x06204C opcode 0x0807 0807 001F btst.l #$1F,D7
    const auto opcode_0x06204C = fetch_checked(api, 0x0807U);
    api.begin_instruction(opcode_0x06204C);
    (void)fetch_checked(api, 0x001FU);
    bit_test_immediate_data(api, 31U, 7U);
    api.finish_instruction(opcode_0x06204C);
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

unsigned instruction_count_from_0x062050(unsigned entry_pc) {
    if (entry_pc == 0x062050U) return 1U;
    return 0;
}

BlockExit execute_0x062050(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062050U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062050U) {
    // guest 0x062050 opcode 0x6700 6700 0006 beq.w loc_062058
    const auto opcode_0x062050 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062050);
    branch_condition(api, 7U, 0x062058U, 14, 0x0006U);
    api.finish_instruction(opcode_0x062050);
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

unsigned instruction_count_from_0x06205A(unsigned entry_pc) {
    if (entry_pc == 0x06205AU) return 1U;
    return 0;
}

BlockExit execute_0x06205A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06205AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06205AU) {
    // guest 0x06205A opcode 0x6A00 6A00 0006 bpl.w loc_062062
    const auto opcode_0x06205A = fetch_checked(api, 0x6A00U);
    api.begin_instruction(opcode_0x06205A);
    branch_condition(api, 10U, 0x062062U, 14, 0x0006U);
    api.finish_instruction(opcode_0x06205A);
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

unsigned instruction_count_from_0x062066(unsigned entry_pc) {
    if (entry_pc == 0x062066U) return 1U;
    return 0;
}

BlockExit execute_0x062066(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062066U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062066U) {
    // guest 0x062066 opcode 0x6300 6300 0004 bls.w loc_06206C
    const auto opcode_0x062066 = fetch_checked(api, 0x6300U);
    api.begin_instruction(opcode_0x062066);
    branch_condition(api, 3U, 0x06206CU, 14, 0x0004U);
    api.finish_instruction(opcode_0x062066);
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

unsigned instruction_count_from_0x0620B6(unsigned entry_pc) {
    if (entry_pc == 0x0620B6U) return 1U;
    return 0;
}

BlockExit execute_0x0620B6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0620B6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0620B6U) {
    // guest 0x0620B6 opcode 0x6400 6400 0014 bcc.w loc_0620CC
    const auto opcode_0x0620B6 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x0620B6);
    branch_condition(api, 4U, 0x0620CCU, 14, 0x0014U);
    api.finish_instruction(opcode_0x0620B6);
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

unsigned instruction_count_from_0x062124(unsigned entry_pc) {
    if (entry_pc == 0x062124U) return 1U;
    return 0;
}

BlockExit execute_0x062124(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062124U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062124U) {
    // guest 0x062124 opcode 0x6700 6700 0026 beq.w loc_06214C
    const auto opcode_0x062124 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062124);
    branch_condition(api, 7U, 0x06214CU, 14, 0x0026U);
    api.finish_instruction(opcode_0x062124);
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

unsigned instruction_count_from_0x06212C(unsigned entry_pc) {
    if (entry_pc == 0x06212CU) return 1U;
    return 0;
}

BlockExit execute_0x06212C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06212CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06212CU) {
    // guest 0x06212C opcode 0x6700 6700 001E beq.w loc_06214C
    const auto opcode_0x06212C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06212C);
    branch_condition(api, 7U, 0x06214CU, 14, 0x001EU);
    api.finish_instruction(opcode_0x06212C);
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

unsigned instruction_count_from_0x0621FA(unsigned entry_pc) {
    if (entry_pc == 0x0621FAU) return 1U;
    return 0;
}

BlockExit execute_0x0621FA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0621FAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0621FAU) {
    // guest 0x0621FA opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x0621FA = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x0621FA);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x0621FA);
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

unsigned instruction_count_from_0x062218(unsigned entry_pc) {
    if (entry_pc == 0x062218U) return 1U;
    return 0;
}

BlockExit execute_0x062218(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062218U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062218U) {
    // guest 0x062218 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x062218 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x062218);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x062218);
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

unsigned instruction_count_from_0x062230(unsigned entry_pc) {
    if (entry_pc == 0x062230U) return 1U;
    return 0;
}

BlockExit execute_0x062230(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062230U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062230U) {
    // guest 0x062230 opcode 0x6600 6600 0018 bne.w loc_06224A
    const auto opcode_0x062230 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062230);
    branch_condition(api, 6U, 0x06224AU, 14, 0x0018U);
    api.finish_instruction(opcode_0x062230);
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

unsigned instruction_count_from_0x06224E(unsigned entry_pc) {
    if (entry_pc == 0x06224EU) return 1U;
    return 0;
}

BlockExit execute_0x06224E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06224EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06224EU) {
    // guest 0x06224E opcode 0x6700 6700 0008 beq.w loc_062258
    const auto opcode_0x06224E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06224E);
    branch_condition(api, 7U, 0x062258U, 14, 0x0008U);
    api.finish_instruction(opcode_0x06224E);
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

unsigned instruction_count_from_0x0623AC(unsigned entry_pc) {
    if (entry_pc == 0x0623ACU) return 1U;
    return 0;
}

BlockExit execute_0x0623AC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0623ACU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0623ACU) {
    // guest 0x0623AC opcode 0x0807 0807 001F btst.l #$1F,D7
    const auto opcode_0x0623AC = fetch_checked(api, 0x0807U);
    api.begin_instruction(opcode_0x0623AC);
    (void)fetch_checked(api, 0x001FU);
    bit_test_immediate_data(api, 31U, 7U);
    api.finish_instruction(opcode_0x0623AC);
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
