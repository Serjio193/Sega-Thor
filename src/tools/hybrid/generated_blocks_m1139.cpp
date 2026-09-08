// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

unsigned instruction_count_from_0x000380(unsigned entry_pc) {
    if (entry_pc == 0x000380U) return 16U;
    if (entry_pc == 0x000382U) return 15U;
    if (entry_pc == 0x000384U) return 14U;
    if (entry_pc == 0x000386U) return 13U;
    if (entry_pc == 0x000388U) return 12U;
    if (entry_pc == 0x00038AU) return 11U;
    if (entry_pc == 0x00038CU) return 10U;
    if (entry_pc == 0x00038EU) return 9U;
    if (entry_pc == 0x000390U) return 8U;
    if (entry_pc == 0x000392U) return 7U;
    if (entry_pc == 0x000394U) return 6U;
    if (entry_pc == 0x000396U) return 5U;
    if (entry_pc == 0x000398U) return 4U;
    if (entry_pc == 0x00039AU) return 3U;
    if (entry_pc == 0x00039CU) return 2U;
    if (entry_pc == 0x00039EU) return 1U;
    return 0;
}

BlockExit execute_0x000380(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x000380U && entry_pc != 0x000382U && entry_pc != 0x000384U && entry_pc != 0x000386U && entry_pc != 0x000388U && entry_pc != 0x00038AU && entry_pc != 0x00038CU && entry_pc != 0x00038EU && entry_pc != 0x000390U && entry_pc != 0x000392U && entry_pc != 0x000394U && entry_pc != 0x000396U && entry_pc != 0x000398U && entry_pc != 0x00039AU && entry_pc != 0x00039CU && entry_pc != 0x00039EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x000380U) {
    // guest 0x000380 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000380 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000380);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000380);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000382U) {
    // guest 0x000382 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000382 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000382);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000382);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000384U) {
    // guest 0x000384 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000384 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000384);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000384);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000386U) {
    // guest 0x000386 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000386 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000386);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000386);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000388U) {
    // guest 0x000388 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000388 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000388);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000388);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x00038AU) {
    // guest 0x00038A opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x00038A = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x00038A);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x00038A);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x00038CU) {
    // guest 0x00038C opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x00038C = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x00038C);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x00038C);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x00038EU) {
    // guest 0x00038E opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x00038E = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x00038E);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x00038E);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000390U) {
    // guest 0x000390 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000390 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000390);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000390);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000392U) {
    // guest 0x000392 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000392 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000392);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000392);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000394U) {
    // guest 0x000394 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000394 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000394);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000394);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000396U) {
    // guest 0x000396 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000396 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000396);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000396);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x000398U) {
    // guest 0x000398 opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x000398 = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x000398);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x000398);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x00039AU) {
    // guest 0x00039A opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x00039A = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x00039A);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x00039A);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x00039CU) {
    // guest 0x00039C opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x00039C = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x00039C);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x00039C);
    ++instructions_executed;
    execute_from_here = true;
    if (api.boundary_reason) {
        const auto reason = api.boundary_reason();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {api.reg(16), reason, instructions_executed};
    }
    }
    if (execute_from_here || entry_pc == 0x00039EU) {
    // guest 0x00039E opcode 0xD058 D058 add.w (A0)+,D0
    const auto opcode_0x00039E = fetch_checked(api, 0xD058U);
    api.begin_instruction(opcode_0x00039E);
    add_w_postincrement_to_data_register(api, 0U, 0U);
    api.finish_instruction(opcode_0x00039E);
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

unsigned instruction_count_from_0x03A864(unsigned entry_pc) {
    if (entry_pc == 0x03A864U) return 1U;
    return 0;
}

BlockExit execute_0x03A864(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A864U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A864U) {
    // guest 0x03A864 opcode 0x6600 6600 FFF8 bne.w loc_03A85E
    const auto opcode_0x03A864 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A864);
    branch_condition(api, 6U, 0x03A85EU, 14, 0xFFF8U);
    api.finish_instruction(opcode_0x03A864);
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
