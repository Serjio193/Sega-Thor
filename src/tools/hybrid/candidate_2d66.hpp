#pragma once

#include "tools/hybrid/contract.hpp"
#include "tools/hybrid/replacement.hpp"
#include <array>
#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace oasis::hybrid {

class Candidate2D66 final : public Replacement {
public:
    Candidate2D66(CandidateApi api, Mode mode, std::span<const std::uint8_t> rom,
                  std::ostream& log);
    void hook(int type, int width, unsigned address, unsigned value) noexcept;
    unsigned target_address() const override { return 0x2D66; }
    bool complete() const { return !active_ && error_.empty(); }
    const std::string& error() const { return error_; }
    ReplacementMetrics metrics() const override {
        return {calls, comparisons, divergences, body_instructions, override_calls,
                interrupt_count};
    }
    unsigned calls{}, comparisons{}, divergences{}, body_instructions{},
             override_calls{}, interrupt_count{};

private:
    using State = std::array<std::uint32_t, 18>;
    struct Write { unsigned address{}; int width{}; unsigned value{}; };
    State state() const;
    int peek(unsigned address) const;
    void begin();
    void finish();
    void apply_override();
    void event(int type, int width, unsigned address, unsigned value);
    void require(bool condition, const std::string& message);
    void fail(const std::string& message);
    std::string compare_state(const State& expected, const State& actual) const;
    std::string compare_memory(unsigned address, std::span<const std::uint8_t> expected,
                               const char* name) const;
    CandidateApi api_;
    Mode mode_;
    std::span<const std::uint8_t> rom_;
    std::ostream& log_;
    bool active_{}, overridden_{};
    unsigned current_pc_{}, return_pc_{}, entry_stack_{}, destination_{}, source_size_{},
             output_size_{}, loop_count_{}, body_starts_{};
    int entry_cycles_{};
    int entry_refresh_{};
    State entry_{};
    std::vector<std::uint8_t> source_, initial_output_, initial_stack_, expected_stack_;
    std::vector<Write> writes_;
    std::string error_;
};

} // namespace oasis::hybrid
