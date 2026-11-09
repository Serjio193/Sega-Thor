#include "tools/hybrid/generated/m68k_recompiled_062a24_p1.hpp"

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

unsigned m68k_recompiled_062a24_p1(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x062A4CU: {
        // 0x062A4C: MOVEQ.l [7201]
        {
            const auto val = static_cast<std::uint32_t>(0x00000001U);
            api.write_register(api.context, kRegD1, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062A4E: BRA.w [6002]
        return 0x062A52U;
    }
    case 0x062A50U: {
        // 0x062A50: MOVEQ.l [7202]
        {
            const auto val = static_cast<std::uint32_t>(0x00000002U);
            api.write_register(api.context, kRegD1, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        return 0x062A52U;
    }
    case 0x062A52U: {
        // 0x062A52: MOVE.b [102E0017]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x17U, 1));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A56: ANDI.b [02000064]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) & 0x64U);
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A5A: OR.b [8200]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) | (api.read_register(api.context, kRegD0) & 0xFFU));
            api.write_register(api.context, kRegD1, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062A5C: MOVE.b [1D410017]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD1) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x17U, 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A60: MOVE.b [102E0016]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x16U, 1));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A64: OR.b [802E0017]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) | api.read(api.context, api.read_register(api.context, kRegA6) + 0x17U, 1));
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062A68: BTST.b [082E00060001]
        {
            const auto addr = api.read_register(api.context, kRegA6) + 0x1U;
            const auto byte_val = api.read(api.context, addr, 1);
            const bool z = ((byte_val >> (6U)) & 1U) == 0;
            
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x062A6E: BNE.w [6602]
        if ((get_ccr(api) & 0x04U) == 0) return 0x062A72U;
        return 0x062A70U;
    }
    case 0x062A70U: {
        // 0x062A70: MOVE.b [1680]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD0) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA3), 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x062A72U;
    }
    case 0x062A72U: {
        // 0x062A72: MOVE.b [1E2E0016]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x16U, 1));
            const auto old = api.read_register(api.context, kRegD7);
            api.write_register(api.context, kRegD7, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x062A76U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
