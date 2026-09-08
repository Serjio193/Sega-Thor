#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace oasis::hybrid {

enum class BlockExitReason {
    CONTINUE_BLOCK,
    NORMAL_EXIT,
    EVENT_BOUNDARY,
    INTERRUPT_BOUNDARY,
    TRACE_BOUNDARY,
    FALLBACK,
};

struct BlockExit {
    unsigned next_pc{};
    BlockExitReason reason{BlockExitReason::NORMAL_EXIT};
    unsigned instructions_executed{};
};

struct BasicBlockApi {
    unsigned (*reg)(unsigned);
    void (*set_reg)(unsigned, unsigned);
    int (*peek)(unsigned);
    unsigned (*fetch16)();
    unsigned (*read)(unsigned, int);
    void (*write)(unsigned, int, unsigned);
    void (*begin_instruction)(unsigned);
    void (*finish_instruction)(unsigned);
    void (*add_cycles)(int);
    void (*skip_bus_refresh)();
    unsigned (*instruction_cycles)(unsigned);
    unsigned (*cpu_field)(unsigned);
    unsigned (*refresh_period)();
    unsigned (*refresh_penalty)();
    BlockExitReason (*boundary_reason)();
};

using GeneratedBlockExecutor = BlockExit (*)(BasicBlockApi&, unsigned entry_pc);
using GeneratedBlockLength = unsigned (*)(unsigned entry_pc);

struct GeneratedBlockSpec {
    unsigned start{};
    unsigned end{};
    unsigned instruction_count{};
    GeneratedBlockExecutor execute{};
    GeneratedBlockLength instruction_count_from_entry{};
};

enum class BasicBlockMode { SHADOW_NATIVE, NATIVE_OVERRIDE };

struct BasicBlockMetrics {
    unsigned natural_entries{};
    unsigned translated_blocks{};
    unsigned translated_entries{};
    unsigned translated_instructions{};
    unsigned shadow_comparisons{};
    unsigned divergences{};
    unsigned original_starts_inside_translated{};
    unsigned fallback_entries{};
    unsigned interrupts{};
    unsigned hardware_accesses{};
    unsigned boundary_yields{};
    unsigned event_boundary_yields{};
    unsigned interrupt_boundary_yields{};
    unsigned trace_boundary_yields{};
    unsigned translated_multi_instruction_entries{};
    unsigned interrupted_resumptions{};
    struct BlockCounts {
        unsigned target{};
        unsigned end{};
        unsigned instruction_count{};
        unsigned natural_entries{};
        unsigned shadow_comparisons{};
        unsigned translated_entries{};
        unsigned boundary_yields{};
        unsigned event_boundary_yields{};
        unsigned interrupt_boundary_yields{};
        unsigned trace_boundary_yields{};
        unsigned interrupted_resumptions{};
    };
    std::vector<BlockCounts> per_block;
};

class BasicBlockRegistry final {
public:
    BasicBlockRegistry(BasicBlockApi api, BasicBlockMode mode, std::ostream& log);

    int dispatch(unsigned pc) noexcept;
    void event(int type, int width, unsigned address, unsigned value) noexcept;
    bool complete() const { return !active_ && error_.empty(); }
    const std::string& error() const { return error_; }
    const BasicBlockMetrics& metrics() const { return metrics_; }

public:
    using State = std::array<std::uint32_t, 18>;
    struct Write { unsigned address{}; int width{}; unsigned value{}; };
    struct Read { unsigned address{}; int width{}; };
    struct Prediction {
        State state{};
        std::vector<Write> writes;
        std::vector<Read> reads;
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

private:

    State state() const;
    unsigned field(unsigned index) const;
    int peek(unsigned address) const;
    unsigned read(unsigned address, int width) const;
    void write(unsigned address, int width, unsigned value) const;
    void require(bool condition, const std::string& message) const;
    void fail(const std::string& message);
    const GeneratedBlockSpec* find(unsigned pc) const;
    std::size_t index_of(unsigned pc) const;
    Prediction capture() const;
    Prediction predict(const GeneratedBlockSpec& block, const Prediction& entry,
                       unsigned entry_pc, unsigned instruction_limit) const;
    BlockExit execute(const GeneratedBlockSpec& block, unsigned entry_pc);
    void start_shadow(unsigned pc);
    void finish_shadow();
    void finish_native(const Prediction& prediction);
    void compare(const Prediction& prediction);
    void compare_shadow_boundary();
    BlockExitReason boundary_reason() const;
    void record_yield(BlockExitReason reason);
    bool in_registered_range(unsigned address) const;
    bool in_block(unsigned pc) const;

    BasicBlockApi api_;
    BasicBlockMode mode_;
    std::ostream& log_;
    bool active_{};
    bool shadow_{};
    bool instruction_exit_seen_{};
    bool entry_event_pending_{};
    unsigned instruction_exits_{};
    unsigned instructions_remaining_{};
    unsigned active_pc_{};
    unsigned active_entry_pc_{};
    unsigned expected_entry_pc_{};
    const GeneratedBlockSpec* active_block_{};
    bool awaiting_interrupt_{};
    bool interrupt_after_yield_{};
    unsigned continuation_pc_{};
    unsigned continuation_block_{};
    Prediction prediction_{};
    std::vector<Write> writes_;
    std::vector<Read> reads_;
    std::string error_;
    BasicBlockMetrics metrics_{};
};

} // namespace oasis::hybrid
