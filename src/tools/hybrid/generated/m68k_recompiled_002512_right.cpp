#include "tools/hybrid/generated/m68k_recompiled_002512_right.hpp"

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

unsigned m68k_recompiled_002512_right(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x002512U: {
        // 0x002512: MOVEA.l [206B0016]
        api.write_register(api.context, kRegA0, api.read(api.context, api.read_register(api.context, kRegA3) + 0x16U, 4));
        api.add_cycles(api.context, 12 * 7);
        // 0x002516: MOVE.w [3A2B001A]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x1AU, 2));
            const auto old = api.read_register(api.context, kRegD5);
            api.write_register(api.context, kRegD5, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00251A: LEA.l [45F900C00000]
        api.write_register(api.context, kRegA2, 0x00C00000U);
        api.add_cycles(api.context, 12 * 7);
        // 0x002520: LEA.l [49EA0004]
        api.write_register(api.context, kRegA4, api.read_register(api.context, kRegA2) + 0x4U);
        api.add_cycles(api.context, 8 * 7);
        // 0x002524: MOVE.w [3C2B001E]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x1EU, 2));
            const auto old = api.read_register(api.context, kRegD6);
            api.write_register(api.context, kRegD6, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002528: BMI.w [6B00006C]
        if ((get_ccr(api) & 0x08U) != 0) return 0x002596U;
        return 0x00252CU;
    }
    case 0x00252CU: {
        // 0x00252C: MOVE.w [322B000C]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0xCU, 2));
            const auto old = api.read_register(api.context, kRegD1);
            api.write_register(api.context, kRegD1, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002530: SUBQ.w [5346]
        {
            const auto old_d = api.read_register(api.context, kRegD6);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD6, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x002532: MOVE.w [3746001E]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD6) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA3) + 0x1EU, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002536: MOVE.w [362B0020]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x20U, 2));
            const auto old = api.read_register(api.context, kRegD3);
            api.write_register(api.context, kRegD3, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00253A: MOVE.w [382B0022]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x22U, 2));
            const auto old = api.read_register(api.context, kRegD4);
            api.write_register(api.context, kRegD4, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00253E: MOVE.w [342B0008]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x8U, 2));
            const auto old = api.read_register(api.context, kRegD2);
            api.write_register(api.context, kRegD2, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002542: MOVE.w [3002]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD2) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002544: ADD.w [D040]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD0) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD0, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x002546: MOVE.w [3E2B000A]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0xAU, 2));
            const auto old = api.read_register(api.context, kRegD7);
            api.write_register(api.context, kRegD7, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00254A: ADDQ.w [5247]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1U) & 0xFFFFU;
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
        // 0x00254C: CMP.w [BE44]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD7) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD4) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00254E: BLT.w [6D000004]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x002554U;
        return 0x002552U;
    }
    case 0x002552U: {
        // 0x002552: SUB.w [9E44]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD4) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        return 0x002554U;
    }
    case 0x002554U: {
        // 0x002554: MOVE.w [3747000A]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD7) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA3) + 0xAU, 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002558: ADDI.w [0647001C]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1CU) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00255C: CMP.w [BE44]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD7) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD4) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00255E: BLT.w [6D000004]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x002564U;
        return 0x002562U;
    }
    case 0x002562U: {
        // 0x002562: SUB.w [9E44]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD4) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        return 0x002564U;
    }
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
