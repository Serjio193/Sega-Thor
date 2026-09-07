#pragma once

#include "tools/hybrid/replacement.hpp"
#include "tools/hybrid/contract.hpp"

#include <array>
#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace oasis::hybrid {

class Candidate604BC final : public Replacement {
public:
    Candidate604BC(CandidateApi api, Mode mode, std::ostream& log);
    void hook(int type, int width, unsigned address, unsigned value) noexcept override;
    unsigned target_address() const override { return 0x604BC; }
    bool complete() const override { return !active_ && error_.empty(); }
    const std::string& error() const override { return error_; }
    ReplacementMetrics metrics() const override {
        return {calls, comparisons, divergences, body_instructions, override_calls,
                interrupts};
    }
    unsigned calls{}, comparisons{}, divergences{}, body_instructions{}, override_calls{}, interrupts{};

private:
    using State = std::array<std::uint32_t, 18>;
    struct Write { unsigned address{}; unsigned value{}; };
    State state() const;
    int peek(unsigned address) const;
    void begin();
    void finish();
    void apply_override();
    void event(int type, int width, unsigned address, unsigned value);
    void require(bool condition, const std::string& message);
    void fail(const std::string& message);
    std::string compare_state(const State& expected, const State& actual) const;
    std::string compare_byte(unsigned address, unsigned expected, const char* name) const;
    CandidateApi api_;
    Mode mode_;
    std::ostream& log_;
    bool active_{};
    unsigned current_pc_{}, return_pc_{}, entry_stack_{}, output_base_{};
    unsigned first_flag_{}, second_flag_{};
    int entry_cycles_{};
    int entry_refresh_{};
    State entry_{};
    std::vector<Write> writes_;
    std::string error_;
};

} // namespace oasis::hybrid
