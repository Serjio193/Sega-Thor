#ifndef OASIS_HYBRID_GENERATED_M68K_RECOMPILED_006E7A_HPP_
#define OASIS_HYBRID_GENERATED_M68K_RECOMPILED_006E7A_HPP_

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::generated {

struct StepMetrics_006e7a {
    unsigned target_entries{};
    unsigned native_calls{};
    unsigned completed_calls{};
    unsigned stepped_instructions{};
    unsigned active_at_entry{};
};

M68k9D70StepResult step_m68k_006e7a(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
const StepMetrics_006e7a& get_metrics_006e7a() noexcept;
void reset_metrics_006e7a() noexcept;

} // namespace oasis::hybrid::generated

#endif // OASIS_HYBRID_GENERATED_M68K_RECOMPILED_006E7A_HPP_
