#include "tools/hybrid/m68k_002512_upload_step.hpp"

namespace oasis::hybrid::step_002512 {

M68k9D70StepResult step_upload_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};

    switch (pc) {
    case 0x0025E2U: { // LSR.W #1, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t val = old_d4 & 0xFFFFU;
        const std::uint16_t c_x = (val & 0x01U) ? 0x11U : 0x00U; // X and C get bit 0
        const std::uint32_t res = (val >> 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | res);
        const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
        set_ccr(api, c_x | z);
        if (!complete_step(api, 0x0025E4U, opcode, 14, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025E4U: { // MOVE.W D5, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        const auto old_d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | d5);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5));
        if (!complete_step(api, 0x0025E6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025E6U: { // ROL.W #2, D5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d5 = api.read_register(api.context, kRegD5);
        const std::uint32_t val = old_d5 & 0xFFFFU;
        const std::uint32_t res = ((val << 2U) | (val >> 14U)) & 0xFFFFU;
        api.write_register(api.context, kRegD5, (old_d5 & 0xFFFF0000U) | res);
        const std::uint16_t old_x = ccr(api) & 0x10U; // keep X unaffected
        const std::uint16_t c = (val & 0x4000U) ? 0x01U : 0x00U; // bit 14 becomes bit 1
        const std::uint16_t n = (res & 0x8000U) ? 0x08U : 0x00U;
        const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
        set_ccr(api, old_x | n | z | c);
        if (!complete_step(api, 0x0025E8U, opcode, 28, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025E8U: { // ANDI.W #$0003, D5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d5 = api.read_register(api.context, kRegD5);
        const std::uint32_t res = (old_d5 & 0xFFFFU) & 0x0003U;
        api.write_register(api.context, kRegD5, (old_d5 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0025ECU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025ECU: { // ANDI.W #$3FFF, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t res = (old_d6 & 0xFFFFU) & 0x3FFFU;
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0025F0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025F0U: { // ORI.W #$4000, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t res = (old_d6 & 0xFFFFU) | 0x4000U;
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0025F4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025F4U: { // MOVE.W D6, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD0);
        const auto val = api.read_register(api.context, kRegD6) & 0xFFFFU;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (old_d0 & 0xFFFF0000U) | val);
        update_logic_ccr16(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x0025F6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025F6U: { // ADD.W D1, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        const std::uint32_t d0 = old_d0 & 0xFFFFU;
        const std::uint32_t res = d0 + d1;
        api.write_register(api.context, kRegD0, (old_d0 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, d1, d0, res);
        if (!complete_step(api, 0x0025F8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025F8U: { // MOVE.W D0, (A4) [VDP control port write]
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        write_be(api, a4, 2, d0);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d0));
        if (!complete_step(api, 0x0025FAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025FAU: { // MOVE.W D5, (A4) [VDP control port write]
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        write_be(api, a4, 2, d5);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5));
        if (!complete_step(api, 0x0025FCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0025FCU: { // MOVE.W #$0028, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | 0x0028U);
        update_logic_ccr16(api, 0x0028U);
        if (!complete_step(api, 0x002600U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }

    // Row Streaming Loop
    case 0x002600U: { // MOVE.W (A0)+, (A2) [VDP data port write]
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto a2 = api.read_register(api.context, kRegA2);
        unsigned tile_word{};
        if (!read_be(api, a0, 2, tile_word)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, a0 + 2U);
        write_be(api, a2, 2, tile_word & 0xFFFFU);
        update_logic_ccr16(api, static_cast<std::uint16_t>(tile_word));
        if (!complete_step(api, 0x002602U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002602U: { // ADDQ.W #1, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t val = old_d2 & 0xFFFFU;
        const std::uint32_t res = val + 1U;
        api.write_register(api.context, kRegD2, (old_d2 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, 1U, val, res);
        if (!complete_step(api, 0x002604U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002604U: { // CMP.W D3, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t src = api.read_register(api.context, kRegD3) & 0xFFFFU;
        const std::uint32_t dst = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const std::uint32_t res = dst - src;
        update_sub_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x002606U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002606U: { // BLT.W $002610
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool branch = (((c & 0x08U) != 0U) ^ ((c & 0x02U) != 0U)); // N ^ V
        const unsigned target_pc = branch ? 0x002610U : 0x00260AU;
        const int adj = branch ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14);
        if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00260AU: { // SUB.W D3, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t src = api.read_register(api.context, kRegD3) & 0xFFFFU;
        const auto old_d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t dst = old_d2 & 0xFFFFU;
        const std::uint32_t res = dst - src;
        api.write_register(api.context, kRegD2, (old_d2 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_sub_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x00260CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00260CU: { // SUBA.W D3, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::int16_t d3_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD3));
        const auto old_a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, old_a0 - static_cast<std::uint32_t>(static_cast<std::int32_t>(d3_w)));
        if (!complete_step(api, 0x00260EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00260EU: { // SUBA.W D3, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::int16_t d3_w = static_cast<std::int16_t>(api.read_register(api.context, kRegD3));
        const auto old_a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, old_a0 - static_cast<std::uint32_t>(static_cast<std::int32_t>(d3_w)));
        if (!complete_step(api, 0x002610U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002610U: { // ADDI.W #2, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = old_d1 & 0xFFFFU;
        const std::uint32_t res = val + 2U;
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, 2U, val, res);
        if (!complete_step(api, 0x002614U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002614U: { // ADDQ.W #1, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto old_d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t val = old_d4 & 0xFFFFU;
        const std::uint32_t res = val + 1U;
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, 1U, val, res);
        if (!complete_step(api, 0x002616U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002616U: { // CMPI.W #$0040, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const std::uint32_t dst = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const std::uint32_t res = dst - 0x0040U;
        update_sub_ccr16(api, 0x0040U, dst, res);
        if (!complete_step(api, 0x00261AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00261AU: { // BLT.W $00262C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool branch = (((c & 0x08U) != 0U) ^ ((c & 0x02U) != 0U)); // N ^ V
        const unsigned target_pc = branch ? 0x00262CU : 0x00261EU;
        const int adj = branch ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14);
        if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00261EU: { // SUBI.W #$0040, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t val = old_d4 & 0xFFFFU;
        const std::uint32_t res = val - 0x0040U;
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_sub_ccr16(api, 0x0040U, val, res);
        if (!complete_step(api, 0x002622U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002622U: { // SUBI.W #$0080, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = old_d1 & 0xFFFFU;
        const std::uint32_t res = val - 0x0080U;
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_sub_ccr16(api, 0x0080U, val, res);
        if (!complete_step(api, 0x002626U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002626U: { // ADD.W D1, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto old_d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t d6 = old_d6 & 0xFFFFU;
        const std::uint32_t res = d6 + d1;
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | (res & 0xFFFFU));
        update_add_ccr16(api, d1, d6, res);
        if (!complete_step(api, 0x002628U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002628U: { // MOVE.W D6, (A4) [VDP control port write]
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a4, 2, d6);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6));
        if (!complete_step(api, 0x00262AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00262AU: { // MOVE.W D5, (A4) [VDP control port write]
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        write_be(api, a4, 2, d5);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5));
        if (!complete_step(api, 0x00262CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00262CU: { // DBF D7, $002600
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto old_d7 = api.read_register(api.context, kRegD7);
        const std::int16_t d7_w = static_cast<std::int16_t>(old_d7 & 0xFFFFU) - 1;
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | static_cast<std::uint32_t>(static_cast<std::uint16_t>(d7_w)));
        const bool taken = (d7_w != -1);
        const unsigned target_pc = taken ? 0x002600U : 0x002630U;
        const int adj = taken ? -14 : 14;
        if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002630U: { // RTS
        return step_rts(api, pc, resume_pc);
    }

    default:
        return M68k9D70StepResult::ContractFault;
    }
}

} // namespace oasis::hybrid::step_002512
