#include "tools/hybrid/generated/m68k_recompiled_002512_p1.hpp"

namespace oasis::hybrid::generated {

namespace {
constexpr unsigned kRegD0 = 0U;
constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD2 = 2U;
constexpr unsigned kRegD3 = 3U;
constexpr unsigned kRegD4 = 4U;
constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegD6 = 6U;
constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA0 = 8U;
constexpr unsigned kRegA1 = 9U;
constexpr unsigned kRegA2 = 10U;
constexpr unsigned kRegA3 = 11U;
constexpr unsigned kRegA4 = 12U;
constexpr unsigned kRegA5 = 13U;
constexpr unsigned kRegA6 = 14U;
constexpr unsigned kRegA7 = 15U;
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

unsigned m68k_recompiled_002512_p1(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x002564U: {
        // 0x002564: MOVE.w [3C2B0024]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x24U, 2));
            const auto old = api.read_register(api.context, kRegD6);
            api.write_register(api.context, kRegD6, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002568: LSL.w [ED6F]
        {
            const auto shift = api.read_register(api.context, kRegD6) & 0x3FU;
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t res = ((old_d & 0xFFFFU) << (shift % 64U)) & 0xFFFFU;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, static_cast<std::uint16_t>(res));
            if (shift != 0) api.add_cycles(api.context, static_cast<int>(shift) * 14);
            api.skip_bus_refresh(api.context);
        }
        api.add_cycles(api.context, 42);
        // 0x00256A: ADD.w [DE40]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD0) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00256C: ADDA.w [D0C7]
        {
            const auto old_a = api.read_register(api.context, kRegA0);
            const std::int32_t diff = static_cast<std::int16_t>((api.read_register(api.context, kRegD7) & 0xFFFFU));
            api.write_register(api.context, kRegA0, old_a + diff);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00256E: MOVE.w [3801]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD1) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD4);
            api.write_register(api.context, kRegD4, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002570: ANDI.w [02410F80]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0xF80U);
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002574: ADDI.w [06410080]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x80U) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002578: ANDI.w [0244007E]
        {
            const auto old_d = api.read_register(api.context, kRegD4);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0x7EU);
            api.write_register(api.context, kRegD4, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00257C: ANDI.w [02410F80]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0xF80U);
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002580: MOVE.w [3E01]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD1) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD7);
            api.write_register(api.context, kRegD7, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002582: ADD.w [DE44]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD4) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x002584: MOVE.w [3747000C]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD7) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA3) + 0xCU, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002588: ADDI.w [06410E00]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0xE00U) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00258C: ANDI.w [02410F80]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0xF80U);
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002590: ADD.w [D244]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD4) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x002592: BRA.w [6000004E]
        return 0x0025E2U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
