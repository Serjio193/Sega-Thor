#include "tools/hybrid/m68k_00ad76_shadow_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_shadow(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned a6, unsigned& resume_pc,
                                      M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AF76U: { // swap d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res = ((d0 & 0xFFFFU) << 16U) | ((d0 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD0, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF78U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF78U: { // move.w 62(a6), d5
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
        if (!complete_step(api, 0x00AF7CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF7CU: { // bmi.w $00AF94
        const bool taken = (ccr(api) & 0x08U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AF94U, 0x00AF80U, resume_pc);
    }
    case 0x00AF80U: { // btst #8, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const bool bit = (d5 & (1U << 8U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AF84U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF84U: { // beq.w $00AF94
        const bool taken = (ccr(api) & 0x04U) != 0U;
        if (taken) {
            metrics.stepped_instructions++;
            return step_bcc_w(api, pc, true, 0x00AF94U, 0x00AF88U, resume_pc);
        }
        return M68k9D70StepResult::NotHandled; // attached part hook 0x00AF88 (bsr $00B15E) -> fallback
    }
    case 0x00AF94U: { // tst.b 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, a6 + 57U, 1, b)) return M68k9D70StepResult::ContractFault;
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF98U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF98U: { // bne.w $00B01A
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00B01AU, 0x00AF9CU, resume_pc);
    }
    case 0x00AF9CU: { // bclr #31, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 31U)) != 0U;
        api.write_register(api.context, kRegD3, d3 & ~(1U << 31U));
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        // Musashi m68k_op_bclr_32_s_d adds 14 master cycles (2 CPU cycles) when bit > 15
        if (!complete_step(api, 0x00AFA0U, opcode, 14, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFA0U: { // beq.w $00AFFA
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AFA4U, resume_pc);
    }
    case 0x00AFA4U: { // cmpi.w #80, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const auto res = (d2 - 80U) & 0xFFFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d2 < 80U);
        const bool v = (((d2 ^ 80U) & (d2 ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFA8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFA8U: { // bcc.w $00AFFA
        const bool taken = (ccr(api) & 0x01U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AFACU, resume_pc);
    }
    case 0x00AFACU: { // tst.b ($FF1927).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, 0x00FF1927U, 1, b)) return M68k9D70StepResult::ContractFault;
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFB2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFB2U: { // bne.w $00AFFA
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AFB6U, resume_pc);
    }
    case 0x00AFB6U: { // move.w d3, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3) & 0xFFFFU;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | d3);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d3 == 0U) new_ccr |= 0x04U;
        if ((d3 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFB8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFB8U: { // swap d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto res = ((d3 & 0xFFFFU) << 16U) | ((d3 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD3, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFBAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFBAU: { // lea %pc@(0xACFE), a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, 0x00ACFEU);
        if (!complete_step(api, 0x00AFBEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFBEU: { // adda.w d3, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = api.read_register(api.context, kRegD3) & 0xFFFFU;
        const auto ext_d3 = (d3 & 0x8000U) ? (d3 | 0xFFFF0000U) : d3;
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + ext_d3);
        if (!complete_step(api, 0x00AFC0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFC0U: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto res = ((d1 & 0xFFFFU) << 16U) | ((d1 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD1, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFC2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFC2U: { // sub.w (a0)+, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto raw_a0 = api.read_register(api.context, kRegA0);
        unsigned w = 0U;
        if (!read_be(api, raw_a0 & kMask24, 2, w)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, raw_a0 + 2U);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto src = w & 0xFFFFU;
        const auto res = (dst - src) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (dst < src);
        const bool v = (((dst ^ src) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFC4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFC4U: { // move.w d1, (a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        write_be(api, a1, 2, d1);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d1 == 0U) new_ccr |= 0x04U;
        if ((d1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFC6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFC6U: { // subiw #96, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto res = (dst - 96U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (dst < 96U);
        const bool v = (((dst ^ 96U) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFCAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFCAU: { // cmpi.w #256, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto res = (d1 - 256U) & 0xFFFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d1 < 256U);
        const bool v = (((d1 ^ 256U) & (d1 ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFCEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFCEU: { // bhi.w $00AFFA
        const auto c = ccr(api);
        const bool taken = ((c & 0x01U) == 0U) && ((c & 0x04U) == 0U);
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AFD2U, resume_pc);
    }
    case 0x00AFD2U: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto res = ((d1 & 0xFFFFU) << 16U) | ((d1 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD1, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFD4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFD4U: { // move.w (a0)+, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto raw_a0 = api.read_register(api.context, kRegA0);
        unsigned w = 0U;
        if (!read_be(api, raw_a0 & kMask24, 2, w)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, raw_a0 + 2U);
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFD6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFD6U: { // move.b d2, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFU;
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFFFF00U) | d2);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFD8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFD8U: { // addq.w #1, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto dst = d4 & 0xFFFFU;
        const auto res = (dst + 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        const bool c = ((dst + 1U) > 0xFFFFU);
        const bool v = (dst == 0x7FFFU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFDAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFDAU: { // move.w d4, 2(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        write_be(api, a1 + 2U, 2, d4);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d4 == 0U) new_ccr |= 0x04U;
        if ((d4 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFDEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFDEU: { // move.w (a0)+, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto raw_a0 = api.read_register(api.context, kRegA0);
        unsigned w = 0U;
        if (!read_be(api, raw_a0 & kMask24, 2, w)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, raw_a0 + 2U);
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFE0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFE0U: { // move.w d6, 4(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1 + 4U, 2, d6);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d6 == 0U) new_ccr |= 0x04U;
        if ((d6 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFE4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFE4U: { // sub.w (a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned w = 0U;
        if (!read_be(api, a0, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto src = w & 0xFFFFU;
        const auto res = (dst - src) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (dst < src);
        const bool v = (((dst ^ src) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFE6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFE6U: { // move.w d1, 6(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        write_be(api, a1 + 6U, 2, d1);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d1 == 0U) new_ccr |= 0x04U;
        if ((d1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AFEAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFEAU: { // subiw #96, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto res = (dst - 96U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (dst < 96U);
        const bool v = (((dst ^ 96U) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFEEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFEEU: { // cmpi.w #352, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto res = (d1 - 352U) & 0xFFFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d1 < 352U);
        const bool v = (((d1 ^ 352U) & (d1 ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFF2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFF2U: { // bhi.w $00AFFA
        const auto c = ccr(api);
        const bool taken = ((c & 0x01U) == 0U) && ((c & 0x04U) == 0U);
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AFF6U, resume_pc);
    }
    case 0x00AFF6U: { // addq.w #1, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto dst = d2 & 0xFFFFU;
        const auto res = (dst + 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | res);
        const bool c = ((dst + 1U) > 0xFFFFU);
        const bool v = (dst == 0x7FFFU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AFF8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AFF8U: { // addq.l #8, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, a1 + 8U);
        if (!complete_step(api, 0x00AFFAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
