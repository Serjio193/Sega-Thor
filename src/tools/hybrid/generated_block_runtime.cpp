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

void arithmetic_flags(BasicBlockApi& api, std::uint32_t lhs, std::uint32_t rhs,
                      std::uint32_t result, unsigned width, bool subtract) {
    const auto mask = width == 1 ? 0xFFU : width == 2 ? 0xFFFFU : 0xFFFFFFFFU;
    const auto sign = width == 1 ? 0x80U : width == 2 ? 0x8000U : 0x80000000U;
    lhs &= mask;
    rhs &= mask;
    result &= mask;
    const auto carry = subtract ? lhs < rhs :
        static_cast<std::uint64_t>(lhs) + rhs > mask;
    const auto overflow = subtract ?
        (((lhs ^ rhs) & (lhs ^ result) & sign) != 0) :
        (((~(lhs ^ rhs) & (lhs ^ result) & sign) != 0));
    auto sr = api.reg(17);
    sr = (sr & ~0x1FU) | (carry ? 0x11U : 0U) |
         (result & sign ? 0x08U : 0U) | (result == 0 ? 0x04U : 0U) |
         (overflow ? 0x02U : 0U);
    api.set_reg(17, sr);
}

void logical_flags(BasicBlockApi& api, std::uint32_t value, unsigned width) {
    const auto mask = width == 1 ? 0xFFU : width == 2 ? 0xFFFFU : 0xFFFFFFFFU;
    const auto sign = width == 1 ? 0x80U : width == 2 ? 0x8000U : 0x80000000U;
    auto sr = api.reg(17);
    sr = (sr & ~0x0FU) | (((value & mask) == 0) ? 0x04U : 0U) |
         (((value & sign) != 0) ? 0x08U : 0U);
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

void move_l_data_to_predecrement_address(BasicBlockApi& api,
                                         unsigned data_register,
                                         unsigned address_register) {
    const auto target = 8U + address_register;
    const auto address = api.reg(target) - 4U;
    api.set_reg(target, address);
    const auto value = api.reg(data_register);
    write(api, (address + 2U) & 0x00FFFFFFU, 2, value >> 16U);
    write(api, address & 0x00FFFFFFU, 2, value & 0xFFFFU);
    move_flags(api, value, 4);
}

void clear_w_data_register(BasicBlockApi& api, unsigned data_register) {
    api.set_reg(data_register, api.reg(data_register) & 0xFFFF0000U);
    move_flags(api, 0, 2);
}

void clear_w_postincrement(BasicBlockApi& api, unsigned address_register) {
    const auto target = 8U + address_register;
    const auto address = api.reg(target);
    write(api, address, 2, 0);
    api.set_reg(target, address + 2U);
    move_flags(api, 0, 2);
}

void clear_b_postincrement(BasicBlockApi& api, unsigned address_register) {
    const auto target = 8U + address_register;
    const auto address = api.reg(target);
    write(api, address, 1, 0);
    api.set_reg(target, address + 1U);
    move_flags(api, 0, 1);
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

void move_b_postincrement_to_postincrement(BasicBlockApi& api,
                                           unsigned source_register,
                                           unsigned destination_register) {
    const auto source = 8U + source_register;
    const auto destination = 8U + destination_register;
    const auto value = read(api, api.reg(source), 1);
    api.set_reg(source, api.reg(source) + 1U);
    write(api, api.reg(destination), 1, value);
    api.set_reg(destination, api.reg(destination) + 1U);
    move_flags(api, value, 1);
}

void move_b_immediate_to_postincrement(BasicBlockApi& api, unsigned immediate,
                                       unsigned address_register) {
    const auto target = 8U + address_register;
    const auto address = api.reg(target);
    write(api, address, 1, immediate & 0xFFU);
    api.set_reg(target, address + 1U);
    move_flags(api, immediate, 1);
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

void add_w_postincrement_to_data_register(BasicBlockApi& api,
                                          unsigned address_register,
                                          unsigned data_register) {
    const auto address = api.reg(8U + address_register);
    const auto rhs = read(api, address, 2) & 0xFFFFU;
    api.set_reg(8U + address_register, address + 2U);
    const auto lhs = api.reg(data_register) & 0xFFFFU;
    const auto result = (lhs + rhs) & 0xFFFFU;
    const auto carry = lhs + rhs > 0xFFFFU;
    const auto overflow = ((~(lhs ^ rhs) & (lhs ^ result)) & 0x8000U) != 0;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFF0000U) | result);
    auto sr = api.reg(17);
    sr = (sr & ~0x1FU) | (carry ? 0x11U : 0U) |
         (result & 0x8000U ? 0x08U : 0U) | (result == 0 ? 0x04U : 0U) |
         (overflow ? 0x02U : 0U);
    api.set_reg(17, sr);
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

void compare_immediate_w_absolute_long(BasicBlockApi& api, unsigned immediate,
                                       std::uint32_t address) {
    const auto value = read(api, address, 2) & 0xFFFFU;
    const auto result = (value - (immediate & 0xFFFFU)) & 0xFFFFU;
    const auto x = api.reg(17) & 0x10U;
    arithmetic_flags(api, value, immediate, result, 2, true);
    api.set_reg(17, api.reg(17) | x);
}

void compare_immediate_b_data(BasicBlockApi& api, unsigned immediate,
                              unsigned data_register) {
    const auto value = api.reg(data_register) & 0xFFU;
    const auto result = (value - (immediate & 0xFFU)) & 0xFFU;
    const auto x = api.reg(17) & 0x10U;
    arithmetic_flags(api, value, immediate, result, 1, true);
    api.set_reg(17, api.reg(17) | x);
}

void bit_test_immediate_absolute_long(BasicBlockApi& api, unsigned immediate,
                                      std::uint32_t address) {
    const auto value = read(api, address, 1);
    auto sr = api.reg(17);
    const auto z = ((value & (1U << (immediate & 7U))) == 0) ? 0x04U : 0U;
    sr = (sr & ~0x04U) | z;
    api.set_reg(17, sr);
}

void bit_test_immediate_displacement_address(BasicBlockApi& api,
                                              unsigned immediate,
                                              unsigned address_register,
                                              int displacement) {
    const auto address = api.reg(8U + address_register) +
        static_cast<std::int32_t>(displacement);
    const auto value = read(api, address, 1);
    auto sr = api.reg(17);
    const auto z = ((value & (1U << (immediate & 7U))) == 0) ? 0x04U : 0U;
    sr = (sr & ~0x04U) | z;
    api.set_reg(17, sr);
}

void bit_test_immediate_data(BasicBlockApi& api, unsigned immediate,
                             unsigned data_register) {
    const auto value = api.reg(data_register);
    auto sr = api.reg(17);
    sr = (sr & ~0x04U) |
         (((value & (1U << (immediate & 31U))) == 0) ? 0x04U : 0U);
    api.set_reg(17, sr);
}

void moveq_data(BasicBlockApi& api, int immediate, unsigned data_register) {
    api.set_reg(data_register, static_cast<std::uint32_t>(immediate));
    move_flags(api, static_cast<std::uint32_t>(immediate), 4);
}

void move_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                         unsigned destination_register) {
    const auto value = api.reg(source_register) & 0xFFFFU;
    api.set_reg(destination_register, (api.reg(destination_register) & 0xFFFF0000U) | value);
    move_flags(api, value, 2);
}

void add_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                        unsigned destination_register) {
    const auto lhs = api.reg(destination_register) & 0xFFFFU;
    const auto rhs = api.reg(source_register) & 0xFFFFU;
    const auto result = (lhs + rhs) & 0xFFFFU;
    api.set_reg(destination_register, (api.reg(destination_register) & 0xFFFF0000U) | result);
    arithmetic_flags(api, lhs, rhs, result, 2, false);
}

void sub_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                        unsigned destination_register) {
    const auto lhs = api.reg(destination_register) & 0xFFFFU;
    const auto rhs = api.reg(source_register) & 0xFFFFU;
    const auto result = (lhs - rhs) & 0xFFFFU;
    api.set_reg(destination_register, (api.reg(destination_register) & 0xFFFF0000U) | result);
    arithmetic_flags(api, lhs, rhs, result, 2, true);
}

void addq_w_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto lhs = api.reg(data_register) & 0xFFFFU;
    const auto rhs = immediate == 0 ? 8U : immediate & 7U;
    const auto result = (lhs + rhs) & 0xFFFFU;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFF0000U) | result);
    arithmetic_flags(api, lhs, rhs, result, 2, false);
}

void subq_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto lhs = api.reg(data_register) & 0xFFU;
    const auto rhs = immediate == 0 ? 8U : immediate & 7U;
    const auto result = (lhs - rhs) & 0xFFU;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFFFF00U) | result);
    arithmetic_flags(api, lhs, rhs, result, 1, true);
}

void subq_w_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto lhs = api.reg(data_register) & 0xFFFFU;
    const auto rhs = immediate == 0 ? 8U : immediate & 7U;
    const auto result = (lhs - rhs) & 0xFFFFU;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFF0000U) | result);
    arithmetic_flags(api, lhs, rhs, result, 2, true);
}

void andi_w_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto result = (api.reg(data_register) & 0xFFFFU) & (immediate & 0xFFFFU);
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFF0000U) | result);
    logical_flags(api, result, 2);
}

void andi_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto result = (api.reg(data_register) & 0xFFU) & (immediate & 0xFFU);
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFFFF00U) | result);
    logical_flags(api, result, 1);
}

void or_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                       unsigned destination_register) {
    const auto result = (api.reg(destination_register) & 0xFFFFU) |
                        (api.reg(source_register) & 0xFFFFU);
    api.set_reg(destination_register, (api.reg(destination_register) & 0xFFFF0000U) | result);
    logical_flags(api, result, 2);
}

void addi_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto lhs = api.reg(data_register) & 0xFFU;
    const auto rhs = immediate & 0xFFU;
    const auto result = (lhs + rhs) & 0xFFU;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFFFF00U) | result);
    arithmetic_flags(api, lhs, rhs, result, 1, false);
}

void subi_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto lhs = api.reg(data_register) & 0xFFU;
    const auto rhs = immediate & 0xFFU;
    const auto result = (lhs - rhs) & 0xFFU;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFFFF00U) | result);
    arithmetic_flags(api, lhs, rhs, result, 1, true);
}

void ror_w_data(BasicBlockApi& api, unsigned count, unsigned data_register) {
    const auto amount = count & 15U;
    auto value = api.reg(data_register) & 0xFFFFU;
    unsigned carry = 0;
    for (unsigned i = 0; i < amount; ++i) {
        carry = value & 1U;
        value = (value >> 1U) | (carry << 15U);
    }
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFF0000U) | value);
    auto sr = api.reg(17);
    sr = (sr & ~0x0FU) | ((value == 0) ? 0x04U : 0U) |
         ((value & 0x8000U) ? 0x08U : 0U) | (carry ? 0x01U : 0U);
    api.set_reg(17, sr);
}

void lsr_w_data(BasicBlockApi& api, unsigned count, unsigned data_register) {
    const auto amount = count & 63U;
    const auto original = api.reg(data_register) & 0xFFFFU;
    const auto result = amount >= 16U ? 0U : original >> amount;
    const auto carry = amount == 0U || amount > 16U ? 0U :
        (original >> (amount - 1U)) & 1U;
    api.set_reg(data_register, (api.reg(data_register) & 0xFFFF0000U) | result);
    auto sr = api.reg(17);
    sr = (sr & ~0x0FU) | ((result == 0) ? 0x04U : 0U) |
         (carry ? 0x11U : 0U);
    api.set_reg(17, sr);
}

void bclr_l_data(BasicBlockApi& api, unsigned immediate, unsigned data_register) {
    const auto bit = immediate & 31U;
    const auto value = api.reg(data_register);
    auto sr = api.reg(17);
    sr = (sr & ~0x04U) |
         (((value & (1U << bit)) == 0) ? 0x04U : 0U);
    api.set_reg(17, sr);
    api.set_reg(data_register, value & ~(1U << bit));
}

void movea_l_address_to_address(BasicBlockApi& api, unsigned source_register,
                                unsigned destination_register) {
    api.set_reg(8U + destination_register, api.reg(8U + source_register));
}

} // namespace oasis::hybrid::generated
