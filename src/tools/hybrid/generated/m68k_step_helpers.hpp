#ifndef OASIS_HYBRID_GENERATED_M68K_STEP_HELPERS_HPP_
#define OASIS_HYBRID_GENERATED_M68K_STEP_HELPERS_HPP_

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/m68k_gameplay_step_flags.hpp"
#include <cstdint>

namespace oasis::hybrid::generated {

constexpr unsigned kMask24 = 0x00FFFFFFU;

constexpr unsigned kRegD0 = 0U; constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD2 = 2U; constexpr unsigned kRegD3 = 3U;
constexpr unsigned kRegD4 = 4U; constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegD6 = 6U; constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA0 = 8U; constexpr unsigned kRegA1 = 9U;
constexpr unsigned kRegA2 = 10U; constexpr unsigned kRegA3 = 11U;
constexpr unsigned kRegA4 = 12U; constexpr unsigned kRegA5 = 13U;
constexpr unsigned kRegA6 = 14U; constexpr unsigned kRegA7 = 15U;
constexpr unsigned kRegPC = 16U;
constexpr unsigned kRegSR = 17U;

inline bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& val) noexcept {
    val = 0;
    if (addr > 0xFFFFFFU || width <= 0 || width > 4 || addr + static_cast<unsigned>(width) > 0x1000000U) return false;
    for (int i = 0; i < width; ++i) {
        const int byte = api.peek(api.context, (addr + static_cast<unsigned>(i)) & kMask24);
        if (byte < 0 || byte > 255) return false;
        val = (val << 8U) | static_cast<unsigned>(byte);
    }
    return true;
}

inline bool branch_to(const M68k9D70StepApi& api, unsigned target, unsigned& resume) noexcept {
    unsigned word{};
    if (!read_be(api, target, 2, word)) return false;
    api.set_return_state(api.context, target, (target + 2U) & kMask24, word);
    resume = target;
    return true;
}

inline bool prepare_step(const M68k9D70StepApi& api, unsigned pc, std::uint16_t opcode) noexcept {
    api.observe_instruction(api.context, pc, opcode);
    api.begin_instruction(api.context, opcode);
    return true;
}

inline bool complete_step(const M68k9D70StepApi& api, unsigned next_pc, std::uint16_t opcode,
                          int cycle_adjustment, unsigned& resume_pc) noexcept {
    unsigned word{};
    if (!read_be(api, next_pc, 2, word)) return false;
    api.set_return_state(api.context, next_pc, (next_pc + 2U) & kMask24, word);
    if (cycle_adjustment != 0 && api.add_cycles) api.add_cycles(api.context, cycle_adjustment);
    api.finish_instruction(api.context, opcode);
    resume_pc = next_pc;
    return true;
}

inline M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    unsigned raw{};
    if (!read_be(api, pc, 2, raw)) return M68k9D70StepResult::ContractFault;
    opcode = static_cast<std::uint16_t>(raw);
    prepare_step(api, pc, opcode);
    const unsigned sp = api.read_register(api.context, kRegA7) & kMask24;
    unsigned return_pc{};
    if (!read_be(api, sp, 4, return_pc)) return M68k9D70StepResult::ContractFault;
    api.write_register(api.context, kRegA7, (sp + 4U) & kMask24);
    if (!branch_to(api, return_pc, resume_pc)) return M68k9D70StepResult::ContractFault;
    api.finish_instruction(api.context, opcode);
    return M68k9D70StepResult::InstructionStepped;
}

inline std::uint16_t get_ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, kRegSR) & 0x1FU);
}

inline void set_ccr(const M68k9D70StepApi& api, std::uint16_t ccr) noexcept {
    const auto sr = api.read_register(api.context, kRegSR);
    api.write_register(api.context, kRegSR, (sr & 0xFFE0U) | (ccr & 0x1FU));
}

inline void update_logic_ccr_b(const M68k9D70StepApi& api, std::uint8_t val) noexcept {
    set_ccr(api, step_flags::byte_logic_flags(val, get_ccr(api)));
}

inline void update_logic_ccr_w(const M68k9D70StepApi& api, std::uint16_t val) noexcept {
    set_ccr(api, step_flags::logic_flags(val, get_ccr(api)));
}

inline void update_logic_ccr_l(const M68k9D70StepApi& api, std::uint32_t val) noexcept {
    set_ccr(api, step_flags::long_logic_flags(val, get_ccr(api)));
}

inline void update_add_ccr_w(const M68k9D70StepApi& api, std::uint16_t d, std::uint16_t s, std::uint32_t r) noexcept {
    set_ccr(api, step_flags::add_flags(d, s, static_cast<std::uint16_t>(r), get_ccr(api), true));
}

inline void update_sub_ccr_w(const M68k9D70StepApi& api, std::uint16_t d, std::uint16_t s, std::uint32_t r) noexcept {
    set_ccr(api, step_flags::sub_flags(d, s, static_cast<std::uint16_t>(r), get_ccr(api), true));
}

inline void update_cmp_ccr_b(const M68k9D70StepApi& api, std::uint8_t d, std::uint8_t s, std::uint32_t r) noexcept {
    set_ccr(api, step_flags::byte_sub_flags(d, s, static_cast<std::uint8_t>(r), get_ccr(api), false));
}

inline void update_cmp_ccr_w(const M68k9D70StepApi& api, std::uint16_t d, std::uint16_t s, std::uint32_t r) noexcept {
    set_ccr(api, step_flags::sub_flags(d, s, static_cast<std::uint16_t>(r), get_ccr(api), false));
}

inline void update_btst_ccr(const M68k9D70StepApi& api, bool is_zero) noexcept {
    const auto old = get_ccr(api);
    set_ccr(api, is_zero ? static_cast<std::uint16_t>(old | 0x04U) : static_cast<std::uint16_t>(old & ~0x04U));
}

inline void update_shift_ccr_lsl_w(const M68k9D70StepApi& api, std::uint16_t old_d, unsigned shift, std::uint16_t val) noexcept {
    const std::uint8_t c_bit = static_cast<std::uint8_t>((old_d >> (16U - shift)) & 1U);
    std::uint8_t new_ccr = (val == 0U ? 0x04U : (val & 0x8000U ? 0x08U : 0x00U)) | c_bit | (c_bit << 4U);
    set_ccr(api, new_ccr);
}

inline void update_shift_ccr_lsr_w(const M68k9D70StepApi& api, std::uint16_t old_d, unsigned shift, std::uint16_t val) noexcept {
    const std::uint8_t c_bit = static_cast<std::uint8_t>((old_d >> (shift - 1U)) & 1U);
    std::uint8_t new_ccr = (val == 0U ? 0x04U : 0x00U) | c_bit | (c_bit << 4U);
    set_ccr(api, new_ccr);
}

inline void update_shift_ccr_asr_w(const M68k9D70StepApi& api, std::uint16_t old_d, unsigned shift, std::uint16_t val) noexcept {
    const std::uint8_t c_bit = static_cast<std::uint8_t>((old_d >> (shift - 1U)) & 1U);
    std::uint8_t new_ccr = (val == 0U ? 0x04U : (val & 0x8000U ? 0x08U : 0x00U)) | c_bit | (c_bit << 4U);
    set_ccr(api, new_ccr);
}

} // namespace oasis::hybrid::generated

#endif // OASIS_HYBRID_GENERATED_M68K_STEP_HELPERS_HPP_
