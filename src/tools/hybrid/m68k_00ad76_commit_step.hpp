#pragma once

#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"

namespace oasis::hybrid {

M68k9D70StepResult step_00ad76_commit(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& active,
                                      M68k00AD76StepMetrics& metrics) noexcept;

} // namespace oasis::hybrid
