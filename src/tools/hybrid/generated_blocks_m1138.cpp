// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

unsigned instruction_count_from_0x0032EE(unsigned entry_pc) {
    if (entry_pc == 0x0032EEU) return 2U;
    if (entry_pc == 0x0032F4U) return 1U;
    return 0;
}

BlockExit execute_0x0032EE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0032EEU && entry_pc != 0x0032F4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0032EEU) {
    // guest 0x0032EE opcode 0x4A79 4A79 00FF 1658 tst.w ($00FF1658).L
    const auto opcode_0x0032EE = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x0032EE);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1658U);
    test_absolute_long(api, 0xFF1658U, 2U);
    api.finish_instruction(opcode_0x0032EE);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x0032F4U) {
    // guest 0x0032F4 opcode 0x66F8 66F8 bne.s loc_0032EE
    const auto opcode_0x0032F4 = fetch_checked(api, 0x66F8U);
    api.begin_instruction(opcode_0x0032F4);
    branch_condition(api, 6U, 0x0032EEU, -14);
    api.finish_instruction(opcode_0x0032F4);
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

unsigned instruction_count_from_0x03A9AC(unsigned entry_pc) {
    if (entry_pc == 0x03A9ACU) return 2U;
    if (entry_pc == 0x03A9B2U) return 1U;
    return 0;
}

BlockExit execute_0x03A9AC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A9ACU && entry_pc != 0x03A9B2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A9ACU) {
    // guest 0x03A9AC opcode 0x4A79 4A79 00FF AFAE tst.w ($00FFAFAE).L
    const auto opcode_0x03A9AC = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x03A9AC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0xAFAEU);
    test_absolute_long(api, 0xFFAFAEU, 2U);
    api.finish_instruction(opcode_0x03A9AC);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x03A9B2U) {
    // guest 0x03A9B2 opcode 0x6608 6608 bne.s loc_03A9BC
    const auto opcode_0x03A9B2 = fetch_checked(api, 0x6608U);
    api.begin_instruction(opcode_0x03A9B2);
    branch_condition(api, 6U, 0x03A9BCU, -14);
    api.finish_instruction(opcode_0x03A9B2);
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

unsigned instruction_count_from_0x03A9B4(unsigned entry_pc) {
    if (entry_pc == 0x03A9B4U) return 2U;
    if (entry_pc == 0x03A9BAU) return 1U;
    return 0;
}

BlockExit execute_0x03A9B4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A9B4U && entry_pc != 0x03A9BAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A9B4U) {
    // guest 0x03A9B4 opcode 0x4A39 4A39 00FF 0BFD tst.b ($00FF0BFD).L
    const auto opcode_0x03A9B4 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x03A9B4);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BFDU);
    test_absolute_long(api, 0xFF0BFDU, 1U);
    api.finish_instruction(opcode_0x03A9B4);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x03A9BAU) {
    // guest 0x03A9BA opcode 0x660E 660E bne.s loc_03A9CA
    const auto opcode_0x03A9BA = fetch_checked(api, 0x660EU);
    api.begin_instruction(opcode_0x03A9BA);
    branch_condition(api, 6U, 0x03A9CAU, -14);
    api.finish_instruction(opcode_0x03A9BA);
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

unsigned instruction_count_from_0x03A9CA(unsigned entry_pc) {
    if (entry_pc == 0x03A9CAU) return 2U;
    if (entry_pc == 0x03A9D0U) return 1U;
    return 0;
}

BlockExit execute_0x03A9CA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A9CAU && entry_pc != 0x03A9D0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A9CAU) {
    // guest 0x03A9CA opcode 0x4A79 4A79 00FF 1654 tst.w ($00FF1654).L
    const auto opcode_0x03A9CA = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x03A9CA);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1654U);
    test_absolute_long(api, 0xFF1654U, 2U);
    api.finish_instruction(opcode_0x03A9CA);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x03A9D0U) {
    // guest 0x03A9D0 opcode 0x6600 6600 FFDA bne.w loc_03A9AC
    const auto opcode_0x03A9D0 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A9D0);
    branch_condition(api, 6U, 0x03A9ACU, 14, 0xFFDAU);
    api.finish_instruction(opcode_0x03A9D0);
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
