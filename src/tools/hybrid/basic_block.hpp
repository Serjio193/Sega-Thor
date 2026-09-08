#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace oasis::hybrid {

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
    std::array<unsigned, 6> natural_by_block{};
    std::array<unsigned, 6> shadow_by_block{};
    std::array<unsigned, 6> translated_by_block{};
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
    };

private:

    State state() const;
    unsigned field(unsigned index) const;
    int peek(unsigned address) const;
    unsigned read(unsigned address, int width) const;
    void write(unsigned address, int width, unsigned value) const;
    void require(bool condition, const std::string& message) const;
    void fail(const std::string& message);
    Prediction predict(unsigned pc, const State& entry) const;
    Prediction predict_2d66(const State& entry) const;
    Prediction predict_604bc(const State& entry) const;
    Prediction predict_61032(const State& entry) const;
    Prediction predict_3a85e(const State& entry) const;
    Prediction predict_3a8ba(const State& entry) const;
    Prediction predict_3a88c(const State& entry) const;
    void execute(unsigned pc);
    void execute_2d66();
    void execute_604bc();
    void execute_61032();
    void execute_3a85e();
    void execute_3a8ba();
    void execute_3a88c();
    void start_shadow(unsigned pc);
    void finish_shadow();
    void finish_native(const Prediction& prediction);
    void compare(const Prediction& prediction);
    bool in_registered_range(unsigned address) const;
    bool in_block(unsigned pc) const;

    BasicBlockApi api_;
    BasicBlockMode mode_;
    std::ostream& log_;
    bool active_{};
    bool shadow_{};
    unsigned active_pc_{};
    Prediction prediction_{};
    std::vector<Write> writes_;
    std::vector<Read> reads_;
    std::string error_;
    BasicBlockMetrics metrics_{};
};

} // namespace oasis::hybrid
