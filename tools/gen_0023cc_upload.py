#!/usr/bin/env python3
"""Generate upload_step for 0x0023CC. <= 500 lines."""
from pathlib import Path
SRC_DIR = Path('.').resolve() / 'src' / 'tools' / 'hybrid'

def generate_upload_step():
    hpp = """#pragma once
#include "tools/hybrid/m68k_0023cc_step_ops.hpp"

namespace oasis::hybrid::step_0023cc {

M68k9D70StepResult step_upload_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_0023cc
"""
    cpp = """#include "tools/hybrid/m68k_0023cc_upload_step.hpp"

namespace oasis::hybrid::step_0023cc {

M68k9D70StepResult step_upload_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    const auto a3 = api.read_register(api.context, kRegA3);

    switch (pc) {
    case 0x0024AEU: { // LSR.W #$07, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t val = d4 & 0xFFFFU;
        const std::uint32_t res = (val >> 7U) & 0xFFFFU;
        const bool carry = ((val >> 6U) & 1U) != 0U;
        const std::uint16_t x = carry ? 0x10U : 0x00U;
        const std::uint16_t n = 0x00U;
        const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
        set_ccr(api, x | n | z | (carry ? 0x01U : 0x00U));
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        if (!complete_step(api, 0x0024B0U, opcode, 98, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024B0U: { // MOVE.W D5, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto d6 = api.read_register(api.context, kRegD6);
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (d5 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5 & 0xFFFFU));
        if (!complete_step(api, 0x0024B2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024B2U: { // ROL.W #$02, D5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d5 = api.read_register(api.context, kRegD5);
        const std::uint32_t val = d5 & 0xFFFFU;
        const std::uint32_t res = ((val << 2U) | (val >> 14U)) & 0xFFFFU;
        const bool carry = ((val >> 14U) & 1U) != 0U;
        const std::uint16_t old = ccr(api) & 0x10U;
        const std::uint16_t n = (res & 0x8000U) ? 0x08U : 0x00U;
        const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
        set_ccr(api, old | n | z | (carry ? 0x01U : 0x00U));
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        if (!complete_step(api, 0x0024B4U, opcode, 28, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024B4U: { // ANDI.W #$0003, D5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const std::uint32_t res = d5 & 0x0003U;
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0024B8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024B8U: { // ANDI.W #$3FFF, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t res = d6 & 0x3FFFU;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0024BCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024BCU: { // ORI.W #$4000, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t res = (d6 & 0xFFFFU) | 0x4000U;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (!complete_step(api, 0x0024C0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024C0U: { // MOVE.W D6, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | (d6 & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6 & 0xFFFFU));
        if (!complete_step(api, 0x0024C2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024C2U: { // ADD.W D1, D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto d0 = api.read_register(api.context, kRegD0);
        const std::uint32_t val = d0 & 0xFFFFU;
        const std::uint32_t sum = val + d1;
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d1, val, sum);
        if (!complete_step(api, 0x0024C4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024C4U: { // MOVE.W D0, (A4)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d0 = api.read_register(api.context, kRegD0) & 0xFFFFU;
        write_be(api, a4, 2, d0);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d0));
        if (!complete_step(api, 0x0024C6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024C6U: { // MOVE.W D5, (A4)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        write_be(api, a4, 2, d5);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5));
        if (!complete_step(api, 0x0024C8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024C8U: { // MOVE.W #$001C, D7
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | 0x001CU);
        update_logic_ccr16(api, 0x001CU);
        if (!complete_step(api, 0x0024CCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024CCU: { // MOVE.W (A0), (A2)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, kRegA0);
        const auto a2 = api.read_register(api.context, kRegA2);
        unsigned word{};
        if (!read_be(api, a0, 2, word)) return M68k9D70StepResult::ContractFault;
        write_be(api, a2, 2, word);
        update_logic_ccr16(api, static_cast<std::uint16_t>(word));
        if (!complete_step(api, 0x0024CEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024CEU: { // SWAP.W D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t swapped = (d2 >> 16U) | (d2 << 16U);
        api.write_register(api.context, kRegD2, swapped);
        const std::uint16_t old = ccr(api) & 0x10U;
        const std::uint16_t n = (swapped & 0x80000000U) ? 0x08U : 0x00U;
        const std::uint16_t z = (swapped == 0U) ? 0x04U : 0x00U;
        set_ccr(api, old | n | z);
        if (!complete_step(api, 0x0024D0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024D0U: { // ADDA.W D2, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = static_cast<std::int32_t>(static_cast<std::int16_t>(api.read_register(api.context, kRegD2) & 0xFFFFU));
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 + static_cast<std::uint32_t>(d2));
        if (!complete_step(api, 0x0024D2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024D2U: { // SWAP.W D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t swapped = (d2 >> 16U) | (d2 << 16U);
        api.write_register(api.context, kRegD2, swapped);
        const std::uint16_t old = ccr(api) & 0x10U;
        const std::uint16_t n = (swapped & 0x80000000U) ? 0x08U : 0x00U;
        const std::uint16_t z = (swapped == 0U) ? 0x04U : 0x00U;
        set_ccr(api, old | n | z);
        if (!complete_step(api, 0x0024D4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024D4U: { // ADDQ.W #$01, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d2 = api.read_register(api.context, kRegD2);
        const std::uint32_t val = d2 & 0xFFFFU;
        const std::uint32_t sum = (val + 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD2, (d2 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 1U, val, val + 1U);
        if (!complete_step(api, 0x0024D6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024D6U: { // CMP.W D3, D2
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const std::uint32_t src = api.read_register(api.context, kRegD3) & 0xFFFFU;
        const std::uint32_t dst = api.read_register(api.context, kRegD2) & 0xFFFFU;
        const std::uint32_t res = dst - src;
        update_cmp_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x0024D8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024D8U: { // BLT.W $0024EC
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        const bool taken = (n != v);
        const unsigned target = taken ? 0x0024ECU : 0x0024DCU;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024DCU: { // MOVE.W 36(A3), D0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d0_w{};
        if (!read_be(api, a3 + 36U, 2, d0_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, kRegD0);
        api.write_register(api.context, kRegD0, (old_d0 & 0xFFFF0000U) | (d0_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d0_w));
        if (!complete_step(api, 0x0024E0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024E0U: { // MOVE.W 34(A3), D3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned d3_w{};
        if (!read_be(api, a3 + 34U, 2, d3_w)) return M68k9D70StepResult::ContractFault;
        const auto old_d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (old_d3 & 0xFFFF0000U) | (d3_w & 0xFFFFU));
        update_logic_ccr16(api, static_cast<std::uint16_t>(d3_w));
        if (!complete_step(api, 0x0024E4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024E4U: { // LSL.W D0, D3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto shift = api.read_register(api.context, kRegD0) & 0x3FU;
        const auto old_d3 = api.read_register(api.context, kRegD3);
        const std::uint32_t val = old_d3 & 0xFFFFU;
        const std::uint32_t res = (val << (shift % 64U)) & 0xFFFFU;
        api.write_register(api.context, kRegD3, (old_d3 & 0xFFFF0000U) | res);
        update_logic_ccr16(api, static_cast<std::uint16_t>(res));
        if (shift != 0) {
            api.add_cycles(api.context, static_cast<int>(shift) * 14);
            if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
        }
        if (!complete_step(api, 0x0024E6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024E6U: { // SUBA.W D3, A0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d3 = static_cast<std::int32_t>(static_cast<std::int16_t>(api.read_register(api.context, kRegD3) & 0xFFFFU));
        const auto a0 = api.read_register(api.context, kRegA0);
        api.write_register(api.context, kRegA0, a0 - static_cast<std::uint32_t>(d3));
        if (!complete_step(api, 0x0024E8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024E8U: { // MOVE.W #$03E8, D3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d3 = api.read_register(api.context, kRegD3);
        api.write_register(api.context, kRegD3, (d3 & 0xFFFF0000U) | 0x03E8U);
        update_logic_ccr16(api, 0x03E8U);
        if (!complete_step(api, 0x0024ECU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024ECU: { // ADDI.W #$0080, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t sum = (val + 0x0080U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 0x0080U, val, val + 0x0080U);
        if (!complete_step(api, 0x0024F0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024F0U: { // ADDQ.W #$01, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t val = d4 & 0xFFFFU;
        const std::uint32_t sum = (val + 1U) & 0xFFFFU;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | sum);
        update_add_ccr16(api, 1U, val, val + 1U);
        if (!complete_step(api, 0x0024F2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024F2U: { // CMPI.W #, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const std::uint32_t src = 0x0020U;
        const std::uint32_t dst = api.read_register(api.context, kRegD4) & 0xFFFFU;
        const std::uint32_t res = dst - src;
        update_cmp_ccr16(api, src, dst, res);
        if (!complete_step(api, 0x0024F6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024F6U: { // BLT.W $002508
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        const bool taken = (n != v);
        const unsigned target = taken ? 0x002508U : 0x0024FAU;
        const int not_taken_adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        const int adj = taken ? 0 : not_taken_adj;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024FAU: { // SUBI.W #$0020, D4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d4 = api.read_register(api.context, kRegD4);
        const std::uint32_t val = d4 & 0xFFFFU;
        const std::uint32_t res = (val - 0x0020U) & 0xFFFFU;
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, 0x0020U, val, val - 0x0020U);
        if (!complete_step(api, 0x0024FEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0024FEU: { // SUBI.W #$1000, D1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d1 = api.read_register(api.context, kRegD1);
        const std::uint32_t val = d1 & 0xFFFFU;
        const std::uint32_t res = (val - 0x1000U) & 0xFFFFU;
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res);
        update_sub_ccr16(api, 0x1000U, val, val - 0x1000U);
        if (!complete_step(api, 0x002502U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002502U: { // ADD.W D1, D6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1) & 0xFFFFU;
        const auto d6 = api.read_register(api.context, kRegD6);
        const std::uint32_t val = d6 & 0xFFFFU;
        const std::uint32_t sum = val + d1;
        api.write_register(api.context, kRegD6, (d6 & 0xFFFF0000U) | (sum & 0xFFFFU));
        update_add_ccr16(api, d1, val, sum);
        if (!complete_step(api, 0x002504U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002504U: { // MOVE.W D6, (A4)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d6 = api.read_register(api.context, kRegD6) & 0xFFFFU;
        write_be(api, a4, 2, d6);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d6));
        if (!complete_step(api, 0x002506U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002506U: { // MOVE.W D5, (A4)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a4 = api.read_register(api.context, kRegA4);
        const auto d5 = api.read_register(api.context, kRegD5) & 0xFFFFU;
        write_be(api, a4, 2, d5);
        update_logic_ccr16(api, static_cast<std::uint16_t>(d5));
        if (!complete_step(api, 0x002508U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002508U: { // DBF D7, $0024CC
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7);
        const auto low16 = static_cast<std::int16_t>(d7 & 0xFFFFU);
        const auto next_val = static_cast<std::uint16_t>(low16 - 1);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | next_val);
        const bool loop_branch = (low16 != 0);
        const unsigned target = loop_branch ? 0x0024CCU : 0x00250CU;
        const int adj = loop_branch ? -14 : 14;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00250CU: { // MOVE.W #$8F02, (A4)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto a4 = api.read_register(api.context, kRegA4);
        write_be(api, a4, 2, 0x8F02U);
        update_logic_ccr16(api, 0x8F02U);
        if (!complete_step(api, 0x002510U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002510U: { // RTS
        return step_rts(api, pc, resume_pc);
    }
    default:
        break;
    }

    return M68k9D70StepResult::NotHandled;
}

} // namespace oasis::hybrid::step_0023cc
"""
    (SRC_DIR / "m68k_0023cc_upload_step.hpp").write_text(hpp, encoding="utf-8")
    (SRC_DIR / "m68k_0023cc_upload_step.cpp").write_text(cpp, encoding="utf-8")
