#include "tools/hybrid/generated/m68k_recompiled_0626a6.hpp"


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

unsigned m68k_recompiled_0626a6_impl(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x0626A6U: {
        // 0x0626A6: MOVE.w [3200]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD0) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD1);
            api.write_register(api.context, kRegD1, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0626A8: ANDI.b [0200000F]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) & 0xFU);
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0626AC: OR.b [802D0004]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) | api.read(api.context, api.read_register(api.context, kRegA5) + 0x4U, 1));
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0626B0: MOVE.b [1680]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD0) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA3), 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0626B2: LSR.w [E849]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t val = old_d & 0xFFFFU;
            const std::uint32_t res = (val >> 4U) & 0xFFFFU;
            const bool c = ((val >> (4U - 1U)) & 1U) != 0;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x10U : 0x00U) | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 98);
        // 0x0626B4: ANDI.b [0201003F]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) & 0x3FU);
            api.write_register(api.context, kRegD1, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0626B8: MOVE.b [1681]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD1) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA3), 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0626BA: RTS. [4E75]
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
M68k9D70StepResult m68k_recompiled_0626a6(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {
    unsigned block_pc = 0x0626A6U;
    unsigned step_count = 0;
    while (block_pc != 0U && step_count < 2000U) {
        ++step_count;
        block_pc = m68k_recompiled_0626a6_impl(api, block_pc, resume_pc);
    }
    return M68k9D70StepResult::InstructionStepped;
}
} // namespace oasis::hybrid::generated
