#pragma once
#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"

namespace oasis::hybrid {
M68k9D70StepResult step_00a9a0_piece(const M68k9D70StepApi& api, unsigned pc,
                                     unsigned a6, unsigned& resume_pc,
                                     bool& child_call_active, unsigned& saved_child_sp,
                                     M68k00A9A0StepMetrics& metrics) noexcept;
} // namespace oasis::hybrid
