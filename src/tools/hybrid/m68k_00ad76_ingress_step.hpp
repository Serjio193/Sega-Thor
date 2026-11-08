#pragma once

#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"

namespace oasis::hybrid {

M68k9D70StepResult step_00ad76_ingress(const M68k9D70StepApi& api, unsigned pc,
                                       unsigned a6, unsigned& resume_pc,
                                       M68k00AD76StepMetrics& metrics) noexcept;

} // namespace oasis::hybrid
