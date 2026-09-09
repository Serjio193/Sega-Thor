// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x060C04(unsigned entry_pc) {
    if (entry_pc == 0x060C04U) return 1U;
    return 0;
}

BlockExit execute_0x060C04(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C04U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C04U) {
    // guest 0x060C04 opcode 0xD681 D681 add.l D1,D3
    const auto opcode_0x060C04 = fetch_checked(api, 0xD681U);
    api.begin_instruction(opcode_0x060C04);
    add_l_data_to_data(api, 1U, 3U);
    api.finish_instruction(opcode_0x060C04);
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

unsigned instruction_count_from_0x060C12(unsigned entry_pc) {
    if (entry_pc == 0x060C12U) return 1U;
    return 0;
}

BlockExit execute_0x060C12(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C12U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C12U) {
    // guest 0x060C12 opcode 0x4DF9 4DF9 00FF 0022 lea.l ($00FF0022).L,A6
    const auto opcode_0x060C12 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C12);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0022U);
    lea_absolute_long(api, 0xFF0022U, 6U);
    api.finish_instruction(opcode_0x060C12);
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

unsigned instruction_count_from_0x060C28(unsigned entry_pc) {
    if (entry_pc == 0x060C28U) return 1U;
    return 0;
}

BlockExit execute_0x060C28(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C28U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C28U) {
    // guest 0x060C28 opcode 0x4DF9 4DF9 00FF 00EC lea.l ($00FF00EC).L,A6
    const auto opcode_0x060C28 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C28);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x00ECU);
    lea_absolute_long(api, 0xFF00ECU, 6U);
    api.finish_instruction(opcode_0x060C28);
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

unsigned instruction_count_from_0x060C3E(unsigned entry_pc) {
    if (entry_pc == 0x060C3EU) return 1U;
    return 0;
}

BlockExit execute_0x060C3E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C3EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C3EU) {
    // guest 0x060C3E opcode 0x4DF9 4DF9 00FF 01B6 lea.l ($00FF01B6).L,A6
    const auto opcode_0x060C3E = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C3E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x01B6U);
    lea_absolute_long(api, 0xFF01B6U, 6U);
    api.finish_instruction(opcode_0x060C3E);
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

unsigned instruction_count_from_0x060C54(unsigned entry_pc) {
    if (entry_pc == 0x060C54U) return 1U;
    return 0;
}

BlockExit execute_0x060C54(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C54U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C54U) {
    // guest 0x060C54 opcode 0x4DF9 4DF9 00FF 0414 lea.l ($00FF0414).L,A6
    const auto opcode_0x060C54 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C54);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0414U);
    lea_absolute_long(api, 0xFF0414U, 6U);
    api.finish_instruction(opcode_0x060C54);
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

unsigned instruction_count_from_0x060C6A(unsigned entry_pc) {
    if (entry_pc == 0x060C6AU) return 1U;
    return 0;
}

BlockExit execute_0x060C6A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C6AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C6AU) {
    // guest 0x060C6A opcode 0x4DF9 4DF9 00FF 049E lea.l ($00FF049E).L,A6
    const auto opcode_0x060C6A = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C6A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x049EU);
    lea_absolute_long(api, 0xFF049EU, 6U);
    api.finish_instruction(opcode_0x060C6A);
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

unsigned instruction_count_from_0x060C80(unsigned entry_pc) {
    if (entry_pc == 0x060C80U) return 1U;
    return 0;
}

BlockExit execute_0x060C80(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C80U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C80U) {
    // guest 0x060C80 opcode 0x4DF9 4DF9 00FF 0528 lea.l ($00FF0528).L,A6
    const auto opcode_0x060C80 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C80);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0528U);
    lea_absolute_long(api, 0xFF0528U, 6U);
    api.finish_instruction(opcode_0x060C80);
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

unsigned instruction_count_from_0x060C9E(unsigned entry_pc) {
    if (entry_pc == 0x060C9EU) return 1U;
    return 0;
}

BlockExit execute_0x060C9E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060C9EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060C9EU) {
    // guest 0x060C9E opcode 0x4DF9 4DF9 00FF 0280 lea.l ($00FF0280).L,A6
    const auto opcode_0x060C9E = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060C9E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0280U);
    lea_absolute_long(api, 0xFF0280U, 6U);
    api.finish_instruction(opcode_0x060C9E);
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

unsigned instruction_count_from_0x060CB4(unsigned entry_pc) {
    if (entry_pc == 0x060CB4U) return 1U;
    return 0;
}

BlockExit execute_0x060CB4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060CB4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060CB4U) {
    // guest 0x060CB4 opcode 0x4DF9 4DF9 00FF 034A lea.l ($00FF034A).L,A6
    const auto opcode_0x060CB4 = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060CB4);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x034AU);
    lea_absolute_long(api, 0xFF034AU, 6U);
    api.finish_instruction(opcode_0x060CB4);
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

unsigned instruction_count_from_0x060CCC(unsigned entry_pc) {
    if (entry_pc == 0x060CCCU) return 1U;
    return 0;
}

BlockExit execute_0x060CCC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060CCCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060CCCU) {
    // guest 0x060CCC opcode 0x4DF9 4DF9 00FF 05B2 lea.l ($00FF05B2).L,A6
    const auto opcode_0x060CCC = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x060CCC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x05B2U);
    lea_absolute_long(api, 0xFF05B2U, 6U);
    api.finish_instruction(opcode_0x060CCC);
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

unsigned instruction_count_from_0x060F8C(unsigned entry_pc) {
    if (entry_pc == 0x060F8CU) return 1U;
    return 0;
}

BlockExit execute_0x060F8C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060F8CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060F8CU) {
    // guest 0x060F8C opcode 0xD481 D481 add.l D1,D2
    const auto opcode_0x060F8C = fetch_checked(api, 0xD481U);
    api.begin_instruction(opcode_0x060F8C);
    add_l_data_to_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x060F8C);
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

unsigned instruction_count_from_0x060F9A(unsigned entry_pc) {
    if (entry_pc == 0x060F9AU) return 1U;
    return 0;
}

BlockExit execute_0x060F9A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060F9AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060F9AU) {
    // guest 0x060F9A opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x060F9A = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x060F9A);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x060F9A);
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

unsigned instruction_count_from_0x060FA8(unsigned entry_pc) {
    if (entry_pc == 0x060FA8U) return 1U;
    return 0;
}

BlockExit execute_0x060FA8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060FA8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060FA8U) {
    // guest 0x060FA8 opcode 0x6700 6700 0008 beq.w loc_060FB2
    const auto opcode_0x060FA8 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x060FA8);
    branch_condition(api, 7U, 0x060FB2U, 14, 0x0008U);
    api.finish_instruction(opcode_0x060FA8);
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

unsigned instruction_count_from_0x060FB2(unsigned entry_pc) {
    if (entry_pc == 0x060FB2U) return 1U;
    return 0;
}

BlockExit execute_0x060FB2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060FB2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060FB2U) {
    // guest 0x060FB2 opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x060FB2 = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x060FB2);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x060FB2);
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

unsigned instruction_count_from_0x061026(unsigned entry_pc) {
    if (entry_pc == 0x061026U) return 1U;
    return 0;
}

BlockExit execute_0x061026(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061026U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061026U) {
    // guest 0x061026 opcode 0x6600 6600 0008 bne.w loc_061030
    const auto opcode_0x061026 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061026);
    branch_condition(api, 6U, 0x061030U, 14, 0x0008U);
    api.finish_instruction(opcode_0x061026);
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

unsigned instruction_count_from_0x06103C(unsigned entry_pc) {
    if (entry_pc == 0x06103CU) return 1U;
    return 0;
}

BlockExit execute_0x06103C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06103CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06103CU) {
    // guest 0x06103C opcode 0x7000 7000 moveq #0,D0
    const auto opcode_0x06103C = fetch_checked(api, 0x7000U);
    api.begin_instruction(opcode_0x06103C);
    moveq_data(api, 0, 0U);
    api.finish_instruction(opcode_0x06103C);
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

unsigned instruction_count_from_0x06104A(unsigned entry_pc) {
    if (entry_pc == 0x06104AU) return 1U;
    return 0;
}

BlockExit execute_0x06104A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06104AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06104AU) {
    // guest 0x06104A opcode 0x6700 6700 0008 beq.w loc_061054
    const auto opcode_0x06104A = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x06104A);
    branch_condition(api, 7U, 0x061054U, 14, 0x0008U);
    api.finish_instruction(opcode_0x06104A);
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
