#include "core/ram_flag_routine.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {
struct Machine final : oasis::core::RamFlagRoutineMachine {
    std::array<std::uint32_t, 18> regs{};
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<oasis::core::RamFlagRoutineStep> steps;
    std::vector<std::uint32_t> writes;
    unsigned yield_after{};
    bool yielded{};
    void begin(oasis::core::RamFlagRoutineStep step) override { steps.push_back(step); }
    void finish(oasis::core::RamFlagRoutineStep) override {}
    std::uint32_t reg(unsigned index) const override { return regs[index]; }
    void set_reg(unsigned index, std::uint32_t value) override { regs[index] = value; }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < width; ++i) value = (value << 8U) | memory[address + i];
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        writes.push_back(address);
        for (unsigned i = 0; i < width; ++i)
            memory[address + i] = static_cast<std::uint8_t>(value >> (8U * (width - i - 1U)));
    }
    oasis::core::RoutineBoundaryReason boundary() override {
        if (!yielded && yield_after && steps.size() == yield_after) {
            yielded = true;
            return oasis::core::RoutineBoundaryReason::INTERRUPT;
        }
        return oasis::core::RoutineBoundaryReason::CONTINUE;
    }
    void complete_return(std::uint32_t return_pc) override { regs[16] = return_pc; }
};

oasis::core::RamFlagRoutineContract contract() {
    return {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 99,
            0xFF0628, 0xFF06F2, 0xFF0016};
}

void prepare(Machine& machine) {
    machine.regs[13] = 0xFFFFFFFEU;
    machine.regs[14] = 0xAAAAU;
    machine.regs[15] = 0x100U;
    machine.regs[17] = 0x2711U;
    machine.memory[0xFF0628] = 0;
    machine.memory[0xFF06F2] = 0x10;
    machine.memory[0x100] = 0;
    machine.memory[0x101] = 0;
    machine.memory[0x102] = 0x12;
    machine.memory[0x103] = 0x34;
}

void run_complete_and_resume() {
    Machine machine;
    prepare(machine);
    machine.yield_after = 2;
    auto continuation = oasis::core::RoutineContinuation{};
    const auto result = oasis::core::execute_ram_flag_routine(
        machine, contract(), 1, continuation);
    assert(result.reason == oasis::core::RoutineExitReason::INTERRUPT);
    assert(result.next_token == 3 && result.guest_instructions == 2);
    continuation = oasis::core::RoutineContinuation{true, result.next_token};
    const auto resumed = oasis::core::execute_ram_flag_routine(
        machine, contract(), result.next_token, continuation);
    assert(resumed.reason == oasis::core::RoutineExitReason::NORMAL);
    assert(!continuation.active && resumed.guest_instructions == 8);
    assert(machine.memory[0xFF0628] == 0x10 && machine.memory[0xFF06F2] == 0x10);
    assert(machine.memory[3] == 0 && machine.memory[4] == 0 && machine.memory[5] == 0);
    assert(machine.memory[0xFF0016] == 0);
    assert(machine.regs[8] == 6 && machine.regs[14] == 0xFF06F2U);
    assert(machine.regs[15] == 0x104 && machine.regs[16] == 0x1234);
    assert((machine.regs[17] & 0x1FU) == 0x11U);
    assert((machine.writes == std::vector<std::uint32_t>{
        0xFF0628, 0xFF06F2, 3, 4, 5, 0xFF0016}));
}

void run_invalid_contract() {
    Machine machine;
    auto invalid = contract();
    invalid.second_flag_token = invalid.first_flag_token;
    auto continuation = oasis::core::RoutineContinuation{};
    bool rejected = false;
    try { (void)oasis::core::execute_ram_flag_routine(machine, invalid, 1, continuation); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    run_complete_and_resume();
    run_invalid_contract();
}
