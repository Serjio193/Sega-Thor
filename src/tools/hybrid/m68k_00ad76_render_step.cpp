#include "tools/hybrid/m68k_00ad76_render_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_render(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned /*a6*/, unsigned& resume_pc,
                                      M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AF18U: { // cmpi.w #80, d2
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
        if (!complete_step(api, 0x00AF1CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF1CU: { // bcc.w $00AFFA
        const bool taken = (ccr(api) & 0x01U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AFFAU, 0x00AF20U, resume_pc);
    }
    case 0x00AF20U: { // move.w (a0), d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned w = 0U;
        if (!read_be(api, a0, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF22U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF22U: { // move.w d6, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | d6);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d6 == 0U) new_ccr |= 0x04U;
        if ((d6 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF24U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF24U: { // ext.w d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto b = d6 & 0xFFU;
        const auto ext_val = (b & 0x80U) ? (b | 0xFF00U) : b;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | ext_val);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (ext_val == 0U) new_ccr |= 0x04U;
        if ((ext_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF26U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF26U: { // add.w a4, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto val = d6 & 0xFFFFU;
        const auto add_v = a4 & 0xFFFFU;
        const auto res = (val + add_v) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = ((val + add_v) > 0xFFFFU);
        const bool v = (((val ^ res) & (add_v ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF28U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF28U: { // move.w d6, (a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1, 2, d6);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d6 == 0U) new_ccr |= 0x04U;
        if ((d6 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF2AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF2AU: { // subiw #96, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto dst = d6 & 0xFFFFU;
        const auto res = (dst - 96U) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (dst < 96U);
        const bool v = (((dst ^ 96U) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF2EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF2EU: { // cmpi.w #256, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        const auto res = (d6 - 256U) & 0xFFFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d6 < 256U);
        const bool v = (((d6 ^ 256U) & (d6 ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF32U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF32U: { // bhi.w $00AF70
        const auto c = ccr(api);
        const bool taken = ((c & 0x01U) == 0U) && ((c & 0x04U) == 0U);
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AF70U, 0x00AF36U, resume_pc);
    }
    case 0x00AF36U: { // move.b d2, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFU;
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFFFF00U) | d2);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF38U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF38U: { // addq.b #1, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto dst = d4 & 0xFFU;
        const auto res = (dst + 1U) & 0xFFU;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFFFF00U) | res);
        const bool c = ((dst + 1U) > 0xFFU);
        const bool v = (dst == 0x7FU);
        const bool z = (res == 0U);
        const bool n = ((res & 0x80U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF3AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF3AU: { // move.w d4, 2(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        write_be(api, a1 + 2U, 2, d4);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d4 == 0U) new_ccr |= 0x04U;
        if ((d4 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF3EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF3EU: { // move.w 2(a0), d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned w = 0U;
        if (!read_be(api, a0 + 2U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF42U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF42U: { // add.w d3, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d3 = api.read_register(api.context, kRegD3);
        const auto val = d6 & 0xFFFFU;
        const auto add_v = d3 & 0xFFFFU;
        const auto res = (val + add_v) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = ((val + add_v) > 0xFFFFU);
        const bool v = (((val ^ res) & (add_v ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF44U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF44U: { // eor.w d0, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res = ((d6 & 0xFFFFU) ^ (d0 & 0xFFFFU));
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF46U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF46U: { // move.w d6, 4(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1 + 4U, 2, d6);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d6 == 0U) new_ccr |= 0x04U;
        if ((d6 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF4AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF4AU: { // move.w 4(a0), d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned w = 0U;
        if (!read_be(api, a0 + 4U, 2, w)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (w & 0xFFFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((w & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((w & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF4EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF4EU: { // btst #11, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const bool bit = (d0 & (1U << 11U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AF52U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF52U: { // beq.w $00AF58
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AF58U, 0x00AF56U, resume_pc);
    }
    case 0x00AF56U: { // lsr.w #8, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto val = d6 & 0xFFFFU;
        const auto res = (val >> 8U) & 0xFFFFU;
        const bool c = ((val >> 7U) & 1U) != 0U;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        std::uint16_t new_ccr = (c ? 0x11U : 0U);
        if (res == 0U) new_ccr |= 0x04U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF58U, opcode, 112, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF58U: { // ext.w d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto b = d6 & 0xFFU;
        const auto ext_val = (b & 0x80U) ? (b | 0xFF00U) : b;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | ext_val);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (ext_val == 0U) new_ccr |= 0x04U;
        if ((ext_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF5AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF5AU: { // add.w d1, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto val = d6 & 0xFFFFU;
        const auto add_v = d1 & 0xFFFFU;
        const auto res = (val + add_v) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = ((val + add_v) > 0xFFFFU);
        const bool v = (((val ^ res) & (add_v ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF5CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF5CU: { // move.w d6, 6(a1)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & kMask24;
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a1 + 6U, 2, d6);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d6 == 0U) new_ccr |= 0x04U;
        if ((d6 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AF60U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF60U: { // subiw #96, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto dst = d6 & 0xFFFFU;
        const auto res = (dst - 96U) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        const bool c = (dst < 96U);
        const bool v = (((dst ^ 96U) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF64U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF64U: { // cmpi.w #352, d6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        const auto res = (d6 - 352U) & 0xFFFFU;
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        const bool c = (d6 < 352U);
        const bool v = (((d6 ^ 352U) & (d6 ^ res) & 0x8000U) != 0U);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AF68U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF68U: { // bhi.w $00AF70
        const auto c = ccr(api);
        const bool taken = ((c & 0x01U) == 0U) && ((c & 0x04U) == 0U);
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AF70U, 0x00AF6CU, resume_pc);
    }
    case 0x00AF6CU: { // addq.w #1, d2
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
        if (!complete_step(api, 0x00AF6EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF6EU: { // addq.l #8, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, a1 + 8U);
        if (!complete_step(api, 0x00AF70U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF70U: { // addq.l #6, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + 6U);
        if (!complete_step(api, 0x00AF72U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AF72U: { // dbf d5, $00AF18
        metrics.stepped_instructions++;
        metrics.piece_loop_steps++;
        return step_dbf_d5(api, pc, 0x00AF18U, 0x00AF76U, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
