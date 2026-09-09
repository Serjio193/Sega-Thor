#include "tools/hybrid/mechanical_primitive.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
struct Machine final : oasis::hybrid::MechanicalMachine {
    std::array<std::uint32_t, 18> regs{};
    std::map<unsigned, std::uint8_t> memory;
    std::vector<unsigned> writes;
    std::vector<oasis::hybrid::BlockExitReason> boundaries;

    unsigned reg(unsigned index) const override { return regs.at(index); }
    void set_reg(unsigned index, unsigned value) override { regs.at(index) = value; }
    void write(unsigned address, unsigned width, unsigned value) override {
        if (width != 1U || value != 0U) throw std::runtime_error("unexpected clear write");
        memory[address] = 0;
        writes.push_back(address);
    }
    void begin(oasis::hybrid::PrimitiveStep) override {}
    void fetch_dbf_displacement() override {}
    void finish(oasis::hybrid::PrimitiveStep, oasis::hybrid::DbfResult) override {}
    oasis::hybrid::BlockExitReason boundary() const override {
        auto& queue = const_cast<std::vector<oasis::hybrid::BlockExitReason>&>(boundaries);
        if (queue.empty()) return oasis::hybrid::BlockExitReason::CONTINUE_BLOCK;
        const auto result = queue.front();
        queue.erase(queue.begin());
        return result;
    }
};

void mid_operation_resume() {
    Machine machine;
    machine.regs[13] = 0xFFFFFFFEU;
    machine.regs[0] = 1U;
    machine.regs[17] = 0x1FU;
    machine.boundaries.push_back(oasis::hybrid::BlockExitReason::EVENT_BOUNDARY);
    const oasis::hybrid::MemoryClearLoopContract contract{0x100U, 0x102U, 0x104U, 5U, 0U};

    const auto first = oasis::hybrid::execute_memory_clear(machine, contract, contract.body_pc);
    assert(first.next_pc == contract.loop_pc);
    assert(first.reason == oasis::hybrid::BlockExitReason::EVENT_BOUNDARY);
    assert(first.guest_instructions == 1U && first.iterations == 1U);
    assert(machine.writes == std::vector<unsigned>{0xFFFFFFFEU});
    assert(machine.regs[13] == 0xFFFFFFFFU && machine.regs[0] == 1U);
    assert((machine.regs[17] & 0x1FU) == 0x14U);

    const auto second = oasis::hybrid::execute_memory_clear(machine, contract, first.next_pc);
    assert(second.next_pc == contract.continuation_pc);
    assert(second.reason == oasis::hybrid::BlockExitReason::NORMAL_EXIT);
    assert(second.guest_instructions == 3U && second.iterations == 1U);
    const std::vector<unsigned> expected_writes{0xFFFFFFFEU, 0xFFFFFFFFU};
    assert(machine.writes == expected_writes);
    assert(machine.regs[13] == 0U && machine.regs[0] == 0xFFFFU);
}

void zero_underflow_is_bounded_by_boundary() {
    Machine machine;
    machine.regs[13] = 0x200U;
    machine.regs[0] = 0xFFFFU;
    machine.boundaries.push_back(oasis::hybrid::BlockExitReason::INTERRUPT_BOUNDARY);
    const oasis::hybrid::MemoryClearLoopContract contract{0x300U, 0x302U, 0x304U, 5U, 0U};
    const auto result = oasis::hybrid::execute_memory_clear(machine, contract, contract.body_pc);
    assert(result.next_pc == contract.loop_pc);
    assert(result.reason == oasis::hybrid::BlockExitReason::INTERRUPT_BOUNDARY);
    assert(result.iterations == 1U && machine.regs[0] == 0xFFFFU);
}
} // namespace

int main() {
    mid_operation_resume();
    zero_underflow_is_bounded_by_boundary();
    return 0;
}
