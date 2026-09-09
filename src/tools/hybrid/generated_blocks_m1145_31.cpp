// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"
namespace oasis::hybrid::generated {


unsigned instruction_count_from_0x062910(unsigned entry_pc) {
    if (entry_pc == 0x062910U) return 1U;
    return 0;
}

BlockExit execute_0x062910(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062910U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062910U) {
    // guest 0x062910 opcode 0xD040 D040 add.w D0,D0
    const auto opcode_0x062910 = fetch_checked(api, 0xD040U);
    api.begin_instruction(opcode_0x062910);
    add_w_data_to_data(api, 0U, 0U);
    api.finish_instruction(opcode_0x062910);
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

unsigned instruction_count_from_0x062912(unsigned entry_pc) {
    if (entry_pc == 0x062912U) return 1U;
    return 0;
}

BlockExit execute_0x062912(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062912U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062912U) {
    // guest 0x062912 opcode 0xD2C0 D2C0 adda.w D0,A1
    const auto opcode_0x062912 = fetch_checked(api, 0xD2C0U);
    api.begin_instruction(opcode_0x062912);
    adda_w_data_to_address(api, 0U, 1U);
    api.finish_instruction(opcode_0x062912);
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

unsigned instruction_count_from_0x062914(unsigned entry_pc) {
    if (entry_pc == 0x062914U) return 1U;
    return 0;
}

BlockExit execute_0x062914(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062914U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062914U) {
    // guest 0x062914 opcode 0xD040 D040 add.w D0,D0
    const auto opcode_0x062914 = fetch_checked(api, 0xD040U);
    api.begin_instruction(opcode_0x062914);
    add_w_data_to_data(api, 0U, 0U);
    api.finish_instruction(opcode_0x062914);
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

unsigned instruction_count_from_0x062916(unsigned entry_pc) {
    if (entry_pc == 0x062916U) return 1U;
    return 0;
}

BlockExit execute_0x062916(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062916U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062916U) {
    // guest 0x062916 opcode 0xD2C0 D2C0 adda.w D0,A1
    const auto opcode_0x062916 = fetch_checked(api, 0xD2C0U);
    api.begin_instruction(opcode_0x062916);
    adda_w_data_to_address(api, 0U, 1U);
    api.finish_instruction(opcode_0x062916);
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

unsigned instruction_count_from_0x062930(unsigned entry_pc) {
    if (entry_pc == 0x062930U) return 1U;
    return 0;
}

BlockExit execute_0x062930(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062930U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062930U) {
    // guest 0x062930 opcode 0x6700 6700 0006 beq.w loc_062938
    const auto opcode_0x062930 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062930);
    branch_condition(api, 7U, 0x062938U, 14, 0x0006U);
    api.finish_instruction(opcode_0x062930);
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

unsigned instruction_count_from_0x06293C(unsigned entry_pc) {
    if (entry_pc == 0x06293CU) return 1U;
    return 0;
}

BlockExit execute_0x06293C(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x06293CU) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x06293CU) {
    // guest 0x06293C opcode 0x0807 0807 001F btst.l #$1F,D7
    const auto opcode_0x06293C = fetch_checked(api, 0x0807U);
    api.begin_instruction(opcode_0x06293C);
    (void)fetch_checked(api, 0x001FU);
    bit_test_immediate_data(api, 31U, 7U);
    api.finish_instruction(opcode_0x06293C);
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

unsigned instruction_count_from_0x062940(unsigned entry_pc) {
    if (entry_pc == 0x062940U) return 1U;
    return 0;
}

BlockExit execute_0x062940(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062940U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062940U) {
    // guest 0x062940 opcode 0x6700 6700 0022 beq.w loc_062964
    const auto opcode_0x062940 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062940);
    branch_condition(api, 7U, 0x062964U, 14, 0x0022U);
    api.finish_instruction(opcode_0x062940);
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

unsigned instruction_count_from_0x062966(unsigned entry_pc) {
    if (entry_pc == 0x062966U) return 1U;
    return 0;
}

BlockExit execute_0x062966(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062966U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062966U) {
    // guest 0x062966 opcode 0x6400 6400 0006 bcc.w loc_06296E
    const auto opcode_0x062966 = fetch_checked(api, 0x6400U);
    api.begin_instruction(opcode_0x062966);
    branch_condition(api, 4U, 0x06296EU, 14, 0x0006U);
    api.finish_instruction(opcode_0x062966);
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

unsigned instruction_count_from_0x062972(unsigned entry_pc) {
    if (entry_pc == 0x062972U) return 1U;
    return 0;
}

BlockExit execute_0x062972(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062972U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062972U) {
    // guest 0x062972 opcode 0x6300 6300 0004 bls.w loc_062978
    const auto opcode_0x062972 = fetch_checked(api, 0x6300U);
    api.begin_instruction(opcode_0x062972);
    branch_condition(api, 3U, 0x062978U, 14, 0x0004U);
    api.finish_instruction(opcode_0x062972);
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

unsigned instruction_count_from_0x062AE0(unsigned entry_pc) {
    if (entry_pc == 0x062AE0U) return 1U;
    return 0;
}

BlockExit execute_0x062AE0(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062AE0U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062AE0U) {
    // guest 0x062AE0 opcode 0x4A39 4A39 00FF 0013 tst.b ($00FF0013).L
    const auto opcode_0x062AE0 = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x062AE0);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0013U);
    test_absolute_long(api, 0xFF0013U, 1U);
    api.finish_instruction(opcode_0x062AE0);
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

unsigned instruction_count_from_0x062AE6(unsigned entry_pc) {
    if (entry_pc == 0x062AE6U) return 1U;
    return 0;
}

BlockExit execute_0x062AE6(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062AE6U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062AE6U) {
    // guest 0x062AE6 opcode 0x6700 6700 0016 beq.w loc_062AFE
    const auto opcode_0x062AE6 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062AE6);
    branch_condition(api, 7U, 0x062AFEU, 14, 0x0016U);
    api.finish_instruction(opcode_0x062AE6);
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

unsigned instruction_count_from_0x062B04(unsigned entry_pc) {
    if (entry_pc == 0x062B04U) return 1U;
    return 0;
}

BlockExit execute_0x062B04(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062B04U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062B04U) {
    // guest 0x062B04 opcode 0x6600 6600 0010 bne.w loc_062B16
    const auto opcode_0x062B04 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x062B04);
    branch_condition(api, 6U, 0x062B16U, 14, 0x0010U);
    api.finish_instruction(opcode_0x062B04);
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

unsigned instruction_count_from_0x062B12(unsigned entry_pc) {
    if (entry_pc == 0x062B12U) return 1U;
    return 0;
}

BlockExit execute_0x062B12(BasicBlockApi& api, unsigned entry_pc) {
    unsigned instructions_executed = 0;
    bool execute_from_here = false;
    if (entry_pc != 0x062B12U) return {entry_pc, BlockExitReason::FALLBACK, 0};
    if (execute_from_here || entry_pc == 0x062B12U) {
    // guest 0x062B12 opcode 0x6700 6700 0008 beq.w loc_062B1C
    const auto opcode_0x062B12 = fetch_checked(api, 0x6700U);
    api.begin_instruction(opcode_0x062B12);
    branch_condition(api, 7U, 0x062B1CU, 14, 0x0008U);
    api.finish_instruction(opcode_0x062B12);
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
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

}
 // namespace oasis::hybrid::generated
