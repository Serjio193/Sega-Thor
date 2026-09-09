#include "tools/hybrid/mechanical_primitive.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
using namespace oasis::hybrid;

struct Machine final : MechanicalMachine {
    struct Access { bool read{}; unsigned address{}; unsigned width{}; unsigned value{}; };
    std::array<std::uint32_t, 18> regs{};
    std::map<unsigned, std::uint8_t> memory;
    std::vector<Access> accesses;
    std::vector<BlockExitReason> boundaries;

    unsigned reg(unsigned index) const override { return regs.at(index); }
    void set_reg(unsigned index, unsigned value) override { regs.at(index) = value; }
    unsigned read(unsigned address, unsigned width) override {
        unsigned value = 0;
        for (unsigned i = 0; i < width; ++i) value = (value << 8U) | memory[address + i];
        accesses.push_back({true, address, width, value});
        return value;
    }
    void write(unsigned address, unsigned width, unsigned value) override {
        accesses.push_back({false, address, width, value});
        for (unsigned i = 0; i < width; ++i)
            memory[address + width - i - 1U] = static_cast<std::uint8_t>(value >> (i * 8U));
    }
    void begin(PrimitiveStep, unsigned) override {}
    void fetch_dbf_displacement(unsigned) override {}
    void finish(PrimitiveStep, DbfResult, unsigned) override {}
    BlockExitReason boundary() const override {
        auto& queue = const_cast<std::vector<BlockExitReason>&>(boundaries);
        if (queue.empty()) return BlockExitReason::CONTINUE_BLOCK;
        const auto result = queue.front();
        queue.erase(queue.begin());
        return result;
    }
};

MechanicalLoopContract clear_byte(unsigned body = 0x100U) {
    return {"synthetic-clear-byte", MechanicalOperation::MEMORY_CLEAR, body, body + 2U,
            body + 4U, 0x421DU, 0x51C8U, 0xFFFCU, 0U, 5U, 0U, 0U, 1U};
}
MechanicalLoopContract copy_byte(unsigned body = 0x200U) {
    return {"synthetic-copy-byte", MechanicalOperation::MEMORY_COPY, body, body + 2U,
            body + 4U, 0x12DAU, 0x51C8U, 0xFFFCU, 0U, 0U, 2U, 1U, 1U};
}
MechanicalLoopContract clear_word(unsigned body = 0x300U) {
    return {"synthetic-clear-word", MechanicalOperation::MEMORY_CLEAR, body, body + 2U,
            body + 4U, 0x4258U, 0x51C8U, 0xFFFCU, 0U, 0U, 0U, 0U, 2U};
}

void resumable_clear_preserves_upper_counter() {
    Machine machine;
    machine.regs[13] = 0xFFFFFFFEU;
    machine.regs[0] = 0xABCD0001U;
    machine.regs[17] = 0x1FU;
    machine.boundaries.push_back(BlockExitReason::EVENT_BOUNDARY);
    const auto contract = clear_byte();
    const auto first = execute_memory_clear(machine, contract, contract.body_pc);
    assert(first.next_pc == contract.loop_pc && first.reason == BlockExitReason::EVENT_BOUNDARY);
    assert(first.guest_instructions == 1U && first.iterations == 1U);
    assert(machine.accesses[0].address == 0xFFFFFFFEU && machine.accesses[0].width == 1U);
    assert(machine.regs[13] == 0xFFFFFFFFU && machine.regs[0] == 0xABCD0001U);
    assert((machine.regs[17] & 0x1FU) == 0x14U);

    const auto second = execute_memory_clear(machine, contract, first.next_pc);
    assert(second.next_pc == contract.continuation_pc && second.reason == BlockExitReason::NORMAL_EXIT);
    assert(second.guest_instructions == 3U && second.iterations == 1U);
    assert(machine.regs[13] == 0U && machine.regs[0] == 0xABCDFFFFU);
}

void copy_is_ordered_and_resumable() {
    Machine machine;
    machine.regs[10] = 0x300U;
    machine.regs[9] = 0x301U;
    machine.regs[0] = 1U;
    machine.memory[0x300U] = 0xA5U;
    machine.memory[0x301U] = 0x5AU;
    machine.boundaries.push_back(BlockExitReason::EVENT_BOUNDARY);
    machine.boundaries.push_back(BlockExitReason::INTERRUPT_BOUNDARY);
    const auto contract = copy_byte();
    const auto first = execute_mechanical_loop(machine, contract, contract.body_pc);
    assert(first.next_pc == contract.loop_pc && first.guest_instructions == 1U);
    const auto second = execute_mechanical_loop(machine, contract, first.next_pc);
    assert(second.next_pc == contract.body_pc && second.reason == BlockExitReason::INTERRUPT_BOUNDARY);
    assert(machine.accesses.size() == 2U && machine.accesses[0].read && !machine.accesses[1].read);
    assert(machine.accesses[0].address == 0x300U && machine.accesses[1].address == 0x301U);
    assert(machine.memory[0x301U] == 0xA5U && machine.regs[10] == 0x301U && machine.regs[9] == 0x302U);
    const auto third = execute_mechanical_loop(machine, contract, second.next_pc);
    assert(third.next_pc == contract.continuation_pc && third.guest_instructions == 2U);
    assert(machine.accesses.size() == 4U && machine.accesses[2].read && !machine.accesses[3].read);
    assert(machine.accesses[2].address == 0x301U && machine.accesses[3].address == 0x302U);
    assert(machine.memory[0x302U] == 0xA5U && machine.regs[10] == 0x302U && machine.regs[9] == 0x303U);
    assert((machine.regs[17] & 0x1FU) == 0x08U);
}

void word_clear_has_exact_width_and_wrap() {
    Machine machine;
    machine.regs[8] = 0xFFFFFFFEU;
    machine.regs[0] = 0U;
    machine.regs[17] = 0x1BU;
    const auto result = execute_mechanical_loop(machine, clear_word(), 0x300U);
    assert(result.next_pc == 0x304U && result.guest_instructions == 2U);
    assert(machine.accesses.size() == 1U && machine.accesses[0].width == 2U);
    assert(machine.accesses[0].address == 0xFFFFFFFEU && machine.regs[8] == 0U);
    assert((machine.regs[17] & 0x1FU) == 0x14U);
}

void odd_word_address_fails_closed() {
    Machine machine;
    machine.regs[8] = 0x101U;
    bool rejected = false;
    try { (void)execute_mechanical_loop(machine, clear_word(), 0x300U); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected && machine.accesses.empty());
}

void registry_metadata_covers_all_proven_forms() {
    std::ostringstream log;
    MechanicalPrimitiveRegistry registry({}, BasicBlockMode::NATIVE_OVERRIDE, log);
    const auto contracts = registry.contracts();
    assert(contracts.size() == 4U);
    assert(registry.handles(0x003A0CU) && registry.handles(0x0038A0U));
    assert(contracts[0].source_register == 2U && contracts[0].destination_register == 1U);
    assert(contracts[1].counter_register == 0U && contracts[2].width == 2U);
    assert(contracts[3].body_opcode == 0x421DU && contracts[3].continuation_pc == 0x06126CU);
}

void unsupported_form_fails_closed() {
    Machine machine;
    auto contract = copy_byte();
    contract.width = 2U;
    bool rejected = false;
    try { (void)execute_mechanical_loop(machine, contract, contract.body_pc); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
} // namespace

int main() {
    resumable_clear_preserves_upper_counter();
    copy_is_ordered_and_resumable();
    word_clear_has_exact_width_and_wrap();
    odd_word_address_fails_closed();
    registry_metadata_covers_all_proven_forms();
    unsupported_form_fails_closed();
    return 0;
}
