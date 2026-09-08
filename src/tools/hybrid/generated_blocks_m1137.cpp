// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

unsigned instruction_count_from_0x0003A0(unsigned entry_pc) {
    if (entry_pc == 0x0003A0U) return 1U;
    return 0;
}

BlockExit execute_0x0003A0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003A0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003A0U) {
    // guest 0x0003A0 opcode 0x51CA 51CA FFDE dbf D2,loc_000380
    const auto opcode_0x0003A0 = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x0003A0);
    dbcc(api, 1U, 2U, 0x000380U, 0xFFDEU);
    api.finish_instruction(opcode_0x0003A0);
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

unsigned instruction_count_from_0x03A8AC(unsigned entry_pc) {
    if (entry_pc == 0x03A8ACU) return 1U;
    return 0;
}

BlockExit execute_0x03A8AC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A8ACU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A8ACU) {
    // guest 0x03A8AC opcode 0x6600 6600 000C bne.w loc_03A8BA
    const auto opcode_0x03A8AC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A8AC);
    branch_condition(api, 6U, 0x03A8BAU, 14, 0x000CU);
    api.finish_instruction(opcode_0x03A8AC);
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

unsigned instruction_count_from_0x060312(unsigned entry_pc) {
    if (entry_pc == 0x060312U) return 1U;
    return 0;
}

BlockExit execute_0x060312(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x060312U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x060312U) {
    // guest 0x060312 opcode 0x51C8 51C8 FFFC dbf D0,loc_060310
    const auto opcode_0x060312 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x060312);
    dbcc(api, 1U, 0U, 0x060310U, 0xFFFCU);
    api.finish_instruction(opcode_0x060312);
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

unsigned instruction_count_from_0x002230(unsigned entry_pc) {
    if (entry_pc == 0x002230U) return 1U;
    return 0;
}

BlockExit execute_0x002230(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002230U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002230U) {
    // guest 0x002230 opcode 0x51C8 51C8 FFFE dbf D0,loc_002230
    const auto opcode_0x002230 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x002230);
    dbcc(api, 1U, 0U, 0x002230U, 0xFFFEU);
    api.finish_instruction(opcode_0x002230);
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

unsigned instruction_count_from_0x061360(unsigned entry_pc) {
    if (entry_pc == 0x061360U) return 1U;
    return 0;
}

BlockExit execute_0x061360(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061360U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061360U) {
    // guest 0x061360 opcode 0x51C8 51C8 FFFC dbf D0,loc_06135E
    const auto opcode_0x061360 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x061360);
    dbcc(api, 1U, 0U, 0x06135EU, 0xFFFCU);
    api.finish_instruction(opcode_0x061360);
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

unsigned instruction_count_from_0x003A0E(unsigned entry_pc) {
    if (entry_pc == 0x003A0EU) return 1U;
    return 0;
}

BlockExit execute_0x003A0E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003A0EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003A0EU) {
    // guest 0x003A0E opcode 0x51CA 51CA FFFC dbf D2,loc_003A0C
    const auto opcode_0x003A0E = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x003A0E);
    dbcc(api, 1U, 2U, 0x003A0CU, 0xFFFCU);
    api.finish_instruction(opcode_0x003A0E);
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

unsigned instruction_count_from_0x0038A0(unsigned entry_pc) {
    if (entry_pc == 0x0038A0U) return 1U;
    return 0;
}

BlockExit execute_0x0038A0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0038A0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0038A0U) {
    // guest 0x0038A0 opcode 0x51C8 51C8 FFFC dbf D0,loc_00389E
    const auto opcode_0x0038A0 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x0038A0);
    dbcc(api, 1U, 0U, 0x00389EU, 0xFFFCU);
    api.finish_instruction(opcode_0x0038A0);
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

unsigned instruction_count_from_0x0003F2(unsigned entry_pc) {
    if (entry_pc == 0x0003F2U) return 1U;
    return 0;
}

BlockExit execute_0x0003F2(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0003F2U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0003F2U) {
    // guest 0x0003F2 opcode 0x51C8 51C8 FFFC dbf D0,loc_0003F0
    const auto opcode_0x0003F2 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x0003F2);
    dbcc(api, 1U, 0U, 0x0003F0U, 0xFFFCU);
    api.finish_instruction(opcode_0x0003F2);
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

unsigned instruction_count_from_0x003818(unsigned entry_pc) {
    if (entry_pc == 0x003818U) return 1U;
    return 0;
}

BlockExit execute_0x003818(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003818U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003818U) {
    // guest 0x003818 opcode 0x66F6 66F6 bne.s loc_003810
    const auto opcode_0x003818 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003818);
    branch_condition(api, 6U, 0x003810U, -14);
    api.finish_instruction(opcode_0x003818);
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

unsigned instruction_count_from_0x0030BE(unsigned entry_pc) {
    if (entry_pc == 0x0030BEU) return 1U;
    return 0;
}

BlockExit execute_0x0030BE(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x0030BEU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x0030BEU) {
    // guest 0x0030BE opcode 0x66F6 66F6 bne.s loc_0030B6
    const auto opcode_0x0030BE = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x0030BE);
    branch_condition(api, 6U, 0x0030B6U, -14);
    api.finish_instruction(opcode_0x0030BE);
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

unsigned instruction_count_from_0x03A758(unsigned entry_pc) {
    if (entry_pc == 0x03A758U) return 1U;
    return 0;
}

BlockExit execute_0x03A758(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x03A758U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x03A758U) {
    // guest 0x03A758 opcode 0x66F6 66F6 bne.s loc_03A750
    const auto opcode_0x03A758 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x03A758);
    branch_condition(api, 6U, 0x03A750U, -14);
    api.finish_instruction(opcode_0x03A758);
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

unsigned instruction_count_from_0x00D994(unsigned entry_pc) {
    if (entry_pc == 0x00D994U) return 1U;
    return 0;
}

BlockExit execute_0x00D994(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x00D994U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x00D994U) {
    // guest 0x00D994 opcode 0x51CB 51CB FFFA dbf D3,loc_00D990
    const auto opcode_0x00D994 = fetch_checked(api, 0x51CBU);
    api.begin_instruction(opcode_0x00D994);
    dbcc(api, 1U, 3U, 0x00D990U, 0xFFFAU);
    api.finish_instruction(opcode_0x00D994);
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

unsigned instruction_count_from_0x003186(unsigned entry_pc) {
    if (entry_pc == 0x003186U) return 1U;
    return 0;
}

BlockExit execute_0x003186(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003186U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003186U) {
    // guest 0x003186 opcode 0x66F6 66F6 bne.s loc_00317E
    const auto opcode_0x003186 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003186);
    branch_condition(api, 6U, 0x00317EU, -14);
    api.finish_instruction(opcode_0x003186);
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

unsigned instruction_count_from_0x003250(unsigned entry_pc) {
    if (entry_pc == 0x003250U) return 1U;
    return 0;
}

BlockExit execute_0x003250(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x003250U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x003250U) {
    // guest 0x003250 opcode 0x66F6 66F6 bne.s loc_003248
    const auto opcode_0x003250 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003250);
    branch_condition(api, 6U, 0x003248U, -14);
    api.finish_instruction(opcode_0x003250);
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

unsigned instruction_count_from_0x061938(unsigned entry_pc) {
    if (entry_pc == 0x061938U) return 1U;
    return 0;
}

BlockExit execute_0x061938(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x061938U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x061938U) {
    // guest 0x061938 opcode 0x6600 6600 000C bne.w loc_061946
    const auto opcode_0x061938 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061938);
    branch_condition(api, 6U, 0x061946U, 14, 0x000CU);
    api.finish_instruction(opcode_0x061938);
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

unsigned instruction_count_from_0x002C18(unsigned entry_pc) {
    if (entry_pc == 0x002C18U) return 1U;
    return 0;
}

BlockExit execute_0x002C18(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002C18U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002C18U) {
    // guest 0x002C18 opcode 0x66F8 66F8 bne.s loc_002C12
    const auto opcode_0x002C18 = fetch_checked(api, 0x66F8U);
    api.begin_instruction(opcode_0x002C18);
    branch_condition(api, 6U, 0x002C12U, -14);
    api.finish_instruction(opcode_0x002C18);
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
