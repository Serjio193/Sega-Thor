#pragma once

#include "tools/hybrid/m68k_02575c_instruction_step.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

namespace oasis::hybrid::step_02575c {

constexpr unsigned kStartPc = 0x02575CU;
constexpr unsigned kMask24 = 0x00FFFFFFU;
constexpr unsigned kCommonJoinPc = 0x0255D8U;

// Child/transfer PCs
constexpr unsigned kChild00F0D2Pc = 0x00F0D2U;
constexpr unsigned kChild00BCD8Pc = 0x00BCD8U;
constexpr unsigned kChild00E7E8Pc = 0x00E7E8U;
constexpr unsigned kChild00E502Pc = 0x00E502U;
constexpr unsigned kChild00F476Pc = 0x00F476U;
constexpr unsigned kChild00E66EPc = 0x00E66EU;
constexpr unsigned kChild00E338Pc = 0x00E338U;
constexpr unsigned kChild00B852Pc = 0x00B852U;
constexpr unsigned kTailJump00E7E8Pc = 0x00E7E8U;

// End-exclusive ROM boundaries for all 11 handlers:
constexpr unsigned kTarget10Entry = 0x0257EEU;
constexpr unsigned kTarget10End = 0x02583EU;
constexpr unsigned kTarget10ChildRet = 0x025822U;
constexpr unsigned kTarget10ExitRts = 0x02583CU;

constexpr unsigned kTarget11Entry = 0x02583EU;
constexpr unsigned kTarget11End = 0x025880U;
constexpr unsigned kTarget11ExitRts = 0x025854U;

constexpr unsigned kTarget12Entry = 0x025880U;
constexpr unsigned kTarget12End = 0x025896U;
constexpr unsigned kTarget12ExitRts = 0x02588AU;

constexpr unsigned kState0Entry = 0x025896U;
constexpr unsigned kState0End = 0x0258A0U;
constexpr unsigned kState0ExitRts = 0x02589EU;

constexpr unsigned kState1Entry = 0x0258A0U;
constexpr unsigned kState1End = 0x025940U;
constexpr unsigned kState1ExitRts1 = 0x0258BAU;
constexpr unsigned kState1ExitRts2 = 0x0258F2U;
constexpr unsigned kState1ExitRts3 = 0x025930U;

constexpr unsigned kState2Entry = 0x025940U;
constexpr unsigned kState2End = 0x025968U;
constexpr unsigned kState2ExitRts = 0x02595CU;

constexpr unsigned kState3Entry = 0x025968U;
constexpr unsigned kState3End = 0x0259ACU;
constexpr unsigned kState3ExitRts = 0x025972U;

constexpr unsigned kState4Entry = 0x0259ACU;
constexpr unsigned kState4End = 0x0259CEU;
constexpr unsigned kState4ExitRts = 0x0259C4U;

constexpr unsigned kState5Entry = 0x0259CEU;
constexpr unsigned kState5End = 0x025A72U;
constexpr unsigned kState5ExitRts1 = 0x0259EAU;
constexpr unsigned kState5ExitRts2 = 0x025A70U;

constexpr unsigned kState6Entry = 0x025A72U;
constexpr unsigned kState6End = 0x025B22U;
constexpr unsigned kState6ExitRts1 = 0x025AAEU;
constexpr unsigned kState6ExitRts2 = 0x025AC0U;

constexpr unsigned kState7Entry = 0x025B22U;
constexpr unsigned kState7End = 0x025B32U;
constexpr unsigned kState7ExitRts = 0x025B30U;

inline void update_ccr_move_l(const M68k9D70StepApi& api, std::uint32_t val) noexcept {
    const auto old_ccr = step_623ac::ccr(api);
    const bool n = (val & 0x80000000U) != 0U;
    const bool z = (val == 0U);
    const std::uint16_t new_ccr = (old_ccr & 0x10U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U);
    step_623ac::set_ccr(api, new_ccr);
}

inline void update_ccr_move_w(const M68k9D70StepApi& api, std::uint16_t val) noexcept {
    const auto old_ccr = step_623ac::ccr(api);
    const bool n = (val & 0x8000U) != 0U;
    const bool z = (val == 0U);
    const std::uint16_t new_ccr = (old_ccr & 0x10U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U);
    step_623ac::set_ccr(api, new_ccr);
}

inline void update_ccr_cmp_w(const M68k9D70StepApi& api, std::uint16_t src, std::uint16_t dst) noexcept {
    const std::uint32_t res = (static_cast<std::uint32_t>(dst) - src) & 0xFFFFU;
    const std::uint8_t sm = (src >> 15U) & 1U;
    const std::uint8_t dm = (dst >> 15U) & 1U;
    const std::uint8_t rm = (res >> 15U) & 1U;
    std::uint16_t new_ccr = step_623ac::ccr(api) & 0x10U;
    if (res == 0U) new_ccr |= 0x04U;
    if (rm != 0U)  new_ccr |= 0x08U;
    if ((!sm && dm && !rm) || (sm && !dm && rm)) new_ccr |= 0x02U;
    if ((sm && !dm) || (rm && !dm) || (sm && rm)) new_ccr |= 0x01U;
    step_623ac::set_ccr(api, new_ccr);
}

inline void update_ccr_sub_l(const M68k9D70StepApi& api, std::uint32_t src, std::uint32_t dst, std::uint32_t res) noexcept {
    const std::uint8_t sm = (src >> 31U) & 1U;
    const std::uint8_t dm = (dst >> 31U) & 1U;
    const std::uint8_t rm = (res >> 31U) & 1U;
    std::uint16_t new_ccr = 0U;
    if (res == 0U) new_ccr |= 0x04U;
    if (rm != 0U)  new_ccr |= 0x08U;
    if ((!sm && dm && !rm) || (sm && !dm && rm)) new_ccr |= 0x02U;
    if ((sm && !dm) || (rm && !dm) || (sm && rm)) new_ccr |= 0x11U;
    step_623ac::set_ccr(api, new_ccr);
}

inline void update_ccr_add_l(const M68k9D70StepApi& api, std::uint32_t src, std::uint32_t dst, std::uint32_t res) noexcept {
    const std::uint8_t sm = (src >> 31U) & 1U;
    const std::uint8_t dm = (dst >> 31U) & 1U;
    const std::uint8_t rm = (res >> 31U) & 1U;
    std::uint16_t new_ccr = 0U;
    if (res == 0U) new_ccr |= 0x04U;
    if (rm != 0U)  new_ccr |= 0x08U;
    if ((!sm && !dm && rm) || (sm && dm && !rm)) new_ccr |= 0x02U;
    if ((sm && dm) || (!rm && dm) || (sm && !rm)) new_ccr |= 0x11U;
    step_623ac::set_ccr(api, new_ccr);
}

inline void update_ccr_subq_b(const M68k9D70StepApi& api, std::uint8_t src, std::uint8_t dst, std::uint8_t res) noexcept {
    const std::uint8_t sm = (src >> 7U) & 1U;
    const std::uint8_t dm = (dst >> 7U) & 1U;
    const std::uint8_t rm = (res >> 7U) & 1U;
    std::uint16_t new_ccr = 0U;
    if (res == 0U) new_ccr |= 0x04U;
    if (rm != 0U)  new_ccr |= 0x08U;
    if ((!sm && dm && !rm) || (sm && !dm && rm)) new_ccr |= 0x02U;
    if ((sm && !dm) || (rm && !dm) || (sm && rm)) new_ccr |= 0x11U;
    step_623ac::set_ccr(api, new_ccr);
}

inline void do_bset_b(const M68k9D70StepApi& api, unsigned addr, unsigned bit) noexcept {
    unsigned byte_val = 0U;
    if (step_623ac::read_be(api, addr, 1, byte_val)) {
        const bool bit_zero = (byte_val & (1U << bit)) == 0U;
        const auto old_ccr = step_623ac::ccr(api);
        step_623ac::set_ccr(api, (old_ccr & ~0x04U) | (bit_zero ? 0x04U : 0U));
        api.write(api.context, addr, 1, byte_val | (1U << bit));
    }
}

inline void do_bclr_b(const M68k9D70StepApi& api, unsigned addr, unsigned bit) noexcept {
    unsigned byte_val = 0U;
    if (step_623ac::read_be(api, addr, 1, byte_val)) {
        const bool bit_zero = (byte_val & (1U << bit)) == 0U;
        const auto old_ccr = step_623ac::ccr(api);
        step_623ac::set_ccr(api, (old_ccr & ~0x04U) | (bit_zero ? 0x04U : 0U));
        api.write(api.context, addr, 1, byte_val & ~(1U << bit));
    }
}

inline M68k9D70StepResult dispatch_child_call(
    const M68k9D70StepApi& api, unsigned child_target_pc,
    unsigned return_pc, unsigned& resume_pc, bool& child_call_active,
    unsigned& saved_child_sp, std::uint16_t opcode) noexcept {
    const unsigned old_sp = api.read_register(api.context, 15U) & kMask24;
    const unsigned sp = (old_sp - 4U) & kMask24;
    if (!step_623ac::in_work_ram(sp, 4U)) return M68k9D70StepResult::ContractFault;
    api.write(api.context, sp, 4U, return_pc);
    api.write_register(api.context, 15U, sp);
    unsigned target_word{};
    if (!step_623ac::read_be(api, child_target_pc, 2U, target_word)) return M68k9D70StepResult::ContractFault;
    api.set_return_state(api.context, child_target_pc, child_target_pc + 2U, target_word);
    api.finish_instruction(api.context, opcode);
    child_call_active = true;
    saved_child_sp = old_sp;
    resume_pc = return_pc;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_target10(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept;

M68k9D70StepResult step_target11(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    M68k02575CStepMetrics& metrics, bool& active) noexcept;

M68k9D70StepResult step_state1(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept;

M68k9D70StepResult step_state2_3_4(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept;

M68k9D70StepResult step_state5(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept;

M68k9D70StepResult step_state6(
    const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc,
    bool& child_call_active, unsigned& saved_child_sp,
    M68k02575CStepMetrics& metrics, bool& active) noexcept;

} // namespace oasis::hybrid::step_02575c
