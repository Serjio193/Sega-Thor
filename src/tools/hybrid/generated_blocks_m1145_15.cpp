// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x03B22E(unsigned entry_pc) {
    if (entry_pc == 0x03B22EU) return 1U;
    return 0;
}

BlockExit execute_0x03B22E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B22EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B22EU) {
    // guest 0x03B22E opcode 0x43F9 43F9 00FF 316C lea.l ($00FF316C).L,A1
    const auto opcode_0x03B22E = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x03B22E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x316CU);
    lea_absolute_long(api, 0xFF316CU, 1U);
    api.finish_instruction(opcode_0x03B22E);
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

unsigned instruction_count_from_0x03B234(unsigned entry_pc) {
    if (entry_pc == 0x03B234U) return 1U;
    return 0;
}

BlockExit execute_0x03B234(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B234U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B234U) {
    // guest 0x03B234 opcode 0x2449 2449 movea.l A1,A2
    const auto opcode_0x03B234 = fetch_checked(api, 0x2449U);
    api.begin_instruction(opcode_0x03B234);
    movea_l_address_to_address(api, 1U, 2U);
    api.finish_instruction(opcode_0x03B234);
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

unsigned instruction_count_from_0x03B270(unsigned entry_pc) {
    if (entry_pc == 0x03B270U) return 1U;
    return 0;
}

BlockExit execute_0x03B270(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B270U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B270U) {
    // guest 0x03B270 opcode 0x6700 6700 0062 beq.w loc_03B2D4
    const auto opcode_0x03B270 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x03B270);
    branch_condition(api, 7U, 0x03B2D4U, 14, 0x0062U);
    api.finish_instruction(opcode_0x03B270);
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

unsigned instruction_count_from_0x03B274(unsigned entry_pc) {
    if (entry_pc == 0x03B274U) return 1U;
    return 0;
}

BlockExit execute_0x03B274(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B274U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B274U) {
    // guest 0x03B274 opcode 0x0839 0839 0001 00FF 164E btst.b #$1,($00FF164E).L
    const auto opcode_0x03B274 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x03B274);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164EU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164EU);
    api.finish_instruction(opcode_0x03B274);
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

unsigned instruction_count_from_0x03B27C(unsigned entry_pc) {
    if (entry_pc == 0x03B27CU) return 1U;
    return 0;
}

BlockExit execute_0x03B27C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B27CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B27CU) {
    // guest 0x03B27C opcode 0x6600 6600 FFF6 bne.w loc_03B274
    const auto opcode_0x03B27C = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03B27C);
    branch_condition(api, 6U, 0x03B274U, 14, 0xFFF6U);
    api.finish_instruction(opcode_0x03B27C);
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

unsigned instruction_count_from_0x03B282(unsigned entry_pc) {
    if (entry_pc == 0x03B282U) return 1U;
    return 0;
}

BlockExit execute_0x03B282(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B282U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B282U) {
    // guest 0x03B282 opcode 0x43F9 43F9 00FF 316C lea.l ($00FF316C).L,A1
    const auto opcode_0x03B282 = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x03B282);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x316CU);
    lea_absolute_long(api, 0xFF316CU, 1U);
    api.finish_instruction(opcode_0x03B282);
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

unsigned instruction_count_from_0x03B288(unsigned entry_pc) {
    if (entry_pc == 0x03B288U) return 1U;
    return 0;
}

BlockExit execute_0x03B288(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B288U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B288U) {
    // guest 0x03B288 opcode 0x2449 2449 movea.l A1,A2
    const auto opcode_0x03B288 = fetch_checked(api, 0x2449U);
    api.begin_instruction(opcode_0x03B288);
    movea_l_address_to_address(api, 1U, 2U);
    api.finish_instruction(opcode_0x03B288);
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

unsigned instruction_count_from_0x03B292(unsigned entry_pc) {
    if (entry_pc == 0x03B292U) return 1U;
    return 0;
}

BlockExit execute_0x03B292(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B292U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B292U) {
    // guest 0x03B292 opcode 0xD4C6 D4C6 adda.w D6,A2
    const auto opcode_0x03B292 = fetch_checked(api, 0xD4C6U);
    api.begin_instruction(opcode_0x03B292);
    adda_w_data_to_address(api, 6U, 2U);
    api.finish_instruction(opcode_0x03B292);
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

unsigned instruction_count_from_0x03B2E4(unsigned entry_pc) {
    if (entry_pc == 0x03B2E4U) return 1U;
    return 0;
}

BlockExit execute_0x03B2E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B2E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B2E4U) {
    // guest 0x03B2E4 opcode 0x6700 6700 0062 beq.w loc_03B348
    const auto opcode_0x03B2E4 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x03B2E4);
    branch_condition(api, 7U, 0x03B348U, 14, 0x0062U);
    api.finish_instruction(opcode_0x03B2E4);
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

unsigned instruction_count_from_0x03B7C0(unsigned entry_pc) {
    if (entry_pc == 0x03B7C0U) return 1U;
    return 0;
}

BlockExit execute_0x03B7C0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7C0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7C0U) {
    // guest 0x03B7C0 opcode 0x47F9 47F9 00FF 134C lea.l ($00FF134C).L,A3
    const auto opcode_0x03B7C0 = fetch_checked(api, 0x47F9U);
    api.begin_instruction(opcode_0x03B7C0);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x134CU);
    lea_absolute_long(api, 0xFF134CU, 3U);
    api.finish_instruction(opcode_0x03B7C0);
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

unsigned instruction_count_from_0x03B7C6(unsigned entry_pc) {
    if (entry_pc == 0x03B7C6U) return 1U;
    return 0;
}

BlockExit execute_0x03B7C6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7C6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7C6U) {
    // guest 0x03B7C6 opcode 0xD6C6 D6C6 adda.w D6,A3
    const auto opcode_0x03B7C6 = fetch_checked(api, 0xD6C6U);
    api.begin_instruction(opcode_0x03B7C6);
    adda_w_data_to_address(api, 6U, 3U);
    api.finish_instruction(opcode_0x03B7C6);
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

unsigned instruction_count_from_0x03B7CE(unsigned entry_pc) {
    if (entry_pc == 0x03B7CEU) return 1U;
    return 0;
}

BlockExit execute_0x03B7CE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7CEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7CEU) {
    // guest 0x03B7CE opcode 0x3002 3002 move.w D2,D0
    const auto opcode_0x03B7CE = fetch_checked(api, 0x3002U);
    api.begin_instruction(opcode_0x03B7CE);
    move_w_data_to_data(api, 2U, 0U);
    api.finish_instruction(opcode_0x03B7CE);
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

unsigned instruction_count_from_0x03B7D0(unsigned entry_pc) {
    if (entry_pc == 0x03B7D0U) return 1U;
    return 0;
}

BlockExit execute_0x03B7D0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7D0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7D0U) {
    // guest 0x03B7D0 opcode 0x0240 0240 000E andi.w #$E,D0
    const auto opcode_0x03B7D0 = fetch_checked(api, 0x0240U);
    api.begin_instruction(opcode_0x03B7D0);
    (void)fetch_checked(api, 0x000EU);
    andi_w_data(api, 14U, 0U);
    api.finish_instruction(opcode_0x03B7D0);
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

unsigned instruction_count_from_0x03B7D4(unsigned entry_pc) {
    if (entry_pc == 0x03B7D4U) return 1U;
    return 0;
}

BlockExit execute_0x03B7D4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7D4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7D4U) {
    // guest 0x03B7D4 opcode 0x3604 3604 move.w D4,D3
    const auto opcode_0x03B7D4 = fetch_checked(api, 0x3604U);
    api.begin_instruction(opcode_0x03B7D4);
    move_w_data_to_data(api, 4U, 3U);
    api.finish_instruction(opcode_0x03B7D4);
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

unsigned instruction_count_from_0x03B7DA(unsigned entry_pc) {
    if (entry_pc == 0x03B7DAU) return 1U;
    return 0;
}

BlockExit execute_0x03B7DA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7DAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7DAU) {
    // guest 0x03B7DA opcode 0x9640 9640 sub.w D0,D3
    const auto opcode_0x03B7DA = fetch_checked(api, 0x9640U);
    api.begin_instruction(opcode_0x03B7DA);
    sub_w_data_to_data(api, 0U, 3U);
    api.finish_instruction(opcode_0x03B7DA);
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

unsigned instruction_count_from_0x03B7E2(unsigned entry_pc) {
    if (entry_pc == 0x03B7E2U) return 1U;
    return 0;
}

BlockExit execute_0x03B7E2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7E2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7E2U) {
    // guest 0x03B7E2 opcode 0xD043 D043 add.w D3,D0
    const auto opcode_0x03B7E2 = fetch_checked(api, 0xD043U);
    api.begin_instruction(opcode_0x03B7E2);
    add_w_data_to_data(api, 3U, 0U);
    api.finish_instruction(opcode_0x03B7E2);
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

unsigned instruction_count_from_0x03B7E4(unsigned entry_pc) {
    if (entry_pc == 0x03B7E4U) return 1U;
    return 0;
}

BlockExit execute_0x03B7E4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03B7E4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03B7E4U) {
    // guest 0x03B7E4 opcode 0x0240 0240 000E andi.w #$E,D0
    const auto opcode_0x03B7E4 = fetch_checked(api, 0x0240U);
    api.begin_instruction(opcode_0x03B7E4);
    (void)fetch_checked(api, 0x000EU);
    andi_w_data(api, 14U, 0U);
    api.finish_instruction(opcode_0x03B7E4);
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
