#pragma once

#include "core/parent_suffix.hpp"
#include "tools/hybrid/contract.hpp"
#include "tools/hybrid/replacement.hpp"

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace oasis::hybrid {

class CandidateParentSuffix final : public Replacement {
public:
    CandidateParentSuffix(CandidateApi api, Mode mode, std::ostream& log);
    unsigned target_address() const override { return 0x604F0; }
    int dispatch(unsigned address) noexcept override;
    void hook(int type, int width, unsigned address, unsigned value) noexcept override;
    bool complete() const override { return !active_ && error_.empty(); }
    const std::string& error() const override { return error_; }
    ReplacementMetrics metrics() const override;

private:
    using State = std::array<std::uint32_t, 18>;
    struct Write { unsigned address{}, width{}, value{}; };
    class ApiMachine;
    class ShadowMachine;
    State state() const;
    void begin_shadow();
    void finish_shadow();
    int execute_native(std::uint32_t token);
    void fail(const std::string& message);
    static int result_code(oasis::core::RoutineExitReason reason);
    unsigned resume_pc() const;
    CandidateApi api_;
    Mode mode_;
    std::ostream& log_;
    bool active_{}, shadow_active_{};
    unsigned calls_{}, comparisons_{}, divergences_{}, override_calls_{};
    unsigned helper_instructions_{}, helper_invocations_{}, helper_yields_{}, helper_resumptions_{};
    unsigned ram_instructions_{}, ram_invocations_{}, ram_yields_{}, ram_resumptions_{};
    unsigned current_pc_{};
    State entry_{};
    std::vector<Write> writes_, expected_writes_;
    std::vector<unsigned> path_;
    State expected_state_{};
    oasis::core::ParentSuffixContinuation continuation_{};
    std::string error_;
};

} // namespace oasis::hybrid
