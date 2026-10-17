#ifndef SEGA_THOR_M68K_00B3B2_STEP_OPS_HPP
#define SEGA_THOR_M68K_00B3B2_STEP_OPS_HPP

#include <array>
#include <cstdint>
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

namespace oasis::hybrid::step_00b3b2 {

inline constexpr unsigned kEntryPc_00b3b2 = 0x0000B3B2U;
inline constexpr unsigned kExpectedCaller_00ad4e = 0x0000AD4EU;
inline constexpr unsigned kExpectedReturn_00ad52 = 0x0000AD52U;
inline constexpr unsigned kEndPc_00b3d2 = 0x0000B3D2U;

inline constexpr std::array<std::uint16_t, 16> kExpectedWords_00b3b2 = {
    0x4A39, 0x00FF, 0x1927, // 0x00B3B2: tst.b $00FF1927
    0x6600, 0xFFC8,         // 0x00B3B8: bne.w 0x00B382
    0x1A2E, 0x0039,         // 0x00B3BC: move.b 57(%a6), %d5
    0x0805, 0x0001,         // 0x00B3C0: btst #1, %d5
    0x6600, 0x0004,         // 0x00B3C4: bne.w 0x00B3CA
    0x4E75,                 // 0x00B3C8: rts
    0x6100, 0x029E,         // 0x00B3CA: bsr.w 0x00B66A
    0x6000, 0xFFF8          // 0x00B3CE: bra.w 0x00B3C8
};

inline constexpr std::uint16_t kExpectedWord_00b382 = 0x4E75;

bool verify_rom_words(const M68k9D70StepApi& api, unsigned start_pc) noexcept;

M68k9D70StepResult step_tst_b_abs(const M68k9D70StepApi& api, unsigned pc, unsigned addr,
                                  unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bne_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_move_b_disp_reg(const M68k9D70StepApi& api, unsigned pc, unsigned areg,
                                        std::int16_t disp, unsigned dreg, unsigned next_pc,
                                        unsigned& resume_pc) noexcept;
M68k9D70StepResult step_btst_imm_reg(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                     unsigned dreg, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bsr_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned return_pc, unsigned& resume_pc, bool& child_active,
                              unsigned& saved_sp) noexcept;
M68k9D70StepResult step_bra_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned& resume_pc) noexcept;
M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned return_pc,
                            unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_00b3b2

#endif // SEGA_THOR_M68K_00B3B2_STEP_OPS_HPP
