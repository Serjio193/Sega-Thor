#include "tools/hybrid/m68k_00ad76_ingress_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_ingress(const M68k9D70StepApi& api, unsigned pc,
                                       unsigned a6, unsigned& resume_pc,
                                       M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AD76U: { // btst #1, 54(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 54U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x02U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AD7CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD7CU: { // bne.w $00B010
        const bool taken = (ccr(api) & 0x04U) == 0U;
        if (taken) metrics.fast_path_hidden_rts++;
        else metrics.full_render_calls++;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, kExitRtsPc, 0x00AD80U, resume_pc);
    }
    case 0x00AD80U: { // movem.l d7, -(sp)
        metrics.stepped_instructions++;
        return step_movem_push_d7(api, pc, 0x00AD84U, resume_pc);
    }
    case 0x00AD84U: { // moveq #0, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegD0, 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | 0x04U); // Z=1, N=0, V=0, C=0
        if (!complete_step(api, 0x00AD86U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD86U: { // move.w 22(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 22U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        const auto new_d0 = (old_d0 & 0xFFFF0000U) | (val & 0xFFFFU);
        api.write_register(api.context, kRegD0, new_d0);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AD8AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD8AU: { // cmpi.b #3, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0_b = static_cast<std::uint8_t>(api.read_register(api.context, kRegD0) & 0xFFU);
        const std::uint8_t imm = 0x03U;
        const std::uint32_t res = (static_cast<std::uint32_t>(d0_b) - static_cast<std::uint32_t>(imm)) & 0xFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x80U) != 0U);
        const bool c = (d0_b < imm);
        const bool v = (((d0_b ^ imm) & (d0_b ^ res) & 0x80U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AD8EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD8EU: { // bne.w $00AD98
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AD98U, 0x00AD92U, resume_pc);
    }
    case 0x00AD92U: { // move.l #$08000001, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, 0x08000001U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr); // N=0, Z=0, V=0, C=0
        if (!complete_step(api, 0x00AD98U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD98U: { // add.w 50(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_w = 0U;
        if (!read_be(api, a6 + 50U, 2, mem_w)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto val = d0 & 0xFFFFU;
        const auto add_v = mem_w & 0xFFFFU;
        const auto res = (val + add_v) & 0xFFFFU;
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res);
        const bool c = ((val + add_v) > 0xFFFFU);
        const bool v = (((val ^ res) & (add_v ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AD9CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD9CU: { // cmp.w 52(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_w = 0U;
        if (!read_be(api, a6 + 52U, 2, mem_w)) return M68k9D70StepResult::ContractFault;
        const auto dst = api.read_register(api.context, kRegD0) & 0xFFFFU;
        const auto src = mem_w & 0xFFFFU;
        const auto res = (dst - src) & 0xFFFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (dst < src);
        const bool v = (((dst ^ src) & (dst ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ADA0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADA0U: { // beq.w $00ADA8
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ADA8U, 0x00ADA4U, resume_pc);
    }
    case 0x00ADA4U: { // move.w d0, 52(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        write_be(api, a6 + 52U, 2, d0);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d0 == 0U) new_ccr |= 0x04U;
        if ((d0 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ADA8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADA8U: { // add.w d0, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto val = d0 & 0xFFFFU;
        const auto res = (val + val) & 0xFFFFU;
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res);
        const bool c = ((val + val) > 0xFFFFU);
        const bool v = (((val ^ res) & (val ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ADAAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADAAU: { // move.w #$FFFF, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | 0xFFFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | 0x08U); // N=1, Z=0, V=0, C=0
        if (!complete_step(api, 0x00ADAEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADAEU: { // move.w 6(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 6U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFF0000U) | (val & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ADB2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ADB2U: { // beq.w $00ADF6
        const bool taken = (ccr(api) & 0x04U) != 0U;
        if (taken) {
            metrics.stepped_instructions++;
            return step_bcc_w(api, pc, true, 0x00ADF6U, 0x00ADB6U, resume_pc);
        }
        return M68k9D70StepResult::NotHandled; // fallback for non-zero 6(a6)
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
