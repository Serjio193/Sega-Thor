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

M68k9D70StepResult step_state5(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept {
    const auto a6 = api.read_register(api.context, 14U) & kMask24;
    std::uint16_t opcode = 0U;

    switch (pc) {
    case 0x0259CEU: { // cmpi.w #12, 48(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 48U, 2, val)) return M68k9D70StepResult::ContractFault;
        update_ccr_cmp_w(api, 12U, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x0259D4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259D4U: { // bne.w $0259E0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, 0x0259E0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0259D8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259D8U: { // tst.w 46(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 46U, 2, val)) return M68k9D70StepResult::ContractFault;
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x0259DCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259DCU: { // beq.w $0259EC
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, 0x0259ECU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0259E0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259E0U: { // btst.b #1, 55(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned byte_val = 0U;
        if (!read_be(api, a6 + 55U, 1, byte_val)) return M68k9D70StepResult::ContractFault;
        const bool bit_zero = (byte_val & 0x02U) == 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (bit_zero ? 0x04U : 0U));
        if (!complete_step(api, 0x0259E6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259E6U: { // bne.w $0255D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, kCommonJoinPc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            metrics.stepped_instructions++; metrics.state5_steps++; metrics.state5_loopback_exits++;
            active = false;
            return M68k9D70StepResult::InstructionStepped;
        }
        if (!complete_step(api, kState5ExitRts1, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        api.add_cycles(api.context, adj);
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259EAU: { // rts (exit 1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state5_steps++; metrics.state5_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259ECU: { // movea.l a6, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 12U, a6);
        if (!complete_step(api, 0x0259EEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259EEU: { // lea ($00FF19E8).l, a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, 14U, 0x00FF19E8U);
        if (!complete_step(api, 0x0259F4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259F4U: { // adda.w 158(a4), a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a4 = api.read_register(api.context, 12U) & kMask24;
        unsigned offset = 0U;
        if (!read_be(api, a4 + 158U, 2, offset)) return M68k9D70StepResult::ContractFault;
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        const auto updated_a6 = (curr_a6 + static_cast<std::int16_t>(offset)) & kMask24;
        api.write_register(api.context, 14U, updated_a6);
        if (!complete_step(api, 0x0259F8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259F8U: { // cmpi.w #$002E, 0(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        unsigned val = 0U;
        if (!read_be(api, curr_a6, 2, val)) return M68k9D70StepResult::ContractFault;
        update_ccr_cmp_w(api, 0x002EU, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x0259FEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259FEU: { // bne.w $025A6E
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, 0x025A6EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025A02U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A02U: { // jsr ($00E66E).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state5_steps++; metrics.state5_child_calls++;
        return dispatch_child_call(api, 0x00E66EU, 0x025A08U, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025A08U: { // sf 61(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 61U, 1, 0U);
        if (!complete_step(api, 0x025A0CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A0CU: { // move.b #14, 56(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 56U, 1, 14U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U));
        if (!complete_step(api, 0x025A12U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A12U: { // bset.b #3, 58(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        do_bset_b(api, curr_a6 + 58U, 3U);
        if (!complete_step(api, 0x025A18U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A18U: { // move.w #12, 4(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 4U, 2, 12U);
        update_ccr_move_w(api, 12U);
        if (!complete_step(api, 0x025A1EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A1EU: { // move.w #12, 42(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 42U, 2, 12U);
        update_ccr_move_w(api, 12U);
        if (!complete_step(api, 0x025A24U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A24U: { // move.w #24, 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 16U, 2, 24U);
        update_ccr_move_w(api, 24U);
        if (!complete_step(api, 0x025A2AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A2AU: { // move.w #60, 2(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 2U, 2, 60U);
        update_ccr_move_w(api, 60U);
        if (!complete_step(api, 0x025A30U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A30U: { // move.l ($00FF19F0).l, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, 0x00FF19F0U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 1U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025A36U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A36U: { // move.l ($00FF19F4).l, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, 0x00FF19F4U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 2U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025A3CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A3CU: { // sub.l 8(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        unsigned src = 0U;
        if (!read_be(api, curr_a6 + 8U, 4, src)) return M68k9D70StepResult::ContractFault;
        const auto dst = api.read_register(api.context, 1U);
        const auto res = dst - src;
        api.write_register(api.context, 1U, res);
        update_ccr_sub_l(api, src, dst, res);
        if (!complete_step(api, 0x025A40U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A40U: { // sub.l 12(a6), d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        unsigned src = 0U;
        if (!read_be(api, curr_a6 + 12U, 4, src)) return M68k9D70StepResult::ContractFault;
        const auto dst = api.read_register(api.context, 2U);
        const auto res = dst - src;
        api.write_register(api.context, 2U, res);
        update_ccr_sub_l(api, src, dst, res);
        if (!complete_step(api, 0x025A44U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A44U: { // asr.l #4, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d1 = static_cast<std::int32_t>(api.read_register(api.context, 1U));
        const auto new_d1 = static_cast<std::uint32_t>(old_d1 >> 4);
        api.write_register(api.context, 1U, new_d1);
        update_ccr_move_l(api, new_d1);
        if (!complete_step(api, 0x025A46U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A46U: { // asr.l #4, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d2 = static_cast<std::int32_t>(api.read_register(api.context, 2U));
        const auto new_d2 = static_cast<std::uint32_t>(old_d2 >> 4);
        api.write_register(api.context, 2U, new_d2);
        update_ccr_move_l(api, new_d2);
        if (!complete_step(api, 0x025A48U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A48U: { // move.l d1, 78(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        const auto d1 = api.read_register(api.context, 1U);
        api.write(api.context, curr_a6 + 78U, 4, d1);
        update_ccr_move_l(api, d1);
        if (!complete_step(api, 0x025A4CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A4CU: { // move.l d2, 82(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        const auto d2 = api.read_register(api.context, 2U);
        api.write(api.context, curr_a6 + 82U, 4, d2);
        update_ccr_move_l(api, d2);
        if (!complete_step(api, 0x025A50U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A50U: { // move.l #$00060000, 86(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        api.write(api.context, curr_a6 + 86U, 4, 0x00060000U);
        update_ccr_move_l(api, 0x00060000U);
        if (!complete_step(api, 0x025A58U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A58U: { // jsr ($00E338).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state5_steps++; metrics.state5_child_calls++;
        return dispatch_child_call(api, 0x00E338U, 0x025A5EU, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025A5EU: { // move.w d3, 22(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto curr_a6 = api.read_register(api.context, 14U) & kMask24;
        const auto d3 = api.read_register(api.context, 3U);
        api.write(api.context, curr_a6 + 22U, 2, d3 & 0xFFFFU);
        update_ccr_move_w(api, static_cast<std::uint16_t>(d3));
        if (!complete_step(api, 0x025A62U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A62U: { // move.l #$00700217, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x00700217U);
        update_ccr_move_l(api, 0x00700217U);
        if (!complete_step(api, 0x025A68U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A68U: { // jsr ($00E7E8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state5_steps++; metrics.state5_child_calls++;
        return dispatch_child_call(api, 0x00E7E8U, 0x025A6EU, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025A6EU: { // movea.l a4, a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, 12U) & kMask24;
        api.write_register(api.context, 14U, a4);
        if (!complete_step(api, kState5ExitRts2, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state5_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A70U: { // rts (exit 2)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state5_steps++; metrics.state5_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_02575c
