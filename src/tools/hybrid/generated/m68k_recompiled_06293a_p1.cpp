#include "tools/hybrid/generated/m68k_recompiled_06293a_p1.hpp"

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

unsigned m68k_recompiled_06293a_p1(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x062964U: {
        // 0x062964: SUBQ.w [5300]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD0, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062966: BCC.w [64000006]
        if ((get_ccr(api) & 0x01U) == 0) return 0x06296EU;
        return 0x06296AU;
    }
    case 0x06296AU: {
        // 0x06296A: MOVEQ.l [7000]
        {
            const auto val = static_cast<std::uint32_t>(0x00000000U);
            api.write_register(api.context, kRegD0, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x06296C: BRA.w [600A]
        return 0x062978U;
    }
    case 0x06296EU: {
        // 0x06296E: CMPI.b [0C00000F]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFU) & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0xFU) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062972: BLS.w [63000004]
        if ((get_ccr(api) & 0x05U) != 0) return 0x062978U;
        return 0x062976U;
    }
    case 0x062976U: {
        // 0x062976: MOVEQ.l [700F]
        {
            const auto val = static_cast<std::uint32_t>(0x0000000FU);
            api.write_register(api.context, kRegD0, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        return 0x062978U;
    }
    case 0x062978U: {
        // 0x062978: MOVE.b [1D400012]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD0) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x12U, 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06297C: EORI.b [0A00000F]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) ^ 0xFU);
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062980: MOVE.b [1D400011]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD0) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x11U, 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062984: RTS. [4E75]
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
