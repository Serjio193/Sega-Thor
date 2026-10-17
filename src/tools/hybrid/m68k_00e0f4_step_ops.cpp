#include "tools/hybrid/m68k_00e0f4_step_ops.hpp"

namespace oasis::hybrid::step_00e0f4 {

namespace {

constexpr unsigned kMask24 = 0x00FFFFFFU;

bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& val) noexcept {
    val = 0U;
    for (int i = 0; i < width; ++i) {
        const int byte_val = api.peek(api.context, (addr + static_cast<unsigned>(i)) & kMask24);
        if (byte_val < 0) return false;
        val = (val << 8U) | static_cast<unsigned>(byte_val & 0xFF);
    }
    return true;
}

void set_ccr(const M68k9D70StepApi& api, std::uint16_t ccr_bits) noexcept {
    const auto sr = static_cast<std::uint16_t>(api.read_register(api.context, 17U));
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (ccr_bits & 0x001FU));
}

std::uint16_t ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U) & 0x001FU);
}

std::uint16_t logic_flags_w(std::uint16_t val, std::uint16_t old_ccr) noexcept {
    std::uint16_t res = old_ccr & 0x0010U;   // keep X
    if (val == 0U) res |= 0x04U;             // Z
    if ((val & 0x8000U) != 0U) res |= 0x08U; // N
    return res;
}

std::uint16_t logic_flags_l(std::uint32_t val, std::uint16_t old_ccr) noexcept {
    std::uint16_t res = old_ccr & 0x0010U;       // keep X
    if (val == 0U) res |= 0x04U;                 // Z
    if ((val & 0x80000000U) != 0U) res |= 0x08U; // N
    return res;
}

std::uint16_t sub_flags_w(std::uint16_t src, std::uint16_t dst, std::uint16_t res,
                          std::uint16_t old_ccr) noexcept {
    const unsigned sm = (src >> 15U) & 1U;
    const unsigned dm = (dst >> 15U) & 1U;
    const unsigned rm = (res >> 15U) & 1U;
    std::uint16_t flags = old_ccr & 0x0010U;
    if (res == 0U) flags |= 0x04U;
    if (rm != 0U) flags |= 0x08U;
    if (((!sm & dm & !rm) | (sm & !dm & rm)) != 0U) flags |= 0x02U;
    if (((sm & !dm) | (rm & !dm) | (sm & rm)) != 0U) flags |= 0x11U; // X and C
    return flags;
}

bool prepare_step(const M68k9D70StepApi& api, unsigned pc, std::uint16_t& opcode) noexcept {
    unsigned op{};
    if (!read_be(api, pc, 2, op)) return false;
    opcode = static_cast<std::uint16_t>(op);
    api.observe_instruction(api.context, pc, opcode);
    api.begin_instruction(api.context, opcode);
    return true;
}

bool complete_step(const M68k9D70StepApi& api, unsigned next_pc, std::uint16_t opcode,
                   int extra_cycles, unsigned& resume_pc) noexcept {
    unsigned next_word{};
    if (!read_be(api, next_pc, 2, next_word)) return false;
    api.set_return_state(api.context, next_pc, next_pc + 2U, next_word);
    if (extra_cycles != 0) api.add_cycles(api.context, extra_cycles);
    api.finish_instruction(api.context, opcode);
    resume_pc = next_pc;
    return true;
}

} // namespace

bool verify_rom_words(const M68k9D70StepApi& api, unsigned start_pc) noexcept {
    for (std::size_t i = 0; i < kExpectedWords_00e0f4.size(); ++i) {
        unsigned word{};
        if (!read_be(api, start_pc + static_cast<unsigned>(i * 2U), 2, word)) return false;
        if (static_cast<std::uint16_t>(word) != kExpectedWords_00e0f4[i]) return false;
    }
    return true;
}

M68k9D70StepResult step_move_w_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                      unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    unsigned val{};
    if (!read_be(api, addr, 2, val)) return M68k9D70StepResult::ContractFault;
    const auto cur = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, (cur & 0xFFFF0000U) | (val & 0xFFFFU));
    set_ccr(api, logic_flags_w(static_cast<std::uint16_t>(val), ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_beq_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const bool taken = (ccr(api) & 0x04U) != 0U;
    const unsigned target_pc = taken ? target : next_pc;
    const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
    if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_subq_w_1_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                     unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto orig = api.read_register(api.context, dreg);
    const auto dst = static_cast<std::uint16_t>(orig);
    const auto res = static_cast<std::uint16_t>(dst - 1U);
    api.write_register(api.context, dreg, (orig & 0xFFFF0000U) | res);
    set_ccr(api, sub_flags_w(1U, dst, res, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_reg_abs(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                      unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    const auto val = static_cast<std::uint16_t>(api.read_register(api.context, dreg));
    api.write(api.context, addr & kMask24, 2, val);
    set_ccr(api, logic_flags_w(val, ccr(api)));
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

M68k9D70StepResult step_bne_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const bool taken = (ccr(api) & 0x04U) == 0U;
    const unsigned target_pc = taken ? target : next_pc;
    const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
    if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lea_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                    unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    (void)api.fetch16(api.context);
    api.write_register(api.context, 8U + areg, addr & kMask24);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_moveq(const M68k9D70StepApi& api, unsigned pc, std::int32_t val,
                              unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto uval = static_cast<std::uint32_t>(val);
    api.write_register(api.context, dreg, uval);
    set_ccr(api, logic_flags_l(uval, ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_move_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, 8U + areg);
    const auto addr = (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24;
    unsigned val{};
    if (!read_be(api, addr, 2, val)) return M68k9D70StepResult::ContractFault;
    const auto cur = api.read_register(api.context, dreg);
    api.write_register(api.context, dreg, (cur & 0xFFFF0000U) | (val & 0xFFFFU));
    set_ccr(api, logic_flags_w(static_cast<std::uint16_t>(val), ccr(api)));
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_ble_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto c = ccr(api);
    const bool z = (c & 0x04U) != 0U;
    const bool n = (c & 0x08U) != 0U;
    const bool v = (c & 0x02U) != 0U;
    const bool taken = z || (n != v);
    const unsigned target_pc = taken ? target : next_pc;
    const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
    if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lea_pc_index_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                         unsigned base_pc, unsigned areg, unsigned next_pc,
                                         unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto dval = static_cast<std::int16_t>(api.read_register(api.context, dreg) & 0xFFFFU);
    const auto target_addr = (base_pc + static_cast<unsigned>(static_cast<std::int32_t>(dval))) & kMask24;
    api.write_register(api.context, 8U + areg, target_addr);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_adda_w_ind_reg(const M68k9D70StepApi& api, unsigned pc, unsigned sreg,
                                       unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto s_addr = api.read_register(api.context, 8U + sreg) & kMask24;
    unsigned disp_word{};
    if (!read_be(api, s_addr, 2, disp_word)) return M68k9D70StepResult::ContractFault;
    const auto sdisp = static_cast<std::int16_t>(disp_word & 0xFFFFU);
    const auto cur_a = api.read_register(api.context, 8U + areg);
    api.write_register(api.context, 8U + areg, (cur_a + static_cast<unsigned>(static_cast<std::int32_t>(sdisp))) & kMask24);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_movem_l_push_d7_a6(const M68k9D70StepApi& api, unsigned pc,
                                           unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto old_sp = api.read_register(api.context, 15U) & kMask24;
    const auto sp = (old_sp - 8U) & kMask24;
    const auto d7 = api.read_register(api.context, 7U);
    const auto a6 = api.read_register(api.context, 8U + 6U);
    api.write(api.context, sp, 4, d7);
    api.write(api.context, sp + 4U, 4, a6);
    api.write_register(api.context, 15U, sp);
    api.add_cycles(api.context, 112);
    if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_jsr_a0(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc,
                               unsigned& resume_pc, bool& child_active, unsigned& saved_sp) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto target = api.read_register(api.context, 8U) & kMask24;
    const auto old_sp = api.read_register(api.context, 15U) & kMask24;
    const auto sp = (old_sp - 4U) & kMask24;
    api.write(api.context, sp, 4, return_pc);
    api.write_register(api.context, 15U, sp);
    unsigned target_word{};
    if (!read_be(api, target, 2, target_word)) return M68k9D70StepResult::ContractFault;
    api.set_return_state(api.context, target, target + 2U, target_word);
    api.finish_instruction(api.context, opcode);
    child_active = true;
    saved_sp = old_sp;
    resume_pc = return_pc;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_movem_l_pop_d7_a6(const M68k9D70StepApi& api, unsigned pc,
                                          unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto sp = api.read_register(api.context, 15U) & kMask24;
    unsigned d7{}, a6{};
    if (!read_be(api, sp, 4, d7) || !read_be(api, sp + 4U, 4, a6)) return M68k9D70StepResult::ContractFault;
    api.write_register(api.context, 7U, d7);
    api.write_register(api.context, 8U + 6U, a6);
    const auto new_sp = (sp + 8U) & kMask24;
    api.write_register(api.context, 15U, new_sp);
    (void)api.read(api.context, new_sp, 2);
    api.add_cycles(api.context, 112);
    if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_lea_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                     std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto base = api.read_register(api.context, 8U + areg);
    api.write_register(api.context, 8U + areg, (base + static_cast<unsigned>(static_cast<std::int32_t>(disp))) & kMask24);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_dbf_d7(const M68k9D70StepApi& api, unsigned pc, unsigned loop_pc,
                               unsigned exit_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto orig = api.read_register(api.context, 7U);
    auto count = static_cast<std::int16_t>(orig & 0xFFFFU);
    count = static_cast<std::int16_t>(count - 1);
    api.write_register(api.context, 7U, (orig & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
    const bool taken = (count != -1);
    const unsigned target_pc = taken ? loop_pc : exit_pc;
    const int adj = taken ? -14 : 14;
    if (!complete_step(api, target_pc, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto sp = api.read_register(api.context, 15U) & kMask24;
    api.write_register(api.context, 15U, (sp + 4U) & kMask24);
    unsigned next_word{};
    if (!read_be(api, return_pc, 2, next_word)) return M68k9D70StepResult::ContractFault;
    api.set_return_state(api.context, return_pc, return_pc + 2U, next_word);
    api.finish_instruction(api.context, opcode);
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_00e0f4
