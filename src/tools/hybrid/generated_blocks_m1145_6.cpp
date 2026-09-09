// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x002A3C(unsigned entry_pc) {
    if (entry_pc == 0x002A3CU) return 1U;
    return 0;
}

BlockExit execute_0x002A3C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A3CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A3CU) {
    // guest 0x002A3C opcode 0x4E71 4E71 nop
    const auto opcode_0x002A3C = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A3C);

    api.finish_instruction(opcode_0x002A3C);
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

unsigned instruction_count_from_0x002A3E(unsigned entry_pc) {
    if (entry_pc == 0x002A3EU) return 1U;
    return 0;
}

BlockExit execute_0x002A3E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A3EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A3EU) {
    // guest 0x002A3E opcode 0x4E71 4E71 nop
    const auto opcode_0x002A3E = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A3E);

    api.finish_instruction(opcode_0x002A3E);
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

unsigned instruction_count_from_0x002A46(unsigned entry_pc) {
    if (entry_pc == 0x002A46U) return 1U;
    return 0;
}

BlockExit execute_0x002A46(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A46U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A46U) {
    // guest 0x002A46 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A46 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A46);

    api.finish_instruction(opcode_0x002A46);
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

unsigned instruction_count_from_0x002A48(unsigned entry_pc) {
    if (entry_pc == 0x002A48U) return 1U;
    return 0;
}

BlockExit execute_0x002A48(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A48U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A48U) {
    // guest 0x002A48 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A48 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A48);

    api.finish_instruction(opcode_0x002A48);
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

unsigned instruction_count_from_0x002A4A(unsigned entry_pc) {
    if (entry_pc == 0x002A4AU) return 1U;
    return 0;
}

BlockExit execute_0x002A4A(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A4AU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A4AU) {
    // guest 0x002A4A opcode 0x4E71 4E71 nop
    const auto opcode_0x002A4A = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A4A);

    api.finish_instruction(opcode_0x002A4A);
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

unsigned instruction_count_from_0x002A4C(unsigned entry_pc) {
    if (entry_pc == 0x002A4CU) return 1U;
    return 0;
}

BlockExit execute_0x002A4C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A4CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A4CU) {
    // guest 0x002A4C opcode 0x4E71 4E71 nop
    const auto opcode_0x002A4C = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A4C);

    api.finish_instruction(opcode_0x002A4C);
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

unsigned instruction_count_from_0x002A4E(unsigned entry_pc) {
    if (entry_pc == 0x002A4EU) return 1U;
    return 0;
}

BlockExit execute_0x002A4E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A4EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A4EU) {
    // guest 0x002A4E opcode 0x4E71 4E71 nop
    const auto opcode_0x002A4E = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A4E);

    api.finish_instruction(opcode_0x002A4E);
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

unsigned instruction_count_from_0x002A50(unsigned entry_pc) {
    if (entry_pc == 0x002A50U) return 1U;
    return 0;
}

BlockExit execute_0x002A50(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A50U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A50U) {
    // guest 0x002A50 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A50 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A50);

    api.finish_instruction(opcode_0x002A50);
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

unsigned instruction_count_from_0x002A52(unsigned entry_pc) {
    if (entry_pc == 0x002A52U) return 1U;
    return 0;
}

BlockExit execute_0x002A52(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A52U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A52U) {
    // guest 0x002A52 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A52 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A52);

    api.finish_instruction(opcode_0x002A52);
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

unsigned instruction_count_from_0x002A54(unsigned entry_pc) {
    if (entry_pc == 0x002A54U) return 1U;
    return 0;
}

BlockExit execute_0x002A54(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A54U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A54U) {
    // guest 0x002A54 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A54 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A54);

    api.finish_instruction(opcode_0x002A54);
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

unsigned instruction_count_from_0x002A5C(unsigned entry_pc) {
    if (entry_pc == 0x002A5CU) return 1U;
    return 0;
}

BlockExit execute_0x002A5C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A5CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A5CU) {
    // guest 0x002A5C opcode 0x4E71 4E71 nop
    const auto opcode_0x002A5C = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A5C);

    api.finish_instruction(opcode_0x002A5C);
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

unsigned instruction_count_from_0x002A5E(unsigned entry_pc) {
    if (entry_pc == 0x002A5EU) return 1U;
    return 0;
}

BlockExit execute_0x002A5E(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A5EU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A5EU) {
    // guest 0x002A5E opcode 0x4E71 4E71 nop
    const auto opcode_0x002A5E = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A5E);

    api.finish_instruction(opcode_0x002A5E);
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

unsigned instruction_count_from_0x002A60(unsigned entry_pc) {
    if (entry_pc == 0x002A60U) return 1U;
    return 0;
}

BlockExit execute_0x002A60(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A60U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A60U) {
    // guest 0x002A60 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A60 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A60);

    api.finish_instruction(opcode_0x002A60);
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

unsigned instruction_count_from_0x002A62(unsigned entry_pc) {
    if (entry_pc == 0x002A62U) return 1U;
    return 0;
}

BlockExit execute_0x002A62(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A62U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A62U) {
    // guest 0x002A62 opcode 0x4E71 4E71 nop
    const auto opcode_0x002A62 = fetch_checked(api, 0x4E71U);
    api.begin_instruction(opcode_0x002A62);

    api.finish_instruction(opcode_0x002A62);
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

unsigned instruction_count_from_0x002A68(unsigned entry_pc) {
    if (entry_pc == 0x002A68U) return 1U;
    return 0;
}

BlockExit execute_0x002A68(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002A68U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002A68U) {
    // guest 0x002A68 opcode 0x6600 6600 003A bne.w loc_002AA4
    const auto opcode_0x002A68 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x002A68);
    branch_condition(api, 6U, 0x002AA4U, 14, 0x003AU);
    api.finish_instruction(opcode_0x002A68);
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

unsigned instruction_count_from_0x002AF4(unsigned entry_pc) {
    if (entry_pc == 0x002AF4U) return 1U;
    return 0;
}

BlockExit execute_0x002AF4(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002AF4U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002AF4U) {
    // guest 0x002AF4 opcode 0x51C8 51C8 FFF8 dbf D0,loc_002AEE
    const auto opcode_0x002AF4 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x002AF4);
    dbcc(api, 1U, 0U, 0x002AEEU, 0xFFF8U);
    api.finish_instruction(opcode_0x002AF4);
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

unsigned instruction_count_from_0x002AFC(unsigned entry_pc) {
    if (entry_pc == 0x002AFCU) return 1U;
    return 0;
}

BlockExit execute_0x002AFC(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x002AFCU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x002AFCU) {
    // guest 0x002AFC opcode 0x43F9 43F9 00FF 0BF2 lea.l ($00FF0BF2).L,A1
    const auto opcode_0x002AFC = fetch_checked(api, 0x43F9U);
    api.begin_instruction(opcode_0x002AFC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BF2U);
    lea_absolute_long(api, 0xFF0BF2U, 1U);
    api.finish_instruction(opcode_0x002AFC);
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

}
 // namespace oasis::hybrid::generated
