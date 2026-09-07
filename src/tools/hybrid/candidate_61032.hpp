#pragma once

#include "tools/hybrid/contract.hpp"
#include "tools/hybrid/replacement.hpp"

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace oasis::hybrid {

class Candidate61032 final : public Replacement {
public:
    Candidate61032(CandidateApi api, Mode mode, std::ostream& log);
    void hook(int type, int width, unsigned address, unsigned value) noexcept override;
    unsigned target_address() const override { return 0x61032; }
    bool complete() const override { return !active_ && error_.empty(); }
    const std::string& error() const override { return error_; }
    ReplacementMetrics metrics() const override {
        return {calls, comparisons, divergences, body_instructions, override_calls,
                interrupts};
    }
    unsigned calls{}, comparisons{}, divergences{}, body_instructions{}, override_calls{}, interrupts{};

private:
    using State = std::array<std::uint32_t, 18>;
    struct Write { unsigned address{}; int width{}; unsigned value{}; };
    State state() const;
    unsigned read(unsigned address, unsigned width) const;
    void begin();
    void finish();
    void apply_override();
    void event(int type, int width, unsigned address, unsigned value);
    void require(bool condition, const std::string& message);
    void fail(const std::string& message);
    std::string compare_state(const State& expected, const State& actual) const;
    CandidateApi api_;
    Mode mode_;
    std::ostream& log_;
    bool active_{};
    unsigned current_pc_{}, return_pc_{}, entry_stack_{}, base_{}, source_{};
    int entry_cycles_{};
    int entry_refresh_{};
    State entry_{};
    std::vector<Write> writes_;
    std::string error_;
};

} // namespace oasis::hybrid
