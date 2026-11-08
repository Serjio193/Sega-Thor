#include "tools/hybrid/m68k_00a9a0_coords_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_coords(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned a6, unsigned& resume_pc,
                                      M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AB00U: { // sub.w 4(a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0 + 4U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
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
        if (!complete_step(api, 0x00AB04U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB04U: { // addi.w #$0080, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d1_w) + 0x0080U;
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d1_w ^ res) & 0x8000U) != 0U && ((d1_w ^ 0x0080U) & 0x8000U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AB08U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB08U: { // movea.w d1, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto ext_val = (d1 & 0x8000U) ? (d1 | 0xFFFF0000U) : d1;
        api.write_register(api.context, kRegA4, ext_val);
        if (!complete_step(api, 0x00AB0AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB0AU: { // suba.w d2, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const auto ext_d2 = (d2 & 0x8000U) ? (d2 | 0xFFFF0000U) : d2;
        api.write_register(api.context, kRegA4, a4 - ext_d2);
        if (!complete_step(api, 0x00AB0CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB0CU: { // swap d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto swapped = (d1 >> 16U) | (d1 << 16U);
        api.write_register(api.context, kRegD1, swapped);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (swapped == 0U) new_ccr |= 0x04U;
        if ((swapped & 0x80000000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB0EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB0EU: { // move.w 8(a6), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 8U, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB12U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB12U: { // sub.w 0(a0), d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a0 = api.read_register(api.context, kRegA0) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, a0, 2, mem_val)) return M68k9D70StepResult::ContractFault;
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
        if (!complete_step(api, 0x00AB16U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB16U: { // addi.w #$0080, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d1_w) + 0x0080U;
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d1_w ^ res) & 0x8000U) != 0U && ((d1_w ^ 0x0080U) & 0x8000U) == 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AB1AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB1AU: { // btst.l #6, d3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        const bool bit = (d3 & (1U << 6U)) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AB1EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB1EU: { // beq.w $00AB4A
        const bool taken = (ccr(api) & 0x04U) != 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AB4AU, 0x00AB22U, resume_pc);
    }
    case 0x00AB22U: { // btst.b #0, 23(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 23U, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (mem_val & 0x01U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x00AB28U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB28U: { // bne.s $00AB3C
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_s(api, pc, taken, 0x00AB3CU, 0x00AB2AU, resume_pc);
    }
    case 0x00AB2AU: { // move.b ($FF1971).l, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF1971U, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFFFF00U) | (mem_val & 0xFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB30U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB30U: { // andi.w #$0001, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto res = static_cast<std::uint16_t>((d5 & 0xFFFFU) & 0x0001U);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB34U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB34U: { // add.w d5, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto d5_w = static_cast<std::uint16_t>(d5 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d5_w) + static_cast<std::uint32_t>(d5_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d5_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AB36U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB36U: { // adda.w d5, a4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        const auto ext_d5 = (d5 & 0x8000U) ? (d5 | 0xFFFF0000U) : d5;
        api.write_register(api.context, kRegA4, a4 + ext_d5);
        if (!complete_step(api, 0x00AB38U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB38U: { // bra.w $00AB4A
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AB4AU, resume_pc);
    }
    case 0x00AB3CU: { // move.b ($FF1971).l, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF1971U, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFFFF00U) | (mem_val & 0xFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB42U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB42U: { // andi.w #$0001, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto res = static_cast<std::uint16_t>((d5 & 0xFFFFU) & 0x0001U);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (res == 0U) new_ccr |= 0x04U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB46U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB46U: { // add.w d5, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto d5_w = static_cast<std::uint16_t>(d5 & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d5_w) + static_cast<std::uint32_t>(d5_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d5_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AB48U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB48U: { // add.w d5, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d1_w = static_cast<std::uint16_t>(d1 & 0xFFFFU);
        const auto d5_w = static_cast<std::uint16_t>(api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto sum = static_cast<std::uint32_t>(d1_w) + static_cast<std::uint32_t>(d5_w);
        const auto res = static_cast<std::uint16_t>(sum & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        const bool c = (sum > 0xFFFFU);
        const bool v = (((d1_w ^ res) & (d5_w ^ res) & 0x8000U) != 0U);
        const bool z = (res == 0U);
        const bool n = ((res & 0x8000U) != 0U);
        set_ccr(api, (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x00AB4AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB4AU: { // lea ($FF13CC).l, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA1, 0x00FF13CCU);
        if (!complete_step(api, 0x00AB50U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB50U: { // adda.w ($FF188C).l, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF188CU, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto a1 = api.read_register(api.context, kRegA1);
        const auto ext_raw = mem_val & 0xFFFFU;
        const auto ext = (ext_raw & 0x8000U) ? (ext_raw | 0xFFFF0000U) : ext_raw;
        api.write_register(api.context, kRegA1, a1 + ext);
        if (!complete_step(api, 0x00AB56U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB56U: { // move.w ($FF188A).l, d2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF188AU, 2, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (mem_val & 0xFFFFU));
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if ((mem_val & 0xFFFFU) == 0U) new_ccr |= 0x04U;
        if ((mem_val & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB5CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB5CU: { // movea.l ($FF1892).l, a2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, 0x00FF1892U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA2, mem_val);
        if (!complete_step(api, 0x00AB62U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB62U: { // tst.b 57(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned mem_val = 0U;
        if (!read_be(api, a6 + 57U, 1, mem_val)) return M68k9D70StepResult::ContractFault;
        const auto val = static_cast<std::uint8_t>(mem_val & 0xFFU);
        const auto old_ccr = ccr(api) & 0x10U;
        std::uint16_t new_ccr = old_ccr;
        if (val == 0U) new_ccr |= 0x04U;
        if ((val & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00AB66U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AB66U: { // bne.w $00AD42
        const bool taken = (ccr(api) & 0x04U) == 0U;
        metrics.stepped_instructions++;
        return step_bcc_w(api, pc, taken, 0x00AD42U, 0x00AB6AU, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}
} // namespace oasis::hybrid
