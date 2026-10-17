#include "tools/hybrid/m68k_00b3b2_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"
#include "tools/hybrid/m68k_gameplay_step_flags.hpp"

namespace oasis::hybrid::step_00b3b2 {
namespace {
using namespace step_623ac;
using namespace step_flags;
constexpr unsigned kMask24 = 0x00FFFFFFU;
} // namespace

bool verify_rom_words(const M68k9D70StepApi& api, unsigned start_pc) noexcept {
    for (std::size_t i = 0; i < kExpectedWords_00b3b2.size(); ++i) {
        unsigned word{};
        if (!read_be(api, start_pc + static_cast<unsigned>(i) * 2U, 2, word) ||
            word != kExpectedWords_00b3b2[i]) {
            return false;
        }
    }
    unsigned rts_word{};
    if (!read_be(api, 0x00B382U, 2, rts_word) || rts_word != kExpectedWord_00b382) {
        return false;
    }
    return true;
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

M68k9D70StepResult step_bne_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept {
    return step_623ac::step_bcc_w(api, pc, 6U, target, next_pc, resume_pc);
}

M68k9D70StepResult step_move_b_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept {
    return step_623ac::step_move_b_mem_reg(api, pc, areg, static_cast<unsigned>(disp), dreg, next_pc, resume_pc);
}

M68k9D70StepResult step_btst_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept {
    return step_623ac::step_btst_imm_reg(api, pc, bit, dreg, next_pc, resume_pc);
}

M68k9D70StepResult step_bsr_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned return_pc, unsigned& resume_pc, bool& child_active,
                              unsigned& saved_sp) noexcept {
    return step_623ac::step_bsr_w(api, pc, target, return_pc, resume_pc, child_active, saved_sp);
}

M68k9D70StepResult step_bra_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned& resume_pc) noexcept {
    return step_623ac::step_bra_w(api, pc, target, resume_pc);
}

M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc,
                            unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const unsigned sp = api.read_register(api.context, 15U) & kMask24;
    if (!in_work_ram(sp, 4U)) return M68k9D70StepResult::ContractFault;
    const unsigned popped_return = api.read(api.context, sp, 4);
    if (popped_return != return_pc) return M68k9D70StepResult::ContractFault;
    api.write_register(api.context, 15U, (sp + 4U) & kMask24);
    if (!branch_to(api, popped_return, resume_pc)) return M68k9D70StepResult::ContractFault;
    api.finish_instruction(api.context, opcode);
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_00b3b2
