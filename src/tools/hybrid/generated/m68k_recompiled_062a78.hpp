#ifndef OASIS_HYBRID_GENERATED_M68K_RECOMPILED_062A78_HPP_
#define OASIS_HYBRID_GENERATED_M68K_RECOMPILED_062A78_HPP_

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::generated {

struct StepMetrics_062a78 {
    unsigned target_entries{};
    unsigned native_calls{};
    unsigned completed_calls{};
    unsigned stepped_instructions{};
    unsigned active_at_entry{};
};

M68k9D70StepResult step_m68k_062a78(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
const StepMetrics_062a78& get_metrics_062a78() noexcept;
void reset_metrics_062a78() noexcept;

} // namespace oasis::hybrid::generated

#endif // OASIS_HYBRID_GENERATED_M68K_RECOMPILED_062A78_HPP_
