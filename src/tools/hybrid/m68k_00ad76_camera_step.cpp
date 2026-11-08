#include "tools/hybrid/m68k_00ad76_camera_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_camera(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned a6, unsigned& resume_pc,
                                      M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AE42U: { // btst #5, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 5U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE46U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE46U: { // bne.w $00AE66
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE66U, 0x00AE4AU, resume_pc);
    }
    case 0x00AE4AU: { // lea ($FF1716).l, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, 0x00FF1716U);
        if (!complete_step(api, 0x00AE50U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE50U: { // btst #7, ($FF16F0).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, 0x00FF16F0U, 1, b)) return M68k9D70StepResult::ContractFault;
        const bool bit = (b & 0x80U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE58U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE58U: { // beq.w $00AE7E
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE7EU, 0x00AE5CU, resume_pc);
    }
    case 0x00AE5CU: { // lea ($FF173E).l, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, 0x00FF173EU);
        if (!complete_step(api, 0x00AE62U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE62U: { // bra.w $00AE7E
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AE7EU, resume_pc);
    }
    case 0x00AE66U: { // lea ($FF173E).l, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, 0x00FF173EU);
        if (!complete_step(api, 0x00AE6CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE6CU: { // btst #7, ($FF16F0).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, 0x00FF16F0U, 1, b)) return M68k9D70StepResult::ContractFault;
        const bool bit = (b & 0x80U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AE74U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE74U: { // beq.w $00AE7E
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AE7EU, 0x00AE78U, resume_pc);
    }
    case 0x00AE78U: { // lea ($FF1716).l, a0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, 0x00FF1716U);
        if (!complete_step(api, 0x00AE7EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE7EU: { // move.w 12(a6), d1
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
        if (!complete_step(api, 0x00AE82U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE82U: { // sub.w 4(a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned cam_y = 0U;
        if (!read_be(api, a0 + 4U, 2, cam_y)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto src = cam_y & 0xFFFFU;
        const auto res = (dst - src) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (dst < src);
        const bool v = (((dst ^ src) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AE86U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE86U: { // addi.w #128, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto res = (dst + 128U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = ((dst + 128U) > 0xFFFFU);
        const bool v = (((dst ^ 128U) & (dst ^ res) & 0x8000U) == 0U && ((dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AE8AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE8AU: { // movea.w d1, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto ext_val = (d1 & 0x8000U) ? (d1 | 0xFFFF0000U) : d1;
        api.write_register(api.context, kRegA4, ext_val);
        if (!complete_step(api, 0x00AE8CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE8CU: { // suba.w d2, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const auto ext_d2 = (d2 & 0x8000U) ? (d2 | 0xFFFF0000U) : d2;
        const auto a4 = api.read_register(api.context, kRegA4);
        api.write_register(api.context, kRegA4, a4 - ext_d2);
        if (!complete_step(api, 0x00AE8EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE8EU: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto res = ((d1 & 0xFFFFU) << 16U) | ((d1 >> 16U) & 0xFFFFU);
        api.write_register(api.context, kRegD1, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AE90U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE90U: { // move.w 8(a6), d1
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
        if (!complete_step(api, 0x00AE94U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE94U: { // sub.w 0(a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0);
        unsigned cam_x = 0U;
        if (!read_be(api, a0, 2, cam_x)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto src = cam_x & 0xFFFFU;
        const auto res = (dst - src) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (dst < src);
        const bool v = (((dst ^ src) & (dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AE98U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE98U: { // addi.w #128, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto dst = d1 & 0xFFFFU;
        const auto res = (dst + 128U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = ((dst + 128U) > 0xFFFFU);
        const bool v = (((dst ^ 128U) & (dst ^ res) & 0x8000U) == 0U && ((dst ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AE9CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AE9CU: { // btst #6, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 6U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AEA0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEA0U: { // beq.w $00AECC
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AECCU, 0x00AEA4U, resume_pc);
    }
    case 0x00AEA4U: { // btst #0, 23(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, a6 + 23U, 1, b)) return M68k9D70StepResult::ContractFault;
        const bool bit = (b & 0x01U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AEAAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEAAU: { // bne.s $00AEBE
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_s(api, pc, taken, 0x00AEBEU, 0x00AEACU, resume_pc);
    }
    case 0x00AEACU: { // move.b ($FF1971).l, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, 0x00FF1971U, 1, b)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFFFF00U) | (b & 0xFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AEB2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEB2U: { // andi.w #1, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto res = (d5 & 1U);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (res == 0U ? 0x04U : 0U));
        if (!complete_step(api, 0x00AEB6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEB6U: { // add.w d5, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto val = d5 & 0xFFFFU;
        const auto res = (val + val) & 0xFFFFU;
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const bool c = ((val + val) > 0xFFFFU);
        const bool v = (((val ^ res) & (val ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AEB8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEB8U: { // adda.w d5, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        const auto ext_d5 = (d5 & 0x8000U) ? (d5 | 0xFFFF0000U) : d5;
        const auto a4 = api.read_register(api.context, kRegA4);
        api.write_register(api.context, kRegA4, a4 + ext_d5);
        if (!complete_step(api, 0x00AEBAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEBAU: { // bra.w $00AECC
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AECCU, resume_pc);
    }
    case 0x00AEBEU: { // move.b ($FF1971).l, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned b = 0U;
        if (!read_be(api, 0x00FF1971U, 1, b)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFFFF00U) | (b & 0xFFU));
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if ((b & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((b & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AEC4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEC4U: { // andi.w #1, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto res = (d5 & 1U);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        set_ccr(api, old_ccr | (res == 0U ? 0x04U : 0U));
        if (!complete_step(api, 0x00AEC8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AEC8U: { // add.w d5, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto val = d5 & 0xFFFFU;
        const auto res = (val + val) & 0xFFFFU;
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const bool c = ((val + val) > 0xFFFFU);
        const bool v = (((val ^ res) & (val ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AECAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AECAU: { // add.w d5, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto val = d1 & 0xFFFFU;
        const auto add_v = d5 & 0xFFFFU;
        const auto res = (val + add_v) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = ((val + add_v) > 0xFFFFU);
        const bool v = (((val ^ res) & (add_v ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AECCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
