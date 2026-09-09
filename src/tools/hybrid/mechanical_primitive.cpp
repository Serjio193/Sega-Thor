#include "tools/hybrid/mechanical_primitive.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
using core::DbfResult;
using core::PrimitiveBoundaryReason;
using core::PrimitiveStep;

constexpr unsigned kPc = 16U;
constexpr unsigned kInterruptEvent = 1U << 15;
constexpr unsigned kPostEvent = 1U << 14;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int dispatch_result(core::PrimitiveExitReason reason) {
    switch (reason) {
    case core::PrimitiveExitReason::NORMAL_EXIT: return 1;
    case core::PrimitiveExitReason::EVENT_BOUNDARY: return 2;
    case core::PrimitiveExitReason::INTERRUPT_BOUNDARY: return 3;
    case core::PrimitiveExitReason::TRACE_BOUNDARY: return 4;
    case core::PrimitiveExitReason::FALLBACK: return 0;
    }
    return 0;
}

PrimitiveBoundaryReason portable_boundary(BlockExitReason reason) {
    switch (reason) {
    case BlockExitReason::CONTINUE_BLOCK: return PrimitiveBoundaryReason::CONTINUE_BLOCK;
    case BlockExitReason::EVENT_BOUNDARY: return PrimitiveBoundaryReason::EVENT_BOUNDARY;
    case BlockExitReason::INTERRUPT_BOUNDARY: return PrimitiveBoundaryReason::INTERRUPT_BOUNDARY;
    case BlockExitReason::TRACE_BOUNDARY: return PrimitiveBoundaryReason::TRACE_BOUNDARY;
    case BlockExitReason::FALLBACK: return PrimitiveBoundaryReason::FALLBACK;
    }
    return PrimitiveBoundaryReason::FALLBACK;
}

class NativeMachine final : public core::MechanicalMachine {
public:
    NativeMachine(BasicBlockApi& api, const MechanicalLoopContract& contract)
        : api_(api), contract_(contract) {}
    std::uint32_t reg(unsigned index) const override { return api_.reg(index); }
    void set_reg(unsigned index, std::uint32_t value) override { api_.set_reg(index, value); }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        require(api_.read != nullptr, "primitive read bridge unavailable");
        return api_.read(address, static_cast<int>(width));
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        require(api_.write != nullptr, "primitive write bridge unavailable");
        api_.write(address, static_cast<int>(width), value);
    }
    void begin(PrimitiveStep step) override {
        require(api_.fetch16 && api_.begin_instruction, "primitive timing bridge unavailable");
        const auto expected_opcode = step == PrimitiveStep::BODY ? contract_.body_opcode : contract_.dbf_opcode;
        if (api_.fetch16() != expected_opcode) throw std::runtime_error("primitive canonical opcode mismatch");
        api_.begin_instruction(expected_opcode);
    }
    void fetch_dbf_displacement() override {
        if (api_.fetch16() != contract_.dbf_displacement)
            throw std::runtime_error("primitive canonical DBF displacement mismatch");
    }
    void finish(PrimitiveStep step, DbfResult result) override {
        require(api_.finish_instruction, "primitive timing bridge unavailable");
        const auto opcode = step == PrimitiveStep::BODY ? contract_.body_opcode : contract_.dbf_opcode;
        api_.finish_instruction(opcode);
        if (step == PrimitiveStep::DBF && api_.add_cycles)
            api_.add_cycles(result == DbfResult::TAKEN ? -14 : 14);
    }
    PrimitiveBoundaryReason boundary() const override {
        return portable_boundary(api_.boundary_reason ? api_.boundary_reason() :
                                  BlockExitReason::CONTINUE_BLOCK);
    }
private:
    BasicBlockApi& api_;
    const MechanicalLoopContract& contract_;
};

class SimulationMachine final : public core::MechanicalMachine {
public:
    SimulationMachine(BasicBlockApi& source, const MechanicalLoopContract& contract,
                      const MechanicalPrimitiveRegistry::Snapshot& initial)
        : source_(source), contract_(contract), snapshot_(initial) {}
    std::uint32_t reg(unsigned index) const override { return snapshot_.registers[index]; }
    void set_reg(unsigned index, std::uint32_t value) override { snapshot_.registers[index] = value; }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        reads_.push_back({address & 0xFFFFFFU, static_cast<int>(width)});
        unsigned value = 0;
        for (unsigned offset = 0; offset < width; ++offset) {
            const auto byte = source_.peek(address + offset);
            if (byte < 0 || byte > 0xFF) throw std::runtime_error("primitive simulation read failed");
            value = (value << 8U) | static_cast<unsigned>(byte);
        }
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        writes_.push_back({address & 0xFFFFFFU, static_cast<int>(width), value});
    }
    void begin(PrimitiveStep step) override {
        const auto expected_opcode = step == PrimitiveStep::BODY ? contract_.body_opcode : contract_.dbf_opcode;
        if (fetch16() != expected_opcode) throw std::runtime_error("simulation canonical opcode mismatch");
        const auto period = source_.refresh_period ? source_.refresh_period() : 0U;
        const auto penalty = source_.refresh_penalty ? source_.refresh_penalty() : 0U;
        if (static_cast<std::int32_t>(snapshot_.cycles) >= static_cast<std::int32_t>(snapshot_.refresh)) {
            snapshot_.refresh = snapshot_.cycles + period;
            snapshot_.cycles += penalty;
        }
        snapshot_.ir = expected_opcode;
    }
    void fetch_dbf_displacement() override {
        if (fetch16() != contract_.dbf_displacement)
            throw std::runtime_error("simulation canonical DBF displacement mismatch");
    }
    void finish(PrimitiveStep step, DbfResult result) override {
        const auto opcode = step == PrimitiveStep::BODY ? contract_.body_opcode : contract_.dbf_opcode;
        snapshot_.ir = opcode;
        snapshot_.cycles += source_.instruction_cycles ? source_.instruction_cycles(opcode) : 0U;
        if (step == PrimitiveStep::DBF)
            snapshot_.cycles = static_cast<unsigned>(static_cast<std::int64_t>(snapshot_.cycles) +
                (result == DbfResult::TAKEN ? -14 : 14));
    }
    PrimitiveBoundaryReason boundary() const override { return PrimitiveBoundaryReason::EVENT_BOUNDARY; }
    const auto& snapshot() const { return snapshot_; }
    const auto& reads() const { return reads_; }
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
    const MechanicalLoopContract& contract_;
    MechanicalPrimitiveRegistry::Snapshot snapshot_{};
    std::vector<MechanicalPrimitiveRegistry::Read> reads_;
    std::vector<MechanicalPrimitiveRegistry::Write> writes_;
};
} // namespace

MechanicalPrimitiveRegistry::MechanicalPrimitiveRegistry(BasicBlockApi api, BasicBlockMode mode,
                                                         std::ostream& log)
    : api_(api), mode_(mode), log_(log), contracts_{
        {"MEMORY_COPY_003A0C", MechanicalOperation::MEMORY_COPY, 0x003A0CU, 0x003A0EU,
         0x003A12U, 0x12DAU, 0x51CAU, 0xFFFCU, 2U, 0U, 2U, 1U, 1U},
        {"MEMORY_COPY_00389E", MechanicalOperation::MEMORY_COPY, 0x00389EU, 0x0038A0U,
         0x0038A4U, 0x12DAU, 0x51C8U, 0xFFFCU, 0U, 0U, 2U, 1U, 1U},
        {"MEMORY_CLEAR_WORD_0003F0", MechanicalOperation::MEMORY_CLEAR, 0x0003F0U,
         0x0003F2U, 0x0003F6U, 0x4258U, 0x51C8U, 0xFFFCU, 0U, 0U, 0U, 0U, 2U},
        {"MEMORY_CLEAR_BYTE_061266", MechanicalOperation::MEMORY_CLEAR, 0x061266U,
         0x061268U, 0x06126CU, 0x421DU, 0x51C8U, 0xFFFCU, 0U, 5U, 0U, 0U, 1U}
      }, candidate_metrics_(4U) {}

const MechanicalLoopContract* MechanicalPrimitiveRegistry::find(unsigned pc) const {
    for (const auto& contract : contracts_)
        if (pc == contract.body_pc || pc == contract.loop_pc) return &contract;
    return nullptr;
}
bool MechanicalPrimitiveRegistry::handles(unsigned pc) const { return find(pc) != nullptr; }
MechanicalPrimitiveMetrics& MechanicalPrimitiveRegistry::current_metrics() {
    return candidate_metrics_.at(static_cast<std::size_t>(active_contract_ - contracts_.data()));
}
const MechanicalPrimitiveMetrics& MechanicalPrimitiveRegistry::current_metrics() const {
    return candidate_metrics_.at(static_cast<std::size_t>(active_contract_ - contracts_.data()));
}
void MechanicalPrimitiveRegistry::add_metric(unsigned MechanicalPrimitiveMetrics::*field) {
    ++(metrics_.*field);
    ++(current_metrics().*field);
}
void MechanicalPrimitiveRegistry::record_invocation(const MechanicalLoopContract& contract) {
    active_contract_ = &contract;
    add_metric(&MechanicalPrimitiveMetrics::invocations);
    const auto counter = api_.reg(contract.counter_register);
    auto& metrics = current_metrics();
    metrics.initial_counter_min = std::min(metrics.initial_counter_min, counter);
    metrics.initial_counter_max = std::max(metrics.initial_counter_max, counter);
}

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
    if (active_contract_) ++current_metrics().divergences;
    error_ = "FIRST_DIVERGENCE mechanical_primitive " + message;
    active_ = false;
}
void MechanicalPrimitiveRegistry::start_shadow(unsigned pc) {
    const auto* contract = find(pc);
    require(contract != nullptr, "shadow entry is not registered");
    if (!active_) {
        if (pc != contract->body_pc) throw std::runtime_error("primitive loop resumed without body entry");
        record_invocation(*contract);
    } else if (contract != active_contract_ || pc != expected_pc_) {
        throw std::runtime_error("primitive continuation PC mismatch");
    } else if (shadow_resume_pending_) {
        add_metric(&MechanicalPrimitiveMetrics::resumptions);
        shadow_resume_pending_ = false;
    }
    expected_ = capture();
    reads_.clear(); writes_.clear();
    SimulationMachine simulation(api_, *contract, expected_);
    const auto result = core::execute_mechanical_loop(
        simulation, contract->portable(), pc, continuation_);
    expected_ = simulation.snapshot();
    expected_reads_.assign(simulation.reads().begin(), simulation.reads().end());
    expected_writes_.assign(simulation.writes().begin(), simulation.writes().end());
    expected_pc_ = result.next_token;
    pending_shadow_step_ = true;
    active_ = true;
    metrics_.iterations += result.iterations;
    current_metrics().iterations += result.iterations;
    metrics_.boundary_yields += result.boundary_yields;
    current_metrics().boundary_yields += result.boundary_yields;
    add_metric(&MechanicalPrimitiveMetrics::shadow_comparisons);
}
void MechanicalPrimitiveRegistry::compare_shadow() {
    const auto actual = capture();
    for (unsigned i = 0; i < 18; ++i)
        if (actual.registers[i] != expected_.registers[i]) throw std::runtime_error("register mismatch");
    require(actual.ir == expected_.ir, "IR mismatch");
    require(actual.cycles == expected_.cycles && actual.refresh == expected_.refresh, "timing mismatch");
    require(actual.cycle_end == expected_.cycle_end && actual.interrupt_level == expected_.interrupt_level &&
            actual.interrupt_mask == expected_.interrupt_mask && actual.tracing == expected_.tracing &&
            actual.stopped == expected_.stopped, "boundary state mismatch");
    require(reads_.size() == expected_reads_.size() && writes_.size() == expected_writes_.size(),
            "memory access count mismatch");
    for (std::size_t i = 0; i < reads_.size(); ++i)
        require(reads_[i].address == expected_reads_[i].address && reads_[i].width == expected_reads_[i].width,
                "memory read mismatch");
    for (std::size_t i = 0; i < writes_.size(); ++i)
        require(writes_[i].address == expected_writes_[i].address && writes_[i].width == expected_writes_[i].width &&
                writes_[i].value == expected_writes_[i].value, "memory write mismatch");
    pending_shadow_step_ = false;
    const auto yielded = api_.boundary_reason && api_.boundary_reason() != BlockExitReason::CONTINUE_BLOCK;
    if (yielded && find(expected_pc_) == active_contract_) add_metric(&MechanicalPrimitiveMetrics::mid_operation_yields);
    active_ = continuation_.active && find(expected_pc_) == active_contract_;
    if (!active_) active_contract_ = nullptr;
}
void MechanicalPrimitiveRegistry::record_read(int width, unsigned address) {
    const auto bus_address = address & 0xFFFFFFU;
    if (bus_address >= 0xA00000U && bus_address < 0xFF0000U)
        add_metric(&MechanicalPrimitiveMetrics::hardware_accesses);
    add_metric(&MechanicalPrimitiveMetrics::reads);
    auto& metrics = current_metrics();
    metrics.min_source_address = std::min(metrics.min_source_address, bus_address);
    metrics.max_source_address = std::max(metrics.max_source_address, bus_address);
    if (pending_shadow_step_) reads_.push_back({bus_address, width});
}
void MechanicalPrimitiveRegistry::record_write(int width, unsigned address, unsigned value) {
    const auto bus_address = address & 0xFFFFFFU;
    if (bus_address >= 0xA00000U && bus_address < 0xFF0000U)
        add_metric(&MechanicalPrimitiveMetrics::hardware_accesses);
    add_metric(&MechanicalPrimitiveMetrics::writes);
    auto& metrics = current_metrics();
    metrics.min_address = std::min(metrics.min_address, bus_address);
    metrics.max_address = std::max(metrics.max_address, bus_address);
    if (active_contract_->operation == MechanicalOperation::MEMORY_COPY) {
        metrics.min_destination_address = std::min(metrics.min_destination_address, bus_address);
        metrics.max_destination_address = std::max(metrics.max_destination_address, bus_address);
    }
    if (pending_shadow_step_) writes_.push_back({bus_address, width, value});
}
int MechanicalPrimitiveRegistry::dispatch(unsigned pc) noexcept {
    const auto* contract = find(pc);
    if (!contract || !error_.empty()) return 0;
    try {
        if (mode_ == BasicBlockMode::SHADOW_NATIVE) return 0;
        if (!active_) {
            if (pc != contract->body_pc) throw std::runtime_error("primitive loop entry without active invocation");
            record_invocation(*contract);
        } else {
            if (contract != active_contract_ || pc != expected_pc_)
                throw std::runtime_error("primitive native continuation mismatch");
            add_metric(&MechanicalPrimitiveMetrics::resumptions);
        }
        active_ = true;
        native_step_active_ = true;
        NativeMachine machine(api_, *contract);
        const auto result = core::execute_mechanical_loop(
            machine, contract->portable(), pc, continuation_);
        native_step_active_ = false;
        add_metric(&MechanicalPrimitiveMetrics::dispatches);
        metrics_.iterations += result.iterations;
        current_metrics().iterations += result.iterations;
        metrics_.boundary_yields += result.boundary_yields;
        current_metrics().boundary_yields += result.boundary_yields;
        expected_pc_ = result.next_token;
        if (result.reason != core::PrimitiveExitReason::NORMAL_EXIT &&
            find(result.next_token) == active_contract_)
            add_metric(&MechanicalPrimitiveMetrics::mid_operation_yields);
        active_ = continuation_.active && find(result.next_token) == active_contract_;
        if (!active_) active_contract_ = nullptr;
        log_ << "{\"primitive\":\"" << contract->name << "\",\"entry\":\"0x" << std::hex << pc
             << "\",\"iterations\":" << std::dec << result.iterations
             << ",\"reason\":" << static_cast<unsigned>(result.reason) << "}\n";
        return dispatch_result(result.reason);
    } catch (const std::exception& error) {
        native_step_active_ = false;
        fail(error.what());
        return 0;
    }
}
void MechanicalPrimitiveRegistry::event(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try {
        if (type == kInterruptEvent) { if (active_) add_metric(&MechanicalPrimitiveMetrics::interrupt_events); return; }
        if (type == kPostEvent) {
            if (mode_ == BasicBlockMode::SHADOW_NATIVE && pending_shadow_step_) compare_shadow();
            return;
        }
        if (type == 2 && (pending_shadow_step_ || native_step_active_)) record_read(width, address);
        if (type == 4 && (pending_shadow_step_ || native_step_active_)) record_write(width, address, value);
        if (type == 1 && mode_ == BasicBlockMode::SHADOW_NATIVE && handles(address)) start_shadow(address);
    } catch (const std::exception& error) { fail(error.what()); }
}
} // namespace oasis::hybrid
