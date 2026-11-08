#!/usr/bin/env python3
"""Generate calc_step.cpp for 0x0023CC. <= 500 lines."""
from pathlib import Path
SRC_DIR = Path('.').resolve() / 'src' / 'tools' / 'hybrid'
def generate_calc_step_cpp():
    # R1: 0x0023CC..0x00245A
    code = """#include "tools/hybrid/m68k_0023cc_calc_step.hpp"
#include "tools/hybrid/m68k_0023cc_calc_left_step.hpp"
namespace oasis::hybrid::step_0023cc {
M68k9D70StepResult step_calc_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    if (pc >= 0x00245EU && pc <= 0x0024AAU) {
        return step_calc_left_range(api, pc, resume_pc);
    }
    std::uint16_t opcode{};
    const auto a3 = api.read_register(api.context, kRegA3);
    switch (pc) {
    case 0x0023CCU: { // MOVEA.L 22(A3), A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned a0_val{};
        if (!read_be(api, a3 + 22U, 4, a0_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA0, a0_val);
        if (!complete_step(api, 0x0023D0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023D0U: { // MOVE.W 26(A3), D5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d5_w{};
        if (!read_be(api, a3 + 26U, 2, d5_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d5 = api.read_register(api.context, kRegD5);
        api.write_register(api.context, kRegD5, (old_d5 & 0xFFFF0000U) | (d5_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5_w));
        if (!complete_step(api, 0x0023D4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023D4U: { // LEA ($C00000).L, A2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA2, 0x00C00000U);
        if (!complete_step(api, 0x0023DAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023DAU: { // LEA 4(A2), A4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a2 = api.read_register(api.context, kRegA2);
        api.write_register(api.context, kRegA4, a2 + 4U);
        if (!complete_step(api, 0x0023DEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023DEU: { // MOVE.W #$8F80, (A4)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a4 = api.read_register(api.context, kRegA4);
        write_be(api, a4, 2, 0x8F80U);
        update_logic_ccr16(api, 0x8F80U);
        if (!complete_step(api, 0x0023E2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023E2U: { // MOVE.W 28(A3), D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d6_w{};
        if (!read_be(api, a3 + 28U, 2, d6_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (old_d6 & 0xFFFF0000U) | (d6_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6_w));
        if (!complete_step(api, 0x0023E6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023E6U: { // BMI.W $00245E
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const bool taken = (ccr(api) & 0x08U) != 0U;
        const unsigned target = taken ? 0x00245EU : 0x0023EAU;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023EAU: { // MOVE.W 12(A3), D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d1_w{};
        if (!read_be(api, a3 + 12U, 2, d1_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d1 = api.read_register(api.context, kRegD1);
        api.write_register(api.context, kRegD1, (old_d1 & 0xFFFF0000U) | (d1_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1_w));
        if (!complete_step(api, 0x0023EEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023EEU: { // SUBQ.W #$01, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t src = 1U;
        const std::uint32_t dst = d6 & 0xFFFFU;
        const std::uint32_t res = (dst - src) & 0xFFFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, src, dst, dst - src);
        if (!complete_step(api, 0x0023F0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023F0U: { // MOVE.W D6, 28(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a3 + 28U, 2, d6);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6));
        if (!complete_step(api, 0x0023F4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023F4U: { // MOVE.W 36(A3), D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d7_w{};
        if (!read_be(api, a3 + 36U, 2, d7_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (d7_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7_w));
        if (!complete_step(api, 0x0023F8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023F8U: { // MOVE.W 32(A3), D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d4_w{};
        if (!read_be(api, a3 + 32U, 2, d4_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (old_d4 & 0xFFFF0000U) | (d4_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d4_w));
        if (!complete_step(api, 0x0023FCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0023FCU: { // MOVE.W 34(A3), D3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d3_w{};
        if (!read_be(api, a3 + 34U, 2, d3_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (old_d3 & 0xFFFF0000U) | (d3_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d3_w));
        if (!complete_step(api, 0x002400U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002400U: { // MOVE.W D4, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (d4 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d4 & 0xFFFFU));
        if (!complete_step(api, 0x002402U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002402U: { // ADD.W D2, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t val = d2 & 0xFFFFU;
        const std::uint32_t sum = val + val;
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, val, val, sum);
        if (!complete_step(api, 0x002404U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002404U: { // SWAP.W D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t swapped = (d2 >> 16U) | (d2 << 16U);
        api.write_register(api.context, kRegD2, swapped);
        const std::uint16_t old = ccr(api) & 0x10U;
        const std::uint16_t n = (swapped & 0x80000000U) ? 0x08U : 0x00U;
        const std::uint16_t z = (swapped == 0U) ? 0x04U : 0x00U;
        set_ccr(api, old | n | z);
        if (!complete_step(api, 0x002406U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002406U: { // MOVE.W 10(A3), D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d2_w{};
        if (!read_be(api, a3 + 10U, 2, d2_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d2 = api.read_register(api.context, kRegD2);
        api.write_register(api.context, kRegD2, (old_d2 & 0xFFFF0000U) | (d2_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d2_w));
        if (!complete_step(api, 0x00240AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00240AU: { // MOVE.W D2, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const auto d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | (d2 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d2 & 0xFFFFU));
        if (!complete_step(api, 0x00240CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00240CU: { // LSL.W D7, D0
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
        if (!complete_step(api, 0x00240EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00240EU: { // MOVE.W 8(A3), D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d7_w{};
        if (!read_be(api, a3 + 8U, 2, d7_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (old_d7 & 0xFFFF0000U) | (d7_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7_w));
        if (!complete_step(api, 0x002412U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002412U: { // ADDQ.W #$01, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = (val + 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, val, 1U, val + 1U);
        if (!complete_step(api, 0x002414U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002414U: { // CMP.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t src = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const std::uint32_t dst = api.read_register(api.context, kRegD7) & 0xFFFFU;
        const std::uint32_t res = dst - src;
        update_cmp_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x002416U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002416U: { // BLT.W $00241C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        const bool taken = (n != v);
        const unsigned target = taken ? 0x00241CU : 0x00241AU;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00241AU: { // SUB.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t res = (val - d4) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, d4, val, val - d4);
        if (!complete_step(api, 0x00241CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00241CU: { // MOVE.W D7, 8(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7) & 0xFFFFU;
        write_be(api, a3 + 8U, 2, d7);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7));
        if (!complete_step(api, 0x002420U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002420U: { // ADDI.W #$0028, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = (val + 0x0028U) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 0x0028U, val, val + 0x0028U);
        if (!complete_step(api, 0x002424U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002424U: { // CMP.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t src = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const std::uint32_t dst = api.read_register(api.context, kRegD7) & 0xFFFFU;
        const std::uint32_t res = dst - src;
        update_cmp_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x002426U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002426U: { // BLT.W $002432
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        const bool taken = (n != v);
        const unsigned target = taken ? 0x002432U : 0x00242AU;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00242AU: { // SUB.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t res = (val - d4) & 0xFFFFU;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, d4, val, val - d4);
        if (!complete_step(api, 0x00242CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00242CU: { // CMP.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t src = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const std::uint32_t dst = api.read_register(api.context, kRegD7) & 0xFFFFU;
        const std::uint32_t res = dst - src;
        update_cmp_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x00242EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00242EU: { // BGE.W $00242A
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        const bool taken = (n == v);
        const unsigned target = taken ? 0x00242AU : 0x002432U;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002432U: { // ADD.W D7, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = val + val;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, val, val, sum);
        if (!complete_step(api, 0x002434U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002434U: { // ADD.W D0, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = val + d0;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d0, val, sum);
        if (!complete_step(api, 0x002436U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002436U: { // ADDA.W D7, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d7 = static_cast<std::int32_t>(static_cast<std::int16_t>(api.read_register(api.context, kRegD7) & 0xFFFFU));
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + static_cast<std::uint32_t>(d7));
        if (!complete_step(api, 0x002438U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002438U: { // MOVE.W D1, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | (d1 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1 & 0xFFFFU));
        if (!complete_step(api, 0x00243AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00243AU: { // ANDI.W #$007E, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = d1 & 0x007EU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x00243EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00243EU: { // ADDQ.W #$02, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t sum = (val + 2U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 2U, val, val + 2U);
        if (!complete_step(api, 0x002440U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002440U: { // ANDI.W #$007E, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = d1 & 0x007EU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x002444U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002444U: { // ANDI.W #$0F80, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t res = d4 & 0x0F80U;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x002448U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002448U: { // MOVE.W D1, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (d1 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d1 & 0xFFFFU));
        if (!complete_step(api, 0x00244AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00244AU: { // ADD.W D4, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d7 = api.read_register(api.context, kRegD7);
        const std::uint32_t val = d7 & 0xFFFFU;
        const std::uint32_t sum = val + d4;
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d4, val, sum);
        if (!complete_step(api, 0x00244CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00244CU: { // MOVE.W D7, 12(A3)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7) & 0xFFFFU;
        write_be(api, a3 + 12U, 2, d7);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d7));
        if (!complete_step(api, 0x002450U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002450U: { // ADDI.W #$0050, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t sum = (val + 0x0050U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 0x0050U, val, val + 0x0050U);
        if (!complete_step(api, 0x002454U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002454U: { // ANDI.W #$007E, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t res = d1 & 0x007EU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x002458U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002458U: { // ADD.W D4, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t sum = val + d4;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d4, val, sum);
        if (!complete_step(api, 0x00245AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00245AU: { // BRA.W $0024AE
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if (!complete_step(api, 0x0024AEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        break;
    }

    return M68k9D70StepResult::NotHandled;
}

} // namespace oasis::hybrid::step_0023cc
"""
    (SRC_DIR / "m68k_0023cc_calc_step.cpp").write_text(code, encoding="utf-8")
