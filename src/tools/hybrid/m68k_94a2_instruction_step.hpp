#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid {

[[nodiscard]] M68k9D70StepResult step_m68k_94a2(
    unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;

} // namespace oasis::hybrid
