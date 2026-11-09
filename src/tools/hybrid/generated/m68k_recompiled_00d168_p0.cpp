#include "tools/hybrid/generated/m68k_recompiled_00d168_p0.hpp"

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

unsigned m68k_recompiled_00d168_p0(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x00D168U: {
        // 0x00D168: BTST.b [082800000027]
        {
            const auto addr = api.read_register(api.context, kRegA0) + 0x27U;
            const auto byte_val = api.read(api.context, addr, 1);
            const bool z = ((byte_val >> (0U)) & 1U) == 0;
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x00D16E: BEQ.w [670000F4]
        if ((get_ccr(api) & 0x04U) != 0) return 0x00D264U;
        return 0x00D172U;
    }
    case 0x00D172U: {
        // 0x00D172: ADD.w [D2680000]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(api.read(api.context, api.read_register(api.context, kRegA0) + 0x0U, 2)) & 0xFFFFU;
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
        // 0x00D176: ADD.w [D4680004]
        {
            const auto old_d = api.read_register(api.context, kRegD2);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(api.read(api.context, api.read_register(api.context, kRegA0) + 0x4U, 2)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD2, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D17A: BRA.w [6000000C]
        return 0x00D188U;
    }
    case 0x00D188U: {
        // 0x00D188: CLR.w [4240]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, old_d & 0xFFFF0000U);
            update_logic_ccr16(api, 0U);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00D18A: MOVE.w [31400012]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD0) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA0) + 0x12U, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D18E: MOVE.w [31400014]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD0) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA0) + 0x14U, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D192: MOVE.w [36280000]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA0) + 0x0U, 2));
            const auto old = api.read_register(api.context, kRegD3);
            api.write_register(api.context, kRegD3, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D196: CMP.w [B243]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD1) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD3) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00D198: BEQ.w [67000060]
        if ((get_ccr(api) & 0x04U) != 0) return 0x00D1FAU;
        return 0x00D19CU;
    }
    case 0x00D19CU: {
        // 0x00D19C: MOVE.w [31410000]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD1) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA0) + 0x0U, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1A0: MOVE.w [3A01]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD1) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD5);
            api.write_register(api.context, kRegD5, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1A2: SUB.w [9A43]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD3) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00D1A4: BTST.b [082800010027]
        {
            const auto addr = api.read_register(api.context, kRegA0) + 0x27U;
            const auto byte_val = api.read(api.context, addr, 1);
            const bool z = ((byte_val >> (1U)) & 1U) == 0;
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x00D1AA: BNE.w [66000012]
        if ((get_ccr(api) & 0x04U) == 0) return 0x00D1BEU;
        return 0x00D1AEU;
    }
    case 0x00D1AEU: {
        // 0x00D1AE: ASR.w [E643]
        {
            const auto old_d = api.read_register(api.context, kRegD3);
            const std::uint32_t val = old_d & 0xFFFFU;
            const std::int16_t sval = static_cast<std::int16_t>(val);
            const std::int16_t sres = sval >> 3U;
            const std::uint32_t res = static_cast<std::uint16_t>(sres);
            const bool c = ((sval >> (3U - 1U)) & 1U) != 0;
            api.write_register(api.context, kRegD3, (old_d & 0xFFFF0000U) | res);
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            set_ccr(api, (c ? 0x10U : 0x00U) | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 84);
        // 0x00D1B0: ASR.w [E641]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t val = old_d & 0xFFFFU;
            const std::int16_t sval = static_cast<std::int16_t>(val);
            const std::int16_t sres = sval >> 3U;
            const std::uint32_t res = static_cast<std::uint16_t>(sres);
            const bool c = ((sval >> (3U - 1U)) & 1U) != 0;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            set_ccr(api, (c ? 0x10U : 0x00U) | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 84);
        // 0x00D1B2: SUB.w [9243]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD3) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00D1B4: BEQ.w [6708]
        if ((get_ccr(api) & 0x04U) != 0) return 0x00D1BEU;
        return 0x00D1B6U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
