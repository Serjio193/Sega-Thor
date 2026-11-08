#pragma once

#include "tools/hybrid/m68k_002512_step_ops.hpp"

namespace oasis::hybrid::step_002512 {

M68k9D70StepResult step_calc_down_range(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_002512
