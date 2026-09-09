#include "core/parent_suffix.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {
struct Machine final : oasis::core::ParentSuffixMachine {
    std::array<std::uint32_t, 18> regs{};
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<oasis::core::ParentSuffixStep> suffix_steps;
    std::vector<oasis::core::RamFlagRoutineStep> ram_steps;
    std::vector<std::uint32_t> writes;
    unsigned boundary_calls{}, yield_at{};
    void begin_suffix(oasis::core::ParentSuffixStep step) override {
        suffix_steps.push_back(step);
        if (step == oasis::core::ParentSuffixStep::RAM_FLAG_CALL) {
            const auto stack = regs[15] - 4U;
            write(stack, 4, 0xCAFEU);
            regs[15] = stack;
        }
    }
    void finish_suffix(oasis::core::ParentSuffixStep) override {}
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
    void begin(oasis::core::RamFlagRoutineStep step) override { ram_steps.push_back(step); }
    void finish(oasis::core::RamFlagRoutineStep) override {}
    oasis::core::RoutineBoundaryReason boundary() override {
        ++boundary_calls;
        if (yield_at && boundary_calls == yield_at) return oasis::core::RoutineBoundaryReason::EVENT;
        return oasis::core::RoutineBoundaryReason::CONTINUE;
    }
    void complete_return(std::uint32_t return_pc) override { regs[16] = return_pc; }
};

oasis::core::RamFlagRoutineContract ram_contract() {
    return {100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
            0x200, 0x210, 0x240};
}

oasis::core::ParentSuffixContract suffix_contract() {
    return {1, 2, 3, 4, 5, 6, 7, 8, 9, 0x300, {0x310, 0x311, 0x312}, 0x320, 0xA5A5};
}

void prepare(Machine& machine) {
    machine.regs[13] = 0xA5A5;
    machine.regs[15] = 0x400;
    machine.regs[17] = 0x2711;
    machine.memory[0x200] = 0;
    machine.memory[0x210] = 0x10;
    machine.memory[0x400] = 0;
    machine.memory[0x401] = 0;
    machine.memory[0x402] = 0x12;
    machine.memory[0x403] = 0x34;
}

void complete_path() {
    Machine machine;
    prepare(machine);
    oasis::core::ParentSuffixContinuation continuation{};
    const auto result = oasis::core::execute_parent_suffix(
        machine, suffix_contract(), ram_contract(), 1, continuation);
    assert(result.reason == oasis::core::RoutineExitReason::NORMAL);
    if (continuation.active || result.next_token != 9 || result.guest_instructions != 17)
        throw std::runtime_error("complete path mismatch active=" + std::to_string(continuation.active) +
                                 " token=" + std::to_string(result.next_token) +
                                 " instructions=" + std::to_string(result.guest_instructions));
    if (!(machine.writes == std::vector<std::uint32_t>{0x300, 0x3FC, 0x200, 0x210, 0xA5AA,
                                                         0xA5AB, 0xA5AC, 0x240, 0x310,
                                                         0x311, 0x312, 0x320}))
        throw std::runtime_error("write sequence mismatch size=" + std::to_string(machine.writes.size()) +
                                 " first=" + (machine.writes.empty() ? std::string("none") : std::to_string(machine.writes.front())));
    assert(machine.regs[8] == 0xA5AD && machine.regs[15] == 0x400);
}

void resume_all_boundaries() {
    for (unsigned boundary = 1; boundary <= 17; ++boundary) {
        Machine machine;
        prepare(machine);
        machine.yield_at = boundary;
        oasis::core::ParentSuffixContinuation continuation{};
        auto result = oasis::core::execute_parent_suffix(
            machine, suffix_contract(), ram_contract(), 1, continuation);
        if (result.reason != oasis::core::RoutineExitReason::EVENT)
            throw std::runtime_error("missing boundary event at " + std::to_string(boundary) +
                                     " calls=" + std::to_string(machine.boundary_calls) +
                                     " instructions=" + std::to_string(result.guest_instructions));
        const auto writes_before = machine.writes;
        continuation.active = true;
        machine.yield_at = 0;
        result = oasis::core::execute_parent_suffix(
            machine, suffix_contract(), ram_contract(), result.next_token, continuation);
        assert(result.reason == oasis::core::RoutineExitReason::NORMAL);
        assert(!continuation.active && result.next_token == 9);
        assert(machine.writes.size() == 12);
        (void)writes_before;
    }
}

void reject_invalid_entries() {
    Machine machine;
    prepare(machine);
    auto continuation = oasis::core::ParentSuffixContinuation{};
    auto contract = suffix_contract();
    contract.expected_base_address = 0xAAAA;
    bool rejected = false;
    try { (void)oasis::core::execute_parent_suffix(machine, contract, ram_contract(), 1, continuation); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    prepare(machine);
    continuation = {true, 999, {}};
    rejected = false;
    try { (void)oasis::core::execute_parent_suffix(machine, suffix_contract(), ram_contract(), 999, continuation); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
}

int main() {
    complete_path();
    resume_all_boundaries();
    reject_invalid_entries();
}
