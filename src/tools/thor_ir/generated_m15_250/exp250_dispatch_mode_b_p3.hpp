#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp250_block_BB_45_0x025628(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025628(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_46_0x02562C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02562C(bus, next_pc)) return false;
  if (!step_ir_0x025630(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_47_0x025636(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025636(bus, next_pc)) return false;
  if (!step_ir_0x025638(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_48_0x02563A(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02563A(bus, next_pc)) return false;
  if (!step_ir_0x02563C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_49_0x025642(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025642(bus, next_pc)) return false;
  if (!step_ir_0x025646(bus, next_pc)) return false;
  if (!step_ir_0x02564A(bus, next_pc)) return false;
  if (!step_ir_0x02564E(bus, next_pc)) return false;
  if (!step_ir_0x025650(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_50_0x025656(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025656(bus, next_pc)) return false;
  if (!step_ir_0x02565A(bus, next_pc)) return false;
  if (!step_ir_0x02565E(bus, next_pc)) return false;
  if (!step_ir_0x025664(bus, next_pc)) return false;
  if (!step_ir_0x02566A(bus, next_pc)) return false;
  if (!step_ir_0x02566C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_51_0x025672(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025672(bus, next_pc)) return false;
  if (!step_ir_0x025674(bus, next_pc)) return false;
  if (!step_ir_0x02567A(bus, next_pc)) return false;
  if (!step_ir_0x02567E(bus, next_pc)) return false;
  if (!step_ir_0x025684(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_52_0x02568A(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02568A(bus, next_pc)) return false;
  if (!step_ir_0x025690(bus, next_pc)) return false;
  if (!step_ir_0x025696(bus, next_pc)) return false;
  if (!step_ir_0x02569C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_53_0x02569E(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02569E(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_54_0x0256A4(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0256A4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_55_0x0256A6(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0256A6(bus, next_pc)) return false;
  if (!step_ir_0x0256AA(bus, next_pc)) return false;
  if (!step_ir_0x0256B0(bus, next_pc)) return false;
  if (!step_ir_0x0256B6(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_56_0x02E8B4(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8B4(bus, next_pc)) return false;
  if (!step_ir_0x02E8BA(bus, next_pc)) return false;
  if (!step_ir_0x02E8C0(bus, next_pc)) return false;
  if (!step_ir_0x02E8C6(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_57_0x02E8C8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8C8(bus, next_pc)) return false;
  if (!step_ir_0x02E8CE(bus, next_pc)) return false;
  if (!step_ir_0x02E8D4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_58_0x02E8D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8D8(bus, next_pc)) return false;
  if (!step_ir_0x02E8DE(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_59_0x02E8E2(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8E2(bus, next_pc)) return false;
  if (!step_ir_0x02E8E8(bus, next_pc)) return false;
  if (!step_ir_0x02E8EC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_mode_b_chunk_3(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x025628U: return run_exp250_block_BB_45_0x025628(bus, out_pc);
    case 0x02562CU: return run_exp250_block_BB_46_0x02562C(bus, out_pc);
    case 0x025636U: return run_exp250_block_BB_47_0x025636(bus, out_pc);
    case 0x02563AU: return run_exp250_block_BB_48_0x02563A(bus, out_pc);
    case 0x025642U: return run_exp250_block_BB_49_0x025642(bus, out_pc);
    case 0x025656U: return run_exp250_block_BB_50_0x025656(bus, out_pc);
    case 0x025672U: return run_exp250_block_BB_51_0x025672(bus, out_pc);
    case 0x02568AU: return run_exp250_block_BB_52_0x02568A(bus, out_pc);
    case 0x02569EU: return run_exp250_block_BB_53_0x02569E(bus, out_pc);
    case 0x0256A4U: return run_exp250_block_BB_54_0x0256A4(bus, out_pc);
    case 0x0256A6U: return run_exp250_block_BB_55_0x0256A6(bus, out_pc);
    case 0x02E8B4U: return run_exp250_block_BB_56_0x02E8B4(bus, out_pc);
    case 0x02E8C8U: return run_exp250_block_BB_57_0x02E8C8(bus, out_pc);
    case 0x02E8D8U: return run_exp250_block_BB_58_0x02E8D8(bus, out_pc);
    case 0x02E8E2U: return run_exp250_block_BB_59_0x02E8E2(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
