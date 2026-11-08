#include "tools/hybrid/m68k_00ad76_attrib_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_attrib(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned a6, unsigned& resume_pc,
                                      M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00ADF6U: { // move.w d2, 62(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        write_be(api, a6 + 62U, 2, d2);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ADFAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADFAU: { // moveq #0, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegD3, 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | 0x04U);
        if (!complete_step(api, 0x00ADFCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADFCU: { // move.b 54(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, a6 + 54U, 1, b)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFFFF00U) | (b & 0xFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE00U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE00U: { // move.w 16(a6), d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned w = 0U;
        if (!read_be(api, a6 + 16U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE04U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE04U: { // bmi.w $00AE28
        const bool taken = (ccr(api) & 0x08U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE28U, 0x00AE08U, resume_pc);
    }
    case 0x00AE08U: { // bne.w $00AE14
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE14U, 0x00AE0CU, resume_pc);
    }
    case 0x00AE0CU: { // btst #2, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 2U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE10U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE10U: { // beq.w $00AE28
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE28U, 0x00AE14U, resume_pc);
    }
    case 0x00AE14U: { // btst #3, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 3U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE18U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE18U: { // bne.w $00AE28
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE28U, 0x00AE1CU, resume_pc);
    }
    case 0x00AE1CU: { // swap d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto res = ((d3 & 0xFFFFU) << 16U) | ((d3 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD3, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE1EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE1EU: { // move.b 61(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, a6 + 61U, 1, b)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFFFF00U) | (b & 0xFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE22U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE22U: { // bset #15, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 15U)) != 0U;
        api.write_register(api.context, kRegD3, d3 | (1U << 15U));
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE26U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE26U: { // swap d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto res = ((d3 & 0xFFFFU) << 16U) | ((d3 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD3, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE28U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE28U: { // btst #4, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 4U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE2CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE2CU: { // beq.w $00AE42
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE42U, 0x00AE30U, resume_pc);
    }
    case 0x00AE30U: { // move.w 12(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned w = 0U;
        if (!read_be(api, a6 + 12U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE34U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE34U: { // movea.w d1, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto ext_val = (d1 & 0x8000U) ? (d1 | 0xFFFF0000U) : d1;
        api.write_register(api.context, kRegA4, ext_val);
        if (!complete_step(api, 0x00AE36U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE36U: { // suba.w d2, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const auto ext_d2 = (d2 & 0x8000U) ? (d2 | 0xFFFF0000U) : d2;
        const auto a4 = api.read_register(api.context, kRegA4);
        api.write_register(api.context, kRegA4, a4 - ext_d2);
        if (!complete_step(api, 0x00AE38U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE38U: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto res = ((d1 & 0xFFFFU) << 16U) | ((d1 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD1, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE3AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE3AU: { // move.w 8(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned w = 0U;
        if (!read_be(api, a6 + 8U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE3EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE3EU: { // bra.w $00AE9C
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AE9CU, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
