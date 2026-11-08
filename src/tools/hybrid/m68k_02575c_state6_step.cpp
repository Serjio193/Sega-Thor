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

M68k9D70StepResult step_state6(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept {
    const auto a6 = api.read_register(api.context, 14U) & kMask24;
    std::uint16_t opcode = 0U;

    switch (pc) {
    case 0x025A72U: { // moveq #1, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 0U, 1U);
        update_ccr_move_l(api, 1U);
        if (!complete_step(api, 0x025A74U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A74U: { // moveq #-1, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 1U, 0xFFFFFFFFU);
        update_ccr_move_l(api, 0xFFFFFFFFU);
        if (!complete_step(api, 0x025A76U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A76U: { // move.w d1, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, 1U);
        const auto old_d2 = api.read_register(api.context, 2U);
        api.write_register(api.context, 2U, (old_d2 & 0xFFFF0000U) | (d1 & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(d1));
        if (!complete_step(api, 0x025A78U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A78U: { // move.w d1, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, 1U);
        const auto old_d3 = api.read_register(api.context, 3U);
        api.write_register(api.context, 3U, (old_d3 & 0xFFFF0000U) | (d1 & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(d1));
        if (!complete_step(api, 0x025A7AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A7AU: { // neg.w d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d3 = api.read_register(api.context, 3U);
        const auto src = static_cast<std::uint16_t>(old_d3);
        const auto res = (0U - static_cast<std::uint32_t>(src)) & 0xFFFFU;
        api.write_register(api.context, 3U, (old_d3 & 0xFFFF0000U) | res);
        const std::uint8_t sm = (src >> 15U) & 1U;
        const std::uint8_t rm = (res >> 15U) & 1U;
        std::uint16_t new_ccr = (res == 0U ? 0x04U : 0U) | (rm ? 0x08U : 0U) |
                                ((sm && rm) ? 0x02U : 0U) | (src != 0U ? 0x11U : 0U);
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x025A7CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A7CU: { // move.w d3, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, 3U);
        const auto old_d4 = api.read_register(api.context, 4U);
        api.write_register(api.context, 4U, (old_d4 & 0xFFFF0000U) | (d3 & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(d3));
        if (!complete_step(api, 0x025A7EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A7EU: { // clr.w d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d5 = api.read_register(api.context, 5U);
        api.write_register(api.context, 5U, old_d5 & 0xFFFF0000U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, 0x025A80U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A80U: { // jsr ($00B852).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state6_steps++; metrics.state6_child_calls++;
        return dispatch_child_call(api, 0x00B852U, 0x025A86U, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025A86U: { // bcc.w $025AC2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x01U) == 0U) { // C == 0
            if (!complete_step(api, 0x025AC2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025A8AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A8AU: { // move.l 78(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 78U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 1U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025A8EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A8EU: { // move.l 82(a6), d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 82U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 2U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025A92U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A92U: { // move.l 86(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 86U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 0U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025A96U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A96U: { // subi.l #$0000C000, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto dst = api.read_register(api.context, 0U);
        const auto res = dst - 0x0000C000U;
        api.write_register(api.context, 0U, res);
        update_ccr_sub_l(api, 0x0000C000U, dst, res);
        if (!complete_step(api, 0x025A9CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025A9CU: { // move.l d0, 86(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        api.write(api.context, a6 + 86U, 4, d0);
        update_ccr_move_l(api, d0);
        if (!complete_step(api, 0x025AA0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AA0U: { // add.l d0, 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        unsigned dst = 0U;
        if (!read_be(api, a6 + 16U, 4, dst)) return M68k9D70StepResult::ContractFault;
        const auto res = dst + d0;
        api.write(api.context, a6 + 16U, 4, res);
        update_ccr_add_l(api, d0, dst, res);
        if (!complete_step(api, 0x025AA4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AA4U: { // ble.s $025AB0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto c = ccr(api);
        const bool z = (c & 0x04U) != 0U;
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        if (z || (n != v)) {
            if (!complete_step(api, 0x025AB0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025AA6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : -14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AA6U: { // add.l d1, 114(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, 1U);
        unsigned dst = 0U;
        if (!read_be(api, a6 + 114U, 4, dst)) return M68k9D70StepResult::ContractFault;
        const auto res = dst + d1;
        api.write(api.context, a6 + 114U, 4, res);
        update_ccr_add_l(api, d1, dst, res);
        if (!complete_step(api, 0x025AAAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AAAU: { // add.l d2, 118(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, 2U);
        unsigned dst = 0U;
        if (!read_be(api, a6 + 118U, 4, dst)) return M68k9D70StepResult::ContractFault;
        const auto res = dst + d2;
        api.write(api.context, a6 + 118U, 4, res);
        update_ccr_add_l(api, d2, dst, res);
        if (!complete_step(api, kState6ExitRts1, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AAEU: { // rts (exit 1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state6_steps++; metrics.state6_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AB0U: { // clr.l 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 16U, 4, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, 0x025AB4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AB4U: { // move.w #14, 4(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write(api.context, a6 + 4U, 2, 14U);
        update_ccr_move_w(api, 14U);
        if (!complete_step(api, 0x025ABAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025ABAU: { // move.w #14, 42(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write(api.context, a6 + 42U, 2, 14U);
        update_ccr_move_w(api, 14U);
        if (!complete_step(api, kState6ExitRts2, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AC0U: { // rts (exit 2)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state6_steps++; metrics.state6_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AC2U: { // btst.b #3, 56(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        unsigned byte_val = 0U;
        if (!read_be(api, a0 + 56U, 1, byte_val)) return M68k9D70StepResult::ContractFault;
        const bool bit_zero = (byte_val & 0x08U) == 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (bit_zero ? 0x04U : 0U));
        if (!complete_step(api, 0x025AC8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AC8U: { // bne.w $025AB4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, 0x025AB4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025ACCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025ACCU: { // move.l a6, 126(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 126U, 4, a6);
        update_ccr_move_l(api, a6);
        if (!complete_step(api, 0x025AD0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AD0U: { // move.b #4, 131(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 131U, 1, 4U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U));
        if (!complete_step(api, 0x025AD6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AD6U: { // clr.b 157(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 157U, 1, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, 0x025ADAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025ADAU: { // move.w 22(a6), 132(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        unsigned val = 0U;
        if (!read_be(api, a6 + 22U, 2, val)) return M68k9D70StepResult::ContractFault;
        api.write(api.context, a0 + 132U, 2, val);
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x025AE0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AE0U: { // move.l #$00800216, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x00800216U);
        update_ccr_move_l(api, 0x00800216U);
        if (!complete_step(api, 0x025AE6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AE6U: { // jsr ($00E7E8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state6_steps++; metrics.state6_child_calls++;
        return dispatch_child_call(api, 0x00E7E8U, 0x025AECU, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025AECU: { // move.l #$0080035B, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x0080035BU);
        update_ccr_move_l(api, 0x0080035BU);
        if (!complete_step(api, 0x025AF2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AF2U: { // jsr ($00E7E8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state6_steps++; metrics.state6_child_calls++;
        return dispatch_child_call(api, 0x00E7E8U, 0x025AF8U, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025AF8U: { // clr.l 78(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 78U, 4, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, 0x025AFCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025AFCU: { // clr.l 82(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 82U, 4, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, 0x025B00U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B00U: { // clr.l 86(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 86U, 4, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, 0x025B04U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B04U: { // move.w #2, 134(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 134U, 2, 2U);
        update_ccr_move_w(api, 2U);
        if (!complete_step(api, 0x025B0AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B0AU: { // tst.l 16(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        unsigned val = 0U;
        if (!read_be(api, a0 + 16U, 4, val)) return M68k9D70StepResult::ContractFault;
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x025B0EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B0EU: { // beq.w $025AB4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, 0x025AB4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025B12U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B12U: { // move.l #$00020000, 86(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 86U, 4, 0x00020000U);
        update_ccr_move_l(api, 0x00020000U);
        if (!complete_step(api, 0x025B1AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B1AU: { // move.w #$8002, 134(a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        api.write(api.context, a0 + 134U, 2, 0x8002U);
        update_ccr_move_w(api, 0x8002U);
        if (!complete_step(api, 0x025B20U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B20U: { // bra.s $025AB4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        if (!complete_step(api, 0x025AB4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state6_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_02575c
