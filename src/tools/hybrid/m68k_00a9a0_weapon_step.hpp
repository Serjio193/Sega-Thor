#pragma once
#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"

namespace oasis::hybrid {
M68k9D70StepResult step_00a9a0_weapon(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp,
                                      M68k00A9A0StepMetrics& metrics) noexcept;
} // namespace oasis::hybrid
