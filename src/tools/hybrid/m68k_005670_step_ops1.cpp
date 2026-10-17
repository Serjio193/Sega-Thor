#include "tools/hybrid/m68k_005670_step_ops.hpp"
#include "tools/hybrid/m68k_gameplay_step_flags.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

namespace oasis::hybrid::step_005670 {
namespace {
using namespace step_flags;
using namespace step_623ac;
constexpr unsigned kMask24 = 0xFFFFFFU;
} // namespace

M68k9D70StepResult step_clr_l_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, addr, 4, 0U);
    set_ccr(api, (ccr(api) & 0x10U) | 4U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_tst_b_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto val = static_cast<std::uint8_t>(api.read_register(api.context, dreg));
    set_ccr(api, byte_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bset_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
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
    api.write(api.context, addr, 1, val | (1U << (bit & 7U)));
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bclr_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
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
    api.write(api.context, addr, 1, val & ~(1U << (bit & 7U)));
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
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

M68k9D70StepResult step_cmpi_w_abs(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    const auto res = static_cast<std::uint16_t>(val - imm);
    set_ccr(api, sub_flags(val, imm, res, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_abs_disp(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    const auto base = api.read_register(api.context, areg);
    const auto dst = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, dst, 2, val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_imm_disp(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto dst = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, dst, 2, imm);
    set_ccr(api, logic_flags(imm, ccr(api)));
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
    set_ccr(api, (ccr(api) & 0x10U) | 4U);
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

M68k9D70StepResult step_tst_w_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    set_ccr(api, logic_flags(val, ccr(api)));
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

M68k9D70StepResult step_moveq(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                              unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto ext = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(imm)));
    api.write_register(api.context, dreg, ext);
    set_ccr(api, long_logic_flags(ext, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_and_b_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto s = static_cast<std::uint8_t>(api.read_register(api.context, src));
    const auto orig = api.read_register(api.context, dst);
    const auto d = static_cast<std::uint8_t>(orig);
    const auto res = static_cast<std::uint8_t>(s & d);
    api.write_register(api.context, dst, (orig & 0xFFFFFF00U) | res);
    set_ccr(api, byte_logic_flags(res, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmp_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto s = static_cast<std::uint16_t>(api.read_register(api.context, src));
    const auto d = static_cast<std::uint16_t>(api.read_register(api.context, dst));
    const auto res = static_cast<std::uint16_t>(d - s);
    set_ccr(api, sub_flags(d, s, res, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_addq_w_reg(const M68k9D70StepApi& api, unsigned pc, unsigned imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto orig = api.read_register(api.context, dreg);
    const auto val = static_cast<std::uint16_t>(orig);
    const auto sum = static_cast<std::uint16_t>(val + imm);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | sum);
    set_ccr(api, add_flags(val, static_cast<std::uint16_t>(imm), sum, ccr(api), true));
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

M68k9D70StepResult step_subq_b_abs(const M68k9D70StepApi& api, unsigned pc, unsigned imm,
                                   unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint8_t>(api.read(api.context, addr & kMask24, 1));
    const auto res = static_cast<std::uint8_t>(val - imm);
    api.write(api.context, addr & kMask24, 1, res);
    set_ccr(api, byte_sub_flags(val, static_cast<std::uint8_t>(imm), res, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_b_reg_disp(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint8_t>(api.read_register(api.context, dreg));
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, addr, 1, val);
    set_ccr(api, byte_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_clr_w_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, addr, 2, 0U);
    set_ccr(api, (ccr(api) & 0x10U) | 4U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_tst_l_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = api.read(api.context, addr, 4);
    set_ccr(api, long_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lea_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                     std::int16_t disp, unsigned dst_areg, unsigned next_pc,
                                     unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, src_areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write_register(api.context, dst_areg, addr);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmpi_w_ind(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto addr = api.read_register(api.context, areg) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    const auto res = static_cast<std::uint16_t>(val - imm);
    set_ccr(api, sub_flags(val, imm, res, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_l_disp_disp(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                         std::int16_t src_disp, unsigned dst_areg, std::int16_t dst_disp,
                                         unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto s_base = api.read_register(api.context, src_areg);
    const auto s_addr = (s_base + static_cast<unsigned>(static_cast<std::int32_t>(src_disp))) & kMask24;
    const auto val = api.read(api.context, s_addr, 4);
    const auto d_base = api.read_register(api.context, dst_areg);
    const auto d_addr = (d_base + static_cast<unsigned>(static_cast<std::int32_t>(dst_disp))) & kMask24;
    api.write(api.context, d_addr, 4, val);
    set_ccr(api, long_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_b_disp_disp(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                         std::int16_t src_disp, unsigned dst_areg, std::int16_t dst_disp,
                                         unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto s_base = api.read_register(api.context, src_areg);
    const auto s_addr = (s_base + static_cast<unsigned>(static_cast<std::int32_t>(src_disp))) & kMask24;
    const auto val = static_cast<std::uint8_t>(api.read(api.context, s_addr, 1));
    const auto d_base = api.read_register(api.context, dst_areg);
    const auto d_addr = (d_base + static_cast<unsigned>(static_cast<std::int32_t>(dst_disp))) & kMask24;
    api.write(api.context, d_addr, 1, val);
    set_ccr(api, byte_logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_005670
