#pragma once

#include "tools/hybrid/basic_block.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace oasis::hybrid {

enum class PrimitiveStep { CLEAR_BYTE_POSTINCREMENT, DBF };
enum class DbfResult { TAKEN, FALLTHROUGH };

class MechanicalMachine {
public:
    virtual ~MechanicalMachine() = default;
    virtual unsigned reg(unsigned index) const = 0;
    virtual void set_reg(unsigned index, unsigned value) = 0;
    virtual void write(unsigned address, unsigned width, unsigned value) = 0;
    virtual void begin(PrimitiveStep step) = 0;
    virtual void fetch_dbf_displacement() = 0;
    virtual void finish(PrimitiveStep step, DbfResult result) = 0;
    virtual BlockExitReason boundary() const = 0;
};

struct MemoryClearLoopContract {
    unsigned body_pc{};
    unsigned loop_pc{};
    unsigned continuation_pc{};
    unsigned address_register{};
    unsigned counter_register{};
};

struct PrimitiveExit {
    unsigned next_pc{};
    BlockExitReason reason{BlockExitReason::NORMAL_EXIT};
    unsigned guest_instructions{};
    unsigned iterations{};
};

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
    unsigned resumptions{};
    unsigned bytes_written{};
    unsigned min_address{0xFFFFFFFFU};
    unsigned max_address{};
    unsigned interrupts{};
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
    struct Write { unsigned address{}; int width{}; unsigned value{}; };

    MechanicalPrimitiveRegistry(BasicBlockApi api, BasicBlockMode mode,
                                std::ostream& log);
    bool handles(unsigned pc) const;
    int dispatch(unsigned pc) noexcept;
    void event(int type, int width, unsigned address, unsigned value) noexcept;
    bool complete() const { return error_.empty(); }
    const std::string& error() const { return error_; }
    const MechanicalPrimitiveMetrics& metrics() const { return metrics_; }

private:
    Snapshot capture() const;
    void fail(const std::string& message);
    void start_shadow(unsigned pc);
    void compare_shadow();
    void record_write(int width, unsigned address, unsigned value);
    bool is_loop_pc(unsigned pc) const;

    BasicBlockApi api_{};
    BasicBlockMode mode_{};
    std::ostream& log_;
    MemoryClearLoopContract contract_{0x061266U, 0x061268U, 0x06126CU, 5U, 0U};
    bool active_{};
    bool shadow_resume_pending_{};
    bool pending_shadow_step_{};
    unsigned expected_pc_{};
    Snapshot expected_{};
    std::vector<Write> expected_writes_;
    std::vector<Write> writes_;
    std::string error_;
    MechanicalPrimitiveMetrics metrics_{};
};

} // namespace oasis::hybrid
