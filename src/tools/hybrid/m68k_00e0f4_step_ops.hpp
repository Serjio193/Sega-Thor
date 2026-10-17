#ifndef SEGA_THOR_M68K_00E0F4_STEP_OPS_HPP
#define SEGA_THOR_M68K_00E0F4_STEP_OPS_HPP

#include <array>
#include <cstdint>
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid::step_00e0f4 {

inline constexpr unsigned kEntryPc_00e0f4 = 0x0000E0F4U;
inline constexpr unsigned kExpectedCaller_008b3a = 0x00008B3AU;
inline constexpr unsigned kExpectedReturn_008b3e = 0x00008B3EU;
inline constexpr unsigned kEndPc_00e140 = 0x0000E140U;

inline constexpr std::array<std::uint16_t, 38> kExpectedWords_00e0f4 = {
    0x3C39, 0x00FF, 0x1856, // 0x00E0F4: move.w $00FF1856, %d6
    0x6700, 0x000A,         // 0x00E0FA: beq.w 0x00E106
    0x5346,                 // 0x00E0FE: subq.w #1, %d6
    0x33C6, 0x00FF, 0x1856, // 0x00E100: move.w %d6, $00FF1856
    0x0839, 0x0001, 0x00FF, 0x17B8, // 0x00E106: btst #1, $00FF17B8
    0x6600, 0x002E,         // 0x00E10E: bne.w 0x00E13E
    0x4DF9, 0x00FF, 0x2ABC, // 0x00E112: lea $00FF2ABC, %a6
    0x7E07,                 // 0x00E118: moveq #7, %d7
    0x3C2E, 0x0000,         // 0x00E11A: move.w 0(%a6), %d6
    0x6F00, 0x0016,         // 0x00E11E: ble.w 0x00E136
    0x3C2E, 0x0004,         // 0x00E122: move.w 4(%a6), %d6
    0x41FB, 0x6018,         // 0x00E126: lea 0x00E140(%pc, %d6.w), %a0
    0xD0D0,                 // 0x00E12A: adda.w (%a0), %a0
    0x48E7, 0x0102,         // 0x00E12C: movem.l %d7/%a6, -(%sp)
    0x4E90,                 // 0x00E130: jsr (%a0)
    0x4CDF, 0x4080,         // 0x00E132: movem.l (%sp)+, %d7/%a6
    0x4DEE, 0x005A,         // 0x00E136: lea 90(%a6), %a6
    0x51CF, 0xFFDE,         // 0x00E13A: dbf %d7, 0x00E11A
    0x4E75                  // 0x00E13E: rts
};

bool verify_rom_words(const M68k9D70StepApi& api, unsigned start_pc) noexcept;

M68k9D70StepResult step_move_w_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                      unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_beq_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_subq_w_1_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                     unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_reg_abs(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                      unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_abs(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                    unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bne_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_abs_reg(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                    unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_moveq(const M68k9D70StepApi& api, unsigned pc, std::int32_t val,
                              unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_w_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_ble_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_pc_index_reg(const M68k9D70StepApi& api, unsigned pc, unsigned dreg,
                                         unsigned base_pc, unsigned areg, unsigned next_pc,
                                         unsigned& resume_pc) noexcept;
M68k9D70StepResult step_adda_w_ind_reg(const M68k9D70StepApi& api, unsigned pc, unsigned sreg,
                                       unsigned areg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movem_l_push_d7_a6(const M68k9D70StepApi& api, unsigned pc,
                                           unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_jsr_a0(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc,
                               unsigned& resume_pc, bool& child_active, unsigned& saved_sp) noexcept;
M68k9D70StepResult step_movem_l_pop_d7_a6(const M68k9D70StepApi& api, unsigned pc,
                                          unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_lea_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                     std::int16_t disp, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_dbf_d7(const M68k9D70StepApi& api, unsigned pc, unsigned loop_pc,
                               unsigned exit_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc) noexcept;

} // namespace oasis::hybrid::step_00e0f4

#endif // SEGA_THOR_M68K_00E0F4_STEP_OPS_HPP
