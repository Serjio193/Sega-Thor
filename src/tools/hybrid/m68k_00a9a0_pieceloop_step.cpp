#include "tools/hybrid/m68k_00a9a0_pieceloop_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_pieceloop(const M68k9D70StepApi& api, unsigned pc,
                                         unsigned a6, unsigned& resume_pc,
                                         M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00ABE0U: { // move.w d7, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7_w = api.read_register(api.context, kRegD7) & 0xFFFFU;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | d7_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d7_w == 0U) new_ccr |= 0x04U;
        if ((d7_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABE2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABE2U: { // eor.w d0, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = api.read_register(api.context, kRegD0) & 0xFFFFU;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto res = (d6 & 0xFFFFU) ^ d0_w;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABE4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABE4U: { // andi.w #$F800, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto res = (d6 & 0xFFFFU) & 0xF800U;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABE8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABE8U: { // add.w a3, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a3_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegA3) & 0xFFFFU);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d6_w = static_cast<std::uint16_t>(d6 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d6_w) + static_cast<std::uint32_t>(a3_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d6_w ^ res) & (a3_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABEAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABEAU: { // add.w d3, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d6_w = static_cast<std::uint16_t>(d6 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d6_w) + static_cast<std::uint32_t>(d3_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d6_w ^ res) & (d3_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABECU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABECU: { // move.w d6, 4(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6_w = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1 + 4U, 2, d6_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d6_w == 0U) new_ccr |= 0x04U;
        if ((d6_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABF0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABF0U: { // move.w 4(a0), d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0 + 4U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ABF4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABF4U: { // btst #11, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const bool bit = (d0 & (1U << 11U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00ABF8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABF8U: { // beq.w $00ABFE
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ABFEU, 0x00ABFCU, resume_pc);
    }
    case 0x00ABFCU: { // lsr.w #8, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto val = static_cast<std::uint16_t>(d6 & 0xFFFFU);
        const bool c = (val & 0x80U) != 0U;
        const auto res = static_cast<std::uint16_t>(val >> 8U);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool z = (res == 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (z ? 0x04U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ABFEU, opcode, 112, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ABFEU: { // ext.w d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto ext_val = static_cast<std::uint16_t>(static_cast<std::int16_t>(static_cast<std::int8_t>(d6 & 0xFFU)));
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | ext_val);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (ext_val == 0U) new_ccr |= 0x04U;
        if ((ext_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC00U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC00U: { // add.w d1, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD1) & 0xFFFFU);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d6_w = static_cast<std::uint16_t>(d6 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d6_w) + static_cast<std::uint32_t>(d1_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d6_w ^ res) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC02U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC02U: { // move.w d6, 6(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6_w = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1 + 6U, 2, d6_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d6_w == 0U) new_ccr |= 0x04U;
        if ((d6_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC06U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC06U: { // subi.w #$0060, d6
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
        if (!complete_step(api, 0x00AC0AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC0AU: { // cmpi.w #$0160, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD6) & 0xFFFFU);
        const std::uint16_t imm = 0x0160U;
        const auto sub = static_cast<std::uint32_t>(d6_w) - static_cast<std::uint32_t>(imm);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d6_w < imm);
        const bool v = (((d6_w ^ imm) & (d6_w ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC0EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC0EU: { // bhi.w $00AC16
        const auto current_ccr = ccr(api);
        const bool c = (current_ccr & 0x01U) != 0U;
        const bool z = (current_ccr & 0x04U) != 0U;
        const bool taken = (!c && !z);
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AC16U, 0x00AC12U, resume_pc);
    }
    case 0x00AC12U: { // addq.w #1, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto cur = static_cast<std::uint16_t>(d2 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(cur) + 1U;
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((cur ^ res) & 0x8000U) != 0U && ((cur ^ 1U) & 0x8000U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC14U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC14U: { // addq.l #8, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, (a1 + 8U));
        if (!complete_step(api, 0x00AC16U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}
} // namespace oasis::hybrid
