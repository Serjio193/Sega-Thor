#include "tools/hybrid/m68k_9c40_instruction_step.hpp"

#include <array>
#include <cstdint>

namespace oasis::hybrid {
namespace {
constexpr unsigned kStart = 0x9C40U;
constexpr unsigned kEnd = 0x9C7CU;
constexpr unsigned kCaller = 0xE376U;
constexpr unsigned kReturn = 0xE37AU;
constexpr unsigned kRamStart = 0xFF0000U;
constexpr unsigned kRamEnd = 0x1000000U;
constexpr unsigned kSelector = 0xFF16F0U;
constexpr unsigned kGridDescriptor = 0xFF1766U;
constexpr unsigned kGridBase = 0xFF5FACU;
constexpr std::array<std::uint16_t, 30> kRomWords{
    0x48E7U, 0x60C0U, 0x41F9U, 0x00FFU, 0x1766U, 0x0839U, 0x0007U,
    0x00FFU, 0x16F0U, 0x6706U, 0x41F9U, 0x00FFU, 0x1770U, 0x322EU,
    0x0008U, 0x342EU, 0x000CU, 0xE641U, 0xE642U, 0x2268U, 0x0000U,
    0xD2C1U, 0x3228U, 0x0008U, 0xE36AU, 0x1A31U, 0x2000U, 0x4CDFU,
    0x0306U, 0x4E75U};
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

bool work_ram(unsigned address, unsigned size) noexcept {
    return size <= kRamEnd - kRamStart && address >= kRamStart &&
        address <= kRamEnd - size;
}

bool caller_matches(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned returned{}, opcode{}, extension{};
    if (!work_ram(sp, 4U) || !read_be(api, sp, 4U, returned) ||
        returned != kReturn || !read_be(api, kCaller, 2U, opcode) ||
        opcode != 0x6100U || !read_be(api, kCaller + 2U, 2U, extension))
        return false;
    return static_cast<unsigned>(static_cast<int>(kCaller + 2U) +
        static_cast<std::int16_t>(extension)) == kStart;
}

bool bounded_entry(const M68k9D70StepApi& api) noexcept {
    const unsigned a6 = api.read_register(api.context, 14U) & 0xFFFFFFU;
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned selector{}, descriptor{}, base{}, shift{};
    if (!work_ram(a6 + 8U, 6U) || !work_ram(sp - 16U, 16U) ||
        !read_be(api, kSelector, 1U, selector) || selector != 0U ||
        !read_be(api, kGridDescriptor, 4U, descriptor) ||
        descriptor != kGridBase ||
        !read_be(api, kGridDescriptor + 8U, 2U, shift) || shift != 6U ||
        !work_ram(descriptor, 4096U)) return false;
    unsigned x{}, y{};
    if (!read_be(api, a6 + 8U, 2U, x) || !read_be(api, a6 + 12U, 2U, y))
        return false;
    const int column = static_cast<std::int16_t>(x) >> 3;
    const int row = static_cast<std::int16_t>(y) >> 3;
    if (column < 0 || column >= 64 || row < 0 || row >= 64) return false;
    const auto address = descriptor + static_cast<unsigned>(column) +
        (static_cast<unsigned>(row) << shift);
    return address >= descriptor && address < descriptor + 4096U;
}

bool validate_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned value{};
        if (!read_be(api, kStart + static_cast<unsigned>(i) * 2U, 2U, value) ||
            value != kRomWords[i]) return false;
    }
    return true;
}

bool is_interrupt_frame_for_resume(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned saved_sr{}, saved_pc{};
    if (!work_ram(sp, 6U) || !read_be(api, sp, 2U, saved_sr) ||
        !read_be(api, sp + 2U, 4U, saved_pc)) return false;
    const unsigned current_sr = api.read_register(api.context, 17U) & 0xFFFFU;
    return saved_pc == resume_pc && (saved_sr & 0x2000U) != 0U &&
        (current_sr & 0x2000U) != 0U &&
        (current_sr & 0x0700U) > (saved_sr & 0x0700U);
}

std::uint16_t ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U));
}

void set_ccr(const M68k9D70StepApi& api, std::uint16_t flags) noexcept {
    const auto sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (flags & 0x1FU));
}

std::uint16_t move_word_flags(std::uint16_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0U ? 4U : 0U) |
        ((value & 0x8000U) != 0U ? 8U : 0U));
}

std::uint16_t arithmetic_shift_three(std::uint16_t value) noexcept {
    return static_cast<std::uint16_t>(static_cast<std::int16_t>(value) >> 3U);
}

std::uint16_t shift_flags(std::uint16_t result, bool carry, bool overflow,
                          std::uint16_t old, bool update_x) noexcept {
    const auto x = update_x ? (carry ? 0x10U : 0U) : (old & 0x10U);
    return static_cast<std::uint16_t>(x | (result == 0U ? 4U : 0U) |
        ((result & 0x8000U) != 0U ? 8U : 0U) | (overflow ? 2U : 0U) |
        (carry ? 1U : 0U));
}

bool fetch_extension(const M68k9D70StepApi& api, std::uint16_t expected) noexcept {
    return (api.fetch16(api.context) & 0xFFFFU) == expected;
}

} // namespace

M68k9D70StepResult step_m68k_9c40(unsigned pc, const M68k9D70StepApi& api,
                                  bool enabled) noexcept {
    pc &= 0xFFFFFFU;
    if (!api.read_register || !api.write_register || !api.peek || !api.read ||
        !api.write || !api.fetch16 || !api.set_return_state ||
        !api.begin_instruction || !api.finish_instruction || !api.add_cycles ||
        !api.byte_not_taken_adjustment || !api.observe_instruction)
        return M68k9D70StepResult::ContractFault;
    if (!active) {
        if (!enabled || pc != kStart || !caller_matches(api) ||
            !bounded_entry(api) || !validate_rom(api))
            return M68k9D70StepResult::NotHandled;
        active = true;
        interrupted = false;
        resume_pc = pc;
    }
    if (interrupted) {
        if (pc != resume_pc) return M68k9D70StepResult::NotHandled;
        interrupted = false;
    } else if (pc != resume_pc) {
        if (!is_interrupt_frame_for_resume(api))
            return M68k9D70StepResult::ContractFault;
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }
    if (pc == 0x9C7AU) {
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

    unsigned dynamic_cycles{};
    bool skip_refresh{}, charge_dynamic_before_finish{}, short_branch{}, branch_taken{};
    const unsigned a0 = api.read_register(api.context, 8U) & 0xFFFFFFU;
    const unsigned a6 = api.read_register(api.context, 14U) & 0xFFFFFFU;
    if (pc == 0x9C40U) {
        if (!fetch_extension(api, 0x60C0U)) return M68k9D70StepResult::ContractFault;
        unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
        constexpr unsigned regs[] = {9U, 8U, 2U, 1U}; // A1, A0, D2, D1 predecrement order.
        for (const unsigned reg : regs) {
            const auto value = api.read_register(api.context, reg);
            sp -= 2U;
            api.write_register(api.context, 15U, sp);
            api.write(api.context, sp, 2, value & 0xFFFFU);
            sp -= 2U;
            api.write_register(api.context, 15U, sp);
            api.write(api.context, sp, 2, value >> 16U);
        }
        dynamic_cycles = 224U; // Four longword transfers: 32 additional core cycles.
        charge_dynamic_before_finish = true;
    } else if (pc == 0x9C44U || pc == 0x9C54U) {
        const unsigned target = pc == 0x9C44U ? 0xFF1766U : 0xFF1770U;
        if (!fetch_extension(api, 0x00FFU) ||
            !fetch_extension(api, static_cast<std::uint16_t>(target & 0xFFFFU)))
            return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 8U, target);
    } else if (pc == 0x9C4AU) {
        if (!fetch_extension(api, 0x0007U) || !fetch_extension(api, 0x00FFU) ||
            !fetch_extension(api, 0x16F0U)) return M68k9D70StepResult::ContractFault;
        const auto value = static_cast<std::uint8_t>(api.read(api.context, kSelector, 1));
        set_ccr(api, static_cast<std::uint16_t>((ccr(api) & 0x1BU) |
            ((value & 0x80U) == 0U ? 0x04U : 0U)));
    } else if (pc == 0x9C52U) {
        short_branch = true;
        branch_taken = (ccr(api) & 0x04U) != 0U;
        if (branch_taken) {
            constexpr unsigned destination = 0x9C5AU;
            unsigned prefetched{};
            if (!read_be(api, destination, 2U, prefetched))
                return M68k9D70StepResult::ContractFault;
            api.set_return_state(api.context, destination, destination + 2U, prefetched);
        }
    } else if (pc == 0x9C5AU || pc == 0x9C5EU) {
        const unsigned displacement = pc == 0x9C5AU ? 8U : 12U;
        if (!fetch_extension(api, static_cast<std::uint16_t>(displacement)))
            return M68k9D70StepResult::ContractFault;
        const unsigned reg = pc == 0x9C5AU ? 1U : 2U;
        const auto value = static_cast<std::uint16_t>(api.read(api.context, a6 + displacement, 2));
        const auto old = api.read_register(api.context, reg);
        api.write_register(api.context, reg, (old & 0xFFFF0000U) | value);
        set_ccr(api, move_word_flags(value, ccr(api)));
    } else if (pc == 0x9C62U || pc == 0x9C64U) {
        const unsigned reg = pc == 0x9C62U ? 1U : 2U;
        const auto old = api.read_register(api.context, reg);
        const auto value = static_cast<std::uint16_t>(old);
        const bool carry = (value & 0x0004U) != 0U;
        const auto result = arithmetic_shift_three(value);
        api.write_register(api.context, reg, (old & 0xFFFF0000U) | result);
        set_ccr(api, shift_flags(result, carry, false, ccr(api), true));
        dynamic_cycles = 42U;
    } else if (pc == 0x9C66U) {
        if (!fetch_extension(api, 0x0000U)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 9U, api.read(api.context, a0, 4));
    } else if (pc == 0x9C6AU) {
        const auto old = api.read_register(api.context, 9U);
        const auto addend = static_cast<std::int16_t>(api.read_register(api.context, 1U));
        api.write_register(api.context, 9U, old + static_cast<std::int32_t>(addend));
    } else if (pc == 0x9C6CU) {
        if (!fetch_extension(api, 8U)) return M68k9D70StepResult::ContractFault;
        const auto value = static_cast<std::uint16_t>(api.read(api.context, a0 + 8U, 2));
        const auto old = api.read_register(api.context, 1U);
        api.write_register(api.context, 1U, (old & 0xFFFF0000U) | value);
        set_ccr(api, move_word_flags(value, ccr(api)));
    } else if (pc == 0x9C70U) {
        const auto count = api.read_register(api.context, 1U) & 0x3FU;
        const auto old = api.read_register(api.context, 2U);
        auto result = static_cast<std::uint16_t>(old);
        bool carry{}, overflow{};
        for (unsigned i = 0; i < count; ++i) {
            const bool old_sign = (result & 0x8000U) != 0U;
            carry = old_sign;
            result = static_cast<std::uint16_t>(result << 1U);
            overflow = overflow || old_sign != ((result & 0x8000U) != 0U);
        }
        api.write_register(api.context, 2U, (old & 0xFFFF0000U) | result);
        set_ccr(api, shift_flags(result, count != 0U && carry, overflow,
                                 ccr(api), count != 0U));
        if (count != 0U) {
            dynamic_cycles = count * 14U;
            skip_refresh = true;
        }
    } else if (pc == 0x9C72U) {
        if (!fetch_extension(api, 0x2000U)) return M68k9D70StepResult::ContractFault;
        const auto index = static_cast<std::int16_t>(api.read_register(api.context, 2U));
        const auto address = static_cast<unsigned>(
            static_cast<int>(api.read_register(api.context, 9U) & 0xFFFFFFU) + index) & 0xFFFFFFU;
        const auto value = static_cast<std::uint8_t>(api.read(api.context, address, 1));
        const auto old = api.read_register(api.context, 5U);
        api.write_register(api.context, 5U, (old & 0xFFFFFF00U) | value);
        const auto flags = static_cast<std::uint16_t>((ccr(api) & 0x10U) |
            (value == 0U ? 4U : 0U) | ((value & 0x80U) != 0U ? 8U : 0U));
        set_ccr(api, flags);
    } else if (pc == 0x9C76U) {
        if (!fetch_extension(api, 0x0306U)) return M68k9D70StepResult::ContractFault;
        unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
        constexpr unsigned regs[] = {1U, 2U, 8U, 9U}; // D1, D2, A0, A1 postincrement order.
        for (const unsigned reg : regs) {
            api.write_register(api.context, reg, api.read(api.context, sp, 4));
            sp += 4U;
        }
        api.write_register(api.context, 15U, sp);
        // GPGX's MOVEM postincrement implementation performs one final word
        // bus read at the address after the register list. Preserve that
        // observable access before the instruction's cycle/refresh handling.
        (void)api.read(api.context, sp, 2);
        dynamic_cycles = 224U;
        charge_dynamic_before_finish = true;
    } else {
        return M68k9D70StepResult::ContractFault;
    }

    if (charge_dynamic_before_finish) {
        api.add_cycles(api.context, static_cast<int>(dynamic_cycles));
        if (!api.skip_bus_refresh) return M68k9D70StepResult::ContractFault;
        api.skip_bus_refresh(api.context);
        api.finish_instruction(api.context, opcode);
    } else if (skip_refresh) {
        if (!api.skip_bus_refresh) return M68k9D70StepResult::ContractFault;
        api.add_cycles(api.context, static_cast<int>(dynamic_cycles));
        api.skip_bus_refresh(api.context);
        api.finish_instruction(api.context, opcode);
    } else {
        api.finish_instruction(api.context, opcode);
        if (dynamic_cycles != 0U)
            api.add_cycles(api.context, static_cast<int>(dynamic_cycles));
    }
    if (short_branch && !branch_taken)
        api.add_cycles(api.context, api.byte_not_taken_adjustment(api.context));
    resume_pc = api.read_register(api.context, 16U) & 0xFFFFFFU;
    if (resume_pc < kStart || resume_pc > 0x9C7AU)
        return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid
