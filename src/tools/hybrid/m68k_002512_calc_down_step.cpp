#include "tools/hybrid/m68k_002512_calc_down_step.hpp"

namespace oasis::hybrid::step_002512 {

M68k9D70StepResult step_calc_down_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    const auto a3 = api.read_register(api.context, kRegA3);

    switch (pc) {
    case 0x002596U: { // MOVE.W 12(A3), D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d1_w{};
        if (!read_be(api, a3 + 12U, 2, d1_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (d1_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1_w));
        if (!complete_step(api, 0x00259AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00259AU: { // ADDQ.W #1, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t val = old_d6 & 0xFFFFU;
        const std::uint32_t res = val + 1U;
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, 1U, val, res);
        if (!complete_step(api, 0x00259CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00259CU: { // MOVE.W D6, 30(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a3 + 30U, 2, d6);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6));
        if (!complete_step(api, 0x0025A0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025A0U: { // MOVE.W 32(A3), D3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d3_w{};
        if (!read_be(api, a3 + 32U, 2, d3_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (old_d3 & 0xFFFF0000U) | (d3_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d3_w));
        if (!complete_step(api, 0x0025A4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025A4U: { // MOVE.W 34(A3), D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d4_w{};
        if (!read_be(api, a3 + 34U, 2, d4_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | (d4_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d4_w));
        if (!complete_step(api, 0x0025A8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025A8U: { // MOVE.W 8(A3), D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d2_w{};
        if (!read_be(api, a3 + 8U, 2, d2_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (old_d2 & 0xFFFF0000U) | (d2_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d2_w));
        if (!complete_step(api, 0x0025ACU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025ACU: { // MOVE.W D2, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (old_d0 & 0xFFFF0000U) | d2);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d2));
        if (!complete_step(api, 0x0025AEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025AEU: { // ADD.W D0, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        const std::uint32_t d0 = old_d0 & 0xFFFFU;
        const std::uint32_t res = d0 + d0;
        api.write_register(api.context, kRegD0, (old_d0 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, d0, d0, res);
        if (!complete_step(api, 0x0025B0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025B0U: { // MOVE.W 10(A3), D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d7_w{};
        if (!read_be(api, a3 + 10U, 2, d7_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (d7_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7_w));
        if (!complete_step(api, 0x0025B4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025B4U: { // SUBQ.W #1, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = old_d7 & 0xFFFFU;
        const std::uint32_t res = val - 1U;
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_sub_ccr16(api, 1U, val, res);
        if (!complete_step(api, 0x0025B6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025B6U: { // BPL.W $0025BC
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const bool branch = (ccr(api) & 0x08U) == 0U; // N flag == 0
        const unsigned target_pc = branch ? 0x0025BCU : 0x0025BAU;
        const int adj = branch ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14);
        if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025BAU: { // ADD.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t d7 = old_d7 & 0xFFFFU;
        const std::uint32_t res = d7 + d4;
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, d4, d7, res);
        if (!complete_step(api, 0x0025BCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025BCU: { // MOVE.W D7, 10(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7) & 0xFFFFU;
        write_be(api, a3 + 10U, 2, d7);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7));
        if (!complete_step(api, 0x0025C0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025C0U: { // MOVE.W 36(A3), D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d6_w{};
        if (!read_be(api, a3 + 36U, 2, d6_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | (d6_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6_w));
        if (!complete_step(api, 0x0025C4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025C4U: { // LSL.W D6, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto shift = api.read_register(api.context, kRegD6) & 0x3FU;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        std::uint32_t val = old_d7 & 0xFFFFU;
        std::uint32_t res = (val << (shift % 64U)) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (shift != 0) {
            api.add_cycles(api.context, static_cast<int>(shift) * 14);
            if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
        }
        if (!complete_step(api, 0x0025C6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025C6U: { // ADD.W D0, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t d7 = old_d7 & 0xFFFFU;
        const std::uint32_t res = d7 + d0;
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, d0, d7, res);
        if (!complete_step(api, 0x0025C8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025C8U: { // ADDA.W D7, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::int16_t d7_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD7));
        const auto old_a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, old_a0 + static_cast<std::uint32_t>(static_cast<std::int32_t>(d7_w)));
        if (!complete_step(api, 0x0025CAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025CAU: { // MOVE.W D1, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto old_d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | d1);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1));
        if (!complete_step(api, 0x0025CCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025CCU: { // ANDI.W #$0F80, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = (old_d1 & 0xFFFFU) & 0x0F80U;
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0025D0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025D0U: { // SUBI.W #$0080, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = old_d1 & 0xFFFFU;
        const std::uint32_t res = val - 0x0080U;
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_sub_ccr16(api, 0x0080U, val, res);
        if (!complete_step(api, 0x0025D4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025D4U: { // ANDI.W #$007E, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t res = (old_d4 & 0xFFFFU) & 0x007EU;
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0025D8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025D8U: { // ANDI.W #$0F80, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = (old_d1 & 0xFFFFU) & 0x0F80U;
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0025DCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025DCU: { // ADD.W D4, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto old_d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t d1 = old_d1 & 0xFFFFU;
        const std::uint32_t res = d1 + d4;
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, d4, d1, res);
        if (!complete_step(api, 0x0025DEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025DEU: { // MOVE.W D1, 12(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        write_be(api, a3 + 12U, 2, d1);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1));
        if (!complete_step(api, 0x0025E2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_002512
