#include "tools/hybrid/generated/m68k_recompiled_06293a_p0.hpp"

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

unsigned m68k_recompiled_06293a_p0(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x06293AU: {
        // 0x06293A: MOVE.b [1018]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA0), 1));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.write_register(api.context, kRegA0, api.read_register(api.context, kRegA0) + 1U);
        api.add_cycles(api.context, 8 * 7);
        // 0x06293C: BTST.l [0807001F]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const bool z = ((old_d >> (31U)) & 1U) == 0;
            
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x062940: BEQ.w [67000022]
        if ((get_ccr(api) & 0x04U) != 0) return 0x062964U;
        return 0x062944U;
    }
    case 0x062944U: {
        // 0x062944: MOVE.b [122E0074]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x74U, 1));
            const auto old = api.read_register(api.context, kRegD1);
            api.write_register(api.context, kRegD1, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062948: BTST.l [08010007]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const bool z = ((old_d >> (7U)) & 1U) == 0;
            
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x06294C: BNE.w [66000006]
        if ((get_ccr(api) & 0x04U) == 0) return 0x062954U;
        return 0x062950U;
    }
    case 0x062950U: {
        // 0x062950: ADD.b [D001]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD1) & 0xFFU)) & 0xFFU;
            const std::uint32_t res = (a + b) & 0xFFU;
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const bool c = (a + b) > 0xFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062952: BRA.w [601A]
        return 0x06296EU;
    }
    case 0x062954U: {
        // 0x062954: NOT.b [4601]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint8_t>(~(old_d & 0xFFU));
            api.write_register(api.context, kRegD1, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U;
            const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062956: ANDI.b [0201000F]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) & 0xFU);
            api.write_register(api.context, kRegD1, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06295A: SUB.b [9001]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD1) & 0xFFU)) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x06295C: BCC.w [64000010]
        if ((get_ccr(api) & 0x01U) == 0) return 0x06296EU;
        return 0x062960U;
    }
    case 0x062960U: {
        // 0x062960: MOVEQ.l [7000]
        {
            const auto val = static_cast<std::uint32_t>(0x00000000U);
            api.write_register(api.context, kRegD0, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062962: BRA.w [6014]
        return 0x062978U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
