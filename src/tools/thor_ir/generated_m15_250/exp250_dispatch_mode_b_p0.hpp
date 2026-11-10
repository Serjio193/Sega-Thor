#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp250_block_BB_00_0x008778(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x008778(bus, next_pc)) return false;
  if (!step_ir_0x00877E(bus, next_pc)) return false;
  if (!step_ir_0x008784(bus, next_pc)) return false;
  if (!step_ir_0x00878A(bus, next_pc)) return false;
  if (!step_ir_0x00878C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_01_0x008790(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x008790(bus, next_pc)) return false;
  if (!step_ir_0x008794(bus, next_pc)) return false;
  if (!step_ir_0x008798(bus, next_pc)) return false;
  if (!step_ir_0x00879C(bus, next_pc)) return false;
  if (!step_ir_0x0087A0(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_02_0x0087A2(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087A2(bus, next_pc)) return false;
  if (!step_ir_0x0087A6(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_03_0x0087AA(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087AA(bus, next_pc)) return false;
  if (!step_ir_0x0087AC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_04_0x0087B0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087B0(bus, next_pc)) return false;
  if (!step_ir_0x0087B2(bus, next_pc)) return false;
  if (!step_ir_0x0087B6(bus, next_pc)) return false;
  if (!step_ir_0x0087B8(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_05_0x0087BC(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087BC(bus, next_pc)) return false;
  if (!step_ir_0x0087BE(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_06_0x0087C2(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087C2(bus, next_pc)) return false;
  if (!step_ir_0x0087C6(bus, next_pc)) return false;
  if (!step_ir_0x0087CA(bus, next_pc)) return false;
  if (!step_ir_0x0087CC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_07_0x0087D0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087D0(bus, next_pc)) return false;
  if (!step_ir_0x0087D2(bus, next_pc)) return false;
  if (!step_ir_0x0087D4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_08_0x0087D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0087D8(bus, next_pc)) return false;
  if (!step_ir_0x0087DA(bus, next_pc)) return false;
  if (!step_ir_0x0087DE(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_09_0x00A4C8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00A4C8(bus, next_pc)) return false;
  if (!step_ir_0x00A4D0(bus, next_pc)) return false;
  if (!step_ir_0x00A4D6(bus, next_pc)) return false;
  if (!step_ir_0x00A4DC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_10_0x00A4E0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00A4E0(bus, next_pc)) return false;
  if (!step_ir_0x00A4E8(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_11_0x00A4EE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00A4EE(bus, next_pc)) return false;
  if (!step_ir_0x00A4F2(bus, next_pc)) return false;
  if (!step_ir_0x00A4F8(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_12_0x00A4FA(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00A4FA(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_13_0x00A4FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00A4FE(bus, next_pc)) return false;
  if (!step_ir_0x00A504(bus, next_pc)) return false;
  if (!step_ir_0x00A50A(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_14_0x00A50E(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00A50E(bus, next_pc)) return false;
  if (!step_ir_0x00A514(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_mode_b_chunk_0(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x008778U: return run_exp250_block_BB_00_0x008778(bus, out_pc);
    case 0x008790U: return run_exp250_block_BB_01_0x008790(bus, out_pc);
    case 0x0087A2U: return run_exp250_block_BB_02_0x0087A2(bus, out_pc);
    case 0x0087AAU: return run_exp250_block_BB_03_0x0087AA(bus, out_pc);
    case 0x0087B0U: return run_exp250_block_BB_04_0x0087B0(bus, out_pc);
    case 0x0087BCU: return run_exp250_block_BB_05_0x0087BC(bus, out_pc);
    case 0x0087C2U: return run_exp250_block_BB_06_0x0087C2(bus, out_pc);
    case 0x0087D0U: return run_exp250_block_BB_07_0x0087D0(bus, out_pc);
    case 0x0087D8U: return run_exp250_block_BB_08_0x0087D8(bus, out_pc);
    case 0x00A4C8U: return run_exp250_block_BB_09_0x00A4C8(bus, out_pc);
    case 0x00A4E0U: return run_exp250_block_BB_10_0x00A4E0(bus, out_pc);
    case 0x00A4EEU: return run_exp250_block_BB_11_0x00A4EE(bus, out_pc);
    case 0x00A4FAU: return run_exp250_block_BB_12_0x00A4FA(bus, out_pc);
    case 0x00A4FEU: return run_exp250_block_BB_13_0x00A4FE(bus, out_pc);
    case 0x00A50EU: return run_exp250_block_BB_14_0x00A50E(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
