#pragma once

#include "tools/hybrid/m68k_02575c_instruction_step.hpp"

namespace oasis::hybrid::step_02575c {

M68k9D70StepResult step_root(
    unsigned pc, const M68k9D70StepApi& api, bool enabled,
    bool& root_active, bool& root_child_active,
    unsigned& root_resume_pc, unsigned& saved_root_child_sp,
    M68k02575CStepMetrics& metrics) noexcept;

} // namespace oasis::hybrid::step_02575c
