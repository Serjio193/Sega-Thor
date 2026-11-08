#include "tools/hybrid/m68k_00a9a0_attrib_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_attrib(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned a6, unsigned& resume_pc,
                                      M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AA06U: { // move.l 94(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 94U, 4, val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegD1, val);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (val == 0U) new_ccr |= 0x04U;
        if ((val & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA0AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA0AU: { // beq.w $00AA60
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA60U, 0x00AA0EU, resume_pc);
    }
    case 0x00AA0EU: { // cmpi.w #8, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU);
        const std::uint16_t imm = 0x0008U;
        const auto sub = static_cast<std::uint32_t>(d3_w) - static_cast<std::uint32_t>(imm);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d3_w < imm);
        const bool v = (((d3_w ^ imm) & (d3_w ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AA12U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA12U: { // beq.w $00AA3A
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA3AU, 0x00AA16U, resume_pc);
    }
    case 0x00AA16U: { // movea.l d1, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegA0, d1);
        if (!complete_step(api, 0x00AA18U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA18U: { // lea -10(a0, d3.w), a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d3 = static_cast<std::int16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU);
        const auto new_a0 = static_cast<unsigned>(static_cast<int>(a0) + d3 - 10);
        api.write_register(api.context, kRegA0, new_a0);
        if (!complete_step(api, 0x00AA1CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA1CU: { // move.w (a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA1EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA1EU: { // beq.w $00AA74
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA74U, 0x00AA22U, resume_pc);
    }
    case 0x00AA22U: { // add.w d0, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d1_w) + static_cast<std::uint32_t>(d0_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d1_w ^ res) & (d0_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AA24U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA24U: { // adda.w d1, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d1_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD1) & 0xFFFFU);
        api.write_register(api.context, kRegA0, static_cast<unsigned>(static_cast<int>(a0) + d1_w));
        if (!complete_step(api, 0x00AA26U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA26U: { // move.w (a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA28U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA28U: { // beq.w $00AA74
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA74U, 0x00AA2CU, resume_pc);
    }
    case 0x00AA2CU: { // adda.w d1, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d1_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD1) & 0xFFFFU);
        api.write_register(api.context, kRegA0, static_cast<unsigned>(static_cast<int>(a0) + d1_w));
        if (!complete_step(api, 0x00AA2EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA2EU: { // move.w (a0)+, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned mem_val = 0U;
        if (!read_be(api, a0 & kMask24, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, (a0 + 2U));
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA30U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA30U: { // move.l (a0)+, ($FF1946).L
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned mem_val = 0U;
        if (!read_be(api, a0 & kMask24, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, (a0 + 4U));
        write_be(api, 0x00FF1946U, 4, mem_val);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (mem_val == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA36U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA36U: { // bra.w $00AA74
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AA74U, resume_pc);
    }
    case 0x00AA3AU: { // addq.w #2, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto cur = static_cast<std::uint16_t>(d3 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(cur) + 2U;
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((cur ^ res) & 0x8000U) != 0U && ((cur ^ 2U) & 0x8000U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AA3CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA3CU: { // movea.l d1, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegA0, d1);
        if (!complete_step(api, 0x00AA3EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA3EU: { // lea -10(a0, d3.w), a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d3 = static_cast<std::int16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU);
        const auto new_a0 = static_cast<unsigned>(static_cast<int>(a0) + d3 - 10);
        api.write_register(api.context, kRegA0, new_a0);
        if (!complete_step(api, 0x00AA42U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA42U: { // move.w (a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA44U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA44U: { // beq.w $00AA74
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA74U, 0x00AA48U, resume_pc);
    }
    case 0x00AA48U: { // add.w d0, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d1_w) + static_cast<std::uint32_t>(d0_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d1_w ^ res) & (d0_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AA4AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA4AU: { // adda.w d1, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d1_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD1) & 0xFFFFU);
        api.write_register(api.context, kRegA0, static_cast<unsigned>(static_cast<int>(a0) + d1_w));
        if (!complete_step(api, 0x00AA4CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA4CU: { // move.w (a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA4EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA4EU: { // beq.w $00AA74
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA74U, 0x00AA52U, resume_pc);
    }
    case 0x00AA52U: { // adda.w d1, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto d1_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD1) & 0xFFFFU);
        api.write_register(api.context, kRegA0, static_cast<unsigned>(static_cast<int>(a0) + d1_w));
        if (!complete_step(api, 0x00AA54U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA54U: { // move.w (a0)+, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned mem_val = 0U;
        if (!read_be(api, a0 & kMask24, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, (a0 + 2U));
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA56U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA56U: { // move.l (a0)+, ($FF1946).L
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned mem_val = 0U;
        if (!read_be(api, a0 & kMask24, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, (a0 + 4U));
        write_be(api, 0x00FF1946U, 4, mem_val);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (mem_val == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA5CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA5CU: { // bra.w $00AA78
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AA78U, resume_pc);
    }
    case 0x00AA60U: { // move.w #$0200, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | 0x0200U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr);
        if (!complete_step(api, 0x00AA64U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA64U: { // movea.l 98(a6), a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 98U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, mem_val);
        if (!complete_step(api, 0x00AA68U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA68U: { // btst.b #7, (a0)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (mem_val & 0x80U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AA6CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA6CU: { // beq.w $00AA74
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AA74U, 0x00AA70U, resume_pc);
    }
    case 0x00AA70U: { // move.w #$0300, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | 0x0300U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr);
        if (!complete_step(api, 0x00AA74U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AA74U: { // move.w d2, 62(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        write_be(api, a6 + 62U, 2, d2);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AA78U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
