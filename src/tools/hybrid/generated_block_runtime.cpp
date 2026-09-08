#include "tools/hybrid/generated_block_runtime.hpp"

#include <stdexcept>

namespace oasis::hybrid::generated {
namespace {

void require_api(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void move_flags(BasicBlockApi& api, std::uint32_t value, unsigned width) {
    const auto mask = width == 1 ? 0x80U : width == 2 ? 0x8000U : 0x80000000U;
    const auto value_mask = width == 1 ? 0xFFU : width == 2 ? 0xFFFFU : 0xFFFFFFFFU;
    auto sr = api.reg(17);
    sr = (sr & ~0x0FU) | (value & value_mask ? 0U : 4U) |
         (value & mask ? 8U : 0U);
    api.set_reg(17, sr);
}

void add_flags(BasicBlockApi& api, std::uint32_t lhs, std::uint32_t rhs,
               std::uint32_t result) {
    const auto carry = (static_cast<std::uint64_t>(lhs) + rhs) > 0xFFFFFFFFULL;
    const auto overflow = ((~(lhs ^ rhs) & (lhs ^ result)) & 0x80000000U) != 0;
    auto sr = api.reg(17);
    sr = (sr & ~0x1FU) | (carry ? 0x11U : 0U) |
         (result & 0x80000000U ? 0x08U : 0U) |
         (result == 0 ? 0x04U : 0U) | (overflow ? 0x02U : 0U);
    api.set_reg(17, sr);
}

bool condition_holds(std::uint32_t sr, unsigned condition) {
    const bool c = (sr & 0x01U) != 0;
    const bool v = (sr & 0x02U) != 0;
    const bool z = (sr & 0x04U) != 0;
    const bool n = (sr & 0x08U) != 0;
    switch (condition & 0x0FU) {
    case 0: return true;
    case 1: return false;
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
    default: return z || n != v;
    }
}

std::uint32_t read(BasicBlockApi& api, unsigned address, int width) {
    require_api(api.read != nullptr, "generated read bridge unavailable");
    return api.read(address, width);
}

void write(BasicBlockApi& api, unsigned address, int width, unsigned value) {
    require_api(api.write != nullptr, "generated write bridge unavailable");
    api.write(address, width, value);
}

} // namespace

std::uint32_t fetch_checked(BasicBlockApi& api, std::uint32_t expected) {
    require_api(api.fetch16 != nullptr, "generated fetch bridge unavailable");
    const auto actual = api.fetch16();
    if (actual != expected) throw std::runtime_error("generated opcode/extension mismatch");
    return actual;
}

void movem_l_predecrement(BasicBlockApi& api, std::uint32_t mask) {
    auto stack = api.reg(15);
    for (int bit = 15; bit >= 0; --bit) {
        if (!(mask & (1U << static_cast<unsigned>(bit)))) continue;
        const auto value = api.reg(bit);
        stack -= 2U;
        write(api, stack, 2, value & 0xFFFFU);
        stack -= 2U;
        write(api, stack, 2, value >> 16U);
    }
    api.set_reg(15, stack);
}

void clear_w_data_register(BasicBlockApi& api, unsigned data_register) {
    api.set_reg(data_register, api.reg(data_register) & 0xFFFF0000U);
    move_flags(api, 0, 2);
}

void move_b_postincrement_to_data_register(BasicBlockApi& api,
                                           unsigned address_register,
                                           unsigned data_register) {
    const auto address = api.reg(8U + address_register);
    const auto value = read(api, address, 1);
    api.set_reg(8U + address_register, address + 1U);
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFFFF00U) | value);
    move_flags(api, value, 1);
}

void lea_absolute_long(BasicBlockApi& api, std::uint32_t address,
                       unsigned address_register) {
    api.set_reg(8U + address_register, address);
}

void adda_w_data_to_address(BasicBlockApi& api, unsigned data_register,
                            unsigned address_register) {
    const auto value = static_cast<std::int16_t>(api.reg(data_register));
    const auto target = 8U + address_register;
    api.set_reg(target, api.reg(target) + static_cast<std::int32_t>(value));
}

void move_w_postincrement_to_postincrement(BasicBlockApi& api,
                                            unsigned source_register,
                                            unsigned destination_register) {
    const auto source = 8U + source_register;
    const auto destination = 8U + destination_register;
    const auto value = read(api, api.reg(source), 2);
    api.set_reg(source, api.reg(source) + 2U);
    write(api, api.reg(destination), 2, value);
    api.set_reg(destination, api.reg(destination) + 2U);
    move_flags(api, value, 2);
}

void add_l_data_to_data(BasicBlockApi& api, unsigned source_register,
                        unsigned destination_register) {
    const auto lhs = api.reg(destination_register);
    const auto rhs = api.reg(source_register);
    const auto result = lhs + rhs;
    api.set_reg(destination_register, result);
    add_flags(api, lhs, rhs, result);
}

void branch_condition(BasicBlockApi& api, unsigned condition, unsigned target,
                      int not_taken_cycles, int extension) {
    if (condition_holds(api.reg(17), condition)) {
        if (extension >= 0) {
            const auto actual = fetch_checked(api, static_cast<unsigned>(extension));
            (void)actual;
        }
        api.set_reg(16, target);
    } else {
        if (extension >= 0) api.set_reg(16, api.reg(16) + 2U);
        if (not_taken_cycles) api.add_cycles(not_taken_cycles);
    }
}

void dbcc(BasicBlockApi& api, unsigned condition, unsigned data_register,
          unsigned target, int extension) {
    if (condition_holds(api.reg(17), condition)) {
        if (extension >= 0) api.set_reg(16, api.reg(16) + 2U);
        return;
    }
    const auto value = api.reg(data_register);
    const auto result = (value - 1U) & 0xFFFFU;
    api.set_reg(data_register, (value & 0xFFFF0000U) | result);
    if (result != 0xFFFFU) {
        if (extension >= 0) {
            const auto actual = fetch_checked(api, static_cast<unsigned>(extension));
            (void)actual;
        }
        api.set_reg(16, target);
        api.add_cycles(-14);
    } else {
        if (extension >= 0) api.set_reg(16, api.reg(16) + 2U);
        api.add_cycles(14);
    }
}

void test_absolute_long(BasicBlockApi& api, std::uint32_t address, unsigned width) {
    const auto value = read(api, address, static_cast<int>(width));
    move_flags(api, value, width);
}

} // namespace oasis::hybrid::generated
