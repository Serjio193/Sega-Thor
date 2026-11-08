#include "tools/hybrid/m68k_00a9a0_ingress_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_ingress(const M68k9D70StepApi& api, unsigned pc,
                                       unsigned a6, unsigned& resume_pc,
                                       M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00A9A0U: { // btst.b #1, 54(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 54U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x02U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00A9A6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9A6U: { // bne.w $00ACFC
        const bool taken = (ccr(api) & 0x04U) == 0U;
        if (taken) metrics.fast_path_hidden_rts++;
        else metrics.full_render_calls++;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, kExitRtsPc, 0x00A9AAU, resume_pc);
    }
    case 0x00A9AAU: { // movem.l d7/a6, -(sp)
        metrics.stepped_instructions++;
        return step_movem_push_d7(api, pc, 0x00A9AEU, resume_pc);
    }
    case 0x00A9AEU: { // moveq #0, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegD0, 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | 0x04U); // Z=1, N=0, V=0, C=0
        if (!complete_step(api, 0x00A9B0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9B0U: { // suba.l a5, a5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA5, 0U);
        if (!complete_step(api, 0x00A9B2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9B2U: { // move.w 22(a6), d0
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
        if (!complete_step(api, 0x00A9B6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9B6U: { // cmpi.b #3, d0
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
        if (!complete_step(api, 0x00A9BAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9BAU: { // bne.w $00A9C4
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00A9C4U, 0x00A9BEU, resume_pc);
    }
    case 0x00A9BEU: { // move.l #$08000001, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, 0x08000001U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr); // N=0, Z=0, V=0, C=0
        if (!complete_step(api, 0x00A9C4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9C4U: { // add.w 50(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 50U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto a = static_cast<std::uint16_t>(d0 & 0xFFFFU);
        const auto b = static_cast<std::uint16_t>(mem_val & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((a ^ res) & (b ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00A9C8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9C8U: { // cmp.w 52(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 52U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto src_w = static_cast<std::uint16_t>(mem_val & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d0_w) - static_cast<std::uint32_t>(src_w);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool c = (d0_w < src_w);
        const bool v = (((d0_w ^ src_w) & (d0_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00A9CCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9CCU: { // beq.w $00A9F8
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00A9F8U, 0x00A9D0U, resume_pc);
    }
    case 0x00A9D0U: { // tst.w 52(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 52U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto val = static_cast<std::uint16_t>(mem_val & 0xFFFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (val == 0U) new_ccr |= 0x04U;
        if ((val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00A9D4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9D4U: { // bmi.s $00A9E4
        const bool taken = (ccr(api) & 0x08U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_s(api, pc, taken, 0x00A9E4U, 0x00A9D6U, resume_pc);
    }
    case 0x00A9D6U: { // move.w 76(a6), d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 76U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00A9DAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9DAU: { // cmp.w ($FF1652).L, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF1652U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d2_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD2) & 0xFFFFU);
        const auto src_w = static_cast<std::int16_t>(mem_val & 0xFFFFU);
        const auto sub = static_cast<std::int32_t>(d2_w) - static_cast<std::int32_t>(src_w);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFF);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (static_cast<std::uint16_t>(d2_w) < static_cast<std::uint16_t>(src_w));
        const bool v = (((d2_w ^ src_w) & (d2_w ^ res) & 0x8000) != 0);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00A9E0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9E0U: { // bgt.w $00A9EE
        const auto current_ccr = ccr(api);
        const bool z = (current_ccr & 0x04U) != 0U;
        const bool n = (current_ccr & 0x08U) != 0U;
        const bool v = (current_ccr & 0x02U) != 0U;
        const bool taken = (!z && (n == v));
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00A9EEU, 0x00A9E4U, resume_pc);
    }
    case 0x00A9E4U: { // move.w d0, 52(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        write_be(api, a6 + 52U, 2, d0);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d0 == 0U) new_ccr |= 0x04U;
        if ((d0 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00A9E8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9E8U: { // addq.w #1, a5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a5 = api.read_register(api.context, kRegA5);
        api.write_register(api.context, kRegA5, a5 + 1U);
        if (!complete_step(api, 0x00A9EAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9EAU: { // bra.w $00A9F8
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00A9F8U, resume_pc);
    }
    case 0x00A9EEU: { // move.w 52(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 52U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00A9F2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9F2U: { // st.b ($FF1651).L
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        write_be(api, 0x00FF1651U, 1, 0xFFU);
        if (!complete_step(api, 0x00A9F8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9F8U: { // add.w d0, d0
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
        if (!complete_step(api, 0x00A9FAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9FAU: { // move.w #$FFFF, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | 0xFFFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | 0x08U); // N=1, Z=0, V=0, C=0
        if (!complete_step(api, 0x00A9FEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00A9FEU: { // move.w 6(a6), d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 6U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA02U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA02U: { // beq.w $00AA74
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA74U, 0x00AA06U, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
