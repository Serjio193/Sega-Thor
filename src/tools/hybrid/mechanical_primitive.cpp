#include "tools/hybrid/mechanical_primitive.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kPc = 16U;
constexpr unsigned kSr = 17U;
constexpr unsigned kCcrMask = 0x0FU;
constexpr unsigned kZeroFlag = 0x04U;
constexpr unsigned kInterruptEvent = 1U << 15;
constexpr unsigned kPostEvent = 1U << 14;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int dispatch_result(BlockExitReason reason) {
    switch (reason) {
    case BlockExitReason::CONTINUE_BLOCK:
    case BlockExitReason::NORMAL_EXIT: return 1;
    case BlockExitReason::EVENT_BOUNDARY: return 2;
    case BlockExitReason::INTERRUPT_BOUNDARY: return 3;
    case BlockExitReason::TRACE_BOUNDARY: return 4;
    case BlockExitReason::FALLBACK: return 0;
    }
    return 0;
}

class NativeMachine final : public MechanicalMachine {
public:
    explicit NativeMachine(BasicBlockApi& api) : api_(api) {}
    unsigned reg(unsigned index) const override { return api_.reg(index); }
    void set_reg(unsigned index, unsigned value) override { api_.set_reg(index, value); }
    void write(unsigned address, unsigned width, unsigned value) override {
        require(api_.write != nullptr, "primitive write bridge unavailable");
        api_.write(address, static_cast<int>(width), value);
    }
    void begin(PrimitiveStep step) override {
        require(api_.fetch16 && api_.begin_instruction, "primitive timing bridge unavailable");
        const auto opcode = api_.fetch16();
        const auto expected = step == PrimitiveStep::CLEAR_BYTE_POSTINCREMENT ? 0x421DU : 0x51C8U;
        if (opcode != expected) throw std::runtime_error("primitive canonical opcode mismatch");
        api_.begin_instruction(opcode);
    }
    void fetch_dbf_displacement() override {
        if (api_.fetch16() != 0xFFFCU)
            throw std::runtime_error("primitive canonical DBF displacement mismatch");
    }
    void finish(PrimitiveStep step, DbfResult result) override {
        require(api_.finish_instruction, "primitive timing bridge unavailable");
        const auto opcode = step == PrimitiveStep::CLEAR_BYTE_POSTINCREMENT ? 0x421DU : 0x51C8U;
        api_.finish_instruction(opcode);
        if (step == PrimitiveStep::DBF && api_.add_cycles)
            api_.add_cycles(result == DbfResult::TAKEN ? -14 : 14);
    }
    BlockExitReason boundary() const override {
        return api_.boundary_reason ? api_.boundary_reason() : BlockExitReason::CONTINUE_BLOCK;
    }
private:
    BasicBlockApi& api_;
};

class SimulationMachine final : public MechanicalMachine {
public:
    using Write = MechanicalPrimitiveRegistry::Write;
    SimulationMachine(BasicBlockApi& source,
                      const MechanicalPrimitiveRegistry::Snapshot& initial)
        : source_(source), snapshot_(initial) {}
    unsigned reg(unsigned index) const override { return snapshot_.registers[index]; }
    void set_reg(unsigned index, unsigned value) override { snapshot_.registers[index] = value; }
    void write(unsigned address, unsigned width, unsigned value) override {
        writes_.push_back({address, static_cast<int>(width), value});
    }
    void begin(PrimitiveStep step) override {
        const auto opcode = step == PrimitiveStep::CLEAR_BYTE_POSTINCREMENT ? 0x421DU : 0x51C8U;
        if (fetch16() != opcode) throw std::runtime_error("simulation canonical opcode mismatch");
        const auto period = source_.refresh_period ? source_.refresh_period() : 0U;
        const auto penalty = source_.refresh_penalty ? source_.refresh_penalty() : 0U;
        if (static_cast<std::int32_t>(snapshot_.cycles) >=
            static_cast<std::int32_t>(snapshot_.refresh)) {
            snapshot_.refresh = snapshot_.cycles + period;
            snapshot_.cycles += penalty;
        }
        snapshot_.ir = opcode;
    }
    void fetch_dbf_displacement() override {
        if (fetch16() != 0xFFFCU)
            throw std::runtime_error("simulation canonical DBF displacement mismatch");
    }
    void finish(PrimitiveStep step, DbfResult result) override {
        const auto opcode = step == PrimitiveStep::CLEAR_BYTE_POSTINCREMENT ? 0x421DU : 0x51C8U;
        snapshot_.ir = opcode;
        snapshot_.cycles += source_.instruction_cycles ? source_.instruction_cycles(opcode) : 0U;
        if (step == PrimitiveStep::DBF)
            snapshot_.cycles = static_cast<unsigned>(static_cast<std::int64_t>(snapshot_.cycles) +
                (result == DbfResult::TAKEN ? -14 : 14));
    }
    BlockExitReason boundary() const override { return BlockExitReason::EVENT_BOUNDARY; }
    const auto& snapshot() const { return snapshot_; }
    const auto& writes() const { return writes_; }
private:
    unsigned fetch16() {
        const auto read_word = [&](unsigned address) {
            unsigned value = 0;
            for (unsigned i = 0; i < 2; ++i) {
                const auto byte = source_.peek(address + i);
                if (byte < 0 || byte > 0xFF) throw std::runtime_error("primitive prefetch failed");
                value = (value << 8U) | static_cast<unsigned>(byte);
            }
            return value;
        };
        if (snapshot_.registers[kPc] != snapshot_.pref_addr)
            snapshot_.pref_data = read_word(snapshot_.registers[kPc]);
        const auto value = snapshot_.pref_data;
        snapshot_.registers[kPc] += 2U;
        snapshot_.pref_addr = snapshot_.registers[kPc];
        snapshot_.pref_data = read_word(snapshot_.pref_addr);
        return value;
    }
    BasicBlockApi& source_;
    MechanicalPrimitiveRegistry::Snapshot snapshot_{};
    std::vector<Write> writes_;
};

} // namespace

PrimitiveExit execute_memory_clear(MechanicalMachine& machine,
                                   const MemoryClearLoopContract& contract,
                                   unsigned entry_pc) {
    require(entry_pc == contract.body_pc || entry_pc == contract.loop_pc,
            "primitive entry outside registered loop");
    PrimitiveExit exit{entry_pc, BlockExitReason::NORMAL_EXIT, 0, 0};
    auto pc = entry_pc;
    for (;;) {
        if (pc == contract.body_pc) {
            machine.begin(PrimitiveStep::CLEAR_BYTE_POSTINCREMENT);
            const auto address_index = 8U + contract.address_register;
            const auto address = machine.reg(address_index);
            machine.write(address, 1U, 0U);
            machine.set_reg(address_index, address + 1U);
            machine.set_reg(kSr, (machine.reg(kSr) & ~kCcrMask) | kZeroFlag);
            machine.set_reg(kPc, contract.loop_pc);
            machine.finish(PrimitiveStep::CLEAR_BYTE_POSTINCREMENT, DbfResult::FALLTHROUGH);
            ++exit.guest_instructions;
            ++exit.iterations;
            const auto reason = machine.boundary();
            if (reason != BlockExitReason::CONTINUE_BLOCK)
                return {contract.loop_pc, reason, exit.guest_instructions, exit.iterations};
            pc = contract.loop_pc;
        }

        machine.begin(PrimitiveStep::DBF);
        const auto value = machine.reg(contract.counter_register);
        const auto result = (value - 1U) & 0xFFFFU;
        machine.set_reg(contract.counter_register, (value & 0xFFFF0000U) | result);
        const auto taken = result != 0xFFFFU;
        if (taken) machine.fetch_dbf_displacement();
        machine.set_reg(kPc, taken ? contract.body_pc : contract.continuation_pc);
        machine.finish(PrimitiveStep::DBF, taken ? DbfResult::TAKEN : DbfResult::FALLTHROUGH);
        ++exit.guest_instructions;
        const auto reason = machine.boundary();
        if (reason != BlockExitReason::CONTINUE_BLOCK)
            return {machine.reg(kPc), reason, exit.guest_instructions, exit.iterations};
        if (!taken) return {contract.continuation_pc, BlockExitReason::NORMAL_EXIT,
                            exit.guest_instructions, exit.iterations};
        pc = contract.body_pc;
    }
}

MechanicalPrimitiveRegistry::MechanicalPrimitiveRegistry(BasicBlockApi api,
                                                         BasicBlockMode mode,
                                                         std::ostream& log)
    : api_(api), mode_(mode), log_(log) {}

bool MechanicalPrimitiveRegistry::is_loop_pc(unsigned pc) const {
    return pc == contract_.body_pc || pc == contract_.loop_pc;
}

bool MechanicalPrimitiveRegistry::handles(unsigned pc) const { return is_loop_pc(pc); }

MechanicalPrimitiveRegistry::Snapshot MechanicalPrimitiveRegistry::capture() const {
    Snapshot result{};
    for (unsigned i = 0; i < 18; ++i) result.registers[i] = api_.reg(i);
    const auto field = [&](unsigned i) { return api_.cpu_field ? api_.cpu_field(i) : 0U; };
    result.ir = field(18); result.pref_addr = field(19); result.pref_data = field(20);
    result.cycles = field(21); result.refresh = field(22); result.cycle_end = field(23);
    result.interrupt_level = field(24); result.interrupt_mask = field(25);
    result.tracing = field(26); result.stopped = field(27);
    return result;
}

void MechanicalPrimitiveRegistry::fail(const std::string& message) {
    ++metrics_.divergences;
    error_ = "FIRST_DIVERGENCE mechanical_primitive " + message;
    active_ = false;
}

void MechanicalPrimitiveRegistry::start_shadow(unsigned pc) {
    if (!active_) {
        if (pc != contract_.body_pc) throw std::runtime_error("primitive loop resumed without body entry");
        ++metrics_.invocations;
    } else if (pc != expected_pc_) {
        throw std::runtime_error("primitive continuation PC mismatch");
    } else {
        if (shadow_resume_pending_) {
            ++metrics_.resumptions;
            shadow_resume_pending_ = false;
        }
    }
    expected_ = capture();
    writes_.clear();
    SimulationMachine simulation(api_, expected_);
    const auto result = execute_memory_clear(simulation, contract_, pc);
    expected_ = simulation.snapshot();
    expected_writes_.assign(simulation.writes().begin(), simulation.writes().end());
    expected_pc_ = result.next_pc;
    pending_shadow_step_ = true;
    active_ = true;
    metrics_.iterations += result.iterations;
    ++metrics_.shadow_comparisons;
}

void MechanicalPrimitiveRegistry::compare_shadow() {
    const auto actual = capture();
    for (unsigned i = 0; i < 18; ++i)
        if (actual.registers[i] != expected_.registers[i])
            throw std::runtime_error("register mismatch");
    if (actual.ir != expected_.ir) {
        std::ostringstream out; out << "IR mismatch actual=0x" << std::hex << actual.ir
                                    << " expected=0x" << expected_.ir; throw std::runtime_error(out.str());
    }
    if (actual.cycles != expected_.cycles || actual.refresh != expected_.refresh) {
        std::ostringstream out; out << "timing mismatch actual=" << std::dec << actual.cycles
                                    << "/" << actual.refresh << " expected=" << expected_.cycles
                                    << "/" << expected_.refresh; throw std::runtime_error(out.str());
    }
    require(actual.cycle_end == expected_.cycle_end &&
            actual.interrupt_level == expected_.interrupt_level &&
            actual.interrupt_mask == expected_.interrupt_mask &&
            actual.tracing == expected_.tracing && actual.stopped == expected_.stopped,
            "boundary state mismatch");
    if (writes_.size() != expected_writes_.size())
        throw std::runtime_error("memory write count mismatch");
    for (std::size_t i = 0; i < writes_.size(); ++i)
        if (writes_[i].address != expected_writes_[i].address ||
            writes_[i].width != expected_writes_[i].width ||
            writes_[i].value != expected_writes_[i].value)
            throw std::runtime_error("memory write mismatch");
    pending_shadow_step_ = false;
    const auto yielded = api_.boundary_reason &&
        api_.boundary_reason() != BlockExitReason::CONTINUE_BLOCK;
    if (yielded && is_loop_pc(expected_pc_)) {
        ++metrics_.mid_operation_yields;
        shadow_resume_pending_ = true;
    }
    active_ = expected_pc_ == contract_.body_pc || expected_pc_ == contract_.loop_pc;
}

void MechanicalPrimitiveRegistry::record_write(int width, unsigned address, unsigned value) {
    const auto bus_address = address & 0xFFFFFFU;
    if (bus_address >= 0xA00000U && bus_address < 0xFF0000U)
        ++metrics_.interrupts;
    if (width == 1 && value == 0U && bus_address >= 0xFF0000U) {
        ++metrics_.bytes_written;
        metrics_.min_address = std::min(metrics_.min_address, bus_address);
        metrics_.max_address = std::max(metrics_.max_address, bus_address);
    }
    if (pending_shadow_step_) writes_.push_back({bus_address, width, value});
}

int MechanicalPrimitiveRegistry::dispatch(unsigned pc) noexcept {
    if (!handles(pc) || !error_.empty()) return 0;
    try {
        if (mode_ == BasicBlockMode::SHADOW_NATIVE) return 0;
        const auto was_active = active_;
        if (!was_active && pc != contract_.body_pc)
            throw std::runtime_error("native primitive loop entry without active invocation");
        if (!was_active) ++metrics_.invocations;
        else ++metrics_.resumptions;
        active_ = true;
        NativeMachine machine(api_);
        const auto result = execute_memory_clear(machine, contract_, pc);
        ++metrics_.dispatches;
        metrics_.iterations += result.iterations;
        if (result.reason != BlockExitReason::NORMAL_EXIT && is_loop_pc(result.next_pc))
            ++metrics_.mid_operation_yields;
        active_ = is_loop_pc(result.next_pc);
        log_ << "{\"primitive\":\"MEMORY_CLEAR\",\"entry\":\"0x" << std::hex << pc
             << "\",\"iterations\":" << std::dec << result.iterations
             << ",\"reason\":" << static_cast<unsigned>(result.reason) << "}\n";
        return dispatch_result(result.reason);
    } catch (const std::exception& error) {
        fail(error.what());
        return 0;
    }
}

void MechanicalPrimitiveRegistry::event(int type, int width, unsigned address,
                                         unsigned value) noexcept {
    if (!error_.empty()) return;
    try {
        if (type == kInterruptEvent) { if (active_) ++metrics_.interrupts; return; }
        if (type == kPostEvent) {
            if (mode_ == BasicBlockMode::SHADOW_NATIVE && pending_shadow_step_) compare_shadow();
            return;
        }
        if (type == 4 && active_) record_write(width, address, value);
        if (type == 1 && mode_ == BasicBlockMode::SHADOW_NATIVE && handles(address))
            start_shadow(address);
    } catch (const std::exception& error) { fail(error.what()); }
}

} // namespace oasis::hybrid
