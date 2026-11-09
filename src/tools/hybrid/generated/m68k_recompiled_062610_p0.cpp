#include "tools/hybrid/generated/m68k_recompiled_062610_p0.hpp"

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

unsigned m68k_recompiled_062610_p0(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x062610U: {
        // 0x062610: MOVE.b [1D6D0003006D]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA5) + 0x3U, 1));
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x6DU, 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062616: BTST.b [082E00020000]
        {
            const auto addr = api.read_register(api.context, kRegA6) + 0x0U;
            const auto byte_val = api.read(api.context, addr, 1);
            const bool z = ((byte_val >> (2U)) & 1U) == 0;
            
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x06261C: BNE.w [66000006]
        if ((get_ccr(api) & 0x04U) == 0) return 0x062624U;
        return 0x062620U;
    }
    case 0x062620U: {
        // 0x062620: BSR.w [61000118]
        return 0x062624U;
    }
    case 0x062624U: {
        // 0x062624: BCLR.b [08AE00020075]
        {
            const auto addr = api.read_register(api.context, kRegA6) + 0x75U;
            const auto byte_val = api.read(api.context, addr, 1);
            const bool z = ((byte_val >> (2U)) & 1U) == 0;
            api.write(api.context, addr, 1, byte_val & ~(1U << 2U));
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x06262A: BEQ.w [67000008]
        if ((get_ccr(api) & 0x04U) != 0) return 0x062634U;
        return 0x06262EU;
    }
    case 0x06262EU: {
        // 0x06262E: MOVE.b [103C00FF]
        {
            const auto val = static_cast<std::uint8_t>(0xFFU);
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062632: MOVE.b [1680]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD0) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA3), 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x062634U;
    }
    case 0x062634U: {
        // 0x062634: MOVE.w [302E001C]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x1CU, 2));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062638: ADD.w [D06E0076]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x76U, 2)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD0, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06263C: MOVEQ.l [7200]
        {
            const auto val = static_cast<std::uint32_t>(0x00000000U);
            api.write_register(api.context, kRegD1, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x06263E: MOVE.b [122E0071]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x71U, 1));
            const auto old = api.read_register(api.context, kRegD1);
            api.write_register(api.context, kRegD1, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062642: ASR.w [E260]
        {
            const auto shift = api.read_register(api.context, kRegD1) & 0x3FU;
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::int16_t sval = static_cast<std::int16_t>(old_d & 0xFFFFU);
            const std::uint32_t res = static_cast<std::uint16_t>(sval >> (shift % 64U));
            api.write_register(api.context, kRegD0, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, static_cast<std::uint16_t>(res));
            if (shift != 0) api.add_cycles(api.context, static_cast<int>(shift) * 14);
            api.skip_bus_refresh(api.context);
        }
        api.add_cycles(api.context, 42);
        // 0x062644: ADD.w [D06E000A]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0xAU, 2)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD0, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x062648: CMPI.w [0C400001]
        {
            const std::uint32_t a = (api.read_register(api.context, kRegD0) & 0xFFFFU) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06264C: BGE.w [6C000006]
        if (((get_ccr(api) & 0x08U) != 0) == ((get_ccr(api) & 0x02U) != 0)) return 0x062654U;
        return 0x062650U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
