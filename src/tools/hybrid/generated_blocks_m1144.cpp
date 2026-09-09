// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

unsigned instruction_count_from_0x03A7AE(unsigned entry_pc) {
    if (entry_pc == 0x03A7AEU) return 2U;
    if (entry_pc == 0x03A7B4U) return 1U;
    return 0;
}

BlockExit execute_0x03A7AE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A7AEU && entry_pc != 0x03A7B4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A7AEU) {
    // guest 0x03A7AE opcode 0x4A79 4A79 00FF 1654 tst.w ($00FF1654).L
    const auto opcode_0x03A7AE = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x03A7AE);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1654U);
    test_absolute_long(api, 0xFF1654U, 2U);
    api.finish_instruction(opcode_0x03A7AE);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x03A7B4U) {
    // guest 0x03A7B4 opcode 0x6600 6600 FFF8 bne.w loc_03A7AE
    const auto opcode_0x03A7B4 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A7B4);
    branch_condition(api, 6U, 0x03A7AEU, 14, 0xFFF8U);
    api.finish_instruction(opcode_0x03A7B4);
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
