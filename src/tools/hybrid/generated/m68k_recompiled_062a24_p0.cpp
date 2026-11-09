#include "tools/hybrid/generated/m68k_recompiled_062a24_p0.hpp"

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

unsigned m68k_recompiled_062a24_p0(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x062A24U: {
        // 0x062A24: MOVE.b [1018]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA0), 1));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.write_register(api.context, kRegA0, api.read_register(api.context, kRegA0) + 1U);
        api.add_cycles(api.context, 8 * 7);
        // 0x062A26: TST.b [4A2E0016]
        {
            const auto val = api.read(api.context, api.read_register(api.context, kRegA6) + 0x16U, 1);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A2A: BEQ.w [6700004A]
        if ((get_ccr(api) & 0x04U) != 0) return 0x062A76U;
        return 0x062A2EU;
    }
    case 0x062A2EU: {
        // 0x062A2E: CMPI.b [0C000020]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFU) & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x20U) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A32: BNE.w [66000006]
        if ((get_ccr(api) & 0x04U) == 0) return 0x062A3AU;
        return 0x062A36U;
    }
    case 0x062A36U: {
        // 0x062A36: MOVEQ.l [7203]
        {
            const auto val = static_cast<std::uint32_t>(0x00000003U);
            api.write_register(api.context, kRegD1, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062A38: BRA.w [6018]
        return 0x062A52U;
    }
    case 0x062A3AU: {
        // 0x062A3A: MOVEQ.l [7200]
        {
            const auto val = static_cast<std::uint32_t>(0x00000000U);
            api.write_register(api.context, kRegD1, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062A3C: CMPI.b [0C00000B]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFU) & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0xBU) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A40: BCS.w [65000010]
        if ((get_ccr(api) & 0x01U) != 0) return 0x062A52U;
        return 0x062A44U;
    }
    case 0x062A44U: {
        // 0x062A44: CMPI.b [0C000016]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFU) & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x16U) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A48: BCC.w [64000006]
        if ((get_ccr(api) & 0x01U) == 0) return 0x062A50U;
        return 0x062A4CU;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
