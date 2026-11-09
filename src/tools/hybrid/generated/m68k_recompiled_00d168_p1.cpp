#include "tools/hybrid/generated/m68k_recompiled_00d168_p1.hpp"

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

unsigned m68k_recompiled_00d168_p1(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x00D1B6U: {
        // 0x00D1B6: MOVE.w [3141001C]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD1) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA0) + 0x1CU, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1BA: BSET.l [08C00000]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const bool z = ((old_d >> (0U)) & 1U) == 0;
            api.write_register(api.context, kRegD0, old_d | (1U << 0U));
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        return 0x00D1BEU;
    }
    case 0x00D1BEU: {
        // 0x00D1BE: MOVE.w [31450012]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD5) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA0) + 0x12U, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1C2: NEG.w [4445]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t res = (0U - a) & 0xFFFFU;
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            const bool c = (a != 0U);
            const bool v = (a == 0x8000U);
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00D1C4: ADD.w [DA68000E]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(api.read(api.context, api.read_register(api.context, kRegA0) + 0xEU, 2)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1C8: ANDI.w [024501FF]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0x1FFU);
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1CC: MOVE.w [3145000E]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD5) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA0) + 0xEU, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1D0: BSET.l [08C00002]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const bool z = ((old_d >> (2U)) & 1U) == 0;
            api.write_register(api.context, kRegD0, old_d | (1U << 2U));
            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));
        }
        api.add_cycles(api.context, 12 * 7);
        // 0x00D1D4: MOVE.w [3C280020]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA0) + 0x20U, 2));
            const auto old = api.read_register(api.context, kRegD6);
            api.write_register(api.context, kRegD6, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1D8: LSL.w [E74E]
        {
            const auto old_d = api.read_register(api.context, kRegD6);
            const std::uint32_t val = old_d & 0xFFFFU;
            const std::uint32_t res = (val << 3U) & 0xFFFFU;
            const bool c = ((val << (3U - 1U)) & 0x8000U) != 0;
            api.write_register(api.context, kRegD6, (old_d & 0xFFFF0000U) | res);
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            set_ccr(api, (c ? 0x10U : 0x00U) | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 84);
        // 0x00D1DA: MOVE.w [3A280000]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA0) + 0x0U, 2));
            const auto old = api.read_register(api.context, kRegD5);
            api.write_register(api.context, kRegD5, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1DE: CMPI.w [0C45D8F0]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD5) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0xD8F0U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1E2: BGT.w [6E000006]
        if ((get_ccr(api) & 0x04U) == 0 && (((get_ccr(api) & 0x08U) != 0) == ((get_ccr(api) & 0x02U) != 0))) return 0x00D1EAU;
        return 0x00D1E6U;
    }
    case 0x00D1E6U: {
        // 0x00D1E6: ADD.w [DA46]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD6) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00D1E8: ADD.w [DA46]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD6) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        return 0x00D1EAU;
    }
    case 0x00D1EAU: {
        // 0x00D1EA: CMPI.w [0C452710]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD5) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x2710U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00D1EE: BLT.w [6D000006]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x00D1F6U;
        return 0x00D1F2U;
    }
    case 0x00D1F2U: {
        // 0x00D1F2: SUB.w [9A46]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD6) & 0xFFFFU)) & 0xFFFFU;
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
        // 0x00D1F4: SUB.w [9A46]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD6) & 0xFFFFU)) & 0xFFFFU;
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
        return 0x00D1F6U;
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
