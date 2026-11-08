#pragma once
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/m68k_0023cc_metrics.hpp"

namespace oasis::hybrid {

void reset_0023cc_state() noexcept;
M68k9D70StepResult step_m68k_0023cc(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;

} // namespace oasis::hybrid
