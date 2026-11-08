#include "tools/hybrid/m68k_00a9a0_commit_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_commit(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& active,
                                      bool& child_call_active, unsigned& saved_child_sp,
                                      M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AC5EU: { // swap d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto swapped = (d0 >> 16U) | (d0 << 16U);
        api.write_register(api.context, kRegD0, swapped);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (swapped == 0U) new_ccr |= 0x04U;
        if ((swapped & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC60U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC60U: { // movea.l 4(sp), a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto sp = api.read_register(api.context, kRegSP) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, sp + 4U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, mem_val);
        if (!complete_step(api, 0x00AC64U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC64U: { // move.w 62(a6), d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 62U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC68U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC68U: { // bmi.w $00AC7C
        const bool taken = (ccr(api) & 0x08U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AC7CU, 0x00AC6CU, resume_pc);
    }
    case 0x00AC6CU: { // btst.l #8, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool bit = (d5 & (1U << 8U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AC70U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC70U: { // beq.w $00AC7C
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AC7CU, 0x00AC74U, resume_pc);
    }
    case 0x00AC74U: { // bsr.w $00B15E
        metrics.stepped_instructions++;
        metrics.bsr_00b15e_calls++;
        return step_bsr_w(api, pc, 0x00B15EU, 0x00AC78U, resume_pc, child_call_active, saved_child_sp);
    }
    case 0x00AC78U: { // movea.l 4(sp), a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto sp = api.read_register(api.context, kRegSP) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, sp + 4U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, mem_val);
        if (!complete_step(api, 0x00AC7CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC7CU: { // tst.b 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 57U, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto val = static_cast<std::uint8_t>(mem_val & 0xFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (val == 0U) new_ccr |= 0x04U;
        if ((val & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC80U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC80U: { // bne.w $00AD4A
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AD4AU, 0x00AC84U, resume_pc);
    }
    case 0x00AC84U: { // bclr.l #31, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & 0x80000000U) != 0U;
        api.write_register(api.context, kRegD3, d3 & ~0x80000000U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        // Musashi m68k_op_bclr_32_s_d adds 14 master cycles (2 CPU cycles) when bit > 15
        if (!complete_step(api, 0x00AC88U, opcode, 14, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC88U: { // beq.w $00ACE0
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ACE0U, 0x00AC8CU, resume_pc);
    }
    case 0x00AC8CU: { // cmpi.w #$0050, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto d2_w = static_cast<std::uint16_t>(d2 & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d2_w) - 0x0050U;
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool c = (d2_w < 0x0050U);
        const bool v = (((d2_w ^ 0x0050U) & (d2_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const auto x = ccr(api) & 0x10U;
        set_ccr(api, x | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC90U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC90U: { // bcc.w $00ACE0
        const bool taken = (ccr(api) & 0x01U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ACE0U, 0x00AC94U, resume_pc);
    }
    case 0x00AC94U: { // tst.b ($FF1927).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF1927U, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto val = static_cast<std::uint8_t>(mem_val & 0xFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (val == 0U) new_ccr |= 0x04U;
        if ((val & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC9AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC9AU: { // bne.w $00ACE0
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ACE0U, 0x00AC9EU, resume_pc);
    }
    case 0x00AC9EU: { // move.w d3, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (d3 & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((d3 & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((d3 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACA0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACE0U: { // suba.l #$00FF13CC, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, a1 - 0x00FF13CCU);
        if (!complete_step(api, 0x00ACE6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACE6U: { // move.w a1, ($FF188C).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & 0xFFFFU;
        write_be(api, 0x00FF188CU, 2, a1);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (a1 == 0U) new_ccr |= 0x04U;
        if ((a1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACECU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACECU: { // move.w d2, ($FF188A).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        write_be(api, 0x00FF188AU, 2, d2);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACF2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACF2U: { // move.l a2, ($FF1892).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a2 = api.read_register(api.context, kRegA2);
        write_be(api, 0x00FF1892U, 4, a2);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (a2 == 0U) new_ccr |= 0x04U;
        if ((a2 & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACF8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACF8U: { // movem.l (sp)+, d7/a6
        metrics.stepped_instructions++;
        return step_movem_pop_d7(api, pc, 0x00ACFCU, resume_pc);
    }
    case 0x00ACFCU: { // rts
        metrics.stepped_instructions++;
        metrics.completed_calls++;
        active = false;
        return step_rts(api, pc, resume_pc);
    }
    // --- SAT OVERFLOW EXIT ---
    case 0x00AD1EU: { // suba.l #$00FF13CC, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, a1 - 0x00FF13CCU);
        if (!complete_step(api, 0x00AD24U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD24U: { // move.w a1, ($FF188C).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & 0xFFFFU;
        write_be(api, 0x00FF188CU, 2, a1);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (a1 == 0U) new_ccr |= 0x04U;
        if ((a1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AD2AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD2AU: { // move.w d2, ($FF188A).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        write_be(api, 0x00FF188AU, 2, d2);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AD30U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD30U: { // move.l a2, ($FF1892).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a2 = api.read_register(api.context, kRegA2);
        write_be(api, 0x00FF1892U, 4, a2);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (a2 == 0U) new_ccr |= 0x04U;
        if ((a2 & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AD36U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD36U: { // movem.l (sp)+, d7/a6
        metrics.stepped_instructions++;
        return step_movem_pop_d7(api, pc, 0x00AD3AU, resume_pc);
    }
    case 0x00AD3AU: { // move.w #$ffff, 52(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a6 = api.read_register(api.context, kRegA6) & kMask24;
        write_be(api, a6 + 52U, 2, 0xFFFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr | 0x08U; // negative
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AD40U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD40U: { // rts
        metrics.stepped_instructions++;
        metrics.completed_calls++;
        active = false;
        return step_rts(api, pc, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}
} // namespace oasis::hybrid
