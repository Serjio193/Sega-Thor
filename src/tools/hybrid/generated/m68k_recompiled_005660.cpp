#include "tools/hybrid/generated/m68k_recompiled_005660.hpp"


namespace oasis::hybrid::generated {

namespace {
constexpr unsigned kRegD0 = 0U; constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD2 = 2U; constexpr unsigned kRegD3 = 3U;
constexpr unsigned kRegD4 = 4U; constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegD6 = 6U; constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA0 = 8U; constexpr unsigned kRegA1 = 9U;
constexpr unsigned kRegA2 = 10U; constexpr unsigned kRegA3 = 11U;
constexpr unsigned kRegA4 = 12U; constexpr unsigned kRegA5 = 13U;
constexpr unsigned kRegA6 = 14U; constexpr unsigned kRegA7 = 15U;
constexpr unsigned kRegSR = 16U;

inline std::uint16_t get_ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, kRegSR) & 0xFFU);
}
inline void set_ccr(const M68k9D70StepApi& api, std::uint16_t ccr) noexcept {
    const auto sr = api.read_register(api.context, kRegSR);
    api.write_register(api.context, kRegSR, (sr & 0xFF00U) | (ccr & 0xFFU));
}
inline void update_logic_ccr16(const M68k9D70StepApi& api, std::uint16_t val) noexcept {
    const std::uint16_t n = (val & 0x8000U) ? 0x08U : 0x00U;
    const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
    set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
}
} // namespace

unsigned m68k_recompiled_005660_impl(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x005660U: {
        // 0x005660: LEA.l [41F900FF1972]
        api.write_register(api.context, kRegA0, 0x00FF1972U);
        api.add_cycles(api.context, 12 * 7);
        // 0x005666: MOVE.w [30EE0008]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x8U, 2));
            const auto addr = api.read_register(api.context, kRegA0);
            api.write(api.context, addr, 2, val);
            api.write_register(api.context, kRegA0, addr + 2U);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00566A: MOVE.w [30AE000C]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0xCU, 2));
            api.write(api.context, api.read_register(api.context, kRegA0), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00566E: RTS. [4E75]
        {
            const auto sp = api.read_register(api.context, kRegA7);
            resume_pc = api.read(api.context, sp, 4);
            api.write_register(api.context, kRegA7, sp + 4U);
            api.add_cycles(api.context, 16 * 7);
            return 0U; // Terminal return
        }
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated


namespace oasis::hybrid::generated {
M68k9D70StepResult m68k_recompiled_005660(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {
    unsigned block_pc = 0x005660U;
    unsigned step_count = 0;
    while (block_pc != 0U && step_count < 2000U) {
        ++step_count;
        block_pc = m68k_recompiled_005660_impl(api, block_pc, resume_pc);
    }
    return M68k9D70StepResult::InstructionStepped;
}
} // namespace oasis::hybrid::generated
