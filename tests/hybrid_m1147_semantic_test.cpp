#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/recomp_generator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

using oasis::hybrid::BasicBlockApi;
using namespace oasis::hybrid::generated;

struct Access {
    char direction{};
    std::uint32_t address{};
    int width{};
    std::uint32_t value{};
    bool operator==(const Access&) const = default;
};

struct Machine {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<Access> accesses;
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
        current->accesses.push_back({'R', address, width, value});
        return value;
    }
    static void write(unsigned address, int width, unsigned value) {
        for (int i = 0; i < width; ++i)
            current->memory[address + static_cast<unsigned>(i)] =
                static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
        current->accesses.push_back({'W', address, width, value});
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

void move_long_predecrement() {
    Machine machine;
    machine.regs[0] = 0x11223344U;
    machine.regs[14] = 0x100U;
    machine.regs[17] = 0xA713U;
    auto api = api_for(machine);
    move_l_data_to_predecrement_address(api, 0, 6);
    check(machine.regs[14] == 0xFCU, "MOVE.L predecrement address mismatch");
    check(machine.memory[0xFC] == 0x33 && machine.memory[0xFD] == 0x44 &&
              machine.memory[0xFE] == 0x11 && machine.memory[0xFF] == 0x22,
          "MOVE.L predecrement byte order mismatch");
    check(machine.accesses == std::vector<Access>{{'W', 0xFE, 2, 0x1122U},
                                                   {'W', 0xFC, 2, 0x3344U}},
          "MOVE.L predecrement bus order mismatch");
    machine.regs[14] = 2U;
    machine.accesses.clear();
    move_l_data_to_predecrement_address(api, 0, 6);
    check(machine.regs[14] == 0xFFFFFFFEU && machine.memory[0xFFFFFEU] == 0x33 &&
              machine.memory[0xFFFFFFU] == 0x44 && machine.memory[0] == 0x11 &&
              machine.memory[1] == 0x22, "MOVE.L address wrap mismatch");
}

void move_byte_copy() {
    Machine machine;
    machine.regs[10] = 0x100U;
    machine.regs[9] = 0x200U;
    machine.regs[17] = 0xA713U;
    machine.memory[0x100] = 0x80U;
    auto api = api_for(machine);
    move_b_postincrement_to_postincrement(api, 2, 1);
    check(machine.regs[10] == 0x101U && machine.regs[9] == 0x201U &&
              machine.memory[0x200] == 0x80U && (machine.regs[17] & 0x1FU) == 0x18U,
          "MOVE.B copy result/flags mismatch");
    check(machine.accesses == std::vector<Access>{{'R', 0x100, 1, 0x80U},
                                                   {'W', 0x200, 1, 0x80U}},
          "MOVE.B copy access order mismatch");

    machine.regs[10] = 0x300U;
    machine.regs[9] = 0x301U;
    machine.memory[0x300] = 0x55U;
    machine.accesses.clear();
    move_b_postincrement_to_postincrement(api, 2, 1);
    check(machine.regs[10] == 0x301U && machine.regs[9] == 0x302U &&
              machine.memory[0x301] == 0x55U,
          "MOVE.B overlap update mismatch");
    check(machine.accesses == std::vector<Access>{{'R', 0x300, 1, 0x55U},
                                                   {'W', 0x301, 1, 0x55U}},
          "MOVE.B overlap access order mismatch");
}

void clear_bit_and_immediate_move() {
    Machine machine;
    machine.regs[13] = 0x400U;
    machine.regs[17] = 0x1FU;
    machine.memory[0x400] = 0xA5U;
    machine.memory[0x401] = 0x5AU;
    auto api = api_for(machine);
    clear_w_postincrement(api, 5);
    check(machine.regs[13] == 0x402U && machine.memory[0x400] == 0 &&
              machine.memory[0x401] == 0 && (machine.regs[17] & 0x1FU) == 0x14U,
          "CLR.W postincrement result/flags mismatch");
    check(machine.accesses == std::vector<Access>{{'W', 0x400, 2, 0}},
          "CLR.W access order mismatch");

    machine.regs[13] = 0x402U;
    machine.memory[0x402] = 0xA5U;
    machine.accesses.clear();
    clear_b_postincrement(api, 5);
    check(machine.regs[13] == 0x403U && machine.memory[0x402] == 0 &&
              (machine.regs[17] & 0x1FU) == 0x14U,
          "CLR.B postincrement result/flags mismatch");
    check(machine.accesses == std::vector<Access>{{'W', 0x402, 1, 0}},
          "CLR.B access order mismatch");

    machine.regs[13] = 0x500U;
    machine.regs[17] = 0x1FU;
    machine.memory[0x504] = 1U;
    machine.accesses.clear();
    bit_test_immediate_displacement_address(api, 0, 5, 4);
    check(machine.regs[17] == 0x1BU && machine.regs[13] == 0x500U,
          "BTST displacement set-bit mismatch");
    check(machine.accesses == std::vector<Access>{{'R', 0x504, 1, 1}},
          "BTST displacement address mismatch");
    machine.memory[0x504] = 0;
    machine.accesses.clear();
    bit_test_immediate_displacement_address(api, 0, 5, 4);
    check((machine.regs[17] & 0x1FU) == 0x1FU, "BTST displacement zero-bit mismatch");

    machine.regs[12] = 0x600U;
    machine.regs[17] = 0x10U;
    machine.accesses.clear();
    move_b_immediate_to_postincrement(api, 0xFFU, 4);
    check(machine.regs[12] == 0x601U && machine.memory[0x600] == 0xFFU &&
              (machine.regs[17] & 0x1FU) == 0x18U,
          "MOVE.B immediate result/flags mismatch");
}

void generator_provenance() {
    std::vector<std::uint8_t> rom(0x100, 0);
    const auto put = [&](unsigned address, std::initializer_list<std::uint8_t> bytes) {
        std::copy(bytes.begin(), bytes.end(), rom.begin() + address);
    };
    put(0x20, {0x2D, 0x00});
    put(0x30, {0x12, 0xDA});
    put(0x40, {0x42, 0x58});
    put(0x50, {0x08, 0x2D, 0x00, 0x00, 0x00, 0x04});
    put(0x60, {0x18, 0xFC, 0x00, 0xFF});
    for (const auto [start, end, helper] : {
             std::tuple{0x20U, 0x22U, "move_l_data_to_predecrement_address"},
             std::tuple{0x30U, 0x32U, "move_b_postincrement_to_postincrement"},
             std::tuple{0x40U, 0x42U, "clear_w_postincrement"},
             std::tuple{0x50U, 0x56U, "bit_test_immediate_displacement_address"},
             std::tuple{0x60U, 0x64U, "move_b_immediate_to_postincrement"}}) {
        const auto block = oasis::hybrid::generate_block(rom, start, end);
        check(block.instructions.size() == 1U, "M11.47 decode count mismatch");
        const auto emitted = oasis::hybrid::emit_translation_unit({block});
        check(emitted.find("guest 0x") != std::string::npos &&
                  emitted.find(helper) != std::string::npos,
              "M11.47 generated helper/provenance mismatch");
    }
}

} // namespace

int main() {
    try {
        move_long_predecrement();
        move_byte_copy();
        clear_bit_and_immediate_move();
        generator_provenance();
        return 0;
    } catch (const std::exception& error) {
        return (std::cerr << error.what() << '\n', 1);
    }
}
