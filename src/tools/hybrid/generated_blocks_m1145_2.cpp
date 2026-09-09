// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x0021AE(unsigned entry_pc) {
    if (entry_pc == 0x0021AEU) return 1U;
    return 0;
}

BlockExit execute_0x0021AE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021AEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021AEU) {
    // guest 0x0021AE opcode 0x6600 6600 06E8 bne.w loc_002898
    const auto opcode_0x0021AE = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x0021AE);
    branch_condition(api, 6U, 0x002898U, 14, 0x06E8U);
    api.finish_instruction(opcode_0x0021AE);
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

unsigned instruction_count_from_0x0021B2(unsigned entry_pc) {
    if (entry_pc == 0x0021B2U) return 1U;
    return 0;
}

BlockExit execute_0x0021B2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021B2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021B2U) {
    // guest 0x0021B2 opcode 0x41F9 41F9 00FF 1668 lea.l ($00FF1668).L,A0
    const auto opcode_0x0021B2 = fetch_checked(api, 0x41F9U);
    api.begin_instruction(opcode_0x0021B2);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1668U);
    lea_absolute_long(api, 0xFF1668U, 0U);
    api.finish_instruction(opcode_0x0021B2);
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

unsigned instruction_count_from_0x0021D8(unsigned entry_pc) {
    if (entry_pc == 0x0021D8U) return 1U;
    return 0;
}

BlockExit execute_0x0021D8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021D8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021D8U) {
    // guest 0x0021D8 opcode 0x5242 5242 addq.w #$1,D2
    const auto opcode_0x0021D8 = fetch_checked(api, 0x5242U);
    api.begin_instruction(opcode_0x0021D8);
    addq_w_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x0021D8);
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

unsigned instruction_count_from_0x0021DE(unsigned entry_pc) {
    if (entry_pc == 0x0021DEU) return 1U;
    return 0;
}

BlockExit execute_0x0021DE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021DEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021DEU) {
    // guest 0x0021DE opcode 0x6300 6300 0006 bls.w loc_0021E6
    const auto opcode_0x0021DE = fetch_checked(api, 0x6300U);
    api.begin_instruction(opcode_0x0021DE);
    branch_condition(api, 3U, 0x0021E6U, 14, 0x0006U);
    api.finish_instruction(opcode_0x0021DE);
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

unsigned instruction_count_from_0x0021F2(unsigned entry_pc) {
    if (entry_pc == 0x0021F2U) return 1U;
    return 0;
}

BlockExit execute_0x0021F2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021F2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021F2U) {
    // guest 0x0021F2 opcode 0x4A79 4A79 00FF 1658 tst.w ($00FF1658).L
    const auto opcode_0x0021F2 = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x0021F2);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1658U);
    test_absolute_long(api, 0xFF1658U, 2U);
    api.finish_instruction(opcode_0x0021F2);
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

unsigned instruction_count_from_0x0021F8(unsigned entry_pc) {
    if (entry_pc == 0x0021F8U) return 1U;
    return 0;
}

BlockExit execute_0x0021F8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0021F8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0021F8U) {
    // guest 0x0021F8 opcode 0x6700 6700 0008 beq.w loc_002202
    const auto opcode_0x0021F8 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0021F8);
    branch_condition(api, 7U, 0x002202U, 14, 0x0008U);
    api.finish_instruction(opcode_0x0021F8);
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

unsigned instruction_count_from_0x002202(unsigned entry_pc) {
    if (entry_pc == 0x002202U) return 1U;
    return 0;
}

BlockExit execute_0x002202(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002202U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002202U) {
    // guest 0x002202 opcode 0x4A79 4A79 00FF 1654 tst.w ($00FF1654).L
    const auto opcode_0x002202 = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x002202);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1654U);
    test_absolute_long(api, 0xFF1654U, 2U);
    api.finish_instruction(opcode_0x002202);
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

unsigned instruction_count_from_0x002208(unsigned entry_pc) {
    if (entry_pc == 0x002208U) return 1U;
    return 0;
}

BlockExit execute_0x002208(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002208U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002208U) {
    // guest 0x002208 opcode 0x6700 6700 0008 beq.w loc_002212
    const auto opcode_0x002208 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x002208);
    branch_condition(api, 7U, 0x002212U, 14, 0x0008U);
    api.finish_instruction(opcode_0x002208);
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

unsigned instruction_count_from_0x002212(unsigned entry_pc) {
    if (entry_pc == 0x002212U) return 1U;
    return 0;
}

BlockExit execute_0x002212(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002212U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002212U) {
    // guest 0x002212 opcode 0x4A39 4A39 00FF 164D tst.b ($00FF164D).L
    const auto opcode_0x002212 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x002212);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    test_absolute_long(api, 0xFF164DU, 1U);
    api.finish_instruction(opcode_0x002212);
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

unsigned instruction_count_from_0x002218(unsigned entry_pc) {
    if (entry_pc == 0x002218U) return 1U;
    return 0;
}

BlockExit execute_0x002218(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002218U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002218U) {
    // guest 0x002218 opcode 0x6700 6700 0166 beq.w loc_002380
    const auto opcode_0x002218 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x002218);
    branch_condition(api, 7U, 0x002380U, 14, 0x0166U);
    api.finish_instruction(opcode_0x002218);
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

unsigned instruction_count_from_0x002224(unsigned entry_pc) {
    if (entry_pc == 0x002224U) return 1U;
    return 0;
}

BlockExit execute_0x002224(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002224U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002224U) {
    // guest 0x002224 opcode 0x6700 6700 0030 beq.w loc_002256
    const auto opcode_0x002224 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x002224);
    branch_condition(api, 7U, 0x002256U, 14, 0x0030U);
    api.finish_instruction(opcode_0x002224);
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

unsigned instruction_count_from_0x00222E(unsigned entry_pc) {
    if (entry_pc == 0x00222EU) return 1U;
    return 0;
}

BlockExit execute_0x00222E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00222EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00222EU) {
    // guest 0x00222E opcode 0x6B04 6B04 bmi.s loc_002234
    const auto opcode_0x00222E = fetch_checked(api, 0x6B04U);
    api.begin_instruction(opcode_0x00222E);
    branch_condition(api, 11U, 0x002234U, -14);
    api.finish_instruction(opcode_0x00222E);
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

unsigned instruction_count_from_0x002234(unsigned entry_pc) {
    if (entry_pc == 0x002234U) return 1U;
    return 0;
}

BlockExit execute_0x002234(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002234U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002234U) {
    // guest 0x002234 opcode 0x4BF9 4BF9 00FF 134C lea.l ($00FF134C).L,A5
    const auto opcode_0x002234 = fetch_checked(api, 0x4BF9U);
    api.begin_instruction(opcode_0x002234);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x134CU);
    lea_absolute_long(api, 0xFF134CU, 5U);
    api.finish_instruction(opcode_0x002234);
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

unsigned instruction_count_from_0x00225E(unsigned entry_pc) {
    if (entry_pc == 0x00225EU) return 1U;
    return 0;
}

BlockExit execute_0x00225E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00225EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00225EU) {
    // guest 0x00225E opcode 0x6700 6700 0014 beq.w loc_002274
    const auto opcode_0x00225E = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x00225E);
    branch_condition(api, 7U, 0x002274U, 14, 0x0014U);
    api.finish_instruction(opcode_0x00225E);
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

unsigned instruction_count_from_0x00227C(unsigned entry_pc) {
    if (entry_pc == 0x00227CU) return 1U;
    return 0;
}

BlockExit execute_0x00227C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00227CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00227CU) {
    // guest 0x00227C opcode 0x6700 6700 0014 beq.w loc_002292
    const auto opcode_0x00227C = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x00227C);
    branch_condition(api, 7U, 0x002292U, 14, 0x0014U);
    api.finish_instruction(opcode_0x00227C);
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

unsigned instruction_count_from_0x00229A(unsigned entry_pc) {
    if (entry_pc == 0x00229AU) return 1U;
    return 0;
}

BlockExit execute_0x00229A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00229AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00229AU) {
    // guest 0x00229A opcode 0x6600 6600 0396 bne.w loc_002632
    const auto opcode_0x00229A = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x00229A);
    branch_condition(api, 6U, 0x002632U, 14, 0x0396U);
    api.finish_instruction(opcode_0x00229A);
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

unsigned instruction_count_from_0x0022A6(unsigned entry_pc) {
    if (entry_pc == 0x0022A6U) return 1U;
    return 0;
}

BlockExit execute_0x0022A6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0022A6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0022A6U) {
    // guest 0x0022A6 opcode 0x6700 6700 00D8 beq.w loc_002380
    const auto opcode_0x0022A6 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0022A6);
    branch_condition(api, 7U, 0x002380U, 14, 0x00D8U);
    api.finish_instruction(opcode_0x0022A6);
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
