#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::step_008dd4 {

M68k9D70StepResult step_tst_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_cmpi_w_abs(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_008dd4
