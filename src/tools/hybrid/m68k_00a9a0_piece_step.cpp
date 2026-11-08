#include "tools/hybrid/m68k_00a9a0_piece_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_piece(const M68k9D70StepApi& api, unsigned pc,
                                     unsigned a6, unsigned& resume_pc,
                                     bool& child_call_active, unsigned& saved_child_sp,
                                     M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AB6AU: { // move.w 62(a6), d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 62U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB6EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB6EU: { // bmi.w $00AB82
        const bool taken = (ccr(api) & 0x08U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AB82U, 0x00AB72U, resume_pc);
    }
    case 0x00AB72U: { // btst #8, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool bit = (d5 & (1U << 8U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AB76U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB76U: { // bne.w $00AB82
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AB82U, 0x00AB7AU, resume_pc);
    }
    case 0x00AB7AU: { // bsr.w $00B15E
        metrics.stepped_instructions++;
        metrics.bsr_00b15e_calls++;
        return step_bsr_w(api, pc, 0x00B15EU, 0x00AB7EU, resume_pc, child_call_active, saved_child_sp);
    }
    case 0x00AB7EU: { // movea.l 4(sp), a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto sp = api.read_register(api.context, kRegSP) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, sp + 4U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, mem_val);
        if (!complete_step(api, 0x00AB82U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB82U: { // move.w 24(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto cur_a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, cur_a6 + 24U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB86U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB86U: { // movea.l 26(a6), a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto cur_a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, cur_a6 + 26U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, mem_val);
        if (!complete_step(api, 0x00AB8AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB8AU: { // move.l 30(a6), d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto cur_a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, cur_a6 + 30U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegD5, mem_val);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (mem_val == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB8EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB8EU: { // lsr.l #1, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool c = (d5 & 1U) != 0U;
        const auto res = d5 >> 1U;
        api.write_register(api.context, kRegD5, res);
        const bool z = (res == 0U);
        const bool n = ((res & 0x80000000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AB90U, opcode, 14, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB90U: { // movea.l d5, a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegA6, d5);
        if (!complete_step(api, 0x00AB92U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB92U: { // adda.w d0, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d0_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        api.write_register(api.context, kRegA0, static_cast<unsigned>(static_cast<int>(a0) + d0_w));
        if (!complete_step(api, 0x00AB94U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB94U: { // swap d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto swapped = (d0 >> 16U) | (d0 << 16U);
        api.write_register(api.context, kRegD0, swapped);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (swapped == 0U) new_ccr |= 0x04U;
        if ((swapped & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB96U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB96U: { // movea.w #$0000, a3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA3, 0U);
        if (!complete_step(api, 0x00AB9AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB9AU: { // moveq #0, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegD5, 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | 0x04U);
        if (!complete_step(api, 0x00AB9CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB9CU: { // cmpa.w #0, a5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a5_w = static_cast<std::int16_t>(api.read_register(api.context, kRegA5) & 0xFFFFU);
        const bool z = (a5_w == 0);
        const bool n = (a5_w < 0);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U));
        if (!complete_step(api, 0x00ABA0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABA0U: { // beq.w $00ABA8
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ABA8U, 0x00ABA4U, resume_pc);
    }
    case 0x00ABA4U: { // bset #31, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool bit = (d5 & 0x80000000U) != 0U;
        api.write_register(api.context, kRegD5, d5 | 0x80000000U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        // Musashi m68k_op_bset_32_s_d adds 14 master cycles (2 CPU cycles) when bit > 15
        if (!complete_step(api, 0x00ABA8U, opcode, 14, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABA8U: { // lea $01AC(pc), a5 -> 0x00AD56
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA5, 0x00AD56U);
        if (!complete_step(api, 0x00ABACU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABACU: { // move.w (a0), d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABAEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABAEU: { // beq.w $00AD1E
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, kSatOverflowPc, 0x00ABB2U, resume_pc);
    }
    case 0x00ABB2U: { // adda.w d5, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d5_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD5) & 0xFFFFU);
        api.write_register(api.context, kRegA0, static_cast<unsigned>(static_cast<int>(a0) + d5_w));
        if (!complete_step(api, 0x00ABB4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABB4U: { // move.w (a0)+, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned mem_val = 0U;
        if (!read_be(api, a0 & kMask24, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, (a0 + 2U));
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABB6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABB6U: { // cmpi.w #$0050, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD2) & 0xFFFFU);
        const std::uint16_t imm = 0x0050U;
        const auto sub = static_cast<std::uint32_t>(d2_w) - static_cast<std::uint32_t>(imm);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d2_w < imm);
        const bool v = (((d2_w ^ imm) & (d2_w ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABBAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        metrics.piece_loop_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABBAU: { // bcc.w $00AD1E
        const bool taken = (ccr(api) & 0x01U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, kSatOverflowPc, 0x00ABBEU, resume_pc);
    }
    case 0x00ABBEU: { // move.w (a0), d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABC0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABC0U: { // move.w d6, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6_w = api.read_register(api.context, kRegD6) & 0xFFFFU;
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | d6_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d6_w == 0U) new_ccr |= 0x04U;
        if ((d6_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABC2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABC2U: { // ext.w d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto ext_val = static_cast<std::uint16_t>(static_cast<std::int16_t>(static_cast<std::int8_t>(d6 & 0xFFU)));
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | ext_val);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (ext_val == 0U) new_ccr |= 0x04U;
        if ((ext_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABC4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABC4U: { // add.w a4, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegA4) & 0xFFFFU);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d6_w = static_cast<std::uint16_t>(d6 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d6_w) + static_cast<std::uint32_t>(a4_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d6_w ^ res) & (a4_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABC6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABC6U: { // move.w d6, (a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6_w = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1, 2, d6_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d6_w == 0U) new_ccr |= 0x04U;
        if ((d6_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABC8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABC8U: { // move.w 2(a0), d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0 + 2U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABCCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABCCU: { // subi.w #$0060, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d6_w = static_cast<std::uint16_t>(d6 & 0xFFFFU);
        const std::uint16_t imm = 0x0060U;
        const auto sub = static_cast<std::uint32_t>(d6_w) - static_cast<std::uint32_t>(imm);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (d6_w < imm);
        const bool v = (((d6_w ^ imm) & (d6_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABD0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABD0U: { // cmpi.w #$0100, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD6) & 0xFFFFU);
        const std::uint16_t imm = 0x0100U;
        const auto sub = static_cast<std::uint32_t>(d6_w) - static_cast<std::uint32_t>(imm);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d6_w < imm);
        const bool v = (((d6_w ^ imm) & (d6_w ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABD4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABD4U: { // bhi.w $00AC16
        const auto current_ccr = ccr(api);
        const bool c = (current_ccr & 0x01U) != 0U;
        const bool z = (current_ccr & 0x04U) != 0U;
        const bool taken = (!c && !z);
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AC16U, 0x00ABD8U, resume_pc);
    }
    case 0x00ABD8U: { // move.b d2, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2_b = api.read_register(api.context, kRegD2) & 0xFFU;
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFFFF00U) | d2_b);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d2_b == 0U) new_ccr |= 0x04U;
        if ((d2_b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABDAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABDAU: { // addq.b #1, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto cur = static_cast<std::uint8_t>(d4 & 0xFFU);
        const auto sum = static_cast<std::uint32_t>(cur) + 1U;
        const auto res = static_cast<std::uint8_t>(sum & 0xFFU);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFFFF00U) | res);
        const bool c = (sum > 0xFFU);
        const bool v = (((cur ^ res) & 0x80U) != 0U && ((cur ^ 1U) & 0x80U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x80U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABDCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABDCU: { // move.w d4, 2(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d4_w = api.read_register(api.context, kRegD4) & 0xFFFFU;
        write_be(api, a1 + 2U, 2, d4_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d4_w == 0U) new_ccr |= 0x04U;
        if ((d4_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABE0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}
} // namespace oasis::hybrid
