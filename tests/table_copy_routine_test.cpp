#include "core/table_copy_routine.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {
using namespace oasis::core;

struct Write {
    std::uint32_t address{};
    unsigned width{};
    std::uint32_t value{};
};

struct Machine final : TableCopyRoutineMachine {
    std::array<std::uint32_t, 18> registers{};
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<Write> writes;
    std::vector<TableCopyStep> steps;
    RoutineBoundaryReason boundary_result{RoutineBoundaryReason::CONTINUE};
    TableCopyStep current_step{};
    bool boundary_pending{};

    std::uint32_t reg(unsigned index) const override { return registers[index]; }
    void set_reg(unsigned index, std::uint32_t value) override { registers[index] = value; }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < width; ++i)
            value = (value << 8U) | memory[address + i];
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        for (unsigned i = 0; i < width; ++i)
            memory[address + i] = static_cast<std::uint8_t>(value >> (8U * (width - i - 1U)));
        writes.push_back({address, width, value & (width == 2U ? 0xFFFFU : 0xFFFFFFFFU)});
    }
    void begin(TableCopyStep step) override { current_step = step; steps.push_back(step); }
    void finish(TableCopyStep) override {}
    RoutineBoundaryReason boundary() override {
        if (boundary_pending && current_step == TableCopyStep::COPY_WORD) {
            boundary_pending = false;
            return boundary_result;
        }
        return RoutineBoundaryReason::CONTINUE;
    }
    void complete_return(std::uint32_t return_pc) override { registers[16] = return_pc; }

    void bytes(std::uint32_t address, std::initializer_list<unsigned char> values) {
        unsigned offset = 0;
        for (const auto value : values) memory[address + offset++] = value;
    }
};

TableCopyRoutineContract contract(std::uint32_t destination_base = 0x2000U) {
    return {0x1000, 0x1002, 0x1004, 0x1006, 0x1008, 0x100A,
            0x100C, 0x100E, 0x1010, 0x1012, 0x1014, destination_base};
}

void seed(Machine& machine, unsigned count) {
    machine.registers[7] = 0xA1B20000U;
    machine.registers[11] = 0x55667788U;
    machine.registers[14] = 0x3000;
    machine.registers[15] = 0x4000;
    machine.registers[16] = 0x1000;
    machine.registers[17] = 0x2711;
    machine.bytes(0x3000, {0x80, static_cast<unsigned char>(count),
                           static_cast<unsigned char>(count == 0U ? 0x00U : 0x12U),
                           static_cast<unsigned char>(count == 0U ? 0x00U : 0x34U),
                           0x80, 0x00, 0x00, 0x01});
    machine.bytes(0x4000, {0x00, 0x00, 0x05, 0x00});
}

void assert_reference(const Machine& machine, unsigned count) {
    assert(machine.registers[7] == 0xA1B20000U);
    assert(machine.registers[11] == 0x55667788U);
    assert(machine.registers[14] == 0x3000U + 2U + 2U * (count + 1U));
    assert(machine.registers[15] == 0x4004U);
    assert(machine.registers[16] == 0x500U);
    const auto expected_ccr = count == 0U ? 0x14U : 0x10U;
    assert((machine.registers[17] & 0x1FU) == expected_ccr);
    for (unsigned i = 0; i < 2U * (count + 1U); ++i)
        assert(machine.memory.at(0x2080U + i) == machine.memory.at(0x3002U + i));
    assert(machine.writes.size() == 4U + count + 1U);
    const std::array<Write, 4> frame{{
        {0x3FFE, 2, 0x7788}, {0x3FFC, 2, 0x5566},
        {0x3FFA, 2, 0x0000}, {0x3FF8, 2, 0xA1B2}}};
    for (unsigned i = 0; i < frame.size(); ++i) {
        assert(machine.writes[i].address == frame[i].address);
        assert(machine.writes[i].width == frame[i].width);
        assert(machine.writes[i].value == frame[i].value);
    }
}

void run_to_return(Machine& machine, RoutineContinuation& continuation,
                   RoutineExitReason expected_first = RoutineExitReason::NORMAL) {
    auto result = execute_table_copy_routine(machine, contract(), 0x1000, continuation);
    assert(result.reason == expected_first);
    while (result.reason != RoutineExitReason::NORMAL || continuation.active) {
        if (!continuation.active) break;
        result = execute_table_copy_routine(machine, contract(), result.next_token, continuation);
    }
    assert(!continuation.active);
    assert(result.next_token == 0x1014);
}

void test_reference_and_zero_count() {
    Machine machine;
    seed(machine, 0);
    RoutineContinuation continuation{};
    run_to_return(machine, continuation);
    assert_reference(machine, 0);
}

void test_reference_and_multi_count() {
    Machine machine;
    seed(machine, 2);
    RoutineContinuation continuation{};
    run_to_return(machine, continuation);
    assert_reference(machine, 2);
}

void test_boundary_resume_does_not_repeat_effect() {
    Machine machine;
    seed(machine, 2);
    machine.boundary_pending = true;
    machine.boundary_result = RoutineBoundaryReason::INTERRUPT;
    RoutineContinuation continuation{};
    auto result = execute_table_copy_routine(machine, contract(), 0x1000, continuation);
    assert(result.reason == RoutineExitReason::INTERRUPT);
    assert(continuation.active && result.next_token == 0x100E);
    const auto writes_after_interrupt = machine.writes.size();
    result = execute_table_copy_routine(machine, contract(), result.next_token, continuation);
    while (continuation.active) {
        result = execute_table_copy_routine(machine, contract(), result.next_token, continuation);
    }
    assert(machine.writes.size() == writes_after_interrupt + 2U);
    assert_reference(machine, 2);
}

void test_address_wrap() {
    Machine machine;
    seed(machine, 2);
    machine.registers[14] = 0xFFFFFFFEU;
    machine.bytes(0xFFFFFFFEU, {0x00, 0x02, 0x12, 0x34, 0x80, 0x00, 0x00, 0x01});
    RoutineContinuation continuation{};
    auto result = execute_table_copy_routine(machine, contract(0xFFFFFFFEU), 0x1000, continuation);
    while (continuation.active)
        result = execute_table_copy_routine(machine, contract(0xFFFFFFFEU), result.next_token, continuation);
    assert(result.reason == RoutineExitReason::NORMAL);
    assert(machine.registers[14] == 0x00000006U);
    assert(machine.memory.at(0xFFFFFFFEU) == 0x12U && machine.memory.at(0xFFFFFFFFU) == 0x34U);
    assert(machine.memory.at(0x00000000U) == 0x80U && machine.memory.at(0x00000001U) == 0x00U);
    assert(machine.memory.at(0x00000002U) == 0x00U && machine.memory.at(0x00000003U) == 0x01U);
    assert(machine.writes[4].address == 0xFFFFFFFEU);
    assert(machine.writes[5].address == 0x00000000U);
    assert(machine.writes[6].address == 0x00000002U);
}

void test_invalid_entry_fails_closed() {
    Machine machine;
    seed(machine, 0);
    RoutineContinuation continuation{};
    bool rejected = false;
    try { (void)execute_table_copy_routine(machine, contract(), 0x1002, continuation); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    test_reference_and_zero_count();
    test_reference_and_multi_count();
    test_boundary_resume_does_not_repeat_effect();
    test_address_wrap();
    test_invalid_entry_fails_closed();
    return 0;
}
