#include "tools/hybrid/m68k_gameplay_step_set.hpp"

#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace oasis::hybrid {
namespace {
enum class Helper : unsigned { None, H9D70, H9D4E, H9DB4 };
constexpr unsigned kRamStart = 0xFF0000U;
constexpr unsigned kRamEnd = 0x1000000U;

using ReadRegister = std::uint32_t (*)(unsigned);
using WriteRegister = void (*)(unsigned, unsigned);
using PeekMemory = int (*)(unsigned);
using ReadMemory = std::uint32_t (*)(unsigned, int);
using WriteMemory = void (*)(unsigned, int, unsigned);
using Fetch16 = std::uint32_t (*)();
using SetReturn = void (*)(unsigned, unsigned, unsigned);
using Instruction = void (*)(unsigned);
using AddCycles = void (*)(int);
using BranchAdjustment = int (*)();
using InstallBlock = void (*)(int (*)(unsigned));

ReadRegister g_read_register{};
WriteRegister g_write_register{};
PeekMemory g_peek{};
ReadMemory g_read{};
WriteMemory g_write{};
Fetch16 g_fetch16{};
SetReturn g_set_return{};
Instruction g_begin{};
Instruction g_finish{};
AddCycles g_add_cycles{};
BranchAdjustment g_branch_adjustment{};
BranchAdjustment g_byte_branch_adjustment{};
void (*g_observe)(unsigned, unsigned){};
void (*g_fault)(unsigned){};
std::array<bool, 3> g_enabled{};
Helper g_active{Helper::None};
std::array<unsigned, 3> g_calls{};

std::uint32_t rr(void*, unsigned index) { return g_read_register(index); }
void wr(void*, unsigned index, std::uint32_t value) { g_write_register(index, value); }
int peek(void*, unsigned address) { return g_peek(address); }
std::uint32_t read(void*, unsigned address, int width) { return g_read(address, width); }
void write(void*, unsigned address, int width, std::uint32_t value) { g_write(address, width, value); }
std::uint32_t fetch16(void*) { return g_fetch16(); }
void set_return(void*, unsigned pc, unsigned pref_addr, unsigned pref_data) {
    g_set_return(pc, pref_addr, pref_data);
}
void begin(void*, unsigned opcode) { g_begin(opcode); }
void finish(void*, unsigned opcode) { g_finish(opcode); }
void add_cycles(void*, int cycles) { g_add_cycles(cycles); }
int branch_adjust(void*) { return g_branch_adjustment(); }
int byte_branch_adjust(void*) { return g_byte_branch_adjustment(); }
void observe(void*, unsigned pc, unsigned opcode) { g_observe(pc, opcode); }

bool be_read(const M68k9D70StepApi& api, unsigned address, unsigned width,
             unsigned& value) noexcept {
    value = 0;
    if (address > 0xFFFFFFU || width > 4U || address + width > 0x1000000U) return false;
    for (unsigned i = 0; i < width; ++i) {
        const int byte = api.peek(api.context, address + i);
        if (byte < 0 || byte > 255) return false;
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return true;
}

bool caller_matches(const M68k9D70StepApi& api, unsigned caller,
                    unsigned target, unsigned return_pc) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned stack_return{}, opcode{}, extension{};
    if (sp < kRamStart || sp > kRamEnd - 4U ||
        !be_read(api, sp, 4U, stack_return) || stack_return != return_pc ||
        !be_read(api, caller, 2U, opcode) || opcode != 0x6100U ||
        !be_read(api, caller + 2U, 2U, extension)) return false;
    const auto displacement = static_cast<std::int16_t>(extension);
    return static_cast<unsigned>(static_cast<int>(caller + 2U) + displacement) == target;
}

std::uint16_t ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U));
}
void set_ccr(const M68k9D70StepApi& api, std::uint16_t value) noexcept {
    const auto sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (value & 0x1FU));
}
std::uint16_t logic_flags(std::uint16_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0 ? 4U : 0U) |
        ((value & 0x8000U) ? 8U : 0U));
}
std::uint16_t byte_logic_flags(std::uint8_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0 ? 4U : 0U) |
        ((value & 0x80U) ? 8U : 0U));
}
std::uint16_t sub_flags(std::uint16_t lhs, std::uint16_t rhs,
                        std::uint16_t result, std::uint16_t old,
                        bool affect_x) noexcept {
    const bool carry = lhs < rhs;
    const bool overflow = ((lhs ^ rhs) & (lhs ^ result) & 0x8000U) != 0;
    return static_cast<std::uint16_t>((affect_x ? (carry ? 0x10U : 0U) : old & 0x10U) |
        (result == 0 ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
}
std::uint16_t add_flags(std::uint16_t lhs, std::uint16_t rhs,
                        std::uint16_t result, std::uint16_t old,
                        bool affect_x) noexcept {
    const bool carry = static_cast<std::uint32_t>(lhs) + rhs > 0xFFFFU;
    const bool overflow = ((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000U) != 0;
    return static_cast<std::uint16_t>((affect_x ? (carry ? 0x10U : 0U) : old & 0x10U) |
        (result == 0 ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
}
bool condition(unsigned cc, std::uint16_t sr) noexcept {
    const bool x = (sr & 0x10U) != 0, n = (sr & 8U) != 0;
    const bool z = (sr & 4U) != 0, v = (sr & 2U) != 0, c = (sr & 1U) != 0;
    switch (cc) {
    case 2: return !c && !z;
    case 3: return c || z;
    case 4: return !c;
    case 5: return c;
    case 6: return !z;
    case 7: return z;
    case 8: return !v;
    case 9: return v;
    case 10: return !n;
    case 11: return n;
    case 12: return n == v;
    case 13: return n != v;
    case 14: return !z && n == v;
    case 15: return z || n != v;
    default: (void)x; return false;
    }
}

bool supported_entry(unsigned pc, const M68k9D70StepApi& api, Helper& helper) noexcept {
    if (pc == 0x9D70U && g_enabled[0] && caller_matches(api, 0x9144U, pc, 0x9148U)) helper = Helper::H9D70;
    else if (pc == 0x9D4EU && g_enabled[1] && caller_matches(api, 0x909EU, pc, 0x90A2U)) helper = Helper::H9D4E;
    else if (pc == 0x9DB4U && g_enabled[2] && caller_matches(api, 0x92CCU, pc, 0x92D0U)) helper = Helper::H9DB4;
    else return false;
    return true;
}

M68k9D70StepResult step(unsigned pc, const M68k9D70StepApi& api) noexcept {
    pc &= 0xFFFFFFU;
    if (!api.read_register || !api.write_register || !api.peek || !api.read ||
        !api.write || !api.fetch16 || !api.set_return_state || !api.begin_instruction ||
        !api.finish_instruction || !api.add_cycles || !api.word_not_taken_adjustment ||
        !api.observe_instruction) return M68k9D70StepResult::ContractFault;
    if (g_active == Helper::None) {
        Helper selected{};
        if (!supported_entry(pc, api, selected)) return M68k9D70StepResult::NotHandled;
        g_active = selected;
    }
    const Helper helper = g_active;
    const unsigned low = helper == Helper::H9D70 ? 0x9D70U :
        helper == Helper::H9D4E ? 0x9D4EU : 0x9DB4U;
    const unsigned high = helper == Helper::H9D70 ? 0x9D92U :
        helper == Helper::H9D4E ? 0x9D70U : 0x9DD6U;
    if (pc == 0x9D86U || pc == 0x9D90U || pc == 0x9D6EU ||
        pc == 0x9DCAU || pc == 0x9DD4U) {
        if (pc < low || pc >= high) return M68k9D70StepResult::ContractFault;
        g_active = Helper::None;
        ++g_calls[static_cast<unsigned>(helper) - 1U];
        return M68k9D70StepResult::NotHandled;
    }
    if (pc < low || pc >= high) return M68k9D70StepResult::ContractFault;

    unsigned raw{};
    if (!be_read(api, pc, 2U, raw)) return M68k9D70StepResult::ContractFault;
    const auto opcode = raw & 0xFFFFU;
    const unsigned a6 = api.read_register(api.context, 14U) & 0xFFFFFFU;
    api.observe_instruction(api.context, pc, opcode);
    if ((api.fetch16(api.context) & 0xFFFFU) != opcode) return M68k9D70StepResult::ContractFault;
    api.begin_instruction(api.context, opcode);
    bool branch{}, taken{};

    if (helper == Helper::H9D70) {
        const auto d3 = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
        if (pc == 0x9D70U && opcode == 0x3601U) {
            const auto d = api.read_register(api.context, 3U), value = api.read_register(api.context, 1U);
            api.write_register(api.context, 3U, (d & 0xFFFF0000U) | (value & 0xFFFFU));
            set_ccr(api, logic_flags(static_cast<std::uint16_t>(value), ccr(api)));
        } else if (pc == 0x9D72U && opcode == 0x966EU) {
            if (api.fetch16(api.context) != 0x46U) return M68k9D70StepResult::ContractFault;
            const auto rhs = static_cast<std::uint16_t>(api.read(api.context, a6 + 0x46U, 2));
            const auto result = static_cast<std::uint16_t>(d3 - rhs);
            api.write_register(api.context, 3U, (api.read_register(api.context, 3U) & 0xFFFF0000U) | result);
            set_ccr(api, sub_flags(d3, rhs, result, ccr(api), true));
        } else if (pc == 0x9D76U || pc == 0x9D7CU || pc == 0x9D82U) {
            branch = true; unsigned ext{};
            if (!be_read(api, pc + 2U, 2U, ext)) return M68k9D70StepResult::ContractFault;
            const unsigned expected = pc == 0x9D76U ? 0x0010U : pc == 0x9D7CU ? 0x0008U : 0x0006U;
            if (ext != expected) return M68k9D70StepResult::ContractFault;
            const auto flags = ccr(api); const unsigned cc = pc == 0x9D76U ? 11U : pc == 0x9D7CU ? 3U : 7U;
            taken = condition(cc, flags);
            const auto extension_pc = api.read_register(api.context, 16U);
            const int displacement = static_cast<std::int16_t>(api.fetch16(api.context));
            if (taken) {
                const auto dest = static_cast<unsigned>(static_cast<int>(extension_pc) + displacement) & 0xFFFFFFU;
                unsigned word{}; if (!be_read(api, dest, 2U, word)) return M68k9D70StepResult::ContractFault;
                api.set_return_state(api.context, dest, dest + 2U, word); }
        } else if (pc == 0x9D7AU && opcode == 0xB043U) {
            const auto d0 = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
            set_ccr(api, sub_flags(d0, d3, static_cast<std::uint16_t>(d0 - d3), ccr(api), false));
        } else if ((pc == 0x9D80U && opcode == 0x3003U) || (pc == 0x9D88U && opcode == 0x4240U)) {
            const auto old = api.read_register(api.context, 0U); const auto val = pc == 0x9D80U ? d3 : 0U;
            api.write_register(api.context, 0U, (old & 0xFFFF0000U) | val);
            set_ccr(api, logic_flags(static_cast<std::uint16_t>(val), ccr(api)));
        } else if (pc == 0x9D8AU && opcode == 0x002EU) {
            if (api.fetch16(api.context) != 0x48U || api.fetch16(api.context) != 0x40U) return M68k9D70StepResult::ContractFault;
            const auto old = static_cast<std::uint8_t>(api.read(api.context, a6 + 0x40U, 1));
            const auto val = static_cast<std::uint8_t>(old | 0x48U); api.write(api.context, a6 + 0x40U, 1, val);
            set_ccr(api, byte_logic_flags(val, ccr(api)));
        } else return M68k9D70StepResult::ContractFault;
    } else if (helper == Helper::H9D4E) {
        if (pc == 0x9D4EU && opcode == 0x3E01U) {
            const auto old = api.read_register(api.context, 7U), src = api.read_register(api.context, 1U);
            api.write_register(api.context, 7U, (old & 0xFFFF0000U) | (src & 0xFFFFU));
            set_ccr(api, logic_flags(static_cast<std::uint16_t>(src), ccr(api)));
        } else if (pc == 0x9D50U && opcode == 0xDE6EU) {
            if (api.fetch16(api.context) != 0x46U) return M68k9D70StepResult::ContractFault;
            const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, 7U));
            const auto rhs = static_cast<std::uint16_t>(api.read(api.context, a6 + 0x46U, 2));
            const auto result = static_cast<std::uint16_t>(lhs + rhs);
            api.write_register(api.context, 7U, (api.read_register(api.context, 7U) & 0xFFFF0000U) | result);
            set_ccr(api, add_flags(lhs, rhs, result, ccr(api), true));
        } else if (pc == 0x9D54U && opcode == 0x3628U) {
            if (api.fetch16(api.context) != 4U) return M68k9D70StepResult::ContractFault;
            const unsigned a0 = api.read_register(api.context, 8U) & 0xFFFFFFU;
            const auto val = static_cast<std::uint16_t>(api.read(api.context, a0 + 4U, 2));
            const auto old = api.read_register(api.context, 3U);
            api.write_register(api.context, 3U, (old & 0xFFFF0000U) | val);
            set_ccr(api, logic_flags(val, ccr(api)));
        } else if (pc == 0x9D58U && opcode == 0xE74BU) {
            auto val = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
            auto flags = ccr(api); bool overflow{}; unsigned count = 3U;
            for (unsigned i = 0; i < count; ++i) { const bool before = (val & 0x8000U) != 0;
                val = static_cast<std::uint16_t>(val << 1U); overflow |= before != ((val & 0x8000U) != 0); }
            const bool carry = (api.read_register(api.context, 3U) & 0x2000U) != 0;
            flags = static_cast<std::uint16_t>((carry ? 0x11U : 0U) | (val == 0 ? 4U : 0U) |
                ((val & 0x8000U) ? 8U : 0U) | (overflow ? 2U : 0U));
            api.write_register(api.context, 3U, (api.read_register(api.context, 3U) & 0xFFFF0000U) | val);
            set_ccr(api, flags);
            // The core's LSL.W immediate handler charges two cycles per bit
            // beyond its base opcode-table cost. This constant is the exact
            // three-bit path used by this ROM instruction (3 * CYC_SHIFT * MUL).
            api.add_cycles(api.context, 42);
        } else if (pc == 0x9D5AU && opcode == 0x5343U) {
            const auto old = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
            const auto val = static_cast<std::uint16_t>(old - 1U);
            api.write_register(api.context, 3U, (api.read_register(api.context, 3U) & 0xFFFF0000U) | val);
            const bool borrow = old == 0U;
            set_ccr(api, static_cast<std::uint16_t>((borrow ? 0x11U : (ccr(api) & 0x10U)) |
                (val == 0 ? 4U : 0U) | ((val & 0x8000U) ? 8U : 0U) | (old == 0x8000U ? 2U : 0U)));
        } else if (pc == 0x9D5CU && opcode == 0x9647U) {
            const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
            const auto rhs = static_cast<std::uint16_t>(api.read_register(api.context, 7U));
            const auto val = static_cast<std::uint16_t>(lhs - rhs);
            api.write_register(api.context, 3U, (api.read_register(api.context, 3U) & 0xFFFF0000U) | val);
            set_ccr(api, sub_flags(lhs, rhs, val, ccr(api), true));
        } else if (pc == 0x9D5EU && opcode == 0xB043U) {
            const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
            const auto rhs = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
            set_ccr(api, sub_flags(lhs, rhs, static_cast<std::uint16_t>(lhs - rhs), ccr(api), false));
        } else if (pc == 0x9D60U && opcode == 0x6300U) {
            branch = true; const auto flags = ccr(api); taken = condition(3U, flags);
            const auto extension_pc = api.read_register(api.context, 16U);
            const auto displacement = static_cast<std::int16_t>(api.fetch16(api.context));
            if (displacement != 0x000CU) return M68k9D70StepResult::ContractFault;
            if (taken) {
                const auto dest = static_cast<unsigned>(static_cast<int>(extension_pc) + displacement) & 0xFFFFFFU;
                unsigned word{}; if (!be_read(api, dest, 2U, word)) return M68k9D70StepResult::ContractFault;
                api.set_return_state(api.context, dest, dest + 2U, word); }
        } else if (pc == 0x9D64U && opcode == 0x3003U) {
            const auto old = api.read_register(api.context, 0U), val = api.read_register(api.context, 3U);
            api.write_register(api.context, 0U, (old & 0xFFFF0000U) | (val & 0xFFFFU));
            set_ccr(api, logic_flags(static_cast<std::uint16_t>(val), ccr(api)));
        } else if (pc == 0x9D66U && opcode == 0x6606U) {
            branch = true; taken = condition(6U, ccr(api));
            if (taken) { const auto displacement = static_cast<std::int8_t>(opcode & 0xFFU);
                const auto dest = static_cast<unsigned>(static_cast<int>(api.read_register(api.context, 16U)) + displacement) & 0xFFFFFFU;
                unsigned word{}; if (!be_read(api, dest, 2U, word)) return M68k9D70StepResult::ContractFault;
                api.set_return_state(api.context, dest, dest + 2U, word); }
        } else if (pc == 0x9D68U && opcode == 0x002EU) {
            if (api.fetch16(api.context) != 0x48U || api.fetch16(api.context) != 0x40U) return M68k9D70StepResult::ContractFault;
            const auto old = static_cast<std::uint8_t>(api.read(api.context, a6 + 0x40U, 1));
            const auto val = static_cast<std::uint8_t>(old | 0x48U); api.write(api.context, a6 + 0x40U, 1, val);
            set_ccr(api, byte_logic_flags(val, ccr(api)));
        } else return M68k9D70StepResult::ContractFault;
    } else {
        if (pc == 0x9DB4U && opcode == 0x3602U) {
            const auto old = api.read_register(api.context, 3U), src = api.read_register(api.context, 2U);
            api.write_register(api.context, 3U, (old & 0xFFFF0000U) | (src & 0xFFFFU));
            set_ccr(api, logic_flags(static_cast<std::uint16_t>(src), ccr(api)));
        } else if (pc == 0x9DB6U && opcode == 0x966EU) {
            if (api.fetch16(api.context) != 0x46U) return M68k9D70StepResult::ContractFault;
            const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
            const auto rhs = static_cast<std::uint16_t>(api.read(api.context, a6 + 0x46U, 2));
            const auto val = static_cast<std::uint16_t>(lhs - rhs);
            api.write_register(api.context, 3U, (api.read_register(api.context, 3U) & 0xFFFF0000U) | val);
            set_ccr(api, sub_flags(lhs, rhs, val, ccr(api), true));
        } else if (pc == 0x9DBAU || pc == 0x9DC0U || pc == 0x9DC6U) {
            branch = true; const unsigned cc = pc == 0x9DBAU ? 11U : pc == 0x9DC0U ? 3U : 7U;
            taken = condition(cc, ccr(api)); const auto extension_pc = api.read_register(api.context, 16U);
            const auto displacement = static_cast<std::int16_t>(api.fetch16(api.context));
            if (taken) { const auto dest = static_cast<unsigned>(static_cast<int>(extension_pc) + displacement) & 0xFFFFFFU;
                unsigned word{}; if (!be_read(api, dest, 2U, word)) return M68k9D70StepResult::ContractFault;
                api.set_return_state(api.context, dest, dest + 2U, word); }
        } else if (pc == 0x9DBEU && opcode == 0xB043U) {
            const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
            const auto rhs = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
            set_ccr(api, sub_flags(lhs, rhs, static_cast<std::uint16_t>(lhs - rhs), ccr(api), false));
        } else if (pc == 0x9DC4U && opcode == 0x3003U) {
            const auto old = api.read_register(api.context, 0U), val = api.read_register(api.context, 3U);
            api.write_register(api.context, 0U, (old & 0xFFFF0000U) | (val & 0xFFFFU));
            set_ccr(api, logic_flags(static_cast<std::uint16_t>(val), ccr(api)));
        } else if (pc == 0x9DCCU && opcode == 0x4240U) {
            api.write_register(api.context, 0U, api.read_register(api.context, 0U) & 0xFFFF0000U);
            set_ccr(api, 0x04U | (ccr(api) & 0x10U));
        } else if (pc == 0x9DCEU && opcode == 0x002EU) {
            if (api.fetch16(api.context) != 0x48U || api.fetch16(api.context) != 0x41U) return M68k9D70StepResult::ContractFault;
            const auto old = static_cast<std::uint8_t>(api.read(api.context, a6 + 0x41U, 1));
            const auto val = static_cast<std::uint8_t>(old | 0x48U); api.write(api.context, a6 + 0x41U, 1, val);
            set_ccr(api, byte_logic_flags(val, ccr(api)));
        } else return M68k9D70StepResult::ContractFault;
    }
    api.finish_instruction(api.context, opcode);
    if (branch && !taken) {
        const bool short_branch = helper == Helper::H9D4E && pc == 0x9D68U;
        const auto adjustment = short_branch && api.byte_not_taken_adjustment ?
            api.byte_not_taken_adjustment(api.context) : api.word_not_taken_adjustment(api.context);
        api.add_cycles(api.context, adjustment);
    }
    return M68k9D70StepResult::InstructionStepped;
}

int block_hook(unsigned pc) {
    const M68k9D70StepApi api{nullptr, rr, wr, peek, read, write, fetch16, set_return,
        begin, finish, add_cycles, branch_adjust, byte_branch_adjust, observe};
    const auto result = step(pc, api);
    if (result == M68k9D70StepResult::ContractFault) { if (g_fault) g_fault(pc); return 1; }
    return result == M68k9D70StepResult::InstructionStepped ? 1 : 0;
}

bool selected(std::string_view list, std::string_view id) {
    std::size_t pos{};
    while (pos < list.size()) {
        const auto end = list.find(',', pos);
        if (list.substr(pos, end == std::string_view::npos ? list.size() - pos : end - pos) == id) return true;
        if (end == std::string_view::npos) break;
        pos = end + 1U;
    }
    return false;
}
} // namespace

void install_m68k_gameplay_step_set(ExternalLibrary& core, std::string_view enabled_helpers,
                                    void (*observer)(unsigned, unsigned),
                                    void (*fault_handler)(unsigned)) {
    g_read_register = core.get<ReadRegister>("retro_9c40_capture_register");
    g_write_register = core.get<WriteRegister>("retro_9c40_capture_set_register");
    g_peek = core.get<PeekMemory>("retro_9c40_capture_peek");
    g_read = core.get<ReadMemory>("retro_hybrid_read");
    g_write = core.get<WriteMemory>("retro_hybrid_write");
    g_fetch16 = core.get<Fetch16>("retro_hybrid_fetch16");
    g_set_return = core.get<SetReturn>("retro_hybrid_set_return_state");
    g_begin = core.get<Instruction>("retro_hybrid_begin_instruction");
    g_finish = core.get<Instruction>("retro_hybrid_finish_instruction");
    g_add_cycles = core.get<AddCycles>("retro_hybrid_add_cycles");
    g_branch_adjustment = core.get<BranchAdjustment>("retro_hybrid_bcc_not_taken_word_adjustment");
    g_byte_branch_adjustment = core.get<BranchAdjustment>("retro_hybrid_bcc_not_taken_adjustment");
    g_observe = observer; g_fault = fault_handler;
    g_enabled = {selected(enabled_helpers, "9d70"), selected(enabled_helpers, "9d4e"), selected(enabled_helpers, "9db4")};
    if ((!g_enabled[0] && !g_enabled[1] && !g_enabled[2]) || !g_observe || !g_fault)
        throw std::runtime_error("instruction step set is empty or lacks callbacks");
    core.get<InstallBlock>("retro_hybrid_install_block")(block_hook);
}

} // namespace oasis::hybrid
