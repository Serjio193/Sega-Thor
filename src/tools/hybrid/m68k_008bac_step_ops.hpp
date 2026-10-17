#pragma once

#include "tools/hybrid/m68k_00f788_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid::step_008bac {

using step_623ac::branch_to;
using step_623ac::ccr;
using step_623ac::complete_step;
using step_623ac::in_work_ram;
using step_623ac::prepare_step;
using step_623ac::read_be;
using step_623ac::set_ccr;
using step_623ac::step_bcc_s;
using step_623ac::step_bcc_w;
using step_623ac::step_bra_w;

using step_00f788::step_move_b_abs_reg;
using step_00f788::step_rts;

M68k9D70StepResult step_not_b_reg(const M68k9D70StepApi& api, unsigned pc, unsigned reg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_andi_b_imm_reg(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                                      unsigned reg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_st_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                               unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_clr_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                 unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_b_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                 unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                 unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_sf_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_abs(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                    unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                      unsigned reg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                    unsigned reg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmpi_w_abs(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                  unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_b_imm_abs(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                                      unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_008bac
