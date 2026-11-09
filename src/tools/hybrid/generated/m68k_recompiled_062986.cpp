#include "tools/hybrid/generated/m68k_recompiled_062986.hpp"


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

unsigned m68k_recompiled_062986_impl(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x062986U: {
        // 0x062986: MOVEQ.l [7000]
        {
            const auto val = static_cast<std::uint32_t>(0x00000000U);
            api.write_register(api.context, kRegD0, val);
            const std::uint16_t n = (val & 0x80000000U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x062988: MOVE.b [1018]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA0), 1));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.write_register(api.context, kRegA0, api.read_register(api.context, kRegA0) + 1U);
        api.add_cycles(api.context, 8 * 7);
        // 0x06298A: ADD.b [D12E0012]
        {
            const auto addr = api.read_register(api.context, kRegA6) + 0x12U;
            const std::uint32_t a = api.read(api.context, addr, 1);
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD0) & 0xFFU)) & 0xFFU;
            const std::uint32_t res = (a + b) & 0xFFU;
            api.write(api.context, addr, 1, res);
            const bool c = (a + b) > 0xFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06298E: CMPI.b [0C2E000F0012]
        {
            const std::uint32_t a = api.read(api.context, api.read_register(api.context, kRegA6) + 0x12U, 1) & 0xFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0xFU) & 0xFFU;
            const std::uint32_t res = (a - b) & 0xFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x80U) != 0;
            const bool n = (res & 0x80U) != 0; const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 16 * 7);
        // 0x062994: BLE.w [6F00000A]
        if ((get_ccr(api) & 0x04U) != 0 || (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0))) return 0x0629A0U;
        return 0x062998U;
    }
    case 0x062998U: {
        // 0x062998: MOVE.b [1D7C000F0012]
        {
            const auto val = static_cast<std::uint8_t>(0xFU);
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x12U, 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x06299E: BRA.w [600C]
        return 0x0629ACU;
    }
    case 0x0629A0U: {
        // 0x0629A0: TST.b [4A2E0012]
        {
            const auto val = api.read(api.context, api.read_register(api.context, kRegA6) + 0x12U, 1);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U;
            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0629A4: BGE.w [6C000006]
        if (((get_ccr(api) & 0x08U) != 0) == ((get_ccr(api) & 0x02U) != 0)) return 0x0629ACU;
        return 0x0629A8U;
    }
    case 0x0629A8U: {
        // 0x0629A8: SF.b [51EE0012]
        api.write(api.context, api.read_register(api.context, kRegA6) + 0x12U, 1, 0x00U);
        api.add_cycles(api.context, 8 * 7);
        return 0x0629ACU;
    }
    case 0x0629ACU: {
        // 0x0629AC: MOVE.b [102E0012]
        {
            const auto val = static_cast<std::uint8_t>(api.read(api.context, api.read_register(api.context, kRegA6) + 0x12U, 1));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFFFF00U) | val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0629B0: EORI.b [0A00000F]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const auto res = static_cast<std::uint8_t>((old_d & 0xFFU) ^ 0xFU);
            api.write_register(api.context, kRegD0, (old_d & 0xFFFFFF00U) | res);
            const std::uint16_t n = (res & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (res == 0U) ? 0x04U : 0x00U;
            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0629B4: MOVE.b [1D400011]
        {
            const auto val = static_cast<std::uint8_t>((api.read_register(api.context, kRegD0) & 0xFFU));
            api.write(api.context, api.read_register(api.context, kRegA6) + 0x11U, 1, val);
            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0629B8: RTS. [4E75]
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
M68k9D70StepResult m68k_recompiled_062986(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {
    unsigned block_pc = 0x062986U;
    unsigned step_count = 0;
    while (block_pc != 0U && step_count < 2000U) {
        ++step_count;
        block_pc = m68k_recompiled_062986_impl(api, block_pc, resume_pc);
    }
    return M68k9D70StepResult::InstructionStepped;
}
} // namespace oasis::hybrid::generated
