#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::step_00ad76 {

constexpr unsigned kStartPc = 0x00AD76U;
constexpr unsigned kMask24 = 0x00FFFFFFU;
constexpr unsigned kRegD0 = 0U;
constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD2 = 2U;
constexpr unsigned kRegD3 = 3U;
constexpr unsigned kRegD4 = 4U;
constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegD6 = 6U;
constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA0 = 8U;
constexpr unsigned kRegA1 = 9U;
constexpr unsigned kRegA2 = 10U;
constexpr unsigned kRegA3 = 11U;
constexpr unsigned kRegA4 = 12U;
constexpr unsigned kRegA5 = 13U;
constexpr unsigned kRegA6 = 14U;
constexpr unsigned kRegSP = 15U;
constexpr unsigned kExitRtsPc = 0x00B010U;

bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& out) noexcept;
void write_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned val) noexcept;
bool in_work_ram(unsigned addr, unsigned size) noexcept;
std::uint16_t ccr(const M68k9D70StepApi& api) noexcept;
void set_ccr(const M68k9D70StepApi& api, std::uint16_t value) noexcept;

bool prepare_step(const M68k9D70StepApi& api, unsigned pc, std::uint16_t& opcode) noexcept;
bool complete_step(const M68k9D70StepApi& api, unsigned next_pc, std::uint16_t opcode,
                   int cycle_adjustment, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_bcc_w(const M68k9D70StepApi& api, unsigned pc, bool taken,
                              unsigned target_pc, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bcc_s(const M68k9D70StepApi& api, unsigned pc, bool taken,
                              unsigned target_pc, unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bra_w(const M68k9D70StepApi& api, unsigned pc, unsigned target_pc,
                              unsigned& resume_pc) noexcept;
M68k9D70StepResult step_bsr_w(const M68k9D70StepApi& api, unsigned pc, unsigned target,
                              unsigned return_pc, unsigned& resume_pc,
                              bool& child_call_active, unsigned& saved_child_sp) noexcept;
M68k9D70StepResult step_dbf_d5(const M68k9D70StepApi& api, unsigned pc, unsigned loop_pc,
                               unsigned exit_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movem_push_d7(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_movem_pop_d7(const M68k9D70StepApi& api, unsigned pc,
                                     unsigned next_pc, unsigned& resume_pc) noexcept;
M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_00ad76
