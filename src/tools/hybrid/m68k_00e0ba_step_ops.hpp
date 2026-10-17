#ifndef SEGA_THOR_M68K_00E0BA_STEP_OPS_HPP
#define SEGA_THOR_M68K_00E0BA_STEP_OPS_HPP

#include <array>
#include <cstdint>
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid::step_00e0ba {

inline constexpr unsigned kEntryPc_00e0ba = 0x0000E0BAU;
inline constexpr unsigned kExpectedCaller_008b36 = 0x00008B36U;
inline constexpr unsigned kExpectedReturn_008b3a = 0x00008B3AU;
inline constexpr unsigned kEndPc_00e0f4 = 0x0000E0F4U;

inline constexpr std::array<std::uint16_t, 29> kExpectedWords_00e0ba = {
    0x0839, 0x0001, 0x00FF, 0x17B8, // 0x00E0BA: btst #1, $00FF17B8
    0x6600, 0x002E,                 // 0x00E0C2: bne.w 0x00E0F2
    0x4DF9, 0x00FF, 0x2D8C,         // 0x00E0C6: lea $00FF2D8C, %a6
    0x7E05,                         // 0x00E0CC: moveq #5, %d7
    0x3C2E, 0x0000,                 // 0x00E0CE: move.w 0(%a6), %d6
    0x6700, 0x0016,                 // 0x00E0D2: beq.w 0x00E0EA
    0x2C2E, 0x0056,                 // 0x00E0D6: move.l 86(%a6), %d6
    0x6700, 0x000E,                 // 0x00E0DA: beq.w 0x00E0EA
    0x2046,                         // 0x00E0DE: movea.l %d6, %a0
    0x48E7, 0x0102,                 // 0x00E0E0: movem.l %d7/%a6, -(%sp)
    0x4E90,                         // 0x00E0E4: jsr (%a0)
    0x4CDF, 0x4080,                 // 0x00E0E6: movem.l (%sp)+, %d7/%a6
    0x4DEE, 0x005A,                 // 0x00E0EA: lea 90(%a6), %a6
    0x51CF, 0xFFDE,                 // 0x00E0EE: dbf %d7, 0x00E0CE
    0x4E75                          // 0x00E0F2: rts
};

bool verify_rom_words(const M68k9D70StepApi& api, unsigned start_pc) noexcept;

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
M68k9D70StepResult step_beq_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_l_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movea_l_reg_reg(const M68k9D70StepApi& api, unsigned pc, unsigned sreg,
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

} // namespace oasis::hybrid::step_00e0ba

#endif // SEGA_THOR_M68K_00E0BA_STEP_OPS_HPP
