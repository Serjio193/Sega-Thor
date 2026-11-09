#include "tools/hybrid/generated/m68k_recompiled_002512_upload.hpp"

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

unsigned m68k_recompiled_002512_upload(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {
    switch (block_pc) {
    case 0x0025E2U: {
        // 0x0025E2: LSR.w [E24C]
        {
            const auto old_d = api.read_register(api.context, kRegD4);
            const std::uint32_t val = old_d & 0xFFFFU;
            const std::uint32_t res = (val >> 1U) & 0xFFFFU;
            const bool c = ((val >> (1U - 1U)) & 1U) != 0;
            api.write_register(api.context, kRegD4, (old_d & 0xFFFF0000U) | res);
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            set_ccr(api, (c ? 0x10U : 0x00U) | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 56);
        // 0x0025E4: MOVE.w [3C05]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD5) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD6);
            api.write_register(api.context, kRegD6, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025E6: ROL.w [E55D]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const std::uint32_t val = old_d & 0xFFFFU;
            const std::uint32_t res = ((val << 2U) | (val >> (16U - 2U))) & 0xFFFFU;
            const bool c = ((val >> (16U - 2U)) & 1U) != 0;
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            set_ccr(api, (get_ccr(api) & 0x10U) | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 70);
        // 0x0025E8: ANDI.w [02450003]
        {
            const auto old_d = api.read_register(api.context, kRegD5);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0x3U);
            api.write_register(api.context, kRegD5, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025EC: ANDI.w [02463FFF]
        {
            const auto old_d = api.read_register(api.context, kRegD6);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) & 0x3FFFU);
            api.write_register(api.context, kRegD6, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025F0: ORI.w [00464000]
        {
            const auto old_d = api.read_register(api.context, kRegD6);
            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) | 0x4000U);
            api.write_register(api.context, kRegD6, (old_d & 0xFFFF0000U) | res);
            update_logic_ccr16(api, res);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025F4: MOVE.w [3006]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD6) & 0xFFFFU));
            const auto old = api.read_register(api.context, kRegD0);
            api.write_register(api.context, kRegD0, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025F6: ADD.w [D041]
        {
            const auto old_d = api.read_register(api.context, kRegD0);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD1) & 0xFFFFU)) & 0xFFFFU;
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
        // 0x0025F8: MOVE.w [3880]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD0) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA4), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025FA: MOVE.w [3885]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD5) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA4), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x0025FC: MOVE.w [3E3C0028]
        {
            const auto val = static_cast<std::uint16_t>(0x28U);
            const auto old = api.read_register(api.context, kRegD7);
            api.write_register(api.context, kRegD7, (old & 0xFFFF0000U) | val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x002600U;
    }
    case 0x002600U: {
        // 0x002600: MOVE.w [3498]
        {
            const auto val = static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA0), 2));
            api.write(api.context, api.read_register(api.context, kRegA2), 2, val);
            update_logic_ccr16(api, val);
        }
        api.write_register(api.context, kRegA0, api.read_register(api.context, kRegA0) + 2U);
        api.add_cycles(api.context, 8 * 7);
        // 0x002602: ADDQ.w [5242]
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
        // 0x002604: CMP.w [B443]
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
        // 0x002606: BLT.w [6D000008]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x002610U;
        return 0x00260AU;
    }
    case 0x00260AU: {
        // 0x00260A: SUB.w [9443]
        {
            const auto old_d = api.read_register(api.context, kRegD2);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>((api.read_register(api.context, kRegD3) & 0xFFFFU)) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            api.write_register(api.context, kRegD2, (old_d & 0xFFFF0000U) | res);
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);
            set_ccr(api, ccr);
        }
        api.add_cycles(api.context, 4 * 7);
        // 0x00260C: SUBA.w [90C3]
        {
            const auto old_a = api.read_register(api.context, kRegA0);
            const std::int32_t diff = static_cast<std::int16_t>((api.read_register(api.context, kRegD3) & 0xFFFFU));
            api.write_register(api.context, kRegA0, old_a - diff);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00260E: SUBA.w [90C3]
        {
            const auto old_a = api.read_register(api.context, kRegA0);
            const std::int32_t diff = static_cast<std::int16_t>((api.read_register(api.context, kRegD3) & 0xFFFFU));
            api.write_register(api.context, kRegA0, old_a - diff);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x002610U;
    }
    case 0x002610U: {
        // 0x002610: ADDI.w [06410002]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x2U) & 0xFFFFU;
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
        // 0x002614: ADDQ.w [5244]
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
        // 0x002616: CMPI.w [0C440040]
        {
            const std::uint32_t a = api.read_register(api.context, kRegD4) & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x40U) & 0xFFFFU;
            const std::uint32_t res = (a - b) & 0xFFFFU;
            const bool c = a < b;
            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;
            const bool n = (res & 0x8000U) != 0;
            const bool z = (res == 0U);
            const std::uint16_t old_x = get_ccr(api) & 0x10U;
            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00261A: BLT.w [6D000010]
        if (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)) return 0x00262CU;
        return 0x00261EU;
    }
    case 0x00261EU: {
        // 0x00261E: SUBI.w [04440040]
        {
            const auto old_d = api.read_register(api.context, kRegD4);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x40U) & 0xFFFFU;
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
        // 0x002622: SUBI.w [04410080]
        {
            const auto old_d = api.read_register(api.context, kRegD1);
            const std::uint32_t a = old_d & 0xFFFFU;
            const std::uint32_t b = static_cast<std::uint32_t>(0x80U) & 0xFFFFU;
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
        // 0x002626: ADD.w [DC41]
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
        // 0x002628: MOVE.w [3886]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD6) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA4), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        // 0x00262A: MOVE.w [3885]
        {
            const auto val = static_cast<std::uint16_t>((api.read_register(api.context, kRegD5) & 0xFFFFU));
            api.write(api.context, api.read_register(api.context, kRegA4), 2, val);
            update_logic_ccr16(api, val);
        }
        api.add_cycles(api.context, 8 * 7);
        return 0x00262CU;
    }
    case 0x00262CU: {
        // 0x00262C: DBRA.w [51CFFFD2]
        {
            const auto old_d = api.read_register(api.context, kRegD7);
            const auto count = static_cast<std::int16_t>(old_d & 0xFFFFU) - 1;
            api.write_register(api.context, kRegD7, (old_d & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
            api.add_cycles(api.context, 10 * 7);
            if (count != -1) return 0x002600U;
            return 0x002630U;
        }
    }
    case 0x002630U: {
        // 0x002630: RTS. [4E75]
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
