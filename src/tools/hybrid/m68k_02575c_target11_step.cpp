#include "tools/hybrid/m68k_02575c_step_ops.hpp"

namespace oasis::hybrid::step_02575c {
namespace {

using step_623ac::read_be;
using step_623ac::in_work_ram;
using step_623ac::prepare_step;
using step_623ac::complete_step;
using step_623ac::branch_to;
using step_623ac::ccr;
using step_623ac::set_ccr;

} // namespace

M68k9D70StepResult step_target11(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    M68k02575CStepMetrics& metrics, bool& active) noexcept {

    std::uint16_t opcode = 0U;
    const auto a6 = api.read_register(api.context, 14U) & kMask24;

    switch (pc) {
    case 0x02583EU: { // move.l 86(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 86U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 0U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025842U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025842U: { // sub.l d0, 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        unsigned height = 0U;
        if (!read_be(api, a6 + 16U, 4, height)) return M68k9D70StepResult::ContractFault;
        const auto next_height = (height - d0) & 0xFFFFFFFFU;
        api.write(api.context, a6 + 16U, 4, next_height);
        update_ccr_sub_l(api, d0, height, next_height);
        if (!complete_step(api, 0x025846U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025846U: { // bmi.w $025856
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x08U) != 0U) {
            if (!complete_step(api, 0x025856U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x02584AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02584AU: { // addi.l #$0000C000, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto old_d0 = api.read_register(api.context, 0U);
        const auto new_d0 = (old_d0 + 0x0000C000U) & 0xFFFFFFFFU;
        api.write_register(api.context, 0U, new_d0);
        update_ccr_add_l(api, 0x0000C000U, old_d0, new_d0);
        if (!complete_step(api, 0x025850U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025850U: { // move.l d0, 86(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        api.write(api.context, a6 + 86U, 4, d0);
        update_ccr_move_l(api, d0);
        if (!complete_step(api, kTarget11ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025854U: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        metrics.target11_rts_exits++;
        metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025856U: { // clr.l 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 16U, 4, 0U);
        set_ccr(api, (ccr(api) & 0x10U) | 0x04U);
        if (!complete_step(api, 0x02585AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02585AU: { // clr.w 46(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 46U, 2, 0U);
        set_ccr(api, (ccr(api) & 0x10U) | 0x04U);
        if (!complete_step(api, 0x02585EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02585EU: { // move.w #$0018, 4(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 4U, 2, 0x0018U);
        set_ccr(api, ccr(api) & 0x10U);
        if (!complete_step(api, 0x025864U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025864U: { // move.l #$00500342, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x00500342U);
        update_ccr_move_l(api, 0x00500342U);
        if (!complete_step(api, 0x02586AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02586AU: { // btst.b #1, 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 57U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x02U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x025870U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025870U: { // beq.w $02587A
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, 0x02587AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025874U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025874U: { // move.l #$00800270, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x00800270U);
        update_ccr_move_l(api, 0x00800270U);
        if (!complete_step(api, 0x02587AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02587AU: { // jmp ($0000E7E8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        if (!complete_step(api, kTailJump00E7E8Pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target11_steps++;
        metrics.target11_transfer_exits++;
        active = false; // Intentional ROM continuation
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_02575c
