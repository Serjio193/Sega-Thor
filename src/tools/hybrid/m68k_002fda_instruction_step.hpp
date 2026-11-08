#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/m68k_002fda_metrics.hpp"

namespace oasis::hybrid {

M68k9D70StepResult step_m68k_002fda(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
void reset_002fda_state() noexcept;

} // namespace oasis::hybrid
