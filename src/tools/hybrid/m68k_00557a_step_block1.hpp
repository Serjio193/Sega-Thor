#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid::step_00557a {

M68k9D70StepResult step_00557a_block1(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept;

} // namespace oasis::hybrid::step_00557a
