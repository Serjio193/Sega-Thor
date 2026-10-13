#include "tools/hybrid/m68k_94a2_instruction_step.hpp"

#include <array>
#include <cstdint>

namespace oasis::hybrid {
namespace {
constexpr unsigned kStart = 0x94A2U;
constexpr unsigned kEnd = 0x94D2U;
constexpr unsigned kRamStart = 0xFF0000U;
constexpr unsigned kRamEnd = 0x1000000U;
constexpr std::array<std::uint16_t, 24> kRomWords{
    0x3A01U, 0x3C01U, 0x9A43U, 0xDC43U, 0xE645U, 0xE646U,
    0x9C45U, 0x2268U, 0x0000U, 0xD2C0U, 0x3828U, 0x0008U,
    0xE96DU, 0xD2C5U, 0x51C4U, 0x50C5U, 0x1411U, 0x8802U,
    0xCA02U, 0xD2E8U, 0x0004U, 0x51CEU, 0xFFF4U, 0x4E75U};
bool active{};

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

bool work_ram_range(unsigned address, unsigned size) noexcept {
    return address >= kRamStart && size <= kRamEnd - kRamStart &&
        address <= kRamEnd - size;
}

bool caller_matches(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & 0xFFFFFFU;
    unsigned returned{}, opcode{}, extension{};
    if (!work_ram_range(sp, 4U) || !read_be(api, sp, 4U, returned) ||
        (returned & 0xFFFFFFU) != 0x946CU ||
        !read_be(api, 0x9468U, 2U, opcode) || opcode != 0x6100U ||
        !read_be(api, 0x946AU, 2U, extension) || extension != 0x0038U)
        return false;
    return true;
}

int arithmetic_shift_three(std::uint16_t raw) noexcept {
    const int value = static_cast<std::int16_t>(raw);
    return value >= 0 ? value / 8 : -((-value + 7) / 8);
}

bool bounded_inputs(const M68k9D70StepApi& api) noexcept {
    const unsigned a0 = api.read_register(api.context, 8U) & 0xFFFFFFU;
    if (!work_ram_range(a0, 10U)) return false;
    unsigned base{}, stride{}, shift{};
    if (!read_be(api, a0, 4U, base) || !read_be(api, a0 + 4U, 2U, stride) ||
        !read_be(api, a0 + 8U, 2U, shift) || stride != 64U || shift != 6U ||
        !work_ram_range(base, 4096U)) return false;

    const auto d0 = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
    const auto d1 = static_cast<std::uint16_t>(api.read_register(api.context, 1U));
    const auto d3 = static_cast<std::uint16_t>(api.read_register(api.context, 3U));
    if (d0 >= 64U || (d3 & 0x8000U) != 0U) return false;
    const auto low_raw = static_cast<std::uint16_t>(d1 - d3);
    const auto high_raw = static_cast<std::uint16_t>(d1 + d3);
    const int first_row = arithmetic_shift_three(low_raw);
    const int last_row = arithmetic_shift_three(high_raw);
    if (first_row < 0 || last_row < first_row || last_row >= 64) return false;
    const auto first = base + d0 + static_cast<unsigned>(first_row) * stride;
    const auto last = base + d0 + static_cast<unsigned>(last_row) * stride;
    return first >= base && last >= first && last < base + 4096U && last < kRamEnd;
}

std::uint16_t ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U));
}

void set_ccr(const M68k9D70StepApi& api, std::uint16_t flags) noexcept {
    const auto sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (flags & 0x1FU));
}

std::uint16_t logic_flags(std::uint16_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0U ? 4U : 0U) |
        ((value & 0x8000U) ? 8U : 0U));
}

std::uint16_t byte_flags(std::uint8_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0U ? 4U : 0U) |
        ((value & 0x80U) ? 8U : 0U));
}

std::uint16_t subtract_flags(std::uint16_t lhs, std::uint16_t rhs,
                             std::uint16_t result) noexcept {
    const bool carry = lhs < rhs;
    const bool overflow = ((lhs ^ rhs) & (lhs ^ result) & 0x8000U) != 0U;
    return static_cast<std::uint16_t>((carry ? 0x11U : 0U) |
        (result == 0U ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
}

std::uint16_t add_flags(std::uint16_t lhs, std::uint16_t rhs,
                        std::uint16_t result) noexcept {
    const bool carry = static_cast<std::uint32_t>(lhs) + rhs > 0xFFFFU;
    const bool overflow = ((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000U) != 0U;
    return static_cast<std::uint16_t>((carry ? 0x11U : 0U) |
        (result == 0U ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
}

bool prevalidate_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned value{};
        if (!read_be(api, kStart + static_cast<unsigned>(i) * 2U, 2U, value) ||
            value != kRomWords[i]) return false;
    }
    return true;
}

bool entry_matches(const M68k9D70StepApi& api) noexcept {
    return caller_matches(api) && bounded_inputs(api) && prevalidate_rom(api);
}

} // namespace

M68k9D70StepResult step_m68k_94a2(unsigned pc, const M68k9D70StepApi& api,
                                  bool enabled) noexcept {
    pc &= 0xFFFFFFU;
    if (!api.read_register || !api.write_register || !api.peek || !api.read ||
        !api.fetch16 || !api.set_return_state || !api.begin_instruction ||
        !api.finish_instruction || !api.add_cycles || !api.observe_instruction)
        return M68k9D70StepResult::ContractFault;
    if (!active) {
        if (!enabled || pc != kStart || !entry_matches(api))
            return M68k9D70StepResult::NotHandled;
        active = true;
    }
    if (pc == 0x94D0U) {
        active = false;
        return M68k9D70StepResult::NotHandled;
    }
    if (pc < kStart || pc >= kEnd) return M68k9D70StepResult::ContractFault;

    unsigned raw{};
    if (!read_be(api, pc, 2U, raw)) return M68k9D70StepResult::ContractFault;
    const auto opcode = static_cast<std::uint16_t>(raw);
    const auto index = (pc - kStart) / 2U;
    if (index >= kRomWords.size() || opcode != kRomWords[index])
        return M68k9D70StepResult::ContractFault;
    api.observe_instruction(api.context, pc, opcode);
    if ((api.fetch16(api.context) & 0xFFFFU) != opcode)
        return M68k9D70StepResult::ContractFault;
    api.begin_instruction(api.context, opcode);

    unsigned dynamic_master_cycles{};
    bool skip_refresh{}, dbf{}, dbf_taken{};
    if (pc == 0x94A2U || pc == 0x94A4U) {
        const unsigned reg = pc == 0x94A2U ? 5U : 6U;
        const auto old = api.read_register(api.context, reg);
        const auto value = api.read_register(api.context, 1U);
        api.write_register(api.context, reg, (old & 0xFFFF0000U) | (value & 0xFFFFU));
        set_ccr(api, logic_flags(static_cast<std::uint16_t>(value), ccr(api)));
    } else if (pc == 0x94A6U || pc == 0x94AEU) {
        const unsigned dst = pc == 0x94A6U ? 5U : 6U;
        const unsigned src = pc == 0x94A6U ? 3U : 5U;
        const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, dst));
        const auto rhs = static_cast<std::uint16_t>(api.read_register(api.context, src));
        const auto result = static_cast<std::uint16_t>(lhs - rhs);
        const auto old = api.read_register(api.context, dst);
        api.write_register(api.context, dst, (old & 0xFFFF0000U) | result);
        set_ccr(api, subtract_flags(lhs, rhs, result));
    } else if (pc == 0x94A8U || pc == 0x94B4U) {
        const unsigned dst = pc == 0x94A8U ? 6U : 9U;
        const unsigned src = pc == 0x94A8U ? 3U : 0U;
        const auto lhs = static_cast<std::uint16_t>(api.read_register(api.context, dst));
        const auto rhs = static_cast<std::uint16_t>(api.read_register(api.context, src));
        const auto result = static_cast<std::uint16_t>(lhs + rhs);
        const auto old = api.read_register(api.context, dst);
        api.write_register(api.context, dst, (old & 0xFFFF0000U) | result);
        set_ccr(api, add_flags(lhs, rhs, result));
    } else if (pc == 0x94AAU || pc == 0x94ACU) {
        const unsigned reg = pc == 0x94AAU ? 5U : 6U;
        const auto old = api.read_register(api.context, reg);
        const auto source = static_cast<std::uint16_t>(old);
        const auto result = static_cast<std::uint16_t>(
            static_cast<std::int16_t>(source) >> 3U);
        const auto carry = static_cast<std::uint16_t>((source >> 2U) & 1U);
        api.write_register(api.context, reg, (old & 0xFFFF0000U) | result);
        set_ccr(api, static_cast<std::uint16_t>((carry ? 0x11U : 0U) |
            (result == 0U ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U)));
        dynamic_master_cycles = 42U;
    } else if (pc == 0x94B0U) {
        if (api.fetch16(api.context) != 0U) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 9U,
            api.read(api.context, api.read_register(api.context, 8U) & 0xFFFFFFU, 4));
    } else if (pc == 0x94B6U) {
        if (api.fetch16(api.context) != 8U) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, 8U) & 0xFFFFFFU;
        const auto value = static_cast<std::uint16_t>(api.read(api.context, a0 + 8U, 2));
        const auto old = api.read_register(api.context, 4U);
        api.write_register(api.context, 4U, (old & 0xFFFF0000U) | value);
        set_ccr(api, logic_flags(value, ccr(api)));
    } else if (pc == 0x94BAU) {
        const auto old = api.read_register(api.context, 5U);
        const auto source = static_cast<std::uint16_t>(old);
        const unsigned count = api.read_register(api.context, 4U) & 0x3FU;
        std::uint16_t result{};
        std::uint16_t carry{};
        auto flags = ccr(api);
        if (count == 0U) {
            result = source;
            flags = static_cast<std::uint16_t>((flags & 0x10U) |
                (source == 0U ? 4U : 0U) | ((source & 0x8000U) ? 8U : 0U));
        } else if (count <= 16U) {
            result = static_cast<std::uint16_t>(source << count);
            carry = static_cast<std::uint16_t>((source >> (16U - count)) & 1U);
            flags = static_cast<std::uint16_t>((carry ? 0x11U : 0U) |
                (result == 0U ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U));
        } else {
            flags = 0x04U;
        }
        api.write_register(api.context, 5U, (old & 0xFFFF0000U) | result);
        set_ccr(api, flags);
        if (count != 0U) {
            dynamic_master_cycles = count * 14U;
            skip_refresh = true;
        }
    } else if (pc == 0x94BCU) {
        const auto old = api.read_register(api.context, 9U);
        const auto value = static_cast<std::int16_t>(api.read_register(api.context, 5U));
        api.write_register(api.context, 9U, old + static_cast<std::int32_t>(value));
      } else if (pc == 0x94BEU) {
          const auto old = api.read_register(api.context, 4U);
          api.write_register(api.context, 4U, old & 0xFFFFFF00U);
      } else if (pc == 0x94C0U) {
          const auto old = api.read_register(api.context, 5U);
          api.write_register(api.context, 5U, (old & 0xFFFFFF00U) | 0xFFU);
    } else if (pc == 0x94C2U) {
        const auto a1 = api.read_register(api.context, 9U) & 0xFFFFFFU;
        const auto value = static_cast<std::uint8_t>(api.read(api.context, a1, 1));
        const auto old = api.read_register(api.context, 2U);
        api.write_register(api.context, 2U, (old & 0xFFFFFF00U) | value);
        set_ccr(api, byte_flags(value, ccr(api)));
    } else if (pc == 0x94C4U || pc == 0x94C6U) {
        const unsigned dst = pc == 0x94C4U ? 4U : 5U;
        const auto old = api.read_register(api.context, dst);
        const auto lhs = static_cast<std::uint8_t>(old);
        const auto rhs = static_cast<std::uint8_t>(api.read_register(api.context, 2U));
        const auto value = static_cast<std::uint8_t>(pc == 0x94C4U ? lhs | rhs : lhs & rhs);
        api.write_register(api.context, dst, (old & 0xFFFFFF00U) | value);
        set_ccr(api, byte_flags(value, ccr(api)));
    } else if (pc == 0x94C8U) {
        if (api.fetch16(api.context) != 4U) return M68k9D70StepResult::ContractFault;
        const auto a0 = api.read_register(api.context, 8U) & 0xFFFFFFU;
        const auto stride = static_cast<std::int16_t>(api.read(api.context, a0 + 4U, 2));
        const auto a1 = api.read_register(api.context, 9U);
        api.write_register(api.context, 9U, a1 + static_cast<std::int32_t>(stride));
    } else if (pc == 0x94CCU) {
        dbf = true;
        const auto extension_pc = api.read_register(api.context, 16U);
        const auto displacement = static_cast<std::int16_t>(api.fetch16(api.context));
        if (displacement != -12) return M68k9D70StepResult::ContractFault;
        const auto old = api.read_register(api.context, 6U);
        const auto count = static_cast<std::uint16_t>(old - 1U);
        api.write_register(api.context, 6U, (old & 0xFFFF0000U) | count);
        dbf_taken = count != 0xFFFFU;
        if (dbf_taken) {
            const auto destination = static_cast<unsigned>(
                static_cast<int>(extension_pc) + displacement) & 0xFFFFFFU;
            unsigned prefetched{};
            if (!read_be(api, destination, 2U, prefetched))
                return M68k9D70StepResult::ContractFault;
            api.set_return_state(api.context, destination, destination + 2U, prefetched);
        }
    } else {
        return M68k9D70StepResult::ContractFault;
    }

    if (skip_refresh) {
        if (!api.skip_bus_refresh) return M68k9D70StepResult::ContractFault;
        if (dynamic_master_cycles != 0U)
            api.add_cycles(api.context, static_cast<int>(dynamic_master_cycles));
        api.skip_bus_refresh(api.context);
        api.finish_instruction(api.context, opcode);
    } else {
        api.finish_instruction(api.context, opcode);
        if (dynamic_master_cycles != 0U)
            api.add_cycles(api.context, static_cast<int>(dynamic_master_cycles));
    }
    if (dbf) api.add_cycles(api.context, dbf_taken ? -14 : 14);
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid
