#include "tools/hybrid/m68k_8ef8_instruction_step.hpp"

#include <array>
#include <cstdint>

namespace oasis::hybrid {
namespace {
constexpr unsigned kStart = 0x8EF8U;
constexpr unsigned kEnd = 0x8F12U;
constexpr unsigned kRamStart = 0xFF0000U;
constexpr unsigned kRamEnd = 0x1000000U;
constexpr std::array<unsigned, 6> kCallers{
    0x58FAU, 0x5928U, 0x5A6AU, 0x5ABAU, 0x5B9AU, 0x662EU};
constexpr std::array<std::uint16_t, 13> kRomWords{
    0x3D40U, 0x002AU, 0x3D7CU, 0xFFFFU, 0x002CU, 0x022EU, 0xFFF9U,
    0x0037U, 0x426EU, 0x0030U, 0x426EU, 0x002EU, 0x4E75U};
bool active{};
bool interrupted{};
unsigned resume_pc{};

bool read_be(const M68k9D70StepApi& api, unsigned address, unsigned width,
             unsigned& value) noexcept {
    value = 0;
    if (address > 0xFFFFFFU || width == 0U || width > 4U ||
        address + width > 0x1000000U) return false;
    for (unsigned i = 0; i < width; ++i) {
        const int byte = api.peek(api.context, address + i);
        if (byte < 0 || byte > 255) return false;
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return true;
}

bool in_work_ram(unsigned address, unsigned size) noexcept {
    return size <= kRamEnd - kRamStart && address >= kRamStart &&
        address <= kRamEnd - size;
}

bool validate_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned word{};
        if (!read_be(api, kStart + static_cast<unsigned>(i) * 2U, 2U, word) ||
            word != kRomWords[i]) return false;
    }
    return true;
}

bool caller_matches(const M68k9D70StepApi& api, unsigned sp) noexcept {
    unsigned return_pc{};
    if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4U, return_pc)) return false;
    for (const unsigned caller : kCallers) {
        unsigned opcode{}, extension{};
        if (!read_be(api, caller, 2U, opcode) || opcode != 0x6100U ||
            !read_be(api, caller + 2U, 2U, extension)) continue;
        const unsigned continuation = caller + 4U;
        const auto destination = static_cast<unsigned>(
            static_cast<int>(caller + 2U) + static_cast<std::int16_t>(extension));
        if (destination == kStart && return_pc == continuation) return true;
    }
    return false;
}

bool valid_entry(const M68k9D70StepApi& api) noexcept {
    const unsigned a6 = api.read_register(api.context, 14U) & 0xFFFFFFU;
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    return in_work_ram(a6, 0x38U) && caller_matches(api, sp) && validate_rom(api);
}

bool interrupt_frame_for_resume(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned saved_sr{}, saved_pc{};
    if (!in_work_ram(sp, 6U) || !read_be(api, sp, 2U, saved_sr) ||
        !read_be(api, sp + 2U, 4U, saved_pc)) return false;
    const unsigned current_sr = api.read_register(api.context, 17U) & 0xFFFFU;
    return saved_pc == resume_pc && (saved_sr & 0x2000U) != 0U &&
        (current_sr & 0x2000U) != 0U &&
        (current_sr & 0x0700U) > (saved_sr & 0x0700U);
}

void set_ccr(const M68k9D70StepApi& api, std::uint16_t flags) noexcept {
    const auto sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (flags & 0x1FU));
}

std::uint16_t logic_flags(std::uint16_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0U ? 4U : 0U) |
        ((value & 0x8000U) ? 8U : 0U));
}

std::uint16_t byte_logic_flags(std::uint8_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0U ? 4U : 0U) |
        ((value & 0x80U) ? 8U : 0U));
}

std::uint16_t clear_flags(std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | 0x04U);
}
} // namespace

M68k9D70StepResult step_m68k_8ef8(unsigned pc, const M68k9D70StepApi& api,
                                 bool enabled) noexcept {
    pc &= 0xFFFFFFU;
    if (!api.read_register || !api.write_register || !api.peek || !api.read ||
        !api.write || !api.fetch16 || !api.begin_instruction ||
        !api.finish_instruction || !api.add_cycles || !api.observe_instruction)
        return M68k9D70StepResult::ContractFault;

    if (!active) {
        if (pc != kStart || !enabled || !valid_entry(api))
            return M68k9D70StepResult::NotHandled;
        active = true;
        interrupted = false;
        resume_pc = pc;
    }
    if (interrupted) {
        if (pc != resume_pc) return M68k9D70StepResult::NotHandled;
        interrupted = false;
    } else if (pc != resume_pc) {
        if (!interrupt_frame_for_resume(api)) return M68k9D70StepResult::ContractFault;
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }
    if (pc == 0x8F10U) {
        active = false;
        interrupted = false;
        return M68k9D70StepResult::NotHandled;
    }
    if (pc < kStart || pc >= kEnd) return M68k9D70StepResult::ContractFault;

    unsigned raw{};
    if (!read_be(api, pc, 2U, raw)) return M68k9D70StepResult::ContractFault;
    const auto opcode = static_cast<std::uint16_t>(raw);
    api.observe_instruction(api.context, pc, opcode);
    if ((api.fetch16(api.context) & 0xFFFFU) != opcode)
        return M68k9D70StepResult::ContractFault;
    api.begin_instruction(api.context, opcode);

    const unsigned a6 = api.read_register(api.context, 14U) & 0xFFFFFFU;
    unsigned displacement{};
    if (pc == 0x8EF8U && opcode == 0x3D40U) {
        if (api.fetch16(api.context) != 0x002AU) return M68k9D70StepResult::ContractFault;
        const auto value = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
        api.write(api.context, a6 + 0x2AU, 2, value);
        set_ccr(api, logic_flags(value, static_cast<std::uint16_t>(api.read_register(api.context, 17U))));
        resume_pc = 0x8EFCU;
    } else if (pc == 0x8EFCU && opcode == 0x3D7CU) {
        if (api.fetch16(api.context) != 0xFFFFU || api.fetch16(api.context) != 0x002CU)
            return M68k9D70StepResult::ContractFault;
        api.write(api.context, a6 + 0x2CU, 2, 0xFFFFU);
        set_ccr(api, logic_flags(0xFFFFU, static_cast<std::uint16_t>(api.read_register(api.context, 17U))));
        resume_pc = 0x8F02U;
    } else if (pc == 0x8F02U && opcode == 0x022EU) {
        if (api.fetch16(api.context) != 0xFFF9U || api.fetch16(api.context) != 0x0037U)
            return M68k9D70StepResult::ContractFault;
        const unsigned address = a6 + 0x37U;
        const auto old = static_cast<std::uint8_t>(api.read(api.context, address, 1));
        const auto value = static_cast<std::uint8_t>(old & 0xF9U);
        api.write(api.context, address, 1, value);
        set_ccr(api, byte_logic_flags(value, static_cast<std::uint16_t>(api.read_register(api.context, 17U))));
        resume_pc = 0x8F08U;
    } else if ((pc == 0x8F08U || pc == 0x8F0CU) && opcode == 0x426EU) {
        displacement = api.fetch16(api.context) & 0xFFFFU;
        const unsigned expected = pc == 0x8F08U ? 0x0030U : 0x002EU;
        if (displacement != expected) return M68k9D70StepResult::ContractFault;
        api.write(api.context, a6 + displacement, 2, 0U);
        set_ccr(api, clear_flags(static_cast<std::uint16_t>(api.read_register(api.context, 17U))));
        resume_pc = pc == 0x8F08U ? 0x8F0CU : 0x8F10U;
    } else {
        return M68k9D70StepResult::ContractFault;
    }

    // finish_instruction charges the canonical opcode-table base cycles and
    // owns the core's normal refresh boundary for this instruction.
    api.finish_instruction(api.context, opcode);
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid
