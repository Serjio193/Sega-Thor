#include "tools/hybrid/generated_block_runtime.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using oasis::hybrid::BasicBlockApi;
using namespace oasis::hybrid::generated;

struct Machine {
    std::array<unsigned, 18> regs{};
    std::map<unsigned, std::uint8_t> memory;
    static Machine* current;

    static unsigned reg(unsigned index) { return current->regs.at(index); }
    static void set_reg(unsigned index, unsigned value) { current->regs.at(index) = value; }
    static int peek(unsigned address) {
        const auto it = current->memory.find(address);
        return it == current->memory.end() ? 0 : it->second;
    }
    static unsigned read(unsigned address, int width) {
        unsigned value = 0;
        for (int i = 0; i < width; ++i)
            value = (value << 8U) | static_cast<unsigned>(peek(address + i));
        return value;
    }
    static void write(unsigned address, int width, unsigned value) {
        for (int i = 0; i < width; ++i)
            current->memory[address + static_cast<unsigned>(i)] =
                static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
    }
    static unsigned fetch() { return 0; }
    static void no_op(unsigned) {}
    static void no_cycles(int) {}
    static void no_refresh() {}
};

Machine* Machine::current = nullptr;

BasicBlockApi api_for(Machine& machine) {
    Machine::current = &machine;
    return {Machine::reg, Machine::set_reg, Machine::peek, Machine::fetch,
            Machine::read, Machine::write, Machine::no_op, Machine::no_op,
            Machine::no_cycles, Machine::no_refresh, nullptr, nullptr, nullptr,
            nullptr, nullptr};
}

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void check_flags(unsigned actual, unsigned expected, const char* operation) {
    check((actual & 0x1FU) == expected,
          std::string("M11.45 ") + operation + " flags mismatch actual=" +
              std::to_string(actual & 0x1FU) +
              " expected=" + std::to_string(expected));
}

void compare_vectors() {
    Machine machine;
    machine.regs[17] = 0x10U;
    machine.memory[0x100] = 0;
    machine.memory[0x101] = 0x0A;
    auto api = api_for(machine);
    compare_immediate_w_absolute_long(api, 0x0A, 0x100);
    check_flags(machine.regs[17], 0x14U, "CMPI.W equal");
    check(machine.regs[17] & 0x10U, "CMPI changed X");

    machine.memory[0x100] = 0x80;
    machine.memory[0x101] = 0;
    compare_immediate_w_absolute_long(api, 1, 0x100);
    check_flags(machine.regs[17], 0x12U, "CMPI.W negative");

    machine.regs[1] = 0x00000060U;
    machine.regs[17] = 0x10U;
    compare_immediate_b_data(api, 0x60, 1);
    check_flags(machine.regs[17], 0x14U, "CMPI.B equal");
    check(machine.regs[1] == 0x60U, "CMPI.B changed destination");
}

void bit_vectors() {
    Machine machine;
    machine.regs[17] = 0x1BU;
    machine.memory[0x200] = 0x04;
    auto api = api_for(machine);
    bit_test_immediate_absolute_long(api, 2, 0x200);
    check_flags(machine.regs[17], 0x1BU & ~0x04U, "BTST memory set");
    check(Machine::read(0x200, 1) == 0x04U, "BTST vector memory mismatch");
    bit_test_immediate_absolute_long(api, 1, 0x200);
    check_flags(machine.regs[17], (0x1BU & ~0x04U) | 0x04U, "BTST memory clear");

    machine.regs[7] = 0x80000000U;
    machine.regs[17] = 0x1BU;
    bit_test_immediate_data(api, 31, 7);
    check_flags(machine.regs[17], 0x1BU & ~0x04U, "BTST register set");
    bclr_l_data(api, 31, 7);
    check(machine.regs[7] == 0, "BCLR did not clear the selected bit");
    check_flags(machine.regs[17], 0x1BU, "BCLR first");
    bclr_l_data(api, 31, 7);
    check_flags(machine.regs[17], 0x1BU | 0x04U, "BCLR second");
}

void arithmetic_vectors() {
    Machine machine;
    auto api = api_for(machine);
    machine.regs[0] = 0x1234007FU;
    machine.regs[17] = 0x10U;
    subq_b_data(api, 1, 0);
    check(machine.regs[0] == 0x1234007EU, "SUBQ.B result mismatch");
    check_flags(machine.regs[17], 0U, "SUBQ.B");

    machine.regs[7] = 0xAAAA0001U;
    machine.regs[17] = 0x10U;
    lsr_w_data(api, 1, 7);
    check(machine.regs[7] == 0xAAAA0000U, "LSR.W result mismatch");
    check_flags(machine.regs[17], 0x15U, "LSR.W");

    machine.regs[0] = 0xCAFE1234U;
    machine.regs[17] = 0x10U;
    ror_w_data(api, 8, 0);
    check(machine.regs[0] == 0xCAFE3412U, "ROR.W result mismatch");
    check_flags(machine.regs[17], 0x10U, "ROR.W");

    machine.regs[2] = 0xAAAA0001U;
    machine.regs[17] = 0x10U;
    addq_w_data(api, 1, 2);
    check(machine.regs[2] == 0xAAAA0002U, "ADDQ.W result mismatch");
    check_flags(machine.regs[17], 0U, "ADDQ.W");
    machine.regs[6] = 0xAAAA0001U;
    machine.regs[17] = 0x10U;
    subq_w_data(api, 1, 6);
    check(machine.regs[6] == 0xAAAA0000U, "SUBQ.W result mismatch");
    check_flags(machine.regs[17], 0x04U, "SUBQ.W");
}

void data_vectors() {
    Machine machine;
    auto api = api_for(machine);
    machine.regs[0] = 0x12345678U;
    moveq_data(api, -1, 0);
    check(machine.regs[0] == 0xFFFFFFFFU, "MOVEQ sign extension mismatch");
    check_flags(machine.regs[17], 0x08U, "MOVEQ");
    machine.regs[1] = 0xAAAA0000U;
    move_w_data_to_data(api, 0, 1);
    check(machine.regs[1] == 0xAAAAFFFFU, "MOVE.W register result mismatch");
    andi_b_data(api, 0x0FU, 1);
    check(machine.regs[1] == 0xAAAAFF0FU,
          "ANDI.B result mismatch actual=" + std::to_string(machine.regs[1]));
    subi_b_data(api, 1, 1);
    check(machine.regs[1] == 0xAAAAFF0EU, "SUBI.B result mismatch");
    machine.regs[0] = 0x00007FFFU;
    machine.regs[1] = 0xAAAA0001U;
    add_w_data_to_data(api, 1, 0);
    check(machine.regs[0] == 0x00008000U, "ADD.W register result mismatch");
    check_flags(machine.regs[17], 0x0AU, "ADD.W");
    machine.regs[0] = 0x00008000U;
    machine.regs[1] = 0xAAAA0001U;
    sub_w_data_to_data(api, 1, 0);
    check(machine.regs[0] == 0x00007FFFU, "SUB.W register result mismatch");
    check_flags(machine.regs[17], 0x02U, "SUB.W");
    machine.regs[2] = 0xAAAA1234U;
    andi_w_data(api, 0x0F0FU, 2);
    check(machine.regs[2] == 0xAAAA0204U, "ANDI.W result mismatch");
    machine.regs[3] = 0x00001000U;
    machine.regs[4] = 0xAAAA0001U;
    or_w_data_to_data(api, 3, 4);
    check(machine.regs[4] == 0xAAAA1001U, "OR.W result mismatch");
    machine.regs[5] = 0xAAAA00FFU;
    addi_b_data(api, 1, 5);
    check(machine.regs[5] == 0xAAAA0000U, "ADDI.B result mismatch");
    check_flags(machine.regs[17], 0x15U, "ADDI.B");
    machine.regs[10] = 0x12345678U;
    machine.regs[11] = 0xAAAA0001U;
    movea_l_address_to_address(api, 2, 3);
    check(machine.regs[11] == 0x12345678U, "MOVEA.L result mismatch");
}

} // namespace

int main() {
    try {
        compare_vectors();
        bit_vectors();
        arithmetic_vectors();
        data_vectors();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
