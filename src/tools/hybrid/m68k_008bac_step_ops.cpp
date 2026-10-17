#include "tools/hybrid/m68k_008bac_step_ops.hpp"
#include "tools/hybrid/m68k_gameplay_step_flags.hpp"

namespace oasis::hybrid::step_008bac {
namespace {
using namespace step_flags;
constexpr unsigned kMask24 = 0xFFFFFFU;
} // namespace

M68k9D70StepResult step_not_b_reg(const M68k9D70StepApi& api, unsigned pc, unsigned reg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto cur = api.read_register(api.context, reg);
    const auto val = static_cast<std::uint8_t>(~cur);
    api.write_register(api.context, reg, (cur & ~0xFFU) | val);
    set_ccr(api, byte_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_andi_b_imm_reg(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                                      unsigned reg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto cur = api.read_register(api.context, reg);
    const auto val = static_cast<std::uint8_t>(cur & imm);
    api.write_register(api.context, reg, (cur & ~0xFFU) | val);
    set_ccr(api, byte_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_st_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                               unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write(api.context, addr & kMask24, 1, 0xFFU);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_clr_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                 unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write(api.context, addr & kMask24, 2, 0U);
    set_ccr(api, logic_flags(0, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_tst_b_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                 unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint8_t>(api.read(api.context, addr & kMask24, 1));
    set_ccr(api, byte_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_tst_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                 unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_sf_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                              unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write(api.context, addr & kMask24, 1, 0U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_btst_imm_abs(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                    unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint8_t>(api.read(api.context, addr & kMask24, 1));
    const bool zero = (val & (1U << (bit & 7U))) == 0U;
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                      unsigned reg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    const auto cur = api.read_register(api.context, reg);
    api.write_register(api.context, reg, (cur & 0xFFFF0000U) | val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_btst_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                    unsigned reg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = api.read_register(api.context, reg);
    const bool zero = (val & (1U << (bit & 31U))) == 0U;
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmpi_w_abs(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                  unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    const auto diff = static_cast<std::uint16_t>(val - imm);
    set_ccr(api, sub_flags(val, imm, diff, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_b_imm_abs(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                                      unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write(api.context, addr & kMask24, 1, imm);
    set_ccr(api, byte_logic_flags(imm, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_008bac
