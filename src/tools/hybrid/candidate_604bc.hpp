#pragma once

#include "tools/hybrid/replacement.hpp"
#include "tools/hybrid/contract.hpp"
#include "core/table_copy_routine.hpp"

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
    int dispatch(unsigned address) noexcept override;
    void hook(int type, int width, unsigned address, unsigned value) noexcept override;
    unsigned target_address() const override { return 0x604BC; }
    bool complete() const override { return !active_ && error_.empty(); }
    const std::string& error() const override { return error_; }
    ReplacementMetrics metrics() const override {
        return {calls, comparisons, divergences, body_instructions, override_calls,
                interrupts, 0, 0, 0, 0, 0, native_instructions,
                native_invocations, native_boundary_yields, native_resumptions};
    }
    unsigned calls{}, comparisons{}, divergences{}, body_instructions{}, override_calls{}, interrupts{};
    unsigned native_instructions{}, native_invocations{}, native_boundary_yields{}, native_resumptions{};

private:
    using State = std::array<std::uint32_t, 18>;
    struct Write { unsigned address{}; unsigned value{}; };
    State state() const;
    int peek(unsigned address) const;
    void begin();
    void finish();
    void apply_override();
    int execute_native(unsigned entry_token);
    static int dispatch_result(oasis::core::RoutineExitReason reason);
    void event(int type, int width, unsigned address, unsigned value);
    void require(bool condition, const std::string& message);
    void fail(const std::string& message);
    std::string compare_state(const State& expected, const State& actual) const;
    std::string compare_byte(unsigned address, unsigned expected, const char* name) const;
    CandidateApi api_;
    Mode mode_;
    std::ostream& log_;
    bool active_{};
    bool block_dispatch_{};
    unsigned current_pc_{}, return_pc_{}, entry_stack_{}, output_base_{};
    unsigned first_flag_{}, second_flag_{};
    int entry_cycles_{};
    int entry_refresh_{};
    State entry_{};
    std::vector<Write> writes_;
    oasis::core::RoutineContinuation continuation_{};
    unsigned native_call_instructions_{};
    std::string error_;
};

} // namespace oasis::hybrid
