#pragma once

#include "tools/hybrid/basic_block.hpp"

#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace oasis::hybrid {

enum class PrimitiveStep { BODY, DBF };
enum class DbfResult { TAKEN, FALLTHROUGH };
enum class MechanicalOperation { MEMORY_CLEAR, MEMORY_COPY };

class MechanicalMachine {
public:
    virtual ~MechanicalMachine() = default;
    virtual unsigned reg(unsigned index) const = 0;
    virtual void set_reg(unsigned index, unsigned value) = 0;
    virtual unsigned read(unsigned address, unsigned width) = 0;
    virtual void write(unsigned address, unsigned width, unsigned value) = 0;
    virtual void begin(PrimitiveStep step, unsigned expected_opcode) = 0;
    virtual void fetch_dbf_displacement(unsigned expected_displacement) = 0;
    virtual void finish(PrimitiveStep step, DbfResult result, unsigned opcode) = 0;
    virtual BlockExitReason boundary() const = 0;
};

struct MechanicalLoopContract {
    const char* name{};
    MechanicalOperation operation{MechanicalOperation::MEMORY_CLEAR};
    unsigned body_pc{};
    unsigned loop_pc{};
    unsigned continuation_pc{};
    unsigned body_opcode{};
    unsigned dbf_opcode{};
    unsigned dbf_displacement{};
    unsigned counter_register{};
    unsigned address_register{};
    unsigned source_register{};
    unsigned destination_register{};
    unsigned width{};
};

using MemoryClearLoopContract = MechanicalLoopContract;

struct PrimitiveExit {
    unsigned next_pc{};
    BlockExitReason reason{BlockExitReason::NORMAL_EXIT};
    unsigned guest_instructions{};
    unsigned iterations{};
    unsigned boundary_yields{};
};

PrimitiveExit execute_mechanical_loop(MechanicalMachine& machine,
                                      const MechanicalLoopContract& contract,
                                      unsigned entry_pc);
PrimitiveExit execute_memory_clear(MechanicalMachine& machine,
                                   const MemoryClearLoopContract& contract,
                                   unsigned entry_pc);

struct MechanicalPrimitiveMetrics {
    unsigned invocations{};
    unsigned iterations{};
    unsigned dispatches{};
    unsigned shadow_comparisons{};
    unsigned divergences{};
    unsigned mid_operation_yields{};
    unsigned boundary_yields{};
    unsigned resumptions{};
    unsigned reads{};
    unsigned writes{};
    unsigned hardware_accesses{};
    unsigned interrupt_events{};
    unsigned initial_counter_min{0xFFFFFFFFU};
    unsigned initial_counter_max{};
    unsigned min_source_address{0xFFFFFFFFU};
    unsigned max_source_address{};
    unsigned min_destination_address{0xFFFFFFFFU};
    unsigned max_destination_address{};
    unsigned min_address{0xFFFFFFFFU};
    unsigned max_address{};
};

class MechanicalPrimitiveRegistry final {
public:
    struct Snapshot {
        std::uint32_t registers[18]{};
        unsigned ir{};
        unsigned pref_addr{};
        unsigned pref_data{};
        unsigned cycles{};
        unsigned refresh{};
        unsigned cycle_end{};
        unsigned interrupt_level{};
        unsigned interrupt_mask{};
        unsigned tracing{};
        unsigned stopped{};
    };
    struct Read { unsigned address{}; int width{}; };
    struct Write { unsigned address{}; int width{}; unsigned value{}; };

    MechanicalPrimitiveRegistry(BasicBlockApi api, BasicBlockMode mode,
                                std::ostream& log);
    bool handles(unsigned pc) const;
    int dispatch(unsigned pc) noexcept;
    void event(int type, int width, unsigned address, unsigned value) noexcept;
    bool complete() const { return !active_ && error_.empty(); }
    const std::string& error() const { return error_; }
    const MechanicalPrimitiveMetrics& metrics() const { return metrics_; }
    std::span<const MechanicalLoopContract> contracts() const { return contracts_; }
    std::span<const MechanicalPrimitiveMetrics> candidate_metrics() const {
        return candidate_metrics_;
    }

private:
    Snapshot capture() const;
    void fail(const std::string& message);
    void start_shadow(unsigned pc);
    void compare_shadow();
    void record_read(int width, unsigned address);
    void record_write(int width, unsigned address, unsigned value);
    const MechanicalLoopContract* find(unsigned pc) const;
    MechanicalPrimitiveMetrics& current_metrics();
    const MechanicalPrimitiveMetrics& current_metrics() const;
    void record_invocation(const MechanicalLoopContract& contract);
    void add_metric(unsigned MechanicalPrimitiveMetrics::*field);

    BasicBlockApi api_{};
    BasicBlockMode mode_{};
    std::ostream& log_;
    std::vector<MechanicalLoopContract> contracts_;
    std::vector<MechanicalPrimitiveMetrics> candidate_metrics_;
    MechanicalPrimitiveMetrics metrics_{};
    const MechanicalLoopContract* active_contract_{};
    bool active_{};
    bool native_step_active_{};
    bool shadow_resume_pending_{};
    bool pending_shadow_step_{};
    unsigned expected_pc_{};
    Snapshot expected_{};
    std::vector<Read> expected_reads_;
    std::vector<Write> expected_writes_;
    std::vector<Read> reads_;
    std::vector<Write> writes_;
    std::string error_;
};

} // namespace oasis::hybrid
