#pragma once
#include "tools/hybrid/m68k_0023cc_step_ops.hpp"

namespace oasis::hybrid::step_0023cc {

M68k9D70StepResult step_calc_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_0023cc
