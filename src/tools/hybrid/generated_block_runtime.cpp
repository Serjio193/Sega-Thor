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

} // namespace oasis::hybrid::generated
