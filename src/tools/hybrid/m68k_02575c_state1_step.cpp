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

M68k9D70StepResult step_state1(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept {

    std::uint16_t opcode = 0U;
    const auto a6 = api.read_register(api.context, 14U) & kMask24;

    switch (pc) {
    case 0x0258A0U: { // clr.w d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 0U, old_d0 & 0xFFFF0000U);
        set_ccr(api, (ccr(api) & 0x10U) | 0x04U);
        if (!complete_step(api, 0x0258A2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258A2U: { // jsr ($00BCD8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state1_steps++; metrics.state1_child_calls++;
        return dispatch_child_call(api, kChild00BCD8Pc, 0x0258A8U, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x0258A8U: { // move.l 78(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 78U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 1U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x0258ACU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258ACU: { // move.l 82(a6), d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 82U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 2U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x0258B0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258B0U: { // cmpi.w #4, 48(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 48U, 2, val)) return M68k9D70StepResult::ContractFault;
        update_ccr_cmp_w(api, 4U, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x0258B6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258B6U: { // bhi.w $0258BC
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool condition = ((c & 0x01U) == 0U) && ((c & 0x04U) == 0U); // C==0 && Z==0
        if (condition) {
            if (!complete_step(api, 0x0258BCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, kState1ExitRts1, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258BAU: { // rts (exit 1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state1_steps++; metrics.state1_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258BCU: { // move.l 86(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 86U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 0U, val);
        update_ccr_move_l(api, val);
        if (!complete_step(api, 0x0258C0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258C0U: { // bmi.w $0258F4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x08U) != 0U) { // N==1
            if (!complete_step(api, 0x0258F4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0258C4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258C4U: { // beq.w $025932
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) { // Z==1
            if (!complete_step(api, 0x025932U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0258C8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258C8U: { // subi.l #$0000C000, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        const auto res = (d0 - 0x0000C000U) & 0xFFFFFFFFU;
        api.write_register(api.context, 0U, res);
        update_ccr_sub_l(api, 0x0000C000U, d0, res);
        if (!complete_step(api, 0x0258CEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258CEU: { // bgt.w $0258E2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool z = (c & 0x04U) != 0U;
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        if (!z && (n == v)) {
            if (!complete_step(api, 0x0258E2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0258D2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258D2U: { // bne.w $0258D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, 0x0258D8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x0258D6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258D6U: { // subq.l #1, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, 0U);
        const auto res = (d0 - 1U) & 0xFFFFFFFFU;
        api.write_register(api.context, 0U, res);
        update_ccr_sub_l(api, 1U, d0, res);
        if (!complete_step(api, 0x0258D8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258D8U: { // move.w #$000C, 48(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write(api.context, a6 + 48U, 2, 0x000CU);
        update_ccr_move_w(api, 0x000CU);
        if (!complete_step(api, 0x0258DEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258DEU: { // clr.w 46(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 46U, 2, 0U);
        set_ccr(api, (ccr(api) & 0x10U) | 0x04U);
        if (!complete_step(api, 0x0258E2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258E2U: { // move.l d0, 86(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        api.write(api.context, a6 + 86U, 4, d0);
        update_ccr_move_l(api, d0);
        if (!complete_step(api, 0x0258E6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258E6U: { // add.l d0, 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        unsigned height = 0U;
        if (!read_be(api, a6 + 16U, 4, height)) return M68k9D70StepResult::ContractFault;
        const auto res = (height + d0) & 0xFFFFFFFFU;
        api.write(api.context, a6 + 16U, 4, res);
        update_ccr_add_l(api, d0, height, res);
        if (!complete_step(api, 0x0258EAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258EAU: { // add.l d1, 114(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, 1U);
        unsigned x = 0U;
        if (!read_be(api, a6 + 114U, 4, x)) return M68k9D70StepResult::ContractFault;
        const auto res = (x + d1) & 0xFFFFFFFFU;
        api.write(api.context, a6 + 114U, 4, res);
        update_ccr_add_l(api, d1, x, res);
        if (!complete_step(api, 0x0258EEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258EEU: { // add.l d2, 118(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, 2U);
        unsigned y = 0U;
        if (!read_be(api, a6 + 118U, 4, y)) return M68k9D70StepResult::ContractFault;
        const auto res = (y + d2) & 0xFFFFFFFFU;
        api.write(api.context, a6 + 118U, 4, res);
        update_ccr_add_l(api, d2, y, res);
        if (!complete_step(api, kState1ExitRts2, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258F2U: { // rts (exit 2)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state1_steps++; metrics.state1_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258F4U: { // subi.l #$0000C000, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        const auto res = (d0 - 0x0000C000U) & 0xFFFFFFFFU;
        api.write_register(api.context, 0U, res);
        update_ccr_sub_l(api, 0x0000C000U, d0, res);
        if (!complete_step(api, 0x0258FAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258FAU: { // move.l d0, 86(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        api.write(api.context, a6 + 86U, 4, d0);
        update_ccr_move_l(api, d0);
        if (!complete_step(api, 0x0258FEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0258FEU: { // add.l d0, 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, 0U);
        unsigned height = 0U;
        if (!read_be(api, a6 + 16U, 4, height)) return M68k9D70StepResult::ContractFault;
        const auto res = (height + d0) & 0xFFFFFFFFU;
        api.write(api.context, a6 + 16U, 4, res);
        update_ccr_add_l(api, d0, height, res);
        if (!complete_step(api, 0x025902U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025902U: { // bgt.w $0258EA
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool z = (c & 0x04U) != 0U;
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        if (!z && (n == v)) {
            if (!complete_step(api, 0x0258EAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025906U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025906U: { // move.w #$0014, 48(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write(api.context, a6 + 48U, 2, 0x0014U);
        update_ccr_move_w(api, 0x0014U);
        if (!complete_step(api, 0x02590CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02590CU: { // clr.l d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 0U, 0U);
        set_ccr(api, (ccr(api) & 0x10U) | 0x04U);
        if (!complete_step(api, 0x02590EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02590EU: { // move.w d0, 46(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 46U, 2, 0U);
        update_ccr_move_w(api, 0U);
        if (!complete_step(api, 0x025912U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025912U: { // move.l d0, 16(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 16U, 4, 0U);
        update_ccr_move_l(api, 0U);
        if (!complete_step(api, 0x025916U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025916U: { // move.l d0, 86(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6 + 86U, 4, 0U);
        update_ccr_move_l(api, 0U);
        if (!complete_step(api, 0x02591AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02591AU: { // btst.b #1, 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 57U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x02U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x025920U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025920U: { // beq.w $025930
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, kState1ExitRts3, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025924U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025924U: { // move.l #$00500218, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, 0U, 0x00500218U);
        update_ccr_move_l(api, 0x00500218U);
        if (!complete_step(api, 0x02592AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02592AU: { // jsr ($00E7E8).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        metrics.stepped_instructions++; metrics.state1_steps++; metrics.state1_child_calls++;
        return dispatch_child_call(api, kChild00E7E8Pc, kState1ExitRts3, resume_pc, child_call_active, saved_child_sp, opcode);
    }
    case 0x025930U: { // rts (exit 3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state1_steps++; metrics.state1_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025932U: { // btst.b #1, 55(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 55U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x02U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x025938U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025938U: { // beq.w $0258F2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, kState1ExitRts2, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x02593CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state1_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02593CU: { // bra.w $0255D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if (!complete_step(api, kCommonJoinPc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state1_steps++; metrics.state1_loopback_exits++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_02575c
