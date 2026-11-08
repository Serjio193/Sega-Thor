#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::step_002fda {

constexpr unsigned kStartPc = 0x002FDAU;
constexpr unsigned kMask24 = 0x00FFFFFFU;
constexpr unsigned kRegD0 = 0U;
constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD4 = 4U;
constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegD6 = 6U;
constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA3 = 11U;
constexpr unsigned kRegA6 = 14U;
constexpr unsigned kRegSP = 15U;
constexpr unsigned kExitRtsPc = 0x00302CU;

bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& out) noexcept;
void write_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned val) noexcept;
bool in_work_ram(unsigned addr, unsigned size) noexcept;
std::uint16_t ccr(const M68k9D70StepApi& api) noexcept;
void set_ccr(const M68k9D70StepApi& api, std::uint16_t value) noexcept;

std::uint16_t ccr_logic16(std::uint16_t val, std::uint16_t old_ccr) noexcept;
unsigned calc_mulu_cycles(std::uint16_t src) noexcept;
unsigned calc_divu_cycles(std::uint32_t dst, std::uint32_t src) noexcept;

bool prepare_step(const M68k9D70StepApi& api, unsigned pc, std::uint16_t& opcode) noexcept;
bool complete_step(const M68k9D70StepApi& api, unsigned next_pc, std::uint16_t opcode,
                   int cycle_adjustment, unsigned& resume_pc) noexcept;

M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept;

} // namespace oasis::hybrid::step_002fda
