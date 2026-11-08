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

M68k9D70StepResult step_target10(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept {

    std::uint16_t opcode = 0U;
    const auto a6 = api.read_register(api.context, 14U) & kMask24;

    switch (pc) {
    case 0x0257EEU: { // btst.b #3, ($00FF1A20).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, 0x00FF1A20U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x08U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x0257F6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0257F6U: { // bne.w $02583C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, kTarget10ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0257FAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0257FAU: { // move.w ($00FF19FC).l, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, 0x00FF19FCU, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 0U, (old_d0 & 0xFFFF0000U) | (val & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x025800U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025800U: { // cmp.w 20(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned src = 0U;
        if (!read_be(api, a6 + 20U, 2, src)) return M68k9D70StepResult::ContractFault;
        const auto dst = static_cast<std::uint16_t>(api.read_register(api.context, 0U) & 0xFFFFU);
        update_ccr_cmp_w(api, static_cast<std::uint16_t>(src), dst);
        if (!complete_step(api, 0x025804U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025804U: { // bne.w $02583C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, kTarget10ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025808U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025808U: { // move.w ($00FF19F0).l, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, 0x00FF19F0U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d1 = api.read_register(api.context, 1U);
        api.write_register(api.context, 1U, (old_d1 & 0xFFFF0000U) | (val & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x02580EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02580EU: { // move.w ($00FF19F4).l, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, 0x00FF19F4U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d2 = api.read_register(api.context, 2U);
        api.write_register(api.context, 2U, (old_d2 & 0xFFFF0000U) | (val & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x025814U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025814U: { // move.w 8(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 8U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d3 = api.read_register(api.context, 3U);
        api.write_register(api.context, 3U, (old_d3 & 0xFFFF0000U) | (val & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x025818U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025818U: { // move.w 12(a6), d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 12U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d4 = api.read_register(api.context, 4U);
        api.write_register(api.context, 4U, (old_d4 & 0xFFFF0000U) | (val & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x02581CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02581CU: { // jsr ($00F0D2).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const unsigned old_sp = api.read_register(api.context, 15U) & kMask24;
        const unsigned sp = (old_sp - 4U) & kMask24;
        if (!in_work_ram(sp, 4U)) return M68k9D70StepResult::ContractFault;
        api.write(api.context, sp, 4U, kTarget10ChildRet);
        api.write_register(api.context, 15U, sp);
        unsigned target_word{};
        if (!read_be(api, kChild00F0D2Pc, 2U, target_word)) return M68k9D70StepResult::ContractFault;
        api.set_return_state(api.context, kChild00F0D2Pc, kChild00F0D2Pc + 2U, target_word);
        api.finish_instruction(api.context, opcode);
        child_call_active = true;
        saved_child_sp = old_sp;
        resume_pc = kTarget10ChildRet;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        metrics.target10_child_calls++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025822U: { // cmpi.w #$0030, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = static_cast<std::uint16_t>(api.read_register(api.context, 0U) & 0xFFFFU);
        update_ccr_cmp_w(api, 0x0030U, d0);
        if (!complete_step(api, 0x025826U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025826U: { // bgt.w $02583C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool z = (c & 0x04U) != 0U;
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        if (!z && (n == v)) {
            if (!complete_step(api, kTarget10ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x02582AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02582AU: { // bclr.b #1, 54(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 54U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool old_bit1 = (val & 0x02U) != 0U;
        api.write(api.context, a6 + 54U, 1, val & ~0x02U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!old_bit1 ? 0x04U : 0U));
        if (!complete_step(api, 0x025830U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025830U: { // bclr.b #0, 55(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 55U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool old_bit0 = (val & 0x01U) != 0U;
        api.write(api.context, a6 + 55U, 1, val & ~0x01U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!old_bit0 ? 0x04U : 0U));
        if (!complete_step(api, 0x025836U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025836U: { // move.w #$0016, 4(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 4U, 2, 0x0016U);
        update_ccr_move_w(api, 0x0016U);
        if (!complete_step(api, kTarget10ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02583CU: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++;
        metrics.target10_steps++;
        metrics.target10_rts_exits++;
        metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_02575c
