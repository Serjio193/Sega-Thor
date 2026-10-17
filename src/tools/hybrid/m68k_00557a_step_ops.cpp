#include "tools/hybrid/m68k_00557a_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"
#include "tools/hybrid/m68k_gameplay_step_flags.hpp"

namespace oasis::hybrid::step_00557a {
namespace {
using namespace step_623ac;
using namespace step_flags;
constexpr unsigned kMask24 = 0xFFFFFFU;
} // namespace

M68k9D70StepResult step_sf_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                               unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write(api.context, addr & kMask24, 1, 0x00U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lea_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write_register(api.context, areg, addr & kMask24);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_clr_w_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto orig = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, orig & 0xFFFF0000U);
    set_ccr(api, (ccr(api) & 0x10U) | 0x04U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                       unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto val = static_cast<std::uint16_t>(api.read_register(api.context, src));
    const auto orig_dst = api.read_register(api.context, dst);
    api.write_register(api.context, dst, (orig_dst & 0xFFFF0000U) | val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_b_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto byte_val = static_cast<std::uint8_t>(api.read(api.context, addr, 1));
    const auto orig = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, (orig & 0xFFFFFF00U) | byte_val);
    set_ccr(api, logic_flags(byte_val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_btst_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                      unsigned areg, std::int16_t disp, unsigned next_pc,
                                      unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint8_t>(api.read(api.context, addr, 1));
    const bool zero = (val & (1U << (bit & 7U))) == 0U;
    set_ccr(api, (ccr(api) & ~0x04U) | (zero ? 0x04U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_exg_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned r1,
                                    unsigned r2, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto val1 = api.read_register(api.context, r1);
    const auto val2 = api.read_register(api.context, r2);
    api.write_register(api.context, r1, val2);
    api.write_register(api.context, r2, val1);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_reg_disp(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read_register(api.context, dreg));
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, addr, 2, val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    const auto orig = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmpi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read_register(api.context, dreg));
    const auto diff = static_cast<std::uint16_t>(val - imm);
    set_ccr(api, sub_flags(val, imm, diff, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lsr_w_reg_1(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                    unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto orig = api.read_register(api.context, dreg);
    const auto val = static_cast<std::uint16_t>(orig);
    const auto c_bit = static_cast<std::uint16_t>((val & 1U) != 0U ? 1U : 0U);
    const auto res = static_cast<std::uint16_t>(val >> 1U);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | res);
    const auto z_bit = static_cast<std::uint16_t>(res == 0U ? 0x04U : 0x00U);
    const auto x_bit = static_cast<std::uint16_t>(c_bit != 0U ? 0x10U : 0x00U);
    set_ccr(api, (ccr(api) & ~0x1FU) | x_bit | z_bit | c_bit);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_addi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto orig = api.read_register(api.context, dreg);
    const auto val = static_cast<std::uint16_t>(orig);
    const auto sum = static_cast<std::uint16_t>(val + imm);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | sum);
    set_ccr(api, add_flags(val, imm, sum, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                       unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    const auto orig = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bsr_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned return_pc, unsigned& resume_pc, bool& child_active,
                              unsigned& saved_sp) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const unsigned old_sp = api.read_register(api.context, 15U) & kMask24;
    const unsigned sp = (old_sp - 4U) & kMask24;
    api.write(api.context, sp, 4, return_pc);
    api.write_register(api.context, 15U, sp);
    unsigned target_word{};
    if (!read_be(api, target, 2U, target_word)) return M68k9D70StepResult::ContractFault;
    api.set_return_state(api.context, target, target + 2U, target_word);
    api.finish_instruction(api.context, opcode);
    child_active = true;
    saved_sp = old_sp;
    resume_pc = return_pc;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_jsr_abs(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                                unsigned return_pc, unsigned& resume_pc, bool& child_active,
                                unsigned& saved_sp) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const unsigned old_sp = api.read_register(api.context, 15U) & kMask24;
    const unsigned sp = (old_sp - 4U) & kMask24;
    api.write(api.context, sp, 4, return_pc);
    api.write_register(api.context, 15U, sp);
    unsigned target_word{};
    if (!read_be(api, target, 2U, target_word)) return M68k9D70StepResult::ContractFault;
    api.set_return_state(api.context, target, target + 2U, target_word);
    api.finish_instruction(api.context, opcode);
    child_active = true;
    saved_sp = old_sp;
    resume_pc = return_pc;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_disp_postinc(const M68k9D70StepApi& api, unsigned pc,
                                            unsigned src_areg, std::int16_t disp,
                                            unsigned dst_areg, unsigned next_pc,
                                            unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto src_base = api.read_register(api.context, src_areg);
    const auto src_addr = (src_base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, src_addr, 2));
    const auto dst_addr = api.read_register(api.context, dst_areg) & kMask24;
    api.write(api.context, dst_addr, 2, val);
    api.write_register(api.context, dst_areg, (dst_addr + 2U) & kMask24);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_disp_ind(const M68k9D70StepApi& api, unsigned pc,
                                        unsigned src_areg, std::int16_t disp,
                                        unsigned dst_areg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto src_base = api.read_register(api.context, src_areg);
    const auto src_addr = (src_base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, src_addr, 2));
    const auto dst_addr = api.read_register(api.context, dst_areg) & kMask24;
    api.write(api.context, dst_addr, 2, val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_l_imm_reg(const M68k9D70StepApi& api, unsigned pc, std::uint32_t imm,
                                       unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write_register(api.context, dreg, imm);
    set_ccr(api, long_logic_flags(imm, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_00557a
