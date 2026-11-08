#include "tools/hybrid/m68k_00a9a0_shadow_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_shadow(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc,
                                      M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00ACA0U: { // swap d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto swapped = (d3 >> 16U) | (d3 << 16U);
        api.write_register(api.context, kRegD3, swapped);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (swapped == 0U) new_ccr |= 0x04U;
        if ((swapped & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACA2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACA2U: { // lea 90(pc, d3.w), a6 -> table at 0x00ACFE + d3.w
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = static_cast<std::int16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU);
        const unsigned target_addr = (0x00ACFEU + static_cast<unsigned>(static_cast<int>(d3))) & kMask24;
        api.write_register(api.context, kRegA6, target_addr);
        if (!complete_step(api, 0x00ACA6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACA6U: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto swapped = (d1 >> 16U) | (d1 << 16U);
        api.write_register(api.context, kRegD1, swapped);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (swapped == 0U) new_ccr |= 0x04U;
        if ((swapped & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACA8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACA8U: { // sub.w (a6)+, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a6 = api.read_register(api.context, kRegA6);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 & kMask24, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, (a6 + 2U));
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto src_w = static_cast<std::uint16_t>(mem_val & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d1_w) - static_cast<std::uint32_t>(src_w);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (d1_w < src_w);
        const bool v = (((d1_w ^ src_w) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACAAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACAAU: { // move.w d1, (a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        write_be(api, a1, 2, d1);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d1 == 0U) new_ccr |= 0x04U;
        if ((d1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACACU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACACU: { // subi.w #$0060, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d1_w) - 0x0060U;
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (d1_w < 0x0060U);
        const bool v = (((d1_w ^ 0x0060U) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACB0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACB0U: { // cmpi.w #$0100, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d1_w) - 0x0100U;
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool c = (d1_w < 0x0100U);
        const bool v = (((d1_w ^ 0x0100U) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const auto x = ccr(api) & 0x10U;
        set_ccr(api, x | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACB4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACB4U: { // bhi.w $00ACE0
        const auto c = ccr(api);
        const bool taken = (c & 0x05U) == 0U; // C=0 and Z=0
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ACE0U, 0x00ACB8U, resume_pc);
    }
    case 0x00ACB8U: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto swapped = (d1 >> 16U) | (d1 << 16U);
        api.write_register(api.context, kRegD1, swapped);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (swapped == 0U) new_ccr |= 0x04U;
        if ((swapped & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACBAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACBAU: { // move.w (a6)+, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a6 = api.read_register(api.context, kRegA6);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 & kMask24, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, (a6 + 2U));
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACBCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACBCU: { // move.b d2, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFFFF00U) | (d2 & 0xFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((d2 & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACBEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACBEU: { // addq.w #1, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto d4_w = static_cast<std::uint16_t>(d4 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d4_w) + 1U;
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d4_w ^ res) & 0x8000U) != 0U && ((d4_w ^ 1U) & 0x8000U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACC0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACC0U: { // move.w d4, 2(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        write_be(api, a1 + 2U, 2, d4);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d4 == 0U) new_ccr |= 0x04U;
        if ((d4 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACC4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACC4U: { // move.w (a6)+, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a6 = api.read_register(api.context, kRegA6);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 & kMask24, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, (a6 + 2U));
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACC6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACC6U: { // move.w d6, 4(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1 + 4U, 2, d6);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d6 == 0U) new_ccr |= 0x04U;
        if ((d6 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACCAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACCAU: { // sub.w (a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a6, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto src_w = static_cast<std::uint16_t>(mem_val & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d1_w) - static_cast<std::uint32_t>(src_w);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (d1_w < src_w);
        const bool v = (((d1_w ^ src_w) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACCCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACCCU: { // move.w d1, 6(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        write_be(api, a1 + 6U, 2, d1);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d1 == 0U) new_ccr |= 0x04U;
        if ((d1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00ACD0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACD0U: { // subi.w #$0060, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d1_w) - 0x0060U;
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (d1_w < 0x0060U);
        const bool v = (((d1_w ^ 0x0060U) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACD4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACD4U: { // cmpi.w #$0160, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d1_w) - 0x0160U;
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool c = (d1_w < 0x0160U);
        const bool v = (((d1_w ^ 0x0160U) & (d1_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const auto x = ccr(api) & 0x10U;
        set_ccr(api, x | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACD8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACD8U: { // bhi.w $00ACE0
        const auto c = ccr(api);
        const bool taken = (c & 0x05U) == 0U; // C=0 and Z=0
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00ACE0U, 0x00ACDCU, resume_pc);
    }
    case 0x00ACDCU: { // addq.w #1, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto d2_w = static_cast<std::uint16_t>(d2 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d2_w) + 1U;
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d2_w ^ res) & 0x8000U) != 0U && ((d2_w ^ 1U) & 0x8000U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00ACDEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00ACDEU: { // addq.l #8, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, (a1 + 8U));
        if (!complete_step(api, 0x00ACE0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}
} // namespace oasis::hybrid
