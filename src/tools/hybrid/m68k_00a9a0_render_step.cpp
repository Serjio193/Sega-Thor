#include "tools/hybrid/m68k_00a9a0_render_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_render(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned a6, unsigned& resume_pc,
                                      M68k00A9A0StepMetrics& metrics) noexcept {
    (void)a6;
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AC16U: { // move.w d4, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4_w = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | d4_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d4_w == 0U) new_ccr |= 0x04U;
        if ((d4_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC18U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC18U: { // andi.w #$0F00, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto res = (d4 & 0xFFFFU) & 0x0F00U;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC1CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC1CU: { // lsr.w #7, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto val = static_cast<std::uint16_t>(d4 & 0xFFFFU);
        const bool c = (val & 0x40U) != 0U;
        const auto res = static_cast<std::uint16_t>(val >> 7U);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        const bool z = (res == 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (z ? 0x04U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC1EU, opcode, 98, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC1EU: { // move.w 0(a5, d4.w), d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a5 = api.read_register(api.context, kRegA5) & kMask24;
        const auto d4_idx = static_cast<std::int16_t>(api.read_register(api.context, kRegD4) & 0xFFFFU);
        const auto ea = static_cast<unsigned>(static_cast<int>(a5) + d4_idx) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, ea, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC22U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC22U: { // adda.w d4, a3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a3 = api.read_register(api.context, kRegA3);
        const auto d4_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD4) & 0xFFFFU);
        api.write_register(api.context, kRegA3, static_cast<unsigned>(static_cast<int>(a3) + d4_w));
        if (!complete_step(api, 0x00AC24U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC24U: { // btst #31, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool bit = (d5 & 0x80000000U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AC28U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC28U: { // beq.w $00AC58
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AC58U, 0x00AC2CU, resume_pc);
    }
    case 0x00AC2CU: { // andi.w #$F000, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto res = (d6 & 0xFFFFU) & 0xF000U;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC30U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC30U: { // andi.l #$000007FF, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto res = d7 & 0x000007FFU;
        api.write_register(api.context, kRegD7, res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC36U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC36U: { // add.w d7, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto val = d7 & 0xFFFFU;
        const auto res = (val + val) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        const bool z = (res == 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (z ? 0x04U : 0U));
        if (!complete_step(api, 0x00AC38U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC38U: { // or.w d6, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6_w = api.read_register(api.context, kRegD6) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto res = (d7 & 0xFFFFU) | d6_w;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC3AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC3AU: { // lsl.l #3, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto res = d7 << 3U;
        api.write_register(api.context, kRegD7, res);
        const bool z = (res == 0U);
        const bool n = ((res & 0x80000000U) != 0U);
        const bool c = (d7 & 0x20000000U) != 0U;
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC3CU, opcode, 42, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC3CU: { // add.l a6, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a6_val = api.read_register(api.context, kRegA6);
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto res = d7 + a6_val;
        api.write_register(api.context, kRegD7, res);
        const bool c = (res < d7);
        const bool v = (((d7 ^ res) & (a6_val ^ res) & 0x80000000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x80000000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC3EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC3EU: { // move.l d7, (a2)+
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a2 = api.read_register(api.context, kRegA2);
        const auto d7 = api.read_register(api.context, kRegD7);
        write_be(api, a2 & kMask24, 4, d7);
        api.write_register(api.context, kRegA2, (a2 + 4U));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d7 == 0U) new_ccr |= 0x04U;
        if ((d7 & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC40U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC40U: { // move.w a3, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a3_w = api.read_register(api.context, kRegA3) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | a3_w);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (a3_w == 0U) new_ccr |= 0x04U;
        if ((a3_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC42U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC42U: { // sub.w d4, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD4) & 0xFFFFU);
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto d7_w = static_cast<std::uint16_t>(d7 & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(d7_w) - static_cast<std::uint32_t>(d4_w);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        const bool c = (d7_w < d4_w);
        const bool v = (((d7_w ^ d4_w) & (d7_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC44U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC44U: { // add.w d3, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU);
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto d7_w = static_cast<std::uint16_t>(d7 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d7_w) + static_cast<std::uint32_t>(d3_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d7_w ^ res) & (d3_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC46U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC46U: { // andi.w #$07FF, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto res = (d7 & 0xFFFFU) & 0x07FFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC4AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC4AU: { // lsl.w #5, d7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto val = static_cast<std::uint16_t>(d7 & 0xFFFFU);
        const auto res = static_cast<std::uint16_t>(val << 5U);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (val & 0x0800U) != 0U;
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC4CU, opcode, 70, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC4CU: { // move.w d7, (a2)+
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a2 = api.read_register(api.context, kRegA2);
        const auto d7_w = api.read_register(api.context, kRegD7) & 0xFFFFU;
        write_be(api, a2 & kMask24, 2, d7_w);
        api.write_register(api.context, kRegA2, (a2 + 2U));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d7_w == 0U) new_ccr |= 0x04U;
        if ((d7_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC4EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC4EU: { // sub.w d4, ($FF1652).L
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d4_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD4) & 0xFFFFU);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF1652U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto mem_w = static_cast<std::uint16_t>(mem_val & 0xFFFFU);
        const auto sub = static_cast<std::uint32_t>(mem_w) - static_cast<std::uint32_t>(d4_w);
        const auto res = static_cast<std::uint16_t>(sub & 0xFFFFU);
        write_be(api, 0x00FF1652U, 2, res);
        const bool c = (mem_w < d4_w);
        const bool v = (((mem_w ^ d4_w) & (mem_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC54U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC54U: { // lsl.w #4, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto val = static_cast<std::uint16_t>(d4 & 0xFFFFU);
        const auto res = static_cast<std::uint16_t>(val << 4U);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (val & 0x1000U) != 0U;
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AC56U, opcode, 56, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC56U: { // move.w d4, (a2)+
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a2 = api.read_register(api.context, kRegA2);
        const auto d4_w = api.read_register(api.context, kRegD4) & 0xFFFFU;
        write_be(api, a2 & kMask24, 2, d4_w);
        api.write_register(api.context, kRegA2, (a2 + 2U));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (d4_w == 0U) new_ccr |= 0x04U;
        if ((d4_w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AC58U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC58U: { // addq.l #6, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, (a0 + 6U));
        if (!complete_step(api, 0x00AC5AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AC5AU: { // dbf d5, $00ABB6
        metrics.stepped_instructions++;
        return step_dbf_d5(api, pc, 0x00ABB6U, 0x00AC5EU, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
