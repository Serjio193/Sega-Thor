#include "tools/hybrid/m68k_00ad76_piece_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_piece(const M68k9D70StepApi& api, unsigned pc,
                                     unsigned a6, unsigned& resume_pc,
                                     M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AECCU: { // lea ($FF13CC).l, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA1, 0x00FF13CCU);
        if (!complete_step(api, 0x00AED2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AED2U: { // adda.w ($FF188C).l, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned offset = 0U;
        if (!read_be(api, 0x00FF188CU, 2, offset)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1);
        const auto ext_off = (offset & 0x8000U) ? (offset | 0xFFFF0000U) : offset;
        api.write_register(api.context, kRegA1, a1 + ext_off);
        if (!complete_step(api, 0x00AED8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AED8U: { // move.w ($FF188A).l, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned count = 0U;
        if (!read_be(api, 0x00FF188AU, 2, count)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (count & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((count & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((count & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AEDEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEDEU: { // tst.b 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, a6 + 57U, 1, b)) return M68k9D70StepResult::ContractFault;
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AEE2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEE2U: { // bne.w $00B012
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00B012U, 0x00AEE6U, resume_pc);
    }
    case 0x00AEE6U: { // move.w 62(a6), d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned w = 0U;
        if (!read_be(api, a6 + 62U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AEEAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEEAU: { // bmi.w $00AF02
        const bool taken = (ccr(api) & 0x08U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AF02U, 0x00AEEEU, resume_pc);
    }
    case 0x00AEEEU: { // btst #8, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool bit = (d5 & (1U << 8U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AEF2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEF2U: { // bne.w $00AF02
        const bool taken = (ccr(api) & 0x04U) == 0U;
        if (taken) {
            metrics.stepped_instructions++;
            return step_bcc_w(api, pc, true, 0x00AF02U, 0x00AEF6U, resume_pc);
        }
        return M68k9D70StepResult::NotHandled; // attached part hook at 0x00AEF6 (bsr $00B15E) -> fallback to ROM
    }
    case 0x00AF02U: { // move.w 24(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned w = 0U;
        if (!read_be(api, a6 + 24U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF06U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF06U: { // movea.l 26(a6), a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned l = 0U;
        if (!read_be(api, a6 + 26U, 4, l)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, l);
        if (!complete_step(api, 0x00AF0AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF0AU: { // adda.w d0, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        const auto ext_d0 = (d0 & 0x8000U) ? (d0 | 0xFFFF0000U) : d0;
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + ext_d0);
        if (!complete_step(api, 0x00AF0CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF0CU: { // swap d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res = ((d0 & 0xFFFFU) << 16U) | ((d0 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD0, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF0EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF0EU: { // move.w (a0), d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned w = 0U;
        if (!read_be(api, a0, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF10U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF10U: { // beq.w $00AFFA
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AF14U, resume_pc);
    }
    case 0x00AF14U: { // adda.w d5, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        const auto ext_d5 = (d5 & 0x8000U) ? (d5 | 0xFFFF0000U) : d5;
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + ext_d5);
        if (!complete_step(api, 0x00AF16U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF16U: { // move.w (a0)+, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto raw_a0 = api.read_register(api.context, kRegA0);
        unsigned w = 0U;
        if (!read_be(api, raw_a0 & kMask24, 2, w)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, raw_a0 + 2U);
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF18U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
