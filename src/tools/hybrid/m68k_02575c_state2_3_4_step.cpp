#include "tools/hybrid/m68k_02575c_step_ops.hpp"
#include <cstdio>

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

M68k9D70StepResult step_state2_3_4(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept {
    const auto a6 = api.read_register(api.context, 14U) & kMask24;
    std::uint16_t opcode = 0U;

    switch (pc) {
    // ----------------------------------------------------
    // State 2: 0x025940..0x025968
    // ----------------------------------------------------
    case 0x025940U: { // subq.b #1, 131(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned old_val = 0U;
        if (!read_be(api, a6 + 131U, 1, old_val)) return M68k9D70StepResult::ContractFault;
        const auto new_val = (old_val - 1U) & 0xFFU;
        api.write(api.context, a6 + 131U, 1, new_val);
        update_ccr_subq_b(api, 1U, static_cast<std::uint8_t>(old_val), static_cast<std::uint8_t>(new_val));
        if (!complete_step(api, 0x025944U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025944U: { // beq.s $02595E
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, 0x02595EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025946U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : -14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025946U: { // bset.b #6, 54(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        do_bset_b(api, a6 + 54U, 6U);
        if (!complete_step(api, 0x02594CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02594CU: { // move.l 78(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 78U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 1U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025950U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025950U: { // add.l d1, 114(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, 1U);
        unsigned dst = 0U;
        if (!read_be(api, a6 + 114U, 4, dst)) return M68k9D70StepResult::ContractFault;
        const auto res = dst + d1;
        api.write(api.context, a6 + 114U, 4, res);
        update_ccr_add_l(api, d1, dst, res);
        if (!complete_step(api, 0x025954U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025954U: { // move.l 82(a6), d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 82U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 2U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025958U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025958U: { // add.l d2, 118(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, 2U);
        unsigned dst = 0U;
        if (!read_be(api, a6 + 118U, 4, dst)) return M68k9D70StepResult::ContractFault;
        const auto res = dst + d2;
        api.write(api.context, a6 + 118U, 4, res);
        update_ccr_add_l(api, d2, dst, res);
        if (!complete_step(api, kState2ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02595CU: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state2_steps++; metrics.state2_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02595EU: { // bset.b #3, 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        do_bset_b(api, a6 + 57U, 3U);
        if (!complete_step(api, 0x025964U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025964U: { // bra.w $0255D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if (!complete_step(api, kCommonJoinPc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state2_steps++; metrics.state2_loopback_exits++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }

    // ----------------------------------------------------
    // State 3: 0x025968..0x0259AC
    // ----------------------------------------------------
    case 0x025968U: { // jsr ($00E502).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state3_steps++; metrics.state3_child_calls++;
        return dispatch_child_call(api, 0x00E502U, 0x02596EU, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x02596EU: { // beq.w $025974
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, 0x025974U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, kState3ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025972U: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state3_steps++; metrics.state3_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025974U: { // bclr #3, 56(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        do_bclr_b(api, a6 + 56U, 3U);
        if (!complete_step(api, 0x02597AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02597AU: { // bclr #3, 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        do_bclr_b(api, a6 + 57U, 3U);
        if (!complete_step(api, 0x025980U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025980U: { // tst.w 138(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 138U, 2, val)) return M68k9D70StepResult::ContractFault;
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x025984U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025984U: { // bne.w $0255D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, kCommonJoinPc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            metrics.stepped_instructions++; metrics.state3_steps++; metrics.state3_loopback_exits++;
            active = false;
            return M68k9D70StepResult::InstructionStepped;
        }
        if (!complete_step(api, 0x025988U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        api.add_cycles(api.context, adj);
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025988U: { // move.w #6, 42(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write(api.context, a6 + 42U, 2, 6U);
        update_ccr_move_w(api, 6U);
        if (!complete_step(api, 0x02598EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02598EU: { // move.w #8, 4(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write(api.context, a6 + 4U, 2, 8U);
        update_ccr_move_w(api, 8U);
        if (!complete_step(api, 0x025994U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025994U: { // bclr #2, 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        do_bclr_b(api, a6 + 57U, 2U);
        if (!complete_step(api, 0x02599AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02599AU: { // ori.b #$0c, 56(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned old_val = 0U;
        if (!read_be(api, a6 + 56U, 1, old_val)) return M68k9D70StepResult::ContractFault;
        const auto new_val = (old_val | 0x0CU) & 0xFFU;
        api.write(api.context, a6 + 56U, 1, new_val);
        const auto old_ccr = ccr(api);
        const bool n = (new_val & 0x80U) != 0U;
        const bool z = (new_val == 0U);
        set_ccr(api, (old_ccr & 0x10U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U));
        if (!complete_step(api, 0x0259A0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259A0U: { // move.l #$0080035A, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x0080035AU);
        update_ccr_move_l(api, 0x0080035AU);
        if (!complete_step(api, 0x0259A6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state3_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259A6U: { // jmp ($00E7E8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        if (!branch_to(api, 0x00E7E8U, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state3_steps++; metrics.state3_transfer_exits++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }

    // ----------------------------------------------------
    // State 4: 0x0259AC..0x0259CE
    // ----------------------------------------------------
    case 0x0259ACU: { // btst.b #1, 55(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned byte_val = 0U;
        if (!read_be(api, a6 + 55U, 1, byte_val)) return M68k9D70StepResult::ContractFault;
        const bool bit_zero = (byte_val & 0x02U) == 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (bit_zero ? 0x04U : 0U));
        if (!complete_step(api, 0x0259B2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state4_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259B2U: { // beq.w $0259C4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, kState4ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0259B6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state4_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259B6U: { // lea 14(pc), a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write_register(api.context, 9U, 0x0259C6U);
        if (!complete_step(api, 0x0259BAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state4_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259BAU: { // jsr ($00F476).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state4_steps++; metrics.state4_child_calls++;
        return dispatch_child_call(api, 0x00F476U, 0x0259C0U, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x0259C0U: { // clr.w 0(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6, 2, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U); // Z=1, N=0, V=0, C=0
        if (!complete_step(api, kState4ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state4_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0259C4U: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state4_steps++; metrics.state4_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_02575c
