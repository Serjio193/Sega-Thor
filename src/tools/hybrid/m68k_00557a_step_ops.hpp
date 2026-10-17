#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::step_00557a {

M68k9D70StepResult step_sf_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                               unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_lea_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_clr_w_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                       unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_b_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;

M68k9D70StepResult step_btst_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                      unsigned areg, std::int16_t disp, unsigned next_pc,
                                      unsigned& resume_pc) noexcept;

M68k9D70StepResult step_exg_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned r1,
                                    unsigned r2, unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_w_reg_disp(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;

M68k9D70StepResult step_cmpi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_lsr_w_reg_1(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                    unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_addi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_w_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                       unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_bsr_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned return_pc, unsigned& resume_pc, bool& child_active,
                              unsigned& saved_sp) noexcept;

M68k9D70StepResult step_jsr_abs(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                                unsigned return_pc, unsigned& resume_pc, bool& child_active,
                                unsigned& saved_sp) noexcept;

M68k9D70StepResult step_move_w_disp_postinc(const M68k9D70StepApi& api, unsigned pc,
                                            unsigned src_areg, std::int16_t disp,
                                            unsigned dst_areg, unsigned next_pc,
                                            unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_w_disp_ind(const M68k9D70StepApi& api, unsigned pc,
                                        unsigned src_areg, std::int16_t disp,
                                        unsigned dst_areg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;

M68k9D70StepResult step_move_l_imm_reg(const M68k9D70StepApi& api, unsigned pc, std::uint32_t imm,
                                       unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_00557a
