#ifndef OASIS_HYBRID_GENERATED_M68K_RECOMPILED_00D2B4_HPP_
#define OASIS_HYBRID_GENERATED_M68K_RECOMPILED_00D2B4_HPP_

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::generated {

struct StepMetrics_00d2b4 {
    unsigned target_entries{};
    unsigned native_calls{};
    unsigned completed_calls{};
    unsigned stepped_instructions{};
    unsigned active_at_entry{};
};

M68k9D70StepResult step_m68k_00d2b4(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
const StepMetrics_00d2b4& get_metrics_00d2b4() noexcept;
void reset_metrics_00d2b4() noexcept;

} // namespace oasis::hybrid::generated

#endif // OASIS_HYBRID_GENERATED_M68K_RECOMPILED_00D2B4_HPP_
