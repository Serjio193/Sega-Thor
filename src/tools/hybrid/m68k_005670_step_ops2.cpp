#include "tools/hybrid/m68k_005670_step_ops.hpp"
#include "tools/hybrid/m68k_gameplay_step_flags.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

namespace oasis::hybrid::step_005670 {
namespace {
using namespace step_flags;
using namespace step_623ac;
constexpr unsigned kMask24 = 0xFFFFFFU;
} // namespace

M68k9D70StepResult step_move_w_disp_disp(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                         std::int16_t src_disp, unsigned dst_areg, std::int16_t dst_disp,
                                         unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto s_base = api.read_register(api.context, src_areg);
    const auto s_addr = (s_base + static_cast<unsigned>(static_cast<std::int32_t>(src_disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, s_addr, 2));
    const auto d_base = api.read_register(api.context, dst_areg);
    const auto d_addr = (d_base + static_cast<unsigned>(static_cast<std::int32_t>(dst_disp))) & kMask24;
    api.write(api.context, d_addr, 2, val);
    set_ccr(api, logic_flags(val, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_sf_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, addr, 1, 0U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_clr_b_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    api.write(api.context, addr, 1, 0U);
    set_ccr(api, (ccr(api) & 0x10U) | 4U);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_andi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto orig = api.read_register(api.context, dreg);
    const auto val = static_cast<std::uint16_t>(orig);
    const auto res = static_cast<std::uint16_t>(val & imm);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | res);
    set_ccr(api, logic_flags(res, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_sub_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                       std::int16_t disp, unsigned dreg, unsigned next_pc,
                                       unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto mem = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    const auto orig = api.read_register(api.context, dreg);
    const auto d = static_cast<std::uint16_t>(orig);
    const auto res = static_cast<std::uint16_t>(d - mem);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | res);
    set_ccr(api, sub_flags(d, mem, res, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_sub_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto s = static_cast<std::uint16_t>(api.read_register(api.context, src));
    const auto orig = api.read_register(api.context, dst);
    const auto d = static_cast<std::uint16_t>(orig);
    const auto res = static_cast<std::uint16_t>(d - s);
    api.write_register(api.context, dst, (orig & 0xFFFF0000U) | res);
    set_ccr(api, sub_flags(d, s, res, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_add_w_reg_abs(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                      unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read_register(api.context, dreg));
    const auto mem = static_cast<std::uint16_t>(api.read(api.context, addr & kMask24, 2));
    const auto sum = static_cast<std::uint16_t>(mem + val);
    api.write(api.context, addr & kMask24, 2, sum);
    set_ccr(api, add_flags(mem, val, sum, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_ori_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                  unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto orig = api.read_register(api.context, dreg);
    const auto val = static_cast<std::uint16_t>(orig);
    const auto res = static_cast<std::uint16_t>(val | imm);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | res);
    set_ccr(api, logic_flags(res, ccr(api)));
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

M68k9D70StepResult step_movea_l_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                         std::int16_t disp, unsigned dst_areg, unsigned next_pc,
                                         unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = api.read(api.context, addr, 4);
    api.write_register(api.context, dst_areg, val & kMask24);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_btst_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = api.read_register(api.context, dreg);
    const bool zero = (val & (1U << (bit & 31U))) == 0U;
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmpi_w_disp(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                    unsigned areg, std::int16_t disp, unsigned next_pc,
                                    unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    const auto res = static_cast<std::uint16_t>(val - imm);
    set_ccr(api, sub_flags(val, imm, res, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bchg_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = api.read_register(api.context, dreg);
    const bool zero = (val & (1U << (bit & 31U))) == 0U;
    api.write_register(api.context, dreg, val ^ (1U << (bit & 31U)));
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_add_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto s = static_cast<std::uint16_t>(api.read_register(api.context, src));
    const auto orig = api.read_register(api.context, dst);
    const auto d = static_cast<std::uint16_t>(orig);
    const auto sum = static_cast<std::uint16_t>(d + s);
    api.write_register(api.context, dst, (orig & 0xFFFF0000U) | sum);
    set_ccr(api, add_flags(d, s, sum, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmp_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                       std::int16_t disp, unsigned dreg, unsigned next_pc,
                                       unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto mem = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    const auto d = static_cast<std::uint16_t>(api.read_register(api.context, dreg));
    const auto res = static_cast<std::uint16_t>(d - mem);
    set_ccr(api, sub_flags(d, mem, res, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_imm_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                       unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto orig = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | imm);
    set_ccr(api, logic_flags(imm, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_addi_w_disp(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                    unsigned areg, std::int16_t disp, unsigned next_pc,
                                    unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    const auto val = static_cast<std::uint16_t>(api.read(api.context, addr, 2));
    const auto sum = static_cast<std::uint16_t>(val + imm);
    api.write(api.context, addr, 2, sum);
    set_ccr(api, add_flags(val, imm, sum, ccr(api), true));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lea_pc_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg_index,
                                        unsigned dst_areg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto dval = static_cast<std::int16_t>(api.read_register(api.context, dreg_index));
    const auto ea = (0x0059BAU + static_cast<unsigned>(static_cast<std::int32_t>(dval))) & kMask24;
    api.write_register(api.context, dst_areg, ea);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_adda_w_ind_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                       unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto a = api.read_register(api.context, areg) & kMask24;
    const auto offset = static_cast<std::int16_t>(api.read(api.context, a, 2));
    const auto new_a = (a + static_cast<unsigned>(static_cast<std::int32_t>(offset))) & kMask24;
    api.write_register(api.context, areg, new_a);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_jmp_ind(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto target = api.read_register(api.context, areg) & kMask24;
    resume_pc = target;
    if (!complete_step(api, target, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmpa_l_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned imm,
                                       unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = api.read_register(api.context, areg);
    const auto res = val - imm;
    const bool carry = val < imm;
    const bool overflow = ((val ^ imm) & (val ^ res) & 0x80000000U) != 0;
    const auto old_ccr = ccr(api);
    const auto new_ccr = static_cast<std::uint16_t>((old_ccr & 0x10U) |
        (res == 0 ? 4U : 0U) | ((res & 0x80000000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
    set_ccr(api, new_ccr);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_movem_l_reg_predec(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                           unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto val = api.read_register(api.context, dreg);
    const auto sp = (api.read_register(api.context, 15U) - 4U) & kMask24;
    api.write_register(api.context, 15U, sp);
    api.write(api.context, sp, 4, val);
    api.add_cycles(api.context, 56);
    if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bset_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit_reg,
                                     unsigned dst_reg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto bit = api.read_register(api.context, bit_reg) & 31U;
    const auto val = api.read_register(api.context, dst_reg);
    const bool zero = (val & (1U << bit)) == 0U;
    api.write_register(api.context, dst_reg, val | (1U << bit));
    set_ccr(api, (ccr(api) & ~4U) | (zero ? 4U : 0U));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_movem_l_postinc_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                            unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto sp = api.read_register(api.context, 15U) & kMask24;
    const auto val = api.read(api.context, sp, 4);
    const auto new_sp = (sp + 4U) & kMask24;
    api.write_register(api.context, 15U, new_sp);
    api.write_register(api.context, dreg, val);
    (void)api.read(api.context, new_sp, 2);
    api.add_cycles(api.context, 56);
    if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_cmp_b_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto s = static_cast<std::uint8_t>(api.read_register(api.context, src));
    const auto d = static_cast<std::uint8_t>(api.read_register(api.context, dst));
    const auto res = static_cast<std::uint8_t>(d - s);
    set_ccr(api, byte_sub_flags(d, s, res, ccr(api), false));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_005670
