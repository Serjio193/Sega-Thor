// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x002BB6(unsigned entry_pc) {
    if (entry_pc == 0x002BB6U) return 1U;
    return 0;
}

BlockExit execute_0x002BB6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002BB6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002BB6U) {
    // guest 0x002BB6 opcode 0x51CF 51CF FFF8 dbf D7,loc_002BB0
    const auto opcode_0x002BB6 = fetch_checked(api, 0x51CFU);
    api.begin_instruction(opcode_0x002BB6);
    dbcc(api, 1U, 7U, 0x002BB0U, 0xFFF8U);
    api.finish_instruction(opcode_0x002BB6);
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

unsigned instruction_count_from_0x002C14(unsigned entry_pc) {
    if (entry_pc == 0x002C14U) return 1U;
    return 0;
}

BlockExit execute_0x002C14(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002C14U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002C14U) {
    // guest 0x002C14 opcode 0x0804 0804 0001 btst.l #$1,D4
    const auto opcode_0x002C14 = fetch_checked(api, 0x0804U);
    api.begin_instruction(opcode_0x002C14);
    (void)fetch_checked(api, 0x0001U);
    bit_test_immediate_data(api, 1U, 4U);
    api.finish_instruction(opcode_0x002C14);
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

unsigned instruction_count_from_0x002CD4(unsigned entry_pc) {
    if (entry_pc == 0x002CD4U) return 1U;
    return 0;
}

BlockExit execute_0x002CD4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002CD4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002CD4U) {
    // guest 0x002CD4 opcode 0x51CA 51CA FFFC dbf D2,loc_002CD2
    const auto opcode_0x002CD4 = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x002CD4);
    dbcc(api, 1U, 2U, 0x002CD2U, 0xFFFCU);
    api.finish_instruction(opcode_0x002CD4);
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

unsigned instruction_count_from_0x002CD8(unsigned entry_pc) {
    if (entry_pc == 0x002CD8U) return 1U;
    return 0;
}

BlockExit execute_0x002CD8(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002CD8U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002CD8U) {
    // guest 0x002CD8 opcode 0xDA83 DA83 add.l D3,D5
    const auto opcode_0x002CD8 = fetch_checked(api, 0xDA83U);
    api.begin_instruction(opcode_0x002CD8);
    add_l_data_to_data(api, 3U, 5U);
    api.finish_instruction(opcode_0x002CD8);
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

unsigned instruction_count_from_0x002CDA(unsigned entry_pc) {
    if (entry_pc == 0x002CDAU) return 1U;
    return 0;
}

BlockExit execute_0x002CDA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002CDAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002CDAU) {
    // guest 0x002CDA opcode 0x51CE 51CE FFF0 dbf D6,loc_002CCC
    const auto opcode_0x002CDA = fetch_checked(api, 0x51CEU);
    api.begin_instruction(opcode_0x002CDA);
    dbcc(api, 1U, 6U, 0x002CCCU, 0xFFF0U);
    api.finish_instruction(opcode_0x002CDA);
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

unsigned instruction_count_from_0x002D3A(unsigned entry_pc) {
    if (entry_pc == 0x002D3AU) return 1U;
    return 0;
}

BlockExit execute_0x002D3A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002D3AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002D3AU) {
    // guest 0x002D3A opcode 0x47F9 47F9 00FF 134C lea.l ($00FF134C).L,A3
    const auto opcode_0x002D3A = fetch_checked(api, 0x47F9U);
    api.begin_instruction(opcode_0x002D3A);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x134CU);
    lea_absolute_long(api, 0xFF134CU, 3U);
    api.finish_instruction(opcode_0x002D3A);
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

unsigned instruction_count_from_0x002D4A(unsigned entry_pc) {
    if (entry_pc == 0x002D4AU) return 1U;
    return 0;
}

BlockExit execute_0x002D4A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002D4AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002D4AU) {
    // guest 0x002D4A opcode 0x51CF 51CF FFF8 dbf D7,loc_002D44
    const auto opcode_0x002D4A = fetch_checked(api, 0x51CFU);
    api.begin_instruction(opcode_0x002D4A);
    dbcc(api, 1U, 7U, 0x002D44U, 0xFFF8U);
    api.finish_instruction(opcode_0x002D4A);
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

unsigned instruction_count_from_0x002D7A(unsigned entry_pc) {
    if (entry_pc == 0x002D7AU) return 1U;
    return 0;
}

BlockExit execute_0x002D7A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002D7AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002D7AU) {
    // guest 0x002D7A opcode 0x51CF 51CF FFFC dbf D7,loc_002D78
    const auto opcode_0x002D7A = fetch_checked(api, 0x51CFU);
    api.begin_instruction(opcode_0x002D7A);
    dbcc(api, 1U, 7U, 0x002D78U, 0xFFFCU);
    api.finish_instruction(opcode_0x002D7A);
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

unsigned instruction_count_from_0x003082(unsigned entry_pc) {
    if (entry_pc == 0x003082U) return 1U;
    return 0;
}

BlockExit execute_0x003082(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003082U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003082U) {
    // guest 0x003082 opcode 0x6700 6700 01BC beq.w loc_003240
    const auto opcode_0x003082 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x003082);
    branch_condition(api, 7U, 0x003240U, 14, 0x01BCU);
    api.finish_instruction(opcode_0x003082);
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

unsigned instruction_count_from_0x003090(unsigned entry_pc) {
    if (entry_pc == 0x003090U) return 1U;
    return 0;
}

BlockExit execute_0x003090(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003090U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003090U) {
    // guest 0x003090 opcode 0x4A39 4A39 00FF 0BFD tst.b ($00FF0BFD).L
    const auto opcode_0x003090 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x003090);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BFDU);
    test_absolute_long(api, 0xFF0BFDU, 1U);
    api.finish_instruction(opcode_0x003090);
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

unsigned instruction_count_from_0x003096(unsigned entry_pc) {
    if (entry_pc == 0x003096U) return 1U;
    return 0;
}

BlockExit execute_0x003096(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003096U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003096U) {
    // guest 0x003096 opcode 0x660C 660C bne.s loc_0030A4
    const auto opcode_0x003096 = fetch_checked(api, 0x660CU);
    api.begin_instruction(opcode_0x003096);
    branch_condition(api, 6U, 0x0030A4U, -14);
    api.finish_instruction(opcode_0x003096);
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

unsigned instruction_count_from_0x0030AA(unsigned entry_pc) {
    if (entry_pc == 0x0030AAU) return 1U;
    return 0;
}

BlockExit execute_0x0030AA(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0030AAU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0030AAU) {
    // guest 0x0030AA opcode 0x6702 6702 beq.s loc_0030AE
    const auto opcode_0x0030AA = fetch_checked(api, 0x6702U);
    api.begin_instruction(opcode_0x0030AA);
    branch_condition(api, 7U, 0x0030AEU, -14);
    api.finish_instruction(opcode_0x0030AA);
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

unsigned instruction_count_from_0x0030B6(unsigned entry_pc) {
    if (entry_pc == 0x0030B6U) return 1U;
    return 0;
}

BlockExit execute_0x0030B6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0030B6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0030B6U) {
    // guest 0x0030B6 opcode 0x0839 0839 0002 00FF 164D btst.b #$2,($00FF164D).L
    const auto opcode_0x0030B6 = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x0030B6);
    (void)fetch_checked(api, 0x0002U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    bit_test_immediate_absolute_long(api, 2U, 0xFF164DU);
    api.finish_instruction(opcode_0x0030B6);
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

unsigned instruction_count_from_0x0030D6(unsigned entry_pc) {
    if (entry_pc == 0x0030D6U) return 1U;
    return 0;
}

BlockExit execute_0x0030D6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0030D6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0030D6U) {
    // guest 0x0030D6 opcode 0x6700 6700 009E beq.w loc_003176
    const auto opcode_0x0030D6 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x0030D6);
    branch_condition(api, 7U, 0x003176U, 14, 0x009EU);
    api.finish_instruction(opcode_0x0030D6);
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

unsigned instruction_count_from_0x00317E(unsigned entry_pc) {
    if (entry_pc == 0x00317EU) return 1U;
    return 0;
}

BlockExit execute_0x00317E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00317EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00317EU) {
    // guest 0x00317E opcode 0x0839 0839 0001 00FF 164D btst.b #$1,($00FF164D).L
    const auto opcode_0x00317E = fetch_checked(api, 0x0839U);
    api.begin_instruction(opcode_0x00317E);
    (void)fetch_checked(api, 0x0001U);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x164DU);
    bit_test_immediate_absolute_long(api, 1U, 0xFF164DU);
    api.finish_instruction(opcode_0x00317E);
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

unsigned instruction_count_from_0x003188(unsigned entry_pc) {
    if (entry_pc == 0x003188U) return 1U;
    return 0;
}

BlockExit execute_0x003188(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003188U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003188U) {
    // guest 0x003188 opcode 0x4A39 4A39 00FF 0BFD tst.b ($00FF0BFD).L
    const auto opcode_0x003188 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x003188);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BFDU);
    test_absolute_long(api, 0xFF0BFDU, 1U);
    api.finish_instruction(opcode_0x003188);
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

unsigned instruction_count_from_0x00318E(unsigned entry_pc) {
    if (entry_pc == 0x00318EU) return 1U;
    return 0;
}

BlockExit execute_0x00318E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00318EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00318EU) {
    // guest 0x00318E opcode 0x6608 6608 bne.s loc_003198
    const auto opcode_0x00318E = fetch_checked(api, 0x6608U);
    api.begin_instruction(opcode_0x00318E);
    branch_condition(api, 6U, 0x003198U, -14);
    api.finish_instruction(opcode_0x00318E);
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
