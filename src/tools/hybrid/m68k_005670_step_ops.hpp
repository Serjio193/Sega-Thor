#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/m68k_00557a_step_ops.hpp"
#include "tools/hybrid/m68k_00f788_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"
#include <cstdint>

namespace oasis::hybrid::step_005670 {

using step_623ac::step_bcc_s;
using step_623ac::step_bcc_w;
using step_623ac::step_bra_s;
using step_623ac::step_bra_w;
using step_623ac::step_subq_b_reg;

using step_00f788::step_move_b_abs_reg;
using step_00f788::step_move_b_reg_abs;
using step_00f788::step_move_l_reg_abs;
using step_00f788::step_rts;

using step_00557a::step_addi_w_reg;
using step_00557a::step_bsr_w;
using step_00557a::step_btst_imm_disp;
using step_00557a::step_clr_w_reg;
using step_00557a::step_cmpi_w_reg;
using step_00557a::step_jsr_abs;
using step_00557a::step_move_b_disp_reg;
using step_00557a::step_move_w_abs_reg;
using step_00557a::step_move_w_disp_reg;
using step_00557a::step_move_w_reg_disp;
using step_00557a::step_move_w_reg_reg;
using step_00557a::step_sf_abs;

// Ops 1..25
M68k9D70StepResult step_clr_l_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_b_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bset_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                      unsigned areg, std::int16_t disp, unsigned next_pc,
                                      unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bclr_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                      unsigned areg, std::int16_t disp, unsigned next_pc,
                                      unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_b_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmpi_w_abs(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_abs_disp(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_imm_disp(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_clr_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_b_imm_abs(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                                       unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_w_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_abs(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_moveq(const M68k9D70StepApi& api, unsigned pc, std::uint8_t imm,
                              unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_and_b_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmp_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_addq_w_reg(const M68k9D70StepApi& api, unsigned pc, unsigned imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_w_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_subq_b_abs(const M68k9D70StepApi& api, unsigned pc, unsigned imm,
                                   unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_b_reg_disp(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_clr_w_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_tst_l_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                     std::int16_t disp, unsigned dst_areg, unsigned next_pc,
                                     unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmpi_w_ind(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_l_disp_disp(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                         std::int16_t src_disp, unsigned dst_areg, std::int16_t dst_disp,
                                         unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_b_disp_disp(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                         std::int16_t src_disp, unsigned dst_areg, std::int16_t dst_disp,
                                         unsigned next_pc, unsigned& resume_pc) noexcept;

// Ops 26..50
M68k9D70StepResult step_move_w_disp_disp(const M68k9D70StepApi& api, unsigned pc, unsigned src_areg,
                                         std::int16_t src_disp, unsigned dst_areg, std::int16_t dst_disp,
                                         unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_sf_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_clr_b_disp(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                   std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_andi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_sub_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                       std::int16_t disp, unsigned dreg, unsigned next_pc,
                                       unsigned& resume_pc) noexcept;
M68k9D70StepResult step_sub_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_add_w_reg_abs(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                      unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_ori_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                  unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_st_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                               unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movea_l_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                         std::int16_t disp, unsigned dst_areg, unsigned next_pc,
                                         unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmpi_w_disp(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                    unsigned areg, std::int16_t disp, unsigned next_pc,
                                    unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bchg_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_add_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmp_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                       std::int16_t disp, unsigned dreg, unsigned next_pc,
                                       unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_imm_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                       unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_addi_w_disp(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                    unsigned areg, std::int16_t disp, unsigned next_pc,
                                    unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_pc_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg_index,
                                        unsigned dst_areg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_adda_w_ind_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                       unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_jmp_ind(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmpa_l_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned imm,
                                       unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movem_l_reg_predec(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                           unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bset_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit_reg,
                                     unsigned dst_reg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movem_l_postinc_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                            unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmp_b_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src,
                                      unsigned dst, unsigned next_pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_005670
