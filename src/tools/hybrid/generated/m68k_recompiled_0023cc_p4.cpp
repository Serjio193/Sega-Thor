#include "tools/hybrid/generated/m68k_recompiled_0023cc_p4.hpp"

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

unsigned m68k_recompiled_0023cc_p4(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x0024CCU: {
        // 0x0024CC: MOVE.w [3490]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA0), 2));
            api.write(api.context, api.read_register(api.context, kRegA2), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024CE: SWAP.w [4842]
        {
            const auto v = api.read_register(api.context, kRegD2);
            const auto swapped = (v >> 16U) | (v << 16U);
            api.write_register(api.context, kRegD2, swapped);
            update_logic_ccr16(api, static_cast<std::uint16_t>(swapped & 0xFFFFU));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x0024D0: ADDA.w [D0C2]
        {
            const auto old_a = api.read_register(api.context, kRegA0);
            const std::int32_t diff = static_cast<std::int16_t>((api.read_register(api.context, kRegD2) & 0xFFFFU));
            api.write_register(api.context, kRegA0, old_a + diff);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024D2: SWAP.w [4842]
        {
            const auto v = api.read_register(api.context, kRegD2);
            const auto swapped = (v >> 16U) | (v << 16U);
            api.write_register(api.context, kRegD2, swapped);
            update_logic_ccr16(api, static_cast<std::uint16_t>(swapped & 0xFFFFU));
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x0024D4: ADDQ.w [5242]
        {
            const auto old_d = api.read_register(api.context, kRegD2);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1U) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD2, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x0024D6: CMP.w [B443]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD2) & 0xFFFFU;
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
        // 0x0024D8: BLT.w [6D000012]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x0024ECU;
        return 0x0024DCU;
    }
    case 0x0024DCU: {
        // 0x0024DC: MOVE.w [302B0024]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x24U, 2));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024E0: MOVE.w [362B0022]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA3) + 0x22U, 2));
            const auto old = api.read_register(api.context, kRegD3);
            api.write_register(api.context, kRegD3, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024E4: LSL.w [E16B]
        {
            const auto shift = api.read_register(api.context, kRegD0) & 0x3FU;
            const auto old_d = api.read_register(api.context, kRegD3);
            const std::uint32_t res = ((old_d & 0xFFFFU) << (shift % 64U)) & 0xFFFFU;
            api.write_register(api.context, kRegD3, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, static_cast<std::uint16_t>(res));
            if (shift != 0) api.add_cycles(api.context, static_cast<int>(shift) * 14);
            api.skip_bus_refresh(api.context);
        }
        api.add_cycles(api.context, 42);
        // 0x0024E6: SUBA.w [90C3]
        {
            const auto old_a = api.read_register(api.context, kRegA0);
            const std::int32_t diff = static_cast<std::int16_t>((api.read_register(api.context, kRegD3) & 0xFFFFU));
            api.write_register(api.context, kRegA0, old_a - diff);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024E8: MOVE.w [363C03E8]
        {
            const auto val = static_cast<std::uint16_t>(0x3E8U);
            const auto old = api.read_register(api.context, kRegD3);
            api.write_register(api.context, kRegD3, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x0024ECU;
    }
    case 0x0024ECU: {
        // 0x0024EC: ADDI.w [06410080]
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
        // 0x0024F0: ADDQ.w [5244]
        {
            const auto old_d = api.read_register(api.context, kRegD4);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1U) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD4, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x0024F2: CMPI.w [0C440020]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD4) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x20U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024F6: BLT.w [6D000010]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x002508U;
        return 0x0024FAU;
    }
    case 0x0024FAU: {
        // 0x0024FA: SUBI.w [04440020]
        {
            const auto old_d = api.read_register(api.context, kRegD4);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x20U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD4, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0024FE: SUBI.w [04411000]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x1000U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002502: ADD.w [DC41]
        {
            const auto old_d = api.read_register(api.context, kRegD6);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD1) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a + b) & 0xFFFFU;
            api.write_register(api.context, kRegD6, (old_d & 0xFFFF0000U) | res);
            const bool c = (a + b) > 0xFFFFU;
            const bool v = (~(a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x002504: MOVE.w [3886]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD6) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA4), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x002506: MOVE.w [3885]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD5) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA4), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x002508U;
    }
    case 0x002508U: {
        // 0x002508: DBRA.w [51CFFFC2]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const auto count = static_cast<std::int16_t>(old_d & 0xFFFFU) - 1;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
            api.add_cycles(api.context, 10 * 7);
            if (count != -1) return 0x0024CCU;
            return 0x00250CU;
        }
    }
    default:
        return 0U;
    }
}
} // namespace oasis::hybrid::generated
