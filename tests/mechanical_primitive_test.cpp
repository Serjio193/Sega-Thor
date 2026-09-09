#include "core/mechanical_primitive.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
using namespace oasis::core;

struct Machine final : MechanicalMachine {
    struct Access { bool read{}; std::uint32_t address{}; unsigned width{}; std::uint32_t value{}; };
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<Access> accesses;
    std::vector<PrimitiveBoundaryReason> boundaries;

    std::uint32_t reg(unsigned index) const override { return regs.at(index); }
    void set_reg(unsigned index, std::uint32_t value) override { regs.at(index) = value; }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < width; ++i) value = (value << 8U) | memory[address + i];
        accesses.push_back({true, address, width, value});
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        accesses.push_back({false, address, width, value});
        for (unsigned i = 0; i < width; ++i)
            memory[address + width - i - 1U] = static_cast<std::uint8_t>(value >> (i * 8U));
    }
    void begin(PrimitiveStep) override {}
    void fetch_dbf_displacement() override {}
    void finish(PrimitiveStep, DbfResult) override {}
    PrimitiveBoundaryReason boundary() const override {
        auto& queue = const_cast<std::vector<PrimitiveBoundaryReason>&>(boundaries);
        if (queue.empty()) return PrimitiveBoundaryReason::CONTINUE_BLOCK;
        const auto result = queue.front();
        queue.erase(queue.begin());
        return result;
    }
};

MechanicalLoopContract clear_byte() {
    return {MechanicalOperation::MEMORY_CLEAR, 0x100U, 0x102U, 0x104U, 0U, 5U, 0U, 0U, 1U};
}
MechanicalLoopContract copy_byte() {
    return {MechanicalOperation::MEMORY_COPY, 0x200U, 0x202U, 0x204U, 0U, 0U, 2U, 1U, 1U};
}
MechanicalLoopContract clear_word() {
    return {MechanicalOperation::MEMORY_CLEAR, 0x300U, 0x302U, 0x304U, 0U, 0U, 0U, 0U, 2U};
}

void clear_is_resumable_and_preserves_ccr_x() {
    Machine machine;
    machine.regs[13] = 0xFFFFFFFEU;
    machine.regs[0] = 0xABCD0001U;
    machine.regs[17] = 0x10U;
    machine.boundaries.push_back(PrimitiveBoundaryReason::EVENT_BOUNDARY);
    PrimitiveContinuation continuation{};
    const auto first = execute_memory_clear(machine, clear_byte(), 0x100U, continuation);
    assert(first.next_token == 0x102U && first.reason == PrimitiveExitReason::EVENT_BOUNDARY);
    assert(first.guest_instructions == 1U && first.iterations == 1U);
    assert(machine.regs[13] == 0xFFFFFFFFU && (machine.regs[17] & 0x1FU) == 0x14U);
    const auto second = execute_memory_clear(machine, clear_byte(), first.next_token, continuation);
    assert(second.next_token == 0x104U && second.reason == PrimitiveExitReason::NORMAL_EXIT);
    assert(second.guest_instructions == 3U && machine.regs[13] == 0U);
    assert(machine.regs[0] == 0xABCDFFFFU);
}

void copy_is_ordered_across_overlap_and_boundaries() {
    Machine machine;
    machine.regs[10] = 0x300U;
    machine.regs[9] = 0x301U;
    machine.regs[0] = 1U;
    machine.regs[17] = 0x10U;
    machine.memory[0x300U] = 0xA5U;
    machine.memory[0x301U] = 0x5AU;
    machine.boundaries.push_back(PrimitiveBoundaryReason::EVENT_BOUNDARY);
    machine.boundaries.push_back(PrimitiveBoundaryReason::INTERRUPT_BOUNDARY);
    PrimitiveContinuation continuation{};
    const auto contract = copy_byte();
    const auto first = execute_mechanical_loop(machine, contract, contract.body_token, continuation);
    const auto second = execute_mechanical_loop(machine, contract, first.next_token, continuation);
    assert(first.next_token == contract.loop_token && first.guest_instructions == 1U);
    assert(second.next_token == contract.body_token && second.reason == PrimitiveExitReason::INTERRUPT_BOUNDARY);
    assert(machine.accesses.size() == 2U && machine.accesses[0].read && !machine.accesses[1].read);
    assert(machine.accesses[0].address == 0x300U && machine.accesses[1].address == 0x301U);
    assert(machine.memory[0x301U] == 0xA5U && machine.regs[10] == 0x301U && machine.regs[9] == 0x302U);
    const auto third = execute_mechanical_loop(machine, contract, second.next_token, continuation);
    assert(third.next_token == contract.continuation_token && third.guest_instructions == 2U);
    assert(machine.memory[0x302U] == 0xA5U && (machine.regs[17] & 0x1FU) == 0x18U);
}

void word_clear_wraps_and_rejects_odd_alignment() {
    Machine machine;
    machine.regs[8] = 0xFFFFFFFEU;
    machine.regs[17] = 0x1BU;
    PrimitiveContinuation continuation{};
    const auto result = execute_mechanical_loop(machine, clear_word(), 0x300U, continuation);
    assert(result.next_token == 0x304U && result.guest_instructions == 2U);
    assert(machine.accesses.size() == 1U && machine.accesses[0].width == 2U);
    assert(machine.accesses[0].address == 0xFFFFFFFEU && machine.regs[8] == 0U);
    assert((machine.regs[17] & 0x1FU) == 0x14U);

    machine = Machine{};
    machine.regs[8] = 0x101U;
    bool rejected = false;
    try { (void)execute_mechanical_loop(machine, clear_word(), 0x300U, continuation); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected && machine.accesses.empty());
}

void invalid_or_repeated_entries_fail_closed() {
    Machine machine;
    PrimitiveContinuation continuation{};
    const auto contract = clear_byte();
    machine.boundaries.push_back(PrimitiveBoundaryReason::EVENT_BOUNDARY);
    (void)execute_mechanical_loop(machine, contract, contract.body_token, continuation);
    bool repeated = false;
    try { (void)execute_mechanical_loop(machine, contract, contract.body_token, continuation); }
    catch (const std::runtime_error&) { repeated = true; }
    assert(repeated);

    auto invalid = contract;
    invalid.width = 4U;
    continuation = {};
    bool rejected = false;
    try { (void)execute_mechanical_loop(machine, invalid, invalid.body_token, continuation); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
} // namespace

int main() {
    clear_is_resumable_and_preserves_ccr_x();
    copy_is_ordered_across_overlap_and_boundaries();
    word_clear_wraps_and_rejects_odd_alignment();
    invalid_or_repeated_entries_fail_closed();
    return 0;
}
