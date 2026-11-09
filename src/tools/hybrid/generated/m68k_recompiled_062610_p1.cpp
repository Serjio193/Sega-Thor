#include "tools/hybrid/generated/m68k_recompiled_062610_p1.hpp"

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

unsigned m68k_recompiled_062610_p1(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x062650U: {
        // 0x062650: MOVEQ.l [7001]
        {
            const auto val = static_cast<std::uint32_t>(0x00000001U);
            api.write_register(api.context, kRegD0, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062652: BRA.w [600C]
        return 0x062660U;
    }
    case 0x062654U: {
        // 0x062654: CMPI.w [0C4003FF]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFFFU) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x3FFU) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062658: BLE.w [6F000006]
        if ((get_ccr(api) & 0x04U) != 0 || (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0))) return 0x062660U;
        return 0x06265CU;
    }
    case 0x06265CU: {
        // 0x06265C: MOVE.w [303C03FF]
        {
            const auto val = static_cast<std::uint16_t>(0x3FFU);
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x062660U;
    }
    case 0x062660U: {
        // 0x062660: CMP.w [B06E0008]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFFFU) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x8U, 2)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062664: BEQ.w [6700002C]
        if ((get_ccr(api) & 0x04U) != 0) return 0x062692U;
        return 0x062668U;
    }
    case 0x062668U: {
        // 0x062668: MOVE.w [3D400008]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD0) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x8U, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06266C: CMPI.b [0C0700E0]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD7) & 0xFFU) & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0xE0U) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062670: BEQ.w [67000022]
        if ((get_ccr(api) & 0x04U) != 0) return 0x062694U;
        return 0x062674U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
