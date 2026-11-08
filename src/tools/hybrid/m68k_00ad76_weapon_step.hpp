#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"

namespace oasis::hybrid {

M68k9D70StepResult step_00ad76_weapon(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp,
                                      M68k00AD76StepMetrics& metrics) noexcept;

} // namespace oasis::hybrid
