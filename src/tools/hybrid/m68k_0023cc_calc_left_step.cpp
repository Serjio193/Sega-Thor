#include "tools/hybrid/m68k_0023cc_calc_left_step.hpp"

namespace oasis::hybrid::step_0023cc {

M68k9D70StepResult step_calc_left_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    const auto a3 = api.read_register(api.context, kRegA3);

    switch (pc) {
    case 0x00245EU: { // MOVE.W 12(A3), D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d1_w{};
        if (!read_be(api, a3 + 12U, 2, d1_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (d1_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1_w));
        if (!complete_step(api, 0x002462U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002462U: { // ADDQ.W #$01, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t val = d6 & 0xFFFFU;
        const std::uint32_t sum = (val + 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 1U, val, val + 1U);
        if (!complete_step(api, 0x002464U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002464U: { // MOVE.W D6, 28(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a3 + 28U, 2, d6);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6));
        if (!complete_step(api, 0x002468U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002468U: { // MOVE.W 36(A3), D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d7_w{};
        if (!read_be(api, a3 + 36U, 2, d7_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (d7_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7_w));
        if (!complete_step(api, 0x00246CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00246CU: { // MOVE.W 32(A3), D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d4_w{};
        if (!read_be(api, a3 + 32U, 2, d4_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | (d4_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d4_w));
        if (!complete_step(api, 0x002470U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002470U: { // MOVE.W 34(A3), D3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d3_w{};
        if (!read_be(api, a3 + 34U, 2, d3_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (old_d3 & 0xFFFF0000U) | (d3_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d3_w));
        if (!complete_step(api, 0x002474U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002474U: { // MOVE.W D4, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (d4 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d4 & 0xFFFFU));
        if (!complete_step(api, 0x002476U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002476U: { // ADD.W D2, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t val = d2 & 0xFFFFU;
        const std::uint32_t sum = val + val;
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, val, val, sum);
        if (!complete_step(api, 0x002478U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002478U: { // SWAP.W D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t swapped = (d2 >> 16U) | (d2 << 16U);
        api.write_register(api.context, kRegD2, swapped);
        const std::uint16_t old = ccr(api) & 0x10U;
        const std::uint16_t n = (swapped & 0x80000000U) ? 0x08U : 0x00U;
        const std::uint16_t z = (swapped == 0U) ? 0x04U : 0x00U;
        set_ccr(api, old | n | z);
        if (!complete_step(api, 0x00247AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00247AU: { // MOVE.W 10(A3), D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d2_w{};
        if (!read_be(api, a3 + 10U, 2, d2_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (old_d2 & 0xFFFF0000U) | (d2_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d2_w));
        if (!complete_step(api, 0x00247EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00247EU: { // MOVE.W D2, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | (d2 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d2 & 0xFFFFU));
        if (!complete_step(api, 0x002480U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002480U: { // LSL.W D7, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto shift = api.read_register(api.context, kRegD7) & 0x3FU;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        const std::uint32_t val = old_d0 & 0xFFFFU;
        const std::uint32_t res = (val << (shift % 64U)) & 0xFFFFU;
        api.write_register(api.context, kRegD0, (old_d0 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (shift != 0) {
            api.add_cycles(api.context, static_cast<int>(shift) * 14);
            if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
        }
        if (!complete_step(api, 0x002482U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002482U: { // MOVE.W 8(A3), D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d7_w{};
        if (!read_be(api, a3 + 8U, 2, d7_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (d7_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7_w));
        if (!complete_step(api, 0x002486U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002486U: { // SUBQ.W #$01, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t res = (val - 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, 1U, val, val - 1U);
        if (!complete_step(api, 0x002488U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002488U: { // BPL.W $00248E
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const bool taken = (ccr(api) & 0x08U) == 0U;
        const unsigned target = taken ? 0x00248EU : 0x00248CU;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00248CU: { // ADD.W D4, D7 (wrapped below zero)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = val + d4;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d4, val, sum);
        if (!complete_step(api, 0x00248EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00248EU: { // MOVE.W D7, 8(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7) & 0xFFFFU;
        write_be(api, a3 + 8U, 2, d7);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7));
        if (!complete_step(api, 0x002492U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002492U: { // ADD.W D7, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = val + val;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, val, val, sum);
        if (!complete_step(api, 0x002494U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002494U: { // ADD.W D0, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = val + d0;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d0, val, sum);
        if (!complete_step(api, 0x002496U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002496U: { // ADDA.W D7, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = static_cast<std::int32_t>(static_cast<std::int16_t>(api.read_register(api.context, kRegD7) & 0xFFFFU));
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + static_cast<std::uint32_t>(d7));
        if (!complete_step(api, 0x002498U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002498U: { // MOVE.W D1, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | (d1 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1 & 0xFFFFU));
        if (!complete_step(api, 0x00249AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00249AU: { // ANDI.W #$007E, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = d1 & 0x007EU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x00249EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00249EU: { // SUBQ.W #$02, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t res = (val - 2U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, 2U, val, val - 2U);
        if (!complete_step(api, 0x0024A0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024A0U: { // ANDI.W #$007E, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = d1 & 0x007EU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0024A4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024A4U: { // ANDI.W #$0F80, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t res = d4 & 0x0F80U;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0024A8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024A8U: { // ADD.W D4, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t sum = val + d4;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d4, val, sum);
        if (!complete_step(api, 0x0024AAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024AAU: { // MOVE.W D1, 12(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        write_be(api, a3 + 12U, 2, d1);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1));
        if (!complete_step(api, 0x0024AEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        break;
    }

    return M68k9D70StepResult::NotHandled;
}

} // namespace oasis::hybrid::step_0023cc
