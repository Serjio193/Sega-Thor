#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid::step_005670 {

M68k9D70StepResult step_005670_block2(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept;

} // namespace oasis::hybrid::step_005670
