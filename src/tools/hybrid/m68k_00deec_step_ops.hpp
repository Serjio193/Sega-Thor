#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>

namespace oasis::hybrid::step_00deec {

inline constexpr unsigned kMask24 = 0x00FFFFFFU;

bool prepare_step(const M68k9D70StepApi& api, unsigned pc, std::uint16_t& opcode) noexcept;
bool complete_step(const M68k9D70StepApi& api, unsigned next_pc, std::uint16_t opcode,
                   int extra_cycles, unsigned& resume_pc) noexcept;
bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& val) noexcept;

void set_ccr(const M68k9D70StepApi& api, std::uint16_t ccr_bits) noexcept;
std::uint16_t ccr(const M68k9D70StepApi& api) noexcept;
std::uint16_t logic_flags_b(std::uint8_t val, std::uint16_t old_ccr) noexcept;
std::uint16_t logic_flags_w(std::uint16_t val, std::uint16_t old_ccr) noexcept;
std::uint16_t sub_flags_w(std::uint16_t src, std::uint16_t dst, std::uint16_t res,
                          std::uint16_t old_ccr) noexcept;
std::uint16_t add_flags_w(std::uint16_t src, std::uint16_t dst, std::uint16_t res,
                          std::uint16_t old_ccr) noexcept;

M68k9D70StepResult step_btst_imm_abs(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bne_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                    unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_moveq(const M68k9D70StepApi& api, unsigned pc, std::int8_t imm,
                              unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_ble_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_cmpi_w_reg(const M68k9D70StepApi& api, unsigned pc, std::uint16_t imm,
                                   unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bcs_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_add_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src_dreg,
                                      unsigned dst_dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movea_l_pc_index(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                         unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movem_l_push_d7_a6(const M68k9D70StepApi& api, unsigned pc,
                                           unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_jsr_a0(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc,
                               unsigned& resume_pc, bool& child_active, unsigned& saved_sp) noexcept;
M68k9D70StepResult step_movem_l_pop_d7_a6(const M68k9D70StepApi& api, unsigned pc,
                                          unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_clr_w_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned src_dreg,
                                       unsigned dst_dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_b_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_disp(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                      unsigned areg, std::int16_t disp, unsigned next_pc,
                                      unsigned& resume_pc) noexcept;
M68k9D70StepResult step_beq_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_exg_d1_d2(const M68k9D70StepApi& api, unsigned pc, unsigned next_pc,
                                  unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_reg_disp(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                        unsigned areg, std::int16_t disp, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_addq_w_reg_1(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                     unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lsr_w_reg_1(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                    unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                     std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_dbf_d7(const M68k9D70StepApi& api, unsigned pc, unsigned loop_pc,
                               unsigned exit_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc) noexcept;

} // namespace oasis::hybrid::step_00deec
